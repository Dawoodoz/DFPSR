// zlib open source license
//
// Copyright (c) 2019 to 2026 David Forsgren Piuva
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 
//    2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 
//    3. This notice may not be removed or altered from any source
//    distribution.

// This virtual machine uses planar memory with one array for each supported data type.
//   Consequences of having a planar memory architecture instead of a single generic buffer:
//   - This makes it impractical to pass a custom structure by reference at runtime,
//       because the reference would need one stack address for each type.
//     Variables in stack memory are pushed to one call stack per type, using one set
//       of frame and stack pointers per type.
//     So choose the built-in types carefully so that they can handle everything that
//       the virtual machine needs to do.
//   + Very easy to keep away undefined behavior, so that it can use complicated types
//       with non-trivial construction and destruction.
//     One can even port the code to a language that does not support pointers and low level memory
//       manipulation, because indices to whole elements in type-safe arrays are used as pointers.
//   + It is easy to share memory between the virtual machine and the rest of the program,
//       because you only need to store handles to the data in the virtual machine.
//     No need to make copies as if it was a physically separate memory space.

// For each data type in the provided instruction set architecture, you must specify move, reset or both instructions.
//   Defining the move instruction for the type allows initializing variables of the type using an explicit initial value.
//     MOVE: target, source
//   Defining the reset instruction for the type allows initializing values of the type without an explicit initial value.
//     RESET: target
// While intrinsic commands allow overloading with different expected arguments to know what standard memory operations are called,
//   non-intrinsic methods still need unique names because otherwise the caller would be forced to provide argument types just to find the method to call.

#ifndef DFPSR_VIRTUAL_MACHINE
#define DFPSR_VIRTUAL_MACHINE

#include <cstdint>
#include "../../base/StorableCallback.h"
#include "../../collection/FixedArray.h"
#include "../../collection/Array.h"
#include "../../collection/List.h"
#include "../../api/timeAPI.h"

// Flags
//#define VIRTUAL_MACHINE_PROFILE // Enable profiling
//#define VIRTUAL_MACHINE_DEBUG_PRINT // Enable debug printing (will affect profiling)
//#define VIRTUAL_MACHINE_DEBUG_FULL_CONTENT // Allow debug printing to show the full content of variables

