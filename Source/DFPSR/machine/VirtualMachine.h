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

#ifndef DFPSR_VIRTUAL_MACHINE
#define DFPSR_VIRTUAL_MACHINE

#include <stdint.h>
#include "../math/FixedPoint.h"
#include "../collection/Array.h"
#include "../collection/List.h"

// Flags
//#define VIRTUAL_MACHINE_PROFILE // Enable profiling
//#define VIRTUAL_MACHINE_DEBUG_PRINT // Enable debug printing (will affect profiling)
//#define VIRTUAL_MACHINE_DEBUG_FULL_CONTENT // Allow debug printing to show the full content of images

namespace dsr {

// TODO: Can this be a template argument?
#define MAX_TYPE_COUNT 3

// Forward declarations
struct VirtualMachine;
struct VMTypeDef;

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
	Unused,
	Immediate,
	Reference
};

// Types
// TODO: Make the use of FixedPoint optional in VirtualMachine
using DataType = int32_t;
static const DataType DataType_FixedPoint = 0;

struct Variable {
	String name;
	AccessType access;
	const VMTypeDef* typeDescription;
	int32_t typeLocalIndex; // The zero-based local index among the members of the same type in the method
	bool global; // A flag that generates negative global indices for referring to global variables in method zero
	Variable(const String& name, AccessType access, const VMTypeDef* typeDescription, int32_t typeLocalIndex, bool global)
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
	const ArgumentType argType = ArgumentType::Unused;
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
	// TODO: Replace with pointers to type definitions (const VMTypeDef*)
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

struct CallState {
	int32_t methodIndex = 0;
	int32_t programCounter = 0;
	int32_t stackPointer[MAX_TYPE_COUNT] = {};
	int32_t framePointer[MAX_TYPE_COUNT] = {};
};

// A planar memory system with one stack and frame pointer for each type of memory.
//   This is possible because the virtual machine only operates on types known in compile-time.
//   The planar stack system:
//     * Removes the need to manually initialize and align classes in generic memory.
	//     * Encapsulates any effects of endianness or signed integer representations in the physical hardware.
//       Because there cannot be accidental reintepretation when the type is known in compile-time.
class PlanarMemory {
public:
	CallState current;
	List<CallState> callStack;
	virtual ~PlanarMemory() {}
	// Store in memory
	virtual void store(int targetStackIndex, const VMA& sourceArg, int sourceFramePointer, DataType type) = 0;
	// Load from memory
	virtual void load(int sourceStackIndex, const VMA& targetArg, int targetFramePointer, DataType type) = 0;
};

// Lambdas without capture is used to create function pointers without objects
inline void MachineOperationTemplate(VirtualMachine& machine, PlanarMemory&, const List<VMA>& args) {}
using MachineOperation = decltype(&MachineOperationTemplate);

struct MachineWord {
	MachineOperation operation;
	List<VMA> args;
	MachineWord(MachineOperation operation, const List<VMA>& args)
	: operation(operation), args(args) {}
	explicit MachineWord(MachineOperation operation)
	: operation(operation) {}
};

struct InsSig {
public:
	ReadableString name;
	int targetCount; // Number of first arguments to present as results
	List<ArgSig> arguments;
	MachineOperation operation;
	InsSig(const ReadableString& name, int targetCount, MachineOperation operation)
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
	static InsSig create(const ReadableString& name, int targetCount, MachineOperation operation, ARGS... args) {
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
			for (int i = 0; i < this->arguments.length(); i++) {
				if (!this->arguments[i].matches(resolvedArguments[i].argType, resolvedArguments[i].dataType)) {
					return false;
				}
			}
			return true;
		}
	}
};

