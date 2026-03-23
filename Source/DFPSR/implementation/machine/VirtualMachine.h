// zlib open source license
//
// Copyright (c) 2019 David Forsgren Piuva
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

#ifndef DFPSR_VIRTUAL_MACHINE
#define DFPSR_VIRTUAL_MACHINE

#include <cstdint>
#include "../../math/FixedPoint.h"
#include "../../collection/FixedArray.h"
#include "../../collection/Array.h"
#include "../../collection/List.h"
#include "../../api/timeAPI.h"

// Flags
//#define VIRTUAL_MACHINE_PROFILE // Enable profiling
//#define VIRTUAL_MACHINE_DEBUG_PRINT // Enable debug printing (will affect profiling)
//#define VIRTUAL_MACHINE_DEBUG_FULL_CONTENT // Allow debug printing to show the full content of images

namespace dsr {

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
	Immediate,
	Reference
};

// Types
// TODO: Make the use of FixedPoint optional in VirtualMachine
using DataType = int32_t;
static const DataType DataType_FixedPoint = 0;

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
	const ArgumentType argType;
	const DataType dataType;
	const FixedPoint value;
	explicit VMA(FixedPoint value)
	: argType(ArgumentType::Immediate), dataType(DataType_FixedPoint), value(value) {}
	VMA(DataType dataType, int32_t globalIndex)
	: argType(ArgumentType::Reference), dataType(dataType), value(FixedPoint::fromMantissa(globalIndex)) {}
};

struct ArgSig {
	ReadableString name;
	bool byValue;
	DataType dataType;
	ArgSig(const ReadableString& name, bool byValue, DataType dataType)
	: name(name), byValue(byValue), dataType(dataType) {}
	bool matches(ArgumentType argType, DataType dataType) const {
		if (this->byValue && this->dataType == DataType_FixedPoint) {
			return dataType == this->dataType && (argType == ArgumentType::Immediate || argType == ArgumentType::Reference);
		} else {
			return dataType == this->dataType && argType == ArgumentType::Reference;
		}
	}
};