namespace dsr {

// Instead of tokenizing, the assembler simply splits everything after colon along commas to get the arguments.
//   To avoid splitting with commas inside of text used to create immediate values, only the commas outside of "" () [] {} are used to split.
static List<String> nestedCommaSplit(const ReadableString& source) {
	List<String> result;
	intptr_t sectionStart = 0;
	bool quoted = false;
	intptr_t depth = 0;
	for (intptr_t i = 0; i < string_length(source); i++) {
		DsrChar c = source[i];
		if (quoted) {
			if (c == U'"') {
				quoted = false;
			} else if (c == U'\\') {
				// Skip one character after escape.
				i++;
			}
		} else {
			if (c == U'"') {
				quoted = true;
			} else if (c == U',' and depth == 0) {
				ReadableString element = string_exclusiveRange(source, sectionStart, i);
				result.push(string_removeOuterWhiteSpace(element));
				sectionStart = i + 1;
			} else if (c == U'(' || c == U'[' || c == U'{') {
				depth++;
			} else if (c == U')' || c == U']' || c == U'}') {
				depth--;
			}
		}
	}
	if (string_length(source) > sectionStart) {
		result.push(string_removeOuterWhiteSpace(string_exclusiveRange(source, sectionStart, string_length(source))));
	}
	if (quoted) {
		throwError(U"Quotes may not contain unmangled line-breaks in virtual machine assembler code!\n");
	}
	if (depth != 0) {
		throwError(U"Immediate constants must balance expressions containing () [] {}, because otherwise parsing of virtual machine assembler code can not know which commas are used to separate arguments!\n");
	}
	return result;
}

struct UnresolvedCommand {
	String command;
	List<String> arguments;
	UnresolvedCommand(const ReadableString &command, const List<String> &arguments)
	: command(command), arguments(arguments) {}
};

struct UnresolvedMethod {
	String name;
	List<String> labels;
	List<UnresolvedCommand> commands;
	UnresolvedMethod(const ReadableString &name)
	: name(name) {}
};

struct UnresolvedProgram {
	List<UnresolvedCommand> commands;
	List<UnresolvedMethod> methods;
};

int32_t findUnresolvedMethod(const UnresolvedProgram &unresolvedProgram, const ReadableString& name) {
	for (int32_t i = 0; i < unresolvedProgram.methods.length(); i++) {
		if (string_caseInsensitiveMatch(unresolvedProgram.methods[i].name, name)) {
			return i;
		}
	}
	return -1;
}

static UnresolvedProgram scanProgram(const ReadableString &code) {
	// An unresolved program structure to be resolved once we have all identifiers.
	UnresolvedProgram program;
	// Index to program.methods.
	int32_t methodIndex = -1;
	string_split_callback([&methodIndex, &program](ReadableString currentLine) {
		// If the line has a comment, then skip everything from #
		int32_t commentIndex = string_findFirst(currentLine, U'#');
		if (commentIndex > -1) {
			currentLine = string_before(currentLine, commentIndex);
		}
		currentLine = string_removeOuterWhiteSpace(currentLine);
		int32_t colonIndex = string_findFirst(currentLine, U':');
		if (colonIndex > -1) {
			ReadableString command = string_removeOuterWhiteSpace(string_before(currentLine, colonIndex));
			ReadableString argumentLine = string_after(currentLine, colonIndex);
			List<String> arguments = nestedCommaSplit(argumentLine);
			if (string_caseInsensitiveMatch(command, U"BEGIN")) {
				if (arguments.length() != 1) {
					throwError(U"Beginning a method should take one argument as the method's identifier!\n");
				} else {
					// The old length becomes the index.
					methodIndex = program.methods.length();
					program.methods.pushConstruct(arguments[0]);
				}
			} else if (string_caseInsensitiveMatch(command, U"END")) {
				// No longer in a method.
				methodIndex = -1;
			} else {
				if (methodIndex == -1) {
					#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
						printText(U"Loading command ", command, U" into global initialization.\n");
					#endif
					// Create unresolved commands globally, which is used to declare global variables.
					program.commands.pushConstruct(command, arguments);
				} else {
					UnresolvedMethod &method = program.methods[methodIndex];
					// TODO: Register names and types.
					if (string_caseInsensitiveMatch(command, U"LABEL")) {
						if (arguments.length() != 1) {
							throwError(U"Labels should take one argument as the label's identifier!\n");
						} else {
							method.labels.push(arguments[0]);
						}
					}
					/*
					if (string_caseInsensitiveMatch(command, U"Hidden")
					 || string_caseInsensitiveMatch(command, U"Input")
					 || string_caseInsensitiveMatch(command, U"Output")) {
						if (arguments.length() == 2) {
							method.variables.pushConstruct(arguments[0], arguments[1], U"");
						} else if (arguments.length() == 3) {
							method.variables.pushConstruct(arguments[0], arguments[1], arguments[2]);
						} else {
							throwError(U"Variable declarations need identifier, type and optionally an initial value!\n");
						}
					}
					*/
					#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
						printText(U"Loading command ", command, U" into ", method.name, U".\n");
					#endif
					// Create unresolved commands in the active method.
					method.commands.pushConstruct(command, arguments);
				}
			}
		} else if (string_length(currentLine) > 0) {
			throwError(U"Unexpected line \"", currentLine, U"\".\n");
		}
	}, code, U'\n');
	return program;
}

// Forward declarations
template <int32_t TYPE_COUNT> struct VirtualMachine;
template <int32_t TYPE_COUNT> struct VMTypeDef;

enum class AccessType {
	Any,
	Hidden,
	Input,
	Output
};
static ReadableString getName(AccessType access) {
	switch(access) {
		case AccessType::Any:    return U"Any";
		case AccessType::Hidden: return U"Hidden";
		case AccessType::Input:  return U"Input";
		case AccessType::Output: return U"Output";
		default:                 return U"?";
	}
}

// Types used in machine instuctions
enum class ArgumentType {
	Immediate,         // Index to MemoryPlane::immediate
	Reference,         // Indirect index translated into both global and stack-relative locations within MemoryPlane::stack
	MethodReference,   // Index to a method to call
	InstructionAddress // Index to a machine instruction, which instructions can jump to.
};

// Types
using DataType = int32_t;

// Instruction addresses uses the list of instructions as their read-only memory.
static const DataType DataType_Label = -1;
static const DataType DataType_InstructionAddress = -2;

template <int32_t TYPE_COUNT>
struct Variable {
	String name;
	AccessType access;
	const VMTypeDef<TYPE_COUNT>* typeDescription;
	int32_t typeLocalIndex; // The zero-based local index among the members of the same type in the method
	bool global; // A flag that generates negative global indices for referring to global variables in method zero
	Variable(const String& name, AccessType access, const VMTypeDef<TYPE_COUNT>* typeDescription, int32_t typeLocalIndex, bool global)
	: name(name), access(access), typeDescription(typeDescription), typeLocalIndex(typeLocalIndex), global(global) {}
	int32_t getGlobalIndex() {
		int32_t result = this->global ? (-this->typeLocalIndex - 1) : this->typeLocalIndex;
		return result;
	}
	int32_t getStackIndex(int32_t framePointer) {
		int32_t result = this->global ? this->typeLocalIndex : this->typeLocalIndex + framePointer;
		return result;
	}
};

// Virtual Machine Argument
struct VMA {
	ArgumentType argType;
	DataType dataType;
	// When argType equals Immediate, the index refers to MemoryPlane::accessByImmediateIndex(index).
	// When argType equals Reference, the index refers to MemoryPlane::accessByGlobalIndex(index, framePointer).
	int32_t index; // Refers to 
	VMA(ArgumentType argType, DataType dataType, int32_t index)
	: argType(argType), dataType(dataType), index(index) {}
};

struct ArgSig {
	ReadableString name;
	bool byValue;
	DataType dataType;
	ArgSig(const ReadableString& name, bool byValue, DataType dataType)
	: name(name), byValue(byValue), dataType(dataType) {}
	bool matches(ArgumentType argType, DataType dataType) const {
		#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
			printText(U"Comparing potential match for argument \"", name, U"\".\n");
		#endif
		if (dataType != this->dataType) {
			#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
				printText(U"	No match because the signature expected data type ", this->dataType, U" but ", dataType, U" was provided instead.\n");
			#endif
			return false;
		}
		if (this->byValue) {
			if (!(argType == ArgumentType::Reference || argType == ArgumentType::Immediate || argType == ArgumentType::InstructionAddress)) {
				#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
					printText(U"	No match because the argument type was not allowed for passing by value.\n");
				#endif
				return false;
			}
		} else {
			if (!(argType == ArgumentType::Reference)) {
				#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
					printText(U"	No match because the argument type was not allowed for passing by reference.\n");
				#endif
				return false;
			}
		}
		#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
			printText(U"	Found a match.\n");
		#endif
		return true;
	}
};

template <typename T>
struct MemoryPlane {
	// Memory that can be pointed to within the address space.
	Array<T> stack;
	// Memory that is only used for nameless immediate constants treated as values.
	List<T> immediate;
	explicit MemoryPlane(int32_t size) : stack(size, T()) {}
	T& accessByStackIndex(int32_t stackIndex) {
		return this->stack[stackIndex];
	}
	// globalIndex uses the negative values starting from -1 to access global memory, and from 0 and up to access local variables on top of the type's own frame pointer.
	T& accessByGlobalIndex(int32_t globalIndex, int32_t framePointer) {
		int32_t stackIndex = (globalIndex < 0) ? -(globalIndex + 1) : (framePointer + globalIndex);
		return this->stack[stackIndex];
	}
	const T& accessByImmediateIndex(int32_t immediateIndex) {
		return this->immediate[immediateIndex];
	}
	T& getRef(const VMA& arg, int32_t framePointer) {
		assert(arg.argType == ArgumentType::Reference);
		return this->accessByGlobalIndex(arg.index, framePointer);
	}
	const T& getImm(const VMA& arg) {
		assert(arg.argType == ArgumentType::Immediate);
		return accessByImmediateIndex(arg.index);
	}
	const T& getRead(const VMA& arg, int32_t framePointer) {
		if (arg.argType == ArgumentType::Immediate) {
			return accessByImmediateIndex(arg.index);
		} else /* if (arg.argType == ArgumentType::Reference) */ {
			return this->accessByGlobalIndex(arg.index, framePointer);
		}
	}
	// TODO: Call this when parsing an immediate value in the source code.
	//       From where can it be called?
	int32_t allocateImmediate(const T& value) {
		return this->immediate.pushGetIndex(value);
	}
};

template <int32_t TYPE_COUNT>
struct CallState {
	int32_t methodIndex = 0;
	int32_t programCounter = 0;
	FixedArray<int32_t, TYPE_COUNT> stackPointer;
	FixedArray<int32_t, TYPE_COUNT> framePointer;
};

// A planar memory system with one stack and frame pointer for each type of memory.
//   This is possible because the virtual machine only operates on types known in compile-time.
//   The planar stack system:
//     * Removes the need to manually initialize and align classes in generic memory.
//     * Encapsulates any effects of endianness or signed integer representations in the physical hardware.
//       Because there can not be accidental reintepretation when the type is known in compile-time.
template <int32_t TYPE_COUNT>
class PlanarMemory {
public:
	CallState<TYPE_COUNT> current;
	List<CallState<TYPE_COUNT>> callStack;
	virtual ~PlanarMemory() {}
	// Store in memory
	virtual void store(int32_t targetStackIndex, const VMA& sourceArg, int32_t sourceFramePointer, DataType type) = 0;
	// Load from memory
	virtual void load(int32_t sourceStackIndex, const VMA& targetArg, int32_t targetFramePointer, DataType type) = 0;
};

// Lambdas without capture is used to create function pointers without objects
template <int32_t TYPE_COUNT>
inline static void MachineOperationTemplate(VirtualMachine<TYPE_COUNT>& machine, PlanarMemory<TYPE_COUNT>& memory, const List<VMA>& args) {}
template <int32_t TYPE_COUNT>
using MachineOperation = decltype(&MachineOperationTemplate<TYPE_COUNT>);

template <int32_t TYPE_COUNT>
struct MachineWord {
	int32_t methodIndex;
	MachineOperation<TYPE_COUNT> operation;
	List<VMA> args;
	MachineWord(int32_t methodIndex, MachineOperation<TYPE_COUNT> operation, const List<VMA>& args)
	: methodIndex(methodIndex), operation(operation), args(args) {}
	explicit MachineWord(int32_t methodIndex, MachineOperation<TYPE_COUNT> operation)
	: methodIndex(methodIndex), operation(operation) {}
};

template <int32_t TYPE_COUNT>
struct InsSig {
public:
	ReadableString name;
	int32_t targetCount; // Number of first arguments to present as results
	List<ArgSig> arguments;
	MachineOperation<TYPE_COUNT> operation;
	InsSig(const ReadableString& name, int32_t targetCount, MachineOperation<TYPE_COUNT> operation)
	: name(name), targetCount(targetCount), operation(operation) {}
private:
	void addArguments() {}
	template <typename... ARGS>
	void addArguments(const ArgSig& head, ARGS... tail) {
		this->arguments.push(head);
		this->addArguments(tail...);
	}
public:
	template <typename... ARGS>
	static InsSig create(const ReadableString& name, int32_t targetCount, MachineOperation<TYPE_COUNT> operation, ARGS... args) {
		InsSig result = InsSig(name, targetCount, operation);
		result.addArguments(args...);
		return result;
	}
	bool matches(const ReadableString& name, List<VMA> resolvedArguments) const {
		if (resolvedArguments.length() != this->arguments.length()) {
			return false;
		} else if (!string_caseInsensitiveMatch(this->name, name)) {
			return false;
		} else {
			for (int32_t i = 0; i < this->arguments.length(); i++) {
				if (!this->arguments[i].matches(resolvedArguments[i].argType, resolvedArguments[i].dataType)) {
					return false;
				}
			}
			return true;
		}
	}
};

// Types
template <int32_t TYPE_COUNT>
inline static void initializeTemplate(VirtualMachine<TYPE_COUNT>& machine, int32_t globalIndex, const ReadableString& defaultValue) {}

template <int32_t TYPE_COUNT>
inline static void debugPrintTemplate(PlanarMemory<TYPE_COUNT>& memory, Variable<TYPE_COUNT>& variable, int32_t globalIndex, const FixedArray<int32_t, TYPE_COUNT>& framePointer, bool fullContent) {}
template <int32_t TYPE_COUNT>
using VMT_DebugPrinter = decltype(&debugPrintTemplate<TYPE_COUNT>);

template <int32_t TYPE_COUNT>
struct VMTypeDef {
	ReadableString name;
	DataType dataType;
	bool allowDefaultValue;
	VMT_DebugPrinter<TYPE_COUNT> debugPrinter;
	VMTypeDef(const ReadableString& name, DataType dataType, bool allowDefaultValue, VMT_DebugPrinter<TYPE_COUNT> debugPrinter)
	: name(name), dataType(dataType), allowDefaultValue(allowDefaultValue), debugPrinter(debugPrinter) {}
};

struct InstructionLabel {
	int32_t address;
	String identifier;
	InstructionLabel(int32_t address, const ReadableString &identifier)
	: address(address), identifier(identifier) {}
};

template <int32_t TYPE_COUNT>
struct Method {
	String name;