// Types
inline void initializeTemplate(VirtualMachine& machine, int globalIndex, const ReadableString& defaultValue) {}
using VMT_Initializer = decltype(&initializeTemplate);
inline void debugPrintTemplate(PlanarMemory& memory, Variable& variable, int globalIndex, int32_t* framePointer, bool fullContent) {}
using VMT_DebugPrinter = decltype(&debugPrintTemplate);
struct VMTypeDef {
	ReadableString name;
	DataType dataType;
	bool allowDefaultValue;
	VMT_Initializer initializer;
	VMT_DebugPrinter debugPrinter;
	VMTypeDef(const ReadableString& name, DataType dataType, bool allowDefaultValue, VMT_Initializer initializer, VMT_DebugPrinter debugPrinter)
	: name(name), dataType(dataType), allowDefaultValue(allowDefaultValue), initializer(initializer), debugPrinter(debugPrinter) {}
};

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
	List<Variable> locals; // locals[0..inputCount-1] are the inputs, while locals[inputCount..inputCount+outputCount-1] are the outputs

	// Type-specific spaces
	int32_t count[MAX_TYPE_COUNT] = {};
	// Look-up table from a combination of type and type-local indices to unified-local indices
	List<int32_t> unifiedLocalIndices[MAX_TYPE_COUNT];

	Method(const String& name, int32_t startAddress, int32_t machineTypeCount) : name(name), startAddress(startAddress) {
		// Increase MAX_TYPE_COUNT if it's not enough
		assert(machineTypeCount <= MAX_TYPE_COUNT);
	}
	Variable* getLocal(const ReadableString& name) {
		for (int i = 0; i < this->locals.length(); i++) {
			if (string_caseInsensitiveMatch(this->locals[i].name, name)) {
				return &this->locals[i];
			}
		}
		return nullptr;
	}
};

// A virtual machine for efficient media processing.
struct VirtualMachine {
	// Methods
	List<Method> methods;
	// Memory
	std::shared_ptr<PlanarMemory> memory;
	// Instruction types
	const InsSig* machineInstructions; int32_t machineInstructionCount;
	const InsSig* getMachineInstructionFromFunction(MachineOperation functionPointer) {
		for (int s = 0; s < this->machineInstructionCount; s++) {
			if (this->machineInstructions[s].operation == functionPointer) {
				return &this->machineInstructions[s];
			}
		}
		return nullptr;
	}
	// Instruction instances
	List<MachineWord> machineWords;
	// Types
	const VMTypeDef* machineTypes; int32_t machineTypeCount;
	const VMTypeDef* getMachineType(const ReadableString& name) {
		for (int s = 0; s < this->machineTypeCount; s++) {
			if (string_caseInsensitiveMatch(this->machineTypes[s].name, name)) {
				return &this->machineTypes[s];
			}
		}
		return nullptr;
	}
	const VMTypeDef* getMachineType(DataType dataType) {
		for (int s = 0; s < this->machineTypeCount; s++) {
			if (this->machineTypes[s].dataType == dataType) {
				return &this->machineTypes[s];
			}
		}
		return nullptr;
	}
	// Constructor
	VirtualMachine(const ReadableString& code, const std::shared_ptr<PlanarMemory>& memory,
	  const InsSig* machineInstructions, int32_t machineInstructionCount,
	  const VMTypeDef* machineTypes, int32_t machineTypeCount);

	int findMethod(const ReadableString& name);
	Variable* getResource(const ReadableString& name, int methodIndex);
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
	static int globalToTypeLocalIndex(int globalIndex) {
		return globalIndex < 0 ? -(globalIndex + 1) : globalIndex;
	}
	static int typeLocalToGlobalIndex(bool isGlobal, int typeLocalIndex) {
		return isGlobal ? -(typeLocalIndex + 1) : typeLocalIndex;
	}

	void addMachineWord(MachineOperation operation, const List<VMA>& args);
	void addMachineWord(MachineOperation operation);
	void addReturnInstruction();
	void addCallInstructions(const List<String>& arguments);
	void interpretCommand(const ReadableString& operation, const List<VMA>& resolvedArguments);
	Variable* declareVariable_aux(const VMTypeDef& typeDef, int methodIndex, AccessType access, const ReadableString& name, bool initialize, const ReadableString& defaultValueText);
	Variable* declareVariable(int methodIndex, AccessType access, const ReadableString& type, const ReadableString& name, bool initialize, const ReadableString& defaultValueText);
	VMA VMAfromText(int methodIndex, const ReadableString& content);
	void interpretMachineWord(const ReadableString& command, const List<String>& arguments);