template <typename T>
struct MemoryPlane {
	Array<T> stack;
	explicit MemoryPlane(int32_t size) : stack(size, T()) {}
	T& accessByStackIndex(int32_t stackIndex) {
		return this->stack[stackIndex];
	}
	// globalIndex uses the negative values starting from -1 to access global memory, and from 0 and up to access local variables on top of the type's own frame pointer.
	T& accessByGlobalIndex(int32_t globalIndex, int32_t framePointer) {
		int32_t stackIndex = (globalIndex < 0) ? -(globalIndex + 1) : (framePointer + globalIndex);
		return this->stack[stackIndex];
	}
	T& getRef(const VMA& arg, int32_t framePointer) {
		assert(arg.argType == ArgumentType::Reference);
		return this->accessByGlobalIndex(arg.value.getMantissa(), framePointer);
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
//       Because there cannot be accidental reintepretation when the type is known in compile-time.
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
	MachineOperation<TYPE_COUNT> operation;
	List<VMA> args;
	MachineWord(MachineOperation<TYPE_COUNT> operation, const List<VMA>& args)
	: operation(operation), args(args) {}
	explicit MachineWord(MachineOperation<TYPE_COUNT> operation)
	: operation(operation) {}
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
using VMT_Initializer = decltype(&initializeTemplate<TYPE_COUNT>);

template <int32_t TYPE_COUNT>
inline static void debugPrintTemplate(PlanarMemory<TYPE_COUNT>& memory, Variable<TYPE_COUNT>& variable, int32_t globalIndex, int32_t* framePointer, bool fullContent) {}
template <int32_t TYPE_COUNT>
using VMT_DebugPrinter = decltype(&debugPrintTemplate<TYPE_COUNT>);

template <int32_t TYPE_COUNT>
struct VMTypeDef {
	ReadableString name;
	DataType dataType;
	bool allowDefaultValue;
	VMT_Initializer<TYPE_COUNT> initializer;
	VMT_DebugPrinter<TYPE_COUNT> debugPrinter;
	VMTypeDef(const ReadableString& name, DataType dataType, bool allowDefaultValue, VMT_Initializer<TYPE_COUNT> initializer, VMT_DebugPrinter<TYPE_COUNT> debugPrinter)
	: name(name), dataType(dataType), allowDefaultValue(allowDefaultValue), initializer(initializer), debugPrinter(debugPrinter) {}
};

template <int32_t TYPE_COUNT>
struct Method {
	String name;

	// Global instruction space
	const int32_t startAddress = 0; // Index to machineWords
	int32_t instructionCount = 0; // Number of machine words (safer than return statements in case of memory corruption)

	// Unified local space
	int32_t inputCount = 0; // Number of inputs declared at the start of locals
	int32_t outputCount = 0; // Number of output declared directly after the inputs

	// TODO: Merge into a state
	bool declaredNonInput = false; // Goes true when a non-input is declared
	bool declaredLocals = false; // Goes true when a local is declared
	List<Variable<TYPE_COUNT>> locals; // locals[0..inputCount-1] are the inputs, while locals[inputCount..inputCount+outputCount-1] are the outputs

	// Type-specific spaces
	FixedArray<int32_t, TYPE_COUNT> count;
	// Look-up table from a combination of type and type-local indices to unified-local indices
	FixedArray<List<int32_t>, TYPE_COUNT> unifiedLocalIndices;

	Method(const String& name, int32_t startAddress, int32_t machineTypeCount) : name(name), startAddress(startAddress) {
		// Increase TYPE_COUNT if it's not enough
		assert(machineTypeCount <= TYPE_COUNT);
	}
	Variable<TYPE_COUNT>* getLocal(const ReadableString& name) {
		for (int32_t i = 0; i < this->locals.length(); i++) {
			if (string_caseInsensitiveMatch(this->locals[i].name, name)) {
				return &this->locals[i];
			}
		}
		return nullptr;
	}
};

// A virtual machine for efficient media processing.
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
	  const VMTypeDef<TYPE_COUNT>* machineTypes, int32_t machineTypeCount)
	: memory(memory), machineInstructions(machineInstructions), machineInstructionCount(machineInstructionCount),
	  machineTypes(machineTypes), machineTypeCount(machineTypeCount) {
		#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
			printText(U"Starting media machine.\n");
		#endif
		this->methods.pushConstruct(U"<init>", 0, this->machineTypeCount);
		#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
			printText(U"Reading assembly.\n");
		#endif
		string_split_callback([this](ReadableString currentLine) {
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
				List<String> arguments = string_split(argumentLine, U',', true);
				this->interpretMachineWord(command, arguments);
			} else if (string_length(currentLine) > 0) {
				throwError(U"Unexpected line \"", currentLine, U"\".\n");
			}
		}, code, U'\n');
		// Calling "<init>" to execute global commands
		#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
			printText(U"Initializing global machine state.\n");
		#endif
		this->executeMethod(0);
	}

	int32_t findMethod(const ReadableString& name) {
		for (int32_t i = 0; i < this->methods.length(); i++) {
			if (string_caseInsensitiveMatch(this->methods[i].name, name)) {
				return i;
			}
		}
		return -1;
	}

	Variable<TYPE_COUNT>* getResource(const ReadableString& name, int32_t methodIndex) {
		Variable<TYPE_COUNT>* result = this->methods[methodIndex].getLocal(name);
		if (result) {
			// If found, take the local variable
			return result;
		} else if (methodIndex > 0) {
			// If not found but having another scope, look for global variables in the global initiation method
			return getResource(name, 0);
		} else {
			return nullptr;
		}
	}

	/*
	Indices
		Global index: (Identifier) The value stores in the mantissas of machine instructions to refer to things
			These are translated into stack indices for run-time lookups
			Useful for storing in compile-time when there's no stack nor frame-pointer for mapping to any real memory address
			Relative to the frame-pointer, so it cannot access anything else then globals (using negative indices) and locals (using natural indices)
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
	*/
	static int32_t globalToTypeLocalIndex(int32_t globalIndex) {
		return globalIndex < 0 ? -(globalIndex + 1) : globalIndex;
	}
	static int32_t typeLocalToGlobalIndex(bool isGlobal, int32_t typeLocalIndex) {
		return isGlobal ? -(typeLocalIndex + 1) : typeLocalIndex;
	}

	void addMachineWord(MachineOperation<TYPE_COUNT> operation, const List<VMA>& args) {
		this->machineWords.pushConstruct(operation, args);
		this->methods[this->methods.length() - 1].instructionCount++;
	}

	void addMachineWord(MachineOperation<TYPE_COUNT> operation) {
		this->machineWords.pushConstruct(operation);
		this->methods[this->methods.length() - 1].instructionCount++;
	}

	void addReturnInstruction() {
		addMachineWord([](VirtualMachine& machine, PlanarMemory<TYPE_COUNT>& memory, const List<VMA>& args) {
			if (memory.callStack.length() > 0) {
				// Return to caller
				#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
					printText(U"Returning from \"", machine.methods[memory.current.methodIndex].name, U"\" to caller \"", machine.methods[memory.callStack.last().methodIndex].name, U"\"\n");
					machine.debugPrintMemory();
				#endif
				memory.current = memory.callStack.last();
				memory.callStack.pop();
				memory.current.programCounter++;
			} else {
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

	void addCallInstructions(const List<String>& arguments) {
		if (arguments.length() < 1) {
			throwError(U"Cannot make a call without the name of a method!\n");
		}
		// TODO: Allow calling methods that aren't defined yet.
		int32_t currentMethodIndex = this->methods.length() - 1;
		ReadableString methodName = string_removeOuterWhiteSpace(arguments[0]);
		int32_t calledMethodIndex = findMethod(methodName);
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
		inputArguments.push(VMA(FixedPoint::fromMantissa(calledMethodIndex)));
		outputArguments.push(VMA(FixedPoint::fromMantissa(calledMethodIndex)));
		int32_t outputCount = 0;
		for (int32_t a = 1; a < arguments.length(); a++) {
			ReadableString content = string_removeOuterWhiteSpace(arguments[a]);
			if (string_length(content) > 0) {
				if (outputCount < calledMethod->outputCount) {
					outputArguments.push(this->VMAfromText(currentMethodIndex, getArg(arguments, a)));
					outputCount++;
				} else {
					inputArguments.push(this->VMAfromText(currentMethodIndex, getArg(arguments, a)));
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
		addMachineWord([](VirtualMachine& machine, PlanarMemory<TYPE_COUNT>& memory, const List<VMA>& args) {
			// Get the method to call
			int32_t calledMethodIndex = args[0].value.getMantissa();
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
			// Assign inputs
			for (int32_t a = 1; a < args.length(); a++) {
				Variable<TYPE_COUNT>* target = &calledMethod->locals[a - 1];
				DataType typeIndex = target->typeDescription->dataType;
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
		addMachineWord([](VirtualMachine& machine, PlanarMemory<TYPE_COUNT>& memory, const List<VMA>& args) {
			int32_t calledMethodIndex = args[0].value.getMantissa();
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
					machine.debugArgument(VMA(typeIndex, source->getGlobalIndex()), calledMethodIndex, memory.current.stackPointer, false);
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

	void interpretCommand(const ReadableString& operation, const List<VMA>& resolvedArguments) {
		// Compare the input with overloads
		for (int32_t s = 0; s < machineInstructionCount; s++) {
			if (machineInstructions[s].matches(operation, resolvedArguments)) {
				this->addMachineWord(machineInstructions[s].operation, resolvedArguments);
				return;
			}
		}
		// TODO: Allow asking the specific machine type what the given types are called.
		String message = string_combine(U"\nError! ", operation, U" does not match any overload for the given arguments:\n");
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
	Variable<TYPE_COUNT>* declareVariable_aux(const VMTypeDef<TYPE_COUNT>& typeDef, int32_t methodIndex, AccessType access, const ReadableString& name, bool initialize, const ReadableString& defaultValueText) {
		// Make commonly used data more readable
		bool global = methodIndex == 0;
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
		if (initialize && access != AccessType::Input) {
			// Generate instructions for assigning the variable's initial value
			typeDef.initializer(*this, globalIndex, defaultValueText);
		}
		return &this->methods[methodIndex].locals.last();
	}

	Variable<TYPE_COUNT>* declareVariable(int32_t methodIndex, AccessType access, const ReadableString& typeName, const ReadableString& name, bool initialize, const ReadableString& defaultValueText) {
		if (this->getResource(name, methodIndex)) {
			throwError(U"A resource named \"", name, U"\" already exists! Be aware that resource names are case insensitive.\n");
			return nullptr;
		} else {
			// Loop over type definitions to find a match
			const VMTypeDef<TYPE_COUNT>* typeDef = getMachineType(typeName);
			if (typeDef) {
				if (string_length(defaultValueText) > 0 && !typeDef->allowDefaultValue) {
					throwError(U"The variable \"", name, U"\" doesn't have an immediate constructor for \"", typeName, U"\".\n");
				}
				return this->declareVariable_aux(*typeDef, methodIndex, access, name, initialize, defaultValueText);
			} else {
				throwError(U"Cannot declare variable of unknown type \"", typeName, U"\"!\n");
				return nullptr;
			}
		}
	}

	VMA VMAfromText(int32_t methodIndex, const ReadableString& content) {
		DsrChar first = content[0];
		DsrChar second = content[1];
		if (first == U'-' && second >= U'0' && second <= U'9') {
			return VMA(FixedPoint::fromText(content));
		} else if (first >= U'0' && first <= U'9') {
			return VMA(FixedPoint::fromText(content));
		} else {
			int32_t leftIndex = string_findFirst(content, U'<');
			int32_t rightIndex = string_findLast(content, U'>');
			if (leftIndex > -1 && rightIndex > -1) {
				ReadableString name = string_removeOuterWhiteSpace(string_before(content, leftIndex));
				ReadableString typeName = string_removeOuterWhiteSpace(string_inclusiveRange(content, leftIndex + 1, rightIndex - 1));
				ReadableString remainder = string_removeOuterWhiteSpace(string_after(content, rightIndex));
				if (string_length(remainder) > 0) {
					throwError(U"No code allowed after > for in-place temp declarations!\n");
				}
				Variable<TYPE_COUNT>* resource = this->declareVariable(methodIndex, AccessType::Hidden, typeName, name, false, U"");
				if (resource) {
					return VMA(resource->typeDescription->dataType, resource->getGlobalIndex());
				} else {
					throwError(U"The resource \"", name, U"\" could not be declared as \"", typeName, U"\"!\n");
					return VMA(FixedPoint());
				}
			} else if (leftIndex > -1) {
				throwError(U"Using < without > for in-place temp allocation.\n");
				return VMA(FixedPoint());
			} else if (rightIndex > -1) {
				throwError(U"Using > without < for in-place temp allocation.\n");
				return VMA(FixedPoint());
			} else {
				Variable<TYPE_COUNT>* resource = getResource(content, methodIndex);
				if (resource) {
					return VMA(resource->typeDescription->dataType, resource->getGlobalIndex());
				} else {
					throwError(U"The resource \"", content, U"\" could not be found! Make sure that it's declared before being used.\n");
					return VMA(FixedPoint());
				}
			}
		}
	}

	void interpretMachineWord(const ReadableString& command, const List<String>& arguments) {
		#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
			printText(U"interpretMachineWord @", this->machineWords.length(), U" ", command, U"(");
			for (int32_t a = 0; a < arguments.length(); a++) {
				if (a > 0) { printText(U", "); }
				printText(getArg(arguments, a));
			}
			printText(U")\n");
		#endif
		if (string_caseInsensitiveMatch(command, U"Begin")) {
			if (this->methods.length() == 1) {
				// When more than one function exists, the init method must end with a return instruction
				//   Otherwise it would start executing instructions in another method and crash
				this->addReturnInstruction();
			}
			this->methods.pushConstruct(getArg(arguments, 0), this->machineWords.length(), this->machineTypeCount);
		} else if (string_caseInsensitiveMatch(command, U"Temp")) {
			for (int32_t a = 1; a < arguments.length(); a++) {
				this->declareVariable(methods.length() - 1, AccessType::Hidden, getArg(arguments, 0), getArg(arguments, a), false, U"");
			}
		} else if (string_caseInsensitiveMatch(command, U"Hidden")) {
			this->declareVariable(methods.length() - 1, AccessType::Hidden, getArg(arguments, 0), getArg(arguments, 1), true, getArg(arguments, 2));
		} else if (string_caseInsensitiveMatch(command, U"Input")) {
			this->declareVariable(methods.length() - 1, AccessType::Input, getArg(arguments, 0), getArg(arguments, 1), true, getArg(arguments, 2));
		} else if (string_caseInsensitiveMatch(command, U"Output")) {
			this->declareVariable(methods.length() - 1, AccessType::Output, getArg(arguments, 0), getArg(arguments, 1), true, getArg(arguments, 2));
		} else if (string_caseInsensitiveMatch(command, U"End")) {
			this->addReturnInstruction();
		} else if (string_caseInsensitiveMatch(command, U"Call")) {
			this->addCallInstructions(arguments);
		} else {
			int32_t methodIndex = this->methods.length() - 1;
			List<VMA> resolvedArguments;
			for (int32_t a = 0; a < arguments.length(); a++) {
				ReadableString content = string_removeOuterWhiteSpace(arguments[a]);
				if (string_length(content) > 0) {
					resolvedArguments.push(this->VMAfromText(methodIndex, getArg(arguments, a)));
				}
			}
			this->interpretCommand(command, resolvedArguments);
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
		void debugArgument(const VMA& data, int32_t methodIndex, int32_t* framePointer, bool fullContent) {
			if (data.argType == ArgumentType::Immediate) {
				printText(data.value);
			} else {
				int32_t globalIndex = data.value.getMantissa();
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
		void debugPrintVariables(int32_t methodIndex, int32_t* framePointer, const ReadableString& indentation) {
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
		void debugPrintMethod(int32_t methodIndex, int32_t* framePointer, int32_t* stackPointer, const ReadableString& indentation) {
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
	int32_t getResourceStackIndex(const ReadableString& name, int32_t methodIndex, DataType dataType, AccessType access = AccessType::Any) {
		Variable<TYPE_COUNT>* variable = getResource(name, methodIndex);
		if (variable) {
			if (variable->typeDescription->dataType != dataType) {
				throwError(U"The machine's resource named \"", variable->name, U"\" had the unexpected type \"", variable->typeDescription->name, U"\"!\n");
			} else if (access != variable->access && access != AccessType::Any) {
				throwError(U"The machine's resource named \"", variable->name, U"\" is not delared as \"", getName(access), U"\"!\n");
			} else {
				return variable->getStackIndex(this->memory->current.framePointer[dataType]);
			}
		} else {
			throwError(U"The machine cannot find any resource named \"", name, U"\"!\n");
		}
		return -1;
	}
};

}

#endif