	// Global instruction space
	int32_t startAddress = 0; // Index to machineWords
	int32_t instructionCount = 0; // Number of machine words (safer than return statements in case of memory corruption)

	// Unified local space
	int32_t inputCount = 0; // Number of inputs declared at the start of locals
	int32_t outputCount = 0; // Number of output declared directly after the inputs

	// TODO: Merge into a state
	bool declaredNonInput = false; // Goes true when a non-input is declared
	bool declaredLocals = false; // Goes true when a local is declared
	List<Variable<TYPE_COUNT>> locals; // locals[0..inputCount-1] are the inputs, while locals[inputCount..inputCount+outputCount-1] are the outputs

	List<InstructionLabel> labels;

	// Type-specific spaces
	FixedArray<int32_t, TYPE_COUNT> count;
	// Look-up table from a combination of type and type-local indices to unified-local indices
	FixedArray<List<int32_t>, TYPE_COUNT> unifiedLocalIndices;

	Method(const String& name) : name(name) {}
	Variable<TYPE_COUNT>* getLocal(const ReadableString& name) {
		for (int32_t i = 0; i < this->locals.length(); i++) {
			#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
				printText(U"Comparing ", name, U" with variable ", this->locals[i].name, U".\n");
			#endif
			if (string_caseInsensitiveMatch(this->locals[i].name, name)) {
				return &this->locals[i];
			}
		}
		return nullptr;
	}
};

template <int32_t TYPE_COUNT>
struct VirtualMachine {
	// Methods
	List<Method<TYPE_COUNT>> methods;
	// Memory
	Handle<PlanarMemory<TYPE_COUNT>> memory;
	// Instruction types
	const InsSig<TYPE_COUNT>* machineInstructions; int32_t machineInstructionCount;
	const InsSig<TYPE_COUNT>* getMachineInstructionFromFunction(MachineOperation<TYPE_COUNT> functionPointer) {
		for (int32_t s = 0; s < this->machineInstructionCount; s++) {
			if (this->machineInstructions[s].operation == functionPointer) {
				return &this->machineInstructions[s];
			}
		}
		return nullptr;
	}
	// Instruction instances
	List<MachineWord<TYPE_COUNT>> machineWords;
	// Resolving of immediate constants in arguments.
	const StorableCallback<VMA(PlanarMemory<TYPE_COUNT> &memory, const ReadableString &argument)> interpretImmediateArgument;
	// Types
	const VMTypeDef<TYPE_COUNT>* machineTypes; int32_t machineTypeCount;
	const VMTypeDef<TYPE_COUNT>* getMachineType(const ReadableString& name) {
		for (int32_t s = 0; s < this->machineTypeCount; s++) {
			if (string_caseInsensitiveMatch(this->machineTypes[s].name, name)) {
				return &this->machineTypes[s];
			}
		}
		return nullptr;
	}
	const VMTypeDef<TYPE_COUNT>* getMachineType(DataType dataType) {
		for (int32_t s = 0; s < this->machineTypeCount; s++) {
			if (this->machineTypes[s].dataType == dataType) {
				return &this->machineTypes[s];
			}
		}
		return nullptr;
	}