	// Run-time debug printing
	#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
		Variable* getDebugInfo(DataType dataType, int globalIndex, int methodIndex) {
			if (globalIndex < 0) { methodIndex = 0; } // Go to the global method if it's a global index
			Method* method = &this->methods[methodIndex];
			int typeLocalIndex = globalToTypeLocalIndex(globalIndex);
			int unifiedLocalIndex = method->unifiedLocalIndices[dataType][typeLocalIndex];
			return &(method->locals[unifiedLocalIndex]);
		}
		void debugArgument(const VMA& data, int methodIndex, int32_t* framePointer, bool fullContent) {
			if (data.argType == ArgumentType::Immediate) {
				printText(data.value);
			} else {
				int globalIndex = data.value.getMantissa();
				Variable* variable = getDebugInfo(data.dataType, globalIndex, methodIndex);
				const VMTypeDef* typeDefinition = getMachineType(data.dataType);
				#ifndef VIRTUAL_MACHINE_DEBUG_FULL_CONTENT
					fullContent = false;
				#endif
				if (typeDefinition) {
					typeDefinition->debugPrinter(*(this->memory.get()), *variable, globalIndex, framePointer, fullContent);
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
		void debugPrintVariables(int methodIndex, int32_t* framePointer, const ReadableString& indentation) {
			Method* method = &this->methods[methodIndex];
			for (int i = 0; i < method->locals.length(); i++) {
				Variable* variable = &method->locals[i];
				printText(indentation, U"* ", getName(variable->access), U" ");
				const VMTypeDef* typeDefinition = getMachineType(variable->typeDescription->dataType);
				if (typeDefinition) {
					typeDefinition->debugPrinter(*(this->memory.get()), *variable, variable->getGlobalIndex(), framePointer, false);
				} else {
					printText(U"?");
				}
				printText(U"\n");
			}
		}
		void debugPrintMethod(int methodIndex, int32_t* framePointer, int32_t* stackPointer, const ReadableString& indentation) {
			printText("  ", this->methods[methodIndex].name, ":\n");
			for (int t = 0; t < this->machineTypeCount; t++) {
				printText(U"    FramePointer[", t, "] = ", framePointer[t], U" Count[", t, "] = ", this->methods[methodIndex].count[t], U" StackPointer[", t, "] = ", stackPointer[t], U"\n");
			}
			debugPrintVariables(methodIndex, framePointer, indentation);
			printText(U"\n");
		}
		void debugPrintMemory() {
			int methodIndex = this->memory->current.methodIndex;
			printText(U"\nMemory:\n");
			// Global memory is at the bottom of the stack.
			int32_t globalFramePointer[MAX_TYPE_COUNT] = {};
			debugPrintMethod(0, globalFramePointer, this->methods[0].count, U"    ");
			// Stack memory for each calling method.
			for (int i = 0; i < memory->callStack.length(); i++) {
				debugPrintMethod(memory->callStack[i].methodIndex, memory->callStack[i].framePointer, memory->callStack[i].stackPointer, U"    ");
			}
			// Stack memory for the current method, which is not in the call stack because that would be slow to access.
			debugPrintMethod(methodIndex, this->memory->current.framePointer, this->memory->current.stackPointer, U"    ");
		}
	#endif
	void executeMethod(int methodIndex);
	int32_t getResourceStackIndex(const ReadableString& name, int methodIndex, DataType dataType, AccessType access = AccessType::Any) {
		Variable* variable = getResource(name, methodIndex);
		if (variable) {
			if (variable->typeDescription->dataType != dataType) {
				throwError("The machine's resource named \"", variable->name, "\" had the unexpected type \"", variable->typeDescription->name, "\"!\n");
			} else if (access != variable->access && access != AccessType::Any) {
				throwError("The machine's resource named \"", variable->name, "\" is not delared as \"", getName(access), "\"!\n");
			} else {
				return variable->getStackIndex(this->memory->current.framePointer[dataType]);
			}
		} else {
			throwError("The machine cannot find any resource named \"", name, "\"!\n");
		}
		return -1;
	}
};

}

#endif