	// Constructor
	VirtualMachine(const ReadableString& code, const Handle<PlanarMemory<TYPE_COUNT>>& memory,
	  const InsSig<TYPE_COUNT>* machineInstructions, int32_t machineInstructionCount,
	  const VMTypeDef<TYPE_COUNT>* machineTypes, int32_t machineTypeCount,
	  StorableCallback<VMA(PlanarMemory<TYPE_COUNT> &memory, const ReadableString &argument)> interpretImmediateArgument)
	: memory(memory), machineInstructions(machineInstructions), machineInstructionCount(machineInstructionCount),
	  interpretImmediateArgument(interpretImmediateArgument), machineTypes(machineTypes), machineTypeCount(machineTypeCount) {
		  // Increase TYPE_COUNT if it's not enough for the supplied list of types.
		assert(machineTypeCount <= TYPE_COUNT);
		#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
			printText(U"Starting media machine.\n");
		#endif
		#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
			printText(U"Reading assembly.\n");
		#endif
		UnresolvedProgram unresolvedProgram = scanProgram(code);

		// Create the <init> method at the end with unresolved variables and commands stolen from the global space.
		unresolvedProgram.methods.pushConstruct(U"<init>");
		int32_t initMethodIndex = unresolvedProgram.methods.length() - 1;
		UnresolvedMethod &initMethod = unresolvedProgram.methods[initMethodIndex];
		initMethod.commands = dsr::move(unresolvedProgram.commands);

		// Allocate empty methods, just so that variables can look at empty lists while looking for duplicate declarations.
		for (int32_t methodIndex = 0; methodIndex < unresolvedProgram.methods.length(); methodIndex++) {
			const UnresolvedMethod &unresolvedMethod = unresolvedProgram.methods[methodIndex];
			this->methods.pushConstruct(unresolvedMethod.name);
			#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
				printText(U"Creating \"", unresolvedMethod.name, U"\" at method index ", methodIndex, U".\n");
			#endif
		}

		// Fill methods with variables, so that there is something to access by name.
		for (int32_t methodIndex = 0; methodIndex < unresolvedProgram.methods.length(); methodIndex++) {
			bool global = methodIndex == initMethodIndex;
			const UnresolvedMethod &unresolvedMethod = unresolvedProgram.methods[methodIndex];
			// Create local variables and commands.
			for (int32_t c = 0; c < unresolvedMethod.commands.length(); c++) {
				const UnresolvedCommand &unresolvedCommand = unresolvedMethod.commands[c];
				if (string_caseInsensitiveMatch(unresolvedCommand.command, U"Temp")) {
					for (int32_t a = 1; a < unresolvedCommand.arguments.length(); a++) {
						this->declareVariable(methodIndex, initMethodIndex, AccessType::Hidden, getArg(unresolvedCommand.arguments, 0), getArg(unresolvedCommand.arguments, a), false, U"", global);
						#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
							printText(U"	Allocating temporary variable \"", getArg(unresolvedCommand.arguments, a), U"\" as \"", getArg(unresolvedCommand.arguments, 0), U"\".\n");
						#endif
					}
				} else if (string_caseInsensitiveMatch(unresolvedCommand.command, U"Hidden")) {
					this->declareVariable(methodIndex, initMethodIndex, AccessType::Hidden, getArg(unresolvedCommand.arguments, 0), getArg(unresolvedCommand.arguments, 1), true, getArg(unresolvedCommand.arguments, 2), global);
					#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
						printText(U"	Allocating hidden variable \"", getArg(unresolvedCommand.arguments, 1), U"\" as \"", getArg(unresolvedCommand.arguments, 0), U"\".\n");
					#endif
				} else if (string_caseInsensitiveMatch(unresolvedCommand.command, U"Input")) {
					this->declareVariable(methodIndex, initMethodIndex, AccessType::Input, getArg(unresolvedCommand.arguments, 0), getArg(unresolvedCommand.arguments, 1), true, getArg(unresolvedCommand.arguments, 2), global);
					#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
						printText(U"	Allocating input variable \"", getArg(unresolvedCommand.arguments, 1), U"\" as \"", getArg(unresolvedCommand.arguments, 0), U"\".\n");
					#endif
				} else if (string_caseInsensitiveMatch(unresolvedCommand.command, U"Output")) {
					this->declareVariable(methodIndex, initMethodIndex, AccessType::Output, getArg(unresolvedCommand.arguments, 0), getArg(unresolvedCommand.arguments, 1), true, getArg(unresolvedCommand.arguments, 2), global);
					#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
						printText(U"	Allocating output variable \"", getArg(unresolvedCommand.arguments, 1), U"\" as \"", getArg(unresolvedCommand.arguments, 0), U"\".\n");
					#endif
				}
			}
		}

		// Iterate over the empty methods and fill them with machine instructions and labels while assigning start addresses.
		for (int32_t methodIndex = 0; methodIndex < this->methods.length(); methodIndex++) {
			bool global = methodIndex == initMethodIndex;
			const UnresolvedMethod &unresolvedMethod = unresolvedProgram.methods[methodIndex];
			this->methods[methodIndex].startAddress = this->machineWords.length();
			#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
				printText(U"Method \"", this->methods[methodIndex].name, U"\" begins at instruction ", this->methods[methodIndex].startAddress, U".\n");
			#endif
			// Create local variables and commands.
			for (int32_t c = 0; c < unresolvedMethod.commands.length(); c++) {
				const UnresolvedCommand &unresolvedCommand = unresolvedMethod.commands[c];
				this->generateLabelsAndStatements(methodIndex, initMethodIndex, unresolvedProgram, unresolvedMethod, unresolvedCommand.command, unresolvedCommand.arguments, global);
			}
			this->addReturnInstruction(methodIndex);
			#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
				printText(U"Creating return instruction at the end of \"", unresolvedMethod.name, U"\".\n");
			#endif
		}

		// Convert indices to local labels into instruction addresses.
		for (int32_t m = 0; m < this->machineWords.length(); m++) {
			MachineWord<TYPE_COUNT> &machineWord = this->machineWords[m];
			for (int32_t a = 0; a < machineWord.args.length(); a++) {
				VMA &argument = machineWord.args[a];
				if (argument.argType == ArgumentType::InstructionAddress
				 && argument.dataType == DataType_Label) {
					// TODO: Look up the address from the label.
					//       This requires knowing the method index.
					const Method<TYPE_COUNT> &method = this->methods[machineWord.methodIndex];
					int32_t localLabelIndex = argument.index;
					if (localLabelIndex < 0 || localLabelIndex >= method.labels.length()) {
						throwError(U"Local label index ", localLabelIndex, U" was out of bound 0..", method.labels.length(), U" in the ", method.name, U" method!\n");
					} else {
						int32_t instructionAddress = method.labels[localLabelIndex].address;
						argument = VMA(ArgumentType::InstructionAddress, DataType_InstructionAddress, instructionAddress);
						#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
							printText(U"Converted LabelIndex ", localLabelIndex, U" into InstructionAddress ", instructionAddress, U".\n");
						#endif
					}
				}
			}
		}

		// Calling the last method "<init>" to execute global commands
		#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
			printText(U"Initializing global machine state.\n");
		#endif
		this->executeMethod(this->methods.length() - 1);
	}

	int32_t findMethod(const ReadableString& name) {
		for (int32_t i = 0; i < this->methods.length(); i++) {
			if (string_caseInsensitiveMatch(this->methods[i].name, name)) {
				return i;
			}
		}
		return -1;
	}

	Variable<TYPE_COUNT>* getResource(const ReadableString& name, int32_t methodIndex, int32_t initMethodIndex) {
		#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
			if (methodIndex == initMethodIndex) {
				printText(U"Looking for resource \"", name, U"\" in <init> method ", methodIndex, U".\n");
			} else {
				printText(U"Looking for resource \"", name, U"\" in method ", methodIndex, U" with <init> at ", initMethodIndex, U" as the fallback namespace.\n");
			}
		#endif
		Variable<TYPE_COUNT>* result = this->methods[methodIndex].getLocal(name);
		if (result) {
			// If found, take the local variable
			return result;
		} else if (methodIndex != initMethodIndex) {
			// If not found but having another scope, look for global variables in the global initiation method
			return getResource(name, initMethodIndex, initMethodIndex);
		} else {
			return nullptr;
		}
	}

	/*
	Indices
		Global index: (Identifier) Negative for globals and natural for stack.
			These are translated into stack indices for run-time lookups
			Useful for storing in compile-time when there's no stack nor frame-pointer for mapping to any real memory address
			Relative to the frame-pointer, so it cannot access anything else than globals (using negative indices) and locals (using natural indices)
		Stack index: (Pointer) The absolute index of a variable at run-time
			Indices to the type's own stack in the machine
			A frame pointer is needed to create them, but the memory of calling methods can be accessed using stack indices
		Type local index: (Frame-pointer offset) The local index of a variable with a type among the same type
			Quick at finding a stack index for the type's own stack
			Useful to store in variables and convert into global and stack indices
			For compile-time generation and run-time variable access
		Unified local index: (Variable) The index of a variable's debug information
			Indices to unifiedLocalIndices in methods
			Can be used to find the name of the variable for debugging
			Unlike the type local index, the unified index knows the type
		Immediate index:
			Only immediate references uses the address space for immediate indices.
	*/
	static int32_t globalToTypeLocalIndex(int32_t globalIndex) {
		return globalIndex < 0 ? -(globalIndex + 1) : globalIndex;
	}
	static int32_t typeLocalToGlobalIndex(bool isGlobal, int32_t typeLocalIndex) {
		return isGlobal ? -(typeLocalIndex + 1) : typeLocalIndex;
	}

	void addMachineWord(int32_t methodIndex, MachineOperation<TYPE_COUNT> operation, const List<VMA>& args = List<VMA>()) {
		#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
			printText(U"Instruction ", this->machineWords.length(), U" is declared in method ", methodIndex, U".\n");
		#endif
		this->machineWords.pushConstruct(methodIndex, operation, args);
		this->methods[methodIndex].instructionCount++;
	}

	void addReturnInstruction(int32_t methodIndex) {
		addMachineWord(methodIndex, [](VirtualMachine& machine, PlanarMemory<TYPE_COUNT>& memory, const List<VMA>& args) {
			if (memory.callStack.length() > 0) {
				// If the call was made from another method in the virtual machine, then we return to that call.
				#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
					printText(U"Returning from \"", machine.methods[memory.current.methodIndex].name, U"\" to caller \"", machine.methods[memory.callStack.last().methodIndex].name, U"\"\n");
					machine.debugPrintMemory();
				#endif
				memory.current = memory.callStack.last();
				memory.callStack.pop();
				memory.current.programCounter++;
			} else {
				// If there was no callers in the call stack, return to the external caller outside of the virtual machine.
				#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
					printText(U"Returning from \"", machine.methods[memory.current.methodIndex].name, U"\"\n");
				#endif
				// Leave the virtual machine
				memory.current.programCounter = -1;
			}
		});
	}

	static ReadableString getArg(const List<String>& arguments, int32_t index) {
		if (index < 0 || index >= arguments.length()) {
			return U"";
		} else {
			return string_removeOuterWhiteSpace(arguments[index]);
		}
	}

	// Generate machine code to call a method using the "call" instruction.
	void addCallInstructions(int32_t methodIndex, int32_t initMethodIndex, const UnresolvedProgram &unresolvedProgram, const UnresolvedMethod &unresolvedMethod, const List<String>& arguments) {
		if (arguments.length() < 1) {
			throwError(U"Cannot make a call without the name of a method!\n");
		}
		ReadableString methodName = string_removeOuterWhiteSpace(arguments[0]);
		int32_t calledMethodIndex = findUnresolvedMethod(unresolvedProgram, methodName);
		if (calledMethodIndex == -1) {
			throwError(U"Tried to make an internal call to the method \"", methodName, U"\", which was not previously defined in the virtual machine! Make sure that the name is spelled correctly and the method is defined above the caller.\n");
		}
		// Check the total number of arguments
		Method<TYPE_COUNT>* calledMethod = &this->methods[calledMethodIndex];
		if (arguments.length() - 1 != calledMethod->outputCount + calledMethod->inputCount) {
			throwError(U"Wrong argument count to \"", calledMethod->name, U"\"! Call arguments should start with the method to call, continue with output references and end with inputs.\n");
		}
		// Split assembler arguments into separate input and output arguments for machine instructions
		List<VMA> inputArguments;
		List<VMA> outputArguments;
		// Instead of storing an index to an immediate constant that requires a type, we use MethodReference to refer directly to the called method.
		VMA methodIndexArgument = VMA(ArgumentType::MethodReference, -1, calledMethodIndex);
		inputArguments.push(methodIndexArgument);
		outputArguments.push(methodIndexArgument);
		int32_t outputCount = 0;
		for (int32_t a = 1; a < arguments.length(); a++) {
			ReadableString content = string_removeOuterWhiteSpace(arguments[a]);
			if (string_length(content) > 0) {
				if (outputCount < calledMethod->outputCount) {
					outputArguments.push(this->VMAfromText(unresolvedProgram, unresolvedMethod, methodIndex, initMethodIndex, getArg(arguments, a), false));
					outputCount++;
				} else {
					inputArguments.push(this->VMAfromText(unresolvedProgram, unresolvedMethod, methodIndex, initMethodIndex, getArg(arguments, a), false));
				}
			}
		}
		// Check types
		for (int32_t a = 1; a < outputArguments.length(); a++) {
			// Output
			Variable<TYPE_COUNT>* variable = &calledMethod->locals[a - 1 + calledMethod->inputCount];
			if (outputArguments[a].argType != ArgumentType::Reference) {
				throwError(U"Output argument for \"", variable->name, U"\" in \"", calledMethod->name, U"\" must be a reference to allow writing its result!\n");
			} else if (outputArguments[a].dataType != variable->typeDescription->dataType) {
				throwError(U"Output argument for \"", variable->name, U"\" in \"", calledMethod->name, U"\" must have the type \"", variable->typeDescription->name, U"\"!\n");
			}
		}
		for (int32_t a = 1; a < inputArguments.length(); a++) {
			// Input
			Variable<TYPE_COUNT>* variable = &calledMethod->locals[a - 1];
			if (inputArguments[a].dataType != variable->typeDescription->dataType) {
				throwError(U"Input argument for \"", variable->name, U"\" in \"", calledMethod->name, U"\" must have the type \"", variable->typeDescription->name, U"\"!\n");
			}
		}
		// Insert a machine instruction customized for calling a specific method
		//   Everything within this lambda will then be executed when the call is made at runtime
		addMachineWord(methodIndex, [](VirtualMachine& machine, PlanarMemory<TYPE_COUNT>& memory, const List<VMA>& args) {
			// Get the method to call
			int32_t calledMethodIndex = args[0].index;
			#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
				int32_t oldMethodIndex = memory.current.methodIndex;
			#endif
			Method<TYPE_COUNT>* calledMethod = &machine.methods[calledMethodIndex];
			#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
				printText(U"Calling \"", calledMethod->name, U"\".\n");
			#endif
			// Calculate new frame pointers
			FixedArray<int32_t, TYPE_COUNT> newFramePointer;
			FixedArray<int32_t, TYPE_COUNT> newStackPointer;
			for (int32_t t = 0; t < TYPE_COUNT; t++) {
				newFramePointer[t] = memory.current.stackPointer[t];
				newStackPointer[t] = memory.current.stackPointer[t] + machine.methods[calledMethodIndex].count[t];
				#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
					printText(U"Allocating stack memory for type ", t, U".\n");
					printText(U"    old frame pointer = ", memory.current.framePointer[t], U"\n");
					printText(U"    old stack pointer = ", memory.current.stackPointer[t], U"\n");
					printText(U"    needed elements = ", machine.methods[oldMethodIndex].count[t], U"\n");
					printText(U"    new frame pointer = ", newFramePointer[t], U"\n");
					printText(U"    new stack pointer = ", newStackPointer[t], U"\n");
				#endif
			}
			// Assign inputs while skipping the method index in the first argument
			for (int32_t a = 1; a < args.length(); a++) {
				// Get the argument
				Variable<TYPE_COUNT>* target = &calledMethod->locals[a - 1];
				// Get the argument's type
				DataType typeIndex = target->typeDescription->dataType;
				// Get the stack location
				int32_t targetStackIndex = target->getStackIndex(newFramePointer[typeIndex]);
				memory.store(targetStackIndex, args[a], memory.current.framePointer[typeIndex], typeIndex);
			}
			// Jump into the method
			memory.callStack.push(memory.current);
			memory.current.methodIndex = calledMethodIndex;
			memory.current.programCounter = machine.methods[calledMethodIndex].startAddress;
			for (int32_t t = 0; t < TYPE_COUNT; t++) {
				memory.current.framePointer[t] = newFramePointer[t];
				memory.current.stackPointer[t] = newStackPointer[t];
			}
		}, inputArguments);
		// Get results from the method
		addMachineWord(methodIndex, [](VirtualMachine& machine, PlanarMemory<TYPE_COUNT>& memory, const List<VMA>& args) {
			int32_t calledMethodIndex = args[0].index;
			Method<TYPE_COUNT>* calledMethod = &machine.methods[calledMethodIndex];
			#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
				printText(U"Writing results after call to \"", calledMethod->name, U"\":\n");
			#endif
			// Assign outputs
			for (int32_t a = 1; a < args.length(); a++) {
				Variable<TYPE_COUNT>* source = &calledMethod->locals[a - 1 + calledMethod->inputCount];
				DataType typeIndex = source->typeDescription->dataType;
				int32_t sourceStackIndex = source->getStackIndex(memory.current.stackPointer[typeIndex]);
				memory.load(sourceStackIndex, args[a], memory.current.framePointer[typeIndex], typeIndex);
				#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
					printText(U"  ");
					//VMA(ArgumentType argType, DataType dataType, int32_t index)
					machine.debugArgument(VMA(ArgumentType::MethodReference, typeIndex, source->getGlobalIndex()), calledMethodIndex, memory.current.stackPointer, false);
					printText(U" -> ");
					machine.debugArgument(args[a], memory.current.methodIndex, memory.current.framePointer, false);
					printText(U"\n");
				#endif
			}
			// TODO: Decrease reference counts for images by zeroing memory above the new stack-pointer
			//       Avoiding temporary memory leaks and making sure that no cloning is needed for operations that clone if needed
			//       Planar memory will receive a new memset operation for a range of stack indices for a given type
			memory.current.programCounter++;
			#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
				machine.debugPrintMemory();
			#endif
		}, outputArguments);
	}

	void interpretCommand(int32_t methodIndex, const ReadableString& operation, const List<VMA>& resolvedArguments) {
		// Compare the input with overloads
		for (int32_t s = 0; s < machineInstructionCount; s++) {
			if (machineInstructions[s].matches(operation, resolvedArguments)) {
				this->addMachineWord(methodIndex, machineInstructions[s].operation, resolvedArguments);
				return;
			}
		}
		// TODO: Allow asking the specific machine type what the given types are called.
		String message = string_combine(U"\nError! ", operation, U" does not match any overload:\n");
		for (int32_t s = 0; s < machineInstructionCount; s++) {
			const InsSig<TYPE_COUNT>* signature = &machineInstructions[s];
			if (string_caseInsensitiveMatch(signature->name, operation)) {
				string_append(message, U"  * ", signature->name, U"(");
				for (int32_t a = 0; a < signature->arguments.length(); a++) {
					if (a > 0) {
						string_append(message, U", ");
					}
					const ArgSig* argument = &signature->arguments[a];
					string_append(message, argument->name);
				}
				string_append(message, U")\n");
			}
		}
		throwError(message);
	}

	// TODO: Inline into declareVariable
	Variable<TYPE_COUNT>* declareVariable_aux(const VMTypeDef<TYPE_COUNT>& typeDef, int32_t methodIndex, AccessType access, const ReadableString& name, bool initialize, const ReadableString& defaultValueText, bool global) {
		// Make commonly used data more readable
		Method<TYPE_COUNT>* currentMethod = &this->methods[methodIndex];

		// Assert correctness
		if (global && (access == AccessType::Input || access == AccessType::Output)) {
			throwError(U"Cannot declare inputs or outputs globally!\n");
		}

		// Count how many variables the method has of each type
		currentMethod->count[typeDef.dataType]++;
		this->methods[methodIndex].unifiedLocalIndices[typeDef.dataType].push(this->methods[methodIndex].locals.length());
		// Count inputs for calling the method
		if (access == AccessType::Input) {
			if (this->methods[methodIndex].declaredNonInput) {
				throwError(U"Cannot declare input \"", name, U"\" after a non-input has been declared. Declare inputs, outputs and locals in order.\n");
			}
			this->methods[methodIndex].inputCount++;
		} else if (access == AccessType::Output) {
			if (this->methods[methodIndex].declaredLocals) {
				throwError(U"Cannot declare output \"", name, U"\" after a local has been declared. Declare inputs, outputs and locals in order.\n");
			}
			this->methods[methodIndex].outputCount++;
			this->methods[methodIndex].declaredNonInput = true;
		} else if (access == AccessType::Hidden) {
			this->methods[methodIndex].declaredLocals = true;
			this->methods[methodIndex].declaredNonInput = true;
		}
		// Declare the variable so that code may find the type and index by name
		int32_t typeLocalIndex = currentMethod->count[typeDef.dataType] - 1;
		int32_t globalIndex = typeLocalToGlobalIndex(global, typeLocalIndex);
		this->methods[methodIndex].locals.pushConstruct(name, access, &typeDef, typeLocalIndex, global);
		// If the variable is supposed to be initialized automatically and no value will be given from an input, then we have to initialize it using hidden machine instructions.
		if (initialize && access != AccessType::Input) {
			// TODO: Give custom error messages when load or reset was not implemented.
			//       If reset is missing, then the type does not have a default constructor and must have one specified.
			//       If move is missing, then the type does not have a constructor accepting immediate constants.
			if (string_length(defaultValueText) > 0) {
				// An initial value was provided explicitly.
				//   Look for a load instruction.
				this->interpretCommand(methodIndex, U"Move", List<VMA>(
					VMA(ArgumentType::Reference, typeDef.dataType, globalIndex),
					this->interpretImmediateArgument(this->memory.getReference(), defaultValueText)
				));
			} else {
				// No initial value was provided.
				//   Look for a reset instruction.
				this->interpretCommand(methodIndex, U"Reset", List<VMA>(
					VMA(ArgumentType::Reference, typeDef.dataType, globalIndex)
				));
			}
		}
		return &this->methods[methodIndex].locals.last();
	}

	Variable<TYPE_COUNT>* declareVariable(int32_t methodIndex, int32_t initMethodIndex, AccessType access, const ReadableString& typeName, const ReadableString& name, bool initialize, const ReadableString& defaultValueText, bool global) {
		if (this->getResource(name, methodIndex, initMethodIndex)) {
			throwError(U"A resource named \"", name, U"\" already exists! Be aware that resource names are case insensitive.\n");
			return nullptr;
		} else {
			// Loop over type definitions to find a match
			const VMTypeDef<TYPE_COUNT>* typeDef = getMachineType(typeName);
			if (typeDef) {
				if (string_length(defaultValueText) > 0 && !typeDef->allowDefaultValue) {
					throwError(U"The variable \"", name, U"\" doesn't have an immediate constructor for \"", typeName, U"\".\n");
				}
				return this->declareVariable_aux(*typeDef, methodIndex, access, name, initialize, defaultValueText, global);
			} else {
				throwError(U"Cannot declare variable of unknown type \"", typeName, U"\"!\n");
				return nullptr;
			}
		}
	}

	VMA VMAfromText(const UnresolvedProgram &unresolvedProgram, const UnresolvedMethod &unresolvedMethod, int32_t methodIndex, int32_t initMethodIndex, const ReadableString& content, bool global) {
		#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
			printText(U"VMAfromText in method ", methodIndex, U" interpreting argument \"", content, U"\".\n");
		#endif
		DsrChar first = content[0];
		DsrChar second = content[1];
		if ((first >= U'a' && first <= U'z') || (first >= U'A' && first <= U'Z') || first >= U'_' || first >= U'<') {
			// If the argument begins with a..z | A..Z | _ | < then it is a non-immediate argument.
			int32_t leftIndex = string_findFirst(content, U'<');
			int32_t rightIndex = string_findLast(content, U'>');
			if (leftIndex > -1 && rightIndex > -1) {
				ReadableString name = string_removeOuterWhiteSpace(string_before(content, leftIndex));
				ReadableString typeName = string_removeOuterWhiteSpace(string_inclusiveRange(content, leftIndex + 1, rightIndex - 1));
				ReadableString remainder = string_removeOuterWhiteSpace(string_after(content, rightIndex));
				if (string_length(remainder) > 0) {
					throwError(U"No code allowed after > for in-place temp declarations!\n");
				}
				Variable<TYPE_COUNT>* resource = this->declareVariable(methodIndex, initMethodIndex, AccessType::Hidden, typeName, name, false, U"", global);
				if (resource) {
					return VMA(ArgumentType::Reference, resource->typeDescription->dataType, resource->getGlobalIndex());
				} else {
					throwError(U"The resource \"", name, U"\" could not be declared as \"", typeName, U"\"!\n");
					return VMA(ArgumentType::Immediate, -1, -1);
				}
			} else if (leftIndex > -1) {
				throwError(U"Using < without > for in-place temp allocation.\n");
				return VMA(ArgumentType::Immediate, -1, -1);
			} else if (rightIndex > -1) {
				throwError(U"Using > without < for in-place temp allocation.\n");
				return VMA(ArgumentType::Immediate, -1, -1);
			} else {
				// Look for a label.
				String labelName = string_removeOuterWhiteSpace(content);
				//debugText(U"Checking if ", labelName, U" is a label in the ", unresolvedMethod.name, U" method.\n");
				for (int32_t l = 0; l < unresolvedMethod.labels.length(); l++) {
					//debugText(U"  * Label ", unresolvedMethod.labels[l], U"\n");
					if (string_caseInsensitiveMatch(unresolvedMethod.labels[l], labelName)) {
						// Once code generation is finished, label indices will be replaced with the final address by looking it up from the label.
						return VMA(ArgumentType::InstructionAddress, DataType_Label, l);
					}
				}
				// If it is not a label then look for variables.
				Variable<TYPE_COUNT>* resource = getResource(content, methodIndex, initMethodIndex);
				if (resource) {
					return VMA(ArgumentType::Reference, resource->typeDescription->dataType, resource->getGlobalIndex());
				} else {
					throwError(U"The resource \"", content, U"\" could not be found from method \"", unresolvedProgram.methods[methodIndex].name, U"\"!\n");
					return VMA(ArgumentType::Immediate, -1, -1);
				}
			}
		} else {
			// Ask the instruction set architecture to interpret the argument, while assuming that it is an immediate constant.
			return this->interpretImmediateArgument(this->memory.getReference(), content);
		}
	}

	void generateLabelsAndStatements(int32_t methodIndex, int32_t initMethodIndex, const UnresolvedProgram &unresolvedProgram, const UnresolvedMethod &unresolvedMethod, const ReadableString& command, const List<String>& arguments, bool global) {
		#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
			printText(U"generateLabelsAndStatements @", this->machineWords.length(), U" in method ", methodIndex, U" ", command, U"(");
			for (int32_t a = 0; a < arguments.length(); a++) {
				if (a > 0) { printText(U", "); }
				printText(arguments[a]);
			}
			printText(U")\n");
		#endif
		if (string_caseInsensitiveMatch(command, U"Begin")) {
			throwError(U"Unexpected BEGIN command found inside of method!\n");
		} else if (string_caseInsensitiveMatch(command, U"End")) {
			throwError(U"Unexpected END command found inside of method!\n");
		} else if (string_caseInsensitiveMatch(command, U"Hidden")
		        || string_caseInsensitiveMatch(command, U"Input")
		        || string_caseInsensitiveMatch(command, U"Output")
		        || string_caseInsensitiveMatch(command, U"Temp")) {
			// Variables should already be generated.
		} else if (string_caseInsensitiveMatch(command, U"Call")) {
			#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
				printText(U"Generating call from method ", methodIndex, U"\n");
			#endif
			this->addCallInstructions(methodIndex, initMethodIndex, unresolvedProgram, unresolvedMethod, arguments);
		} else if (string_caseInsensitiveMatch(command, U"Label")) {
			// TODO: Move this into a method for creating a label.
			// TODO: Prevent overlapping names between methods, local variables and local labels.
			if (arguments.length() > 1) {
				throwError(U"Labels may not have more than one argument containing the identifier!\n");
			} else {
				int32_t targetAddress = this->machineWords.length();
				String labelIdentifier = arguments[0];
				// TODO: Reject invalid label names.
				#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
					printText(U"Declaring label ", labelIdentifier, U" @ ", targetAddress, U"\n");
				#endif
				this->methods[methodIndex].labels.pushConstruct(targetAddress, labelIdentifier);
			}
		} else {
			#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
				printText(U"Generating machine instructions for command \"", command, U"\" in method ", methodIndex, U"\n");
			#endif
			// If the command did not match with any of the generic instructions, resolve the arguments and look for a custom machine instruction.
			List<VMA> resolvedArguments;
			for (int32_t a = 0; a < arguments.length(); a++) {
				ReadableString content = string_removeOuterWhiteSpace(arguments[a]);
				if (string_length(content) > 0) {
					#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
						printText(U"	Resolving \"", content, U"\" in method ", methodIndex, U"\n");
					#endif
					resolvedArguments.push(this->VMAfromText(unresolvedProgram, unresolvedMethod, methodIndex, initMethodIndex, getArg(arguments, a), global));
				}
			}
			this->interpretCommand(methodIndex, command, resolvedArguments);
		}
	}

	// Run-time debug printing
	#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
		Variable<TYPE_COUNT>* getDebugInfo(DataType dataType, int32_t globalIndex, int32_t methodIndex) {
			if (globalIndex < 0) { methodIndex = 0; } // Go to the global method if it's a global index
			Method<TYPE_COUNT>* method = &this->methods[methodIndex];
			int32_t typeLocalIndex = globalToTypeLocalIndex(globalIndex);
			int32_t unifiedLocalIndex = method->unifiedLocalIndices[dataType][typeLocalIndex];
			return &(method->locals[unifiedLocalIndex]);
		}
		void debugArgument(const VMA& data, int32_t methodIndex, const FixedArray<int32_t, TYPE_COUNT>& framePointer, bool fullContent) {
			if (data.argType == ArgumentType::Immediate) {
				// TODO: Should debug printing be done here or in the implementation that actually knows the types?
				printText(U"Immediate index ", data.index);
			} else if (data.argType == ArgumentType::InstructionAddress) {
				printText(U"Instruction ", data.index);
			} else if (data.argType == ArgumentType::MethodReference) {
				printText(U"Method ", data.index);
			} else {
				int32_t globalIndex = data.index;
				Variable<TYPE_COUNT>* variable = getDebugInfo(data.dataType, globalIndex, methodIndex);
				const VMTypeDef<TYPE_COUNT>* typeDefinition = getMachineType(data.dataType);
				#ifndef VIRTUAL_MACHINE_DEBUG_FULL_CONTENT
					fullContent = false;
				#endif
				if (typeDefinition) {
					typeDefinition->debugPrinter(*(this->memory.getUnsafe()), *variable, globalIndex, framePointer, fullContent);
					if (globalIndex < 0) {
						printText(U" @gi(", globalIndex, U")");
					} else {
						printText(U" @gi(", globalIndex, U") + fp(", framePointer[typeDefinition->dataType], U")");
					}
				} else {
					printText(U"?");
				}
			}
		}
		void debugPrintVariables(int32_t methodIndex, const FixedArray<int32_t, TYPE_COUNT>& framePointer, const ReadableString& indentation) {
			Method<TYPE_COUNT>* method = &this->methods[methodIndex];
			for (int32_t i = 0; i < method->locals.length(); i++) {
				Variable<TYPE_COUNT>* variable = &method->locals[i];
				printText(indentation, U"* ", getName(variable->access), U" ");
				const VMTypeDef<TYPE_COUNT>* typeDefinition = getMachineType(variable->typeDescription->dataType);
				if (typeDefinition) {
					typeDefinition->debugPrinter(*(this->memory.getUnsafe()), *variable, variable->getGlobalIndex(), framePointer, false);
				} else {
					printText(U"?");
				}
				printText(U"\n");
			}
		}
		void debugPrintMethod(int32_t methodIndex, const FixedArray<int32_t, TYPE_COUNT>& framePointer, const FixedArray<int32_t, TYPE_COUNT>& stackPointer, const ReadableString& indentation) {
			printText("  ", this->methods[methodIndex].name, ":\n");
			for (int32_t t = 0; t < this->machineTypeCount; t++) {
				printText(U"    FramePointer[", t, "] = ", framePointer[t], U" Count[", t, "] = ", this->methods[methodIndex].count[t], U" StackPointer[", t, "] = ", stackPointer[t], U"\n");
			}
			debugPrintVariables(methodIndex, framePointer, indentation);
			printText(U"\n");
		}
		void debugPrintMemory() {
			int32_t methodIndex = this->memory->current.methodIndex;
			printText(U"\nMemory:\n");
			// Global memory is at the bottom of the stack.
			FixedArray<int32_t, TYPE_COUNT> globalFramePointer;
			debugPrintMethod(0, globalFramePointer, this->methods[0].count, U"    ");
			// Stack memory for each calling method.
			for (int32_t i = 0; i < memory->callStack.length(); i++) {
				debugPrintMethod(memory->callStack[i].methodIndex, memory->callStack[i].framePointer, memory->callStack[i].stackPointer, U"    ");
			}
			// Stack memory for the current method, which is not in the call stack because that would be slow to access.
			debugPrintMethod(methodIndex, this->memory->current.framePointer, this->memory->current.stackPointer, U"    ");
		}
	#endif
	void executeMethod(int32_t methodIndex) {
		if (methodIndex < 0 || methodIndex >= this->methods.length()) {
			throwError(U"Can not call a method of index ", methodIndex, U" using executeMethod, because it is out of bound 0..", this->methods.length() - 1, U"!\n");
		}
		Method<TYPE_COUNT>* rootMethod = &this->methods[methodIndex];

		#ifdef VIRTUAL_MACHINE_PROFILE
			if (rootMethod->instructionCount < 1) {
				// TODO: Assert that each method ends with a return or jump instruction after compiling
				printText(U"Cannot call \"", rootMethod->name, U"\", because it doesn't have any instructions.\n");
				return;
			}
		#endif

		// Create a new current state
		this->memory->current.methodIndex = methodIndex;
		this->memory->current.programCounter = rootMethod->startAddress;
		for (int32_t t = 0; t < this->machineTypeCount; t++) {
			int32_t framePointer = this->methods[0].count[t];
			this->memory->current.framePointer[t] = framePointer;
			this->memory->current.stackPointer[t] = framePointer + this->methods[methodIndex].count[t];
		}

		#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
			this->debugPrintMemory();
		#endif
		#ifdef VIRTUAL_MACHINE_PROFILE
			printText(U"Calling \"", rootMethod->name, U"\":\n");
			double startTime = time_getSeconds();
		#endif

		// Execute until the program counter is out of bound (-1)
		while (true) {
			int32_t pc = this->memory->current.programCounter;
			if (pc < 0 || pc >= this->machineWords.length()) {
				// Return statements will set the program counter to -1 if there are no more callers saved in the stack
				if (pc != -1) {
					throwError(U"Unexpected program counter! @", pc, U" outside of 0..", (this->machineWords.length() - 1), U"\n");
				}
				break;
			}
			MachineWord<TYPE_COUNT>* word = &this->machineWords[pc];
			#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
				const InsSig<TYPE_COUNT>* signature = getMachineInstructionFromFunction(word->operation);
				if (signature) {
					printText(U"Executing @", pc, U" ", signature->name, U"(");
					for (int32_t a = signature->targetCount; a < word->args.length(); a++) {
						if (a > signature->targetCount) {
							printText(U", ");
						}
						debugArgument(word->args[a], this->memory->current.methodIndex, this->memory->current.framePointer, false);
					}
					printText(U")");
				}
				word->operation(*this, this->memory.getReference(), word->args);
				if (signature) {
					if (signature->targetCount > 0) {
						printText(U" -> ");
						for (int32_t a = 0; a < signature->targetCount; a++) {
							if (a > 0) {
								printText(U", ");
							}
							debugArgument(word->args[a], this->memory->current.methodIndex, this->memory->current.framePointer, true);
						}
					}
				}
				printText(U"\n");
			#else
				word->operation(*this, this->memory.getReference(), word->args);
			#endif
		}
		#ifdef VIRTUAL_MACHINE_PROFILE
			double endTime = time_getSeconds();
			printText(U"Done calling \"", rootMethod->name, U"\" after ", (endTime - startTime) * 1000000.0, U" microseconds.\n");
			#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
				printText(U" (debug prints are active)\n");
			#endif
		#endif
	}
};

}

#endif
