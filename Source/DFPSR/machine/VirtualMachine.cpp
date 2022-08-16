﻿// zlib open source license
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

#include "VirtualMachine.h"
#include "../api/timeAPI.h"

using namespace dsr;

VirtualMachine::VirtualMachine(const ReadableString& code, const std::shared_ptr<PlanarMemory>& memory,
  const InsSig* machineInstructions, int32_t machineInstructionCount,
  const VMTypeDef* machineTypes, int32_t machineTypeCount)
: memory(memory), machineInstructions(machineInstructions), machineInstructionCount(machineInstructionCount),
  machineTypes(machineTypes), machineTypeCount(machineTypeCount) {
	#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
		printText("Starting media machine.\n");
	#endif
	this->methods.pushConstruct(U"<init>", 0, this->machineTypeCount);
	#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
		printText("Reading assembly.\n");
	#endif
	string_split_callback([this](ReadableString currentLine) {
		// If the line has a comment, then skip everything from #
		int commentIndex = string_findFirst(currentLine, U'#');
		if (commentIndex > -1) {
			currentLine = string_before(currentLine, commentIndex);
		}
		currentLine = string_removeOuterWhiteSpace(currentLine);
		int colonIndex = string_findFirst(currentLine, U':');
		if (colonIndex > -1) {
			ReadableString command = string_removeOuterWhiteSpace(string_before(currentLine, colonIndex));
			ReadableString argumentLine = string_after(currentLine, colonIndex);
			List<String> arguments = string_split(argumentLine, U',', true);
			this->interpretMachineWord(command, arguments);
		} else if (string_length(currentLine) > 0) {
			throwError("Unexpected line \"", currentLine, "\".\n");
		}
	}, code, U'\n');
	// Calling "<init>" to execute global commands
	#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
		printText("Initializing global machine state.\n");
	#endif
	this->executeMethod(0);
}

int VirtualMachine::findMethod(const ReadableString& name) {
	for (int i = 0; i < this->methods.length(); i++) {
		if (string_caseInsensitiveMatch(this->methods[i].name, name)) {
			return i;
		}
	}
	return -1;
}

Variable* VirtualMachine::getResource(const ReadableString& name, int methodIndex) {
	Variable* result = this->methods[methodIndex].getLocal(name);
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

void VirtualMachine::addMachineWord(MachineOperation operation, const List<VMA>& args) {
	this->machineWords.pushConstruct(operation, args);
	this->methods[this->methods.length() - 1].instructionCount++;
}

void VirtualMachine::addMachineWord(MachineOperation operation) {
	this->machineWords.pushConstruct(operation);
	this->methods[this->methods.length() - 1].instructionCount++;
}

void VirtualMachine::interpretCommand(const ReadableString& operation, const List<VMA>& resolvedArguments) {
	// Compare the input with overloads
	for (int s = 0; s < machineInstructionCount; s++) {
		if (machineInstructions[s].matches(operation, resolvedArguments)) {
			this->addMachineWord(machineInstructions[s].operation, resolvedArguments);
			return;
		}
	}
	// TODO: Allow asking the specific machine type what the given types are called.
	String message = string_combine(U"\nError! ", operation, U" does not match any overload for the given arguments:\n");
	for (int s = 0; s < machineInstructionCount; s++) {
		const InsSig* signature = &machineInstructions[s];
		if (string_caseInsensitiveMatch(signature->name, operation)) {
			string_append(message, "  * ", signature->name, "(");
			for (int a = 0; a < signature->arguments.length(); a++) {
				if (a > 0) {
					string_append(message, ", ");
				}
				const ArgSig* argument = &signature->arguments[a];
				string_append(message, argument->name);
			}
			string_append(message, ")\n");
		}
	}
	throwError(message);
}

// TODO: Inline into declareVariable
Variable* VirtualMachine::declareVariable_aux(const VMTypeDef& typeDef, int methodIndex, AccessType access, const ReadableString& name, bool initialize, const ReadableString& defaultValueText) {
	// Make commonly used data more readable
	bool global = methodIndex == 0;
	Method* currentMethod = &this->methods[methodIndex];

	// Assert correctness
	if (global && (access == AccessType::Input || access == AccessType::Output)) {
		throwError("Cannot declare inputs or outputs globally!\n");
	}

	// Count how many variables the method has of each type
	currentMethod->count[typeDef.dataType]++;
	this->methods[methodIndex].unifiedLocalIndices[typeDef.dataType].push(this->methods[methodIndex].locals.length());
	// Count inputs for calling the method
	if (access == AccessType::Input) {
		if (this->methods[methodIndex].declaredNonInput) {
			throwError("Cannot declare input \"", name, "\" after a non-input has been declared. Declare inputs, outputs and locals in order.\n");
		}
		this->methods[methodIndex].inputCount++;
	} else if (access == AccessType::Output) {
		if (this->methods[methodIndex].declaredLocals) {
			throwError("Cannot declare output \"", name, "\" after a local has been declared. Declare inputs, outputs and locals in order.\n");
		}
		this->methods[methodIndex].outputCount++;
		this->methods[methodIndex].declaredNonInput = true;
	} else if (access == AccessType::Hidden) {
		this->methods[methodIndex].declaredLocals = true;
		this->methods[methodIndex].declaredNonInput = true;
	}
	// Declare the variable so that code may find the type and index by name
	int typeLocalIndex = currentMethod->count[typeDef.dataType] - 1;
	int globalIndex = typeLocalToGlobalIndex(global, typeLocalIndex);
	this->methods[methodIndex].locals.pushConstruct(name, access, &typeDef, typeLocalIndex, global);
	if (initialize && access != AccessType::Input) {
		// Generate instructions for assigning the variable's initial value
		typeDef.initializer(*this, globalIndex, defaultValueText);
	}
	return &this->methods[methodIndex].locals.last();
}

Variable* VirtualMachine::declareVariable(int methodIndex, AccessType access, const ReadableString& typeName, const ReadableString& name, bool initialize, const ReadableString& defaultValueText) {
	if (this->getResource(name, methodIndex)) {
		throwError("A resource named \"", name, "\" already exists! Be aware that resource names are case insensitive.\n");
		return nullptr;
	} else {
		// Loop over type definitions to find a match
		const VMTypeDef* typeDef = getMachineType(typeName);
		if (typeDef) {
			if (string_length(defaultValueText) > 0 && !typeDef->allowDefaultValue) {
				throwError("The variable \"", name, "\" doesn't have an immediate constructor for \"", typeName, "\".\n");
			}
			return this->declareVariable_aux(*typeDef, methodIndex, access, name, initialize, defaultValueText);
		} else {
			throwError("Cannot declare variable of unknown type \"", typeName, "\"!\n");
			return nullptr;
		}
	}
}

VMA VirtualMachine::VMAfromText(int methodIndex, const ReadableString& content) {
	DsrChar first = content[0];
	DsrChar second = content[1];
	if (first == U'-' && second >= U'0' && second <= U'9') {
		return VMA(FixedPoint::fromText(content));
	} else if (first >= U'0' && first <= U'9') {
		return VMA(FixedPoint::fromText(content));
	} else {
		int leftIndex = string_findFirst(content, U'<');
		int rightIndex = string_findLast(content, U'>');
		if (leftIndex > -1 && rightIndex > -1) {
			ReadableString name = string_removeOuterWhiteSpace(string_before(content, leftIndex));
			ReadableString typeName = string_removeOuterWhiteSpace(string_inclusiveRange(content, leftIndex + 1, rightIndex - 1));
			ReadableString remainder = string_removeOuterWhiteSpace(string_after(content, rightIndex));
			if (string_length(remainder) > 0) {
				throwError("No code allowed after > for in-place temp declarations!\n");
			}
			Variable* resource = this->declareVariable(methodIndex, AccessType::Hidden, typeName, name, false, U"");
			if (resource) {
				return VMA(resource->typeDescription->dataType, resource->getGlobalIndex());
			} else {
				throwError("The resource \"", name, "\" could not be declared as \"", typeName, "\"!\n");
				return VMA(FixedPoint());
			}
		} else if (leftIndex > -1) {
			throwError("Using < without > for in-place temp allocation.\n");
			return VMA(FixedPoint());
		} else if (rightIndex > -1) {
			throwError("Using > without < for in-place temp allocation.\n");
			return VMA(FixedPoint());
		} else {
			Variable* resource = getResource(content, methodIndex);
			if (resource) {
				return VMA(resource->typeDescription->dataType, resource->getGlobalIndex());
			} else {
				throwError("The resource \"", content, "\" could not be found! Make sure that it's declared before being used.\n");
				return VMA(FixedPoint());
			}
		}
	}
}

static ReadableString getArg(const List<String>& arguments, int32_t index) {
	if (index < 0 || index >= arguments.length()) {
		return U"";
	} else {
		return string_removeOuterWhiteSpace(arguments[index]);
	}
}
void VirtualMachine::addReturnInstruction() {
	addMachineWord([](VirtualMachine& machine, PlanarMemory& memory, const List<VMA>& args) {
		if (memory.callStack.length() > 0) {
			// Return to caller
			#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
				printText("Returning from \"", machine.methods[memory.current.methodIndex].name, "\" to caller \"", machine.methods[memory.callStack.last().methodIndex].name, "\"\n");
				machine.debugPrintMemory();
			#endif
			memory.current = memory.callStack.last();
			memory.callStack.pop();
			memory.current.programCounter++;
		} else {
			#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
				printText("Returning from \"", machine.methods[memory.current.methodIndex].name, "\"\n");
			#endif
			// Leave the virtual machine
			memory.current.programCounter = -1;
		}
	});
}
void VirtualMachine::addCallInstructions(const List<String>& arguments) {
	if (arguments.length() < 1) {
		throwError("Cannot make a call without the name of a method!\n");
	}
	// TODO: Allow calling methods that aren't defined yet.
	int currentMethodIndex = this->methods.length() - 1;
	int calledMethodIndex = findMethod(string_removeOuterWhiteSpace(arguments[0]));
	// Check the total number of arguments
	Method* calledMethod = &this->methods[calledMethodIndex];
	if (arguments.length() - 1 != calledMethod->outputCount + calledMethod->inputCount) {
		throwError("Wrong argument count to \"", calledMethod->name, "\"! Call arguments should start with the method to call, continue with output references and end with inputs.\n");
	}
	// Split assembler arguments into separate input and output arguments for machine instructions
	List<VMA> inputArguments;
	List<VMA> outputArguments;
	inputArguments.push(VMA(FixedPoint::fromMantissa(calledMethodIndex)));
	outputArguments.push(VMA(FixedPoint::fromMantissa(calledMethodIndex)));
	int outputCount = 0;
	for (int a = 1; a < arguments.length(); a++) {
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
	for (int a = 1; a < outputArguments.length(); a++) {
		// Output
		Variable* variable = &calledMethod->locals[a - 1 + calledMethod->inputCount];
		if (outputArguments[a].argType != ArgumentType::Reference) {
			throwError("Output argument for \"", variable->name, "\" in \"", calledMethod->name, "\" must be a reference to allow writing its result!\n");
		} else if (outputArguments[a].dataType != variable->typeDescription->dataType) {
			throwError("Output argument for \"", variable->name, "\" in \"", calledMethod->name, "\" must have the type \"", variable->typeDescription->name, "\"!\n");
		}
	}
	for (int a = 1; a < inputArguments.length(); a++) {
		// Input
		Variable* variable = &calledMethod->locals[a - 1];
		if (inputArguments[a].dataType != variable->typeDescription->dataType) {
			throwError("Input argument for \"", variable->name, "\" in \"", calledMethod->name, "\" must have the type \"", variable->typeDescription->name, "\"!\n");
		}
	}
	addMachineWord([](VirtualMachine& machine, PlanarMemory& memory, const List<VMA>& args) {
		// Get the method to call
		int calledMethodIndex = args[0].value.getMantissa();
		int oldMethodIndex = memory.current.methodIndex;
		Method* calledMethod = &machine.methods[calledMethodIndex];
		#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
			printText("Calling \"", calledMethod->name, "\".\n");
		#endif
		// Calculate new frame pointers
		int32_t newFramePointer[MAX_TYPE_COUNT] = {};
		int32_t newStackPointer[MAX_TYPE_COUNT] = {};
		for (int t = 0; t < MAX_TYPE_COUNT; t++) {
			newFramePointer[t] = memory.current.stackPointer[t];
			newStackPointer[t] = memory.current.stackPointer[t] + machine.methods[calledMethodIndex].count[t];
		}
		// Assign inputs
		for (int a = 1; a < args.length(); a++) {
			Variable* target = &calledMethod->locals[a - 1];
			DataType typeIndex = target->typeDescription->dataType;
			int targetStackIndex = target->getStackIndex(newFramePointer[typeIndex]);
			memory.store(targetStackIndex, args[a], memory.current.framePointer[typeIndex], typeIndex);
		}
		// Jump into the method
		memory.callStack.push(memory.current);
		memory.current.methodIndex = calledMethodIndex;
		memory.current.programCounter = machine.methods[calledMethodIndex].startAddress;
		for (int t = 0; t < MAX_TYPE_COUNT; t++) {
			memory.current.framePointer[t] = newFramePointer[t];
			memory.current.stackPointer[t] = newStackPointer[t];
		}
	}, inputArguments);
	// Get results from the method
	addMachineWord([](VirtualMachine& machine, PlanarMemory& memory, const List<VMA>& args) {
		int calledMethodIndex = args[0].value.getMantissa();
		Method* calledMethod = &machine.methods[calledMethodIndex];
		#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
			printText("Writing results after call to \"", calledMethod->name, "\":\n");
		#endif
		// Assign outputs
		for (int a = 1; a < args.length(); a++) {
			Variable* source = &calledMethod->locals[a - 1 + calledMethod->inputCount];
			DataType typeIndex = source->typeDescription->dataType;
			int sourceStackIndex = source->getStackIndex(memory.current.stackPointer[typeIndex]);
			memory.load(sourceStackIndex, args[a], memory.current.framePointer[typeIndex], typeIndex);
			#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
				printText("  ");
				machine.debugArgument(VMA(typeIndex, source->getGlobalIndex()), calledMethodIndex, memory.current.stackPointer, false);
				printText(" -> ");
				machine.debugArgument(args[a], memory.current.methodIndex, memory.current.framePointer, false);
				printText("\n");
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

void VirtualMachine::interpretMachineWord(const ReadableString& command, const List<String>& arguments) {
	#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
		printText("interpretMachineWord @", this->machineWords.length(), " ", command, "(");
		for (int a = 0; a < arguments.length(); a++) {
			if (a > 0) { printText(", "); }
			printText(getArg(arguments, a));
		}
		printText(")\n");
	#endif
	if (string_caseInsensitiveMatch(command, U"Begin")) {
		if (this->methods.length() == 1) {
			// When more than one function exists, the init method must end with a return instruction
			//   Otherwise it would start executing instructions in another method and crash
			this->addReturnInstruction();
		}
		this->methods.pushConstruct(getArg(arguments, 0), this->machineWords.length(), this->machineTypeCount);
	} else if (string_caseInsensitiveMatch(command, U"Temp")) {
		for (int a = 1; a < arguments.length(); a++) {
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
		int methodIndex = this->methods.length() - 1;
		List<VMA> resolvedArguments;
		for (int a = 0; a < arguments.length(); a++) {
			ReadableString content = string_removeOuterWhiteSpace(arguments[a]);
			if (string_length(content) > 0) {
				resolvedArguments.push(this->VMAfromText(methodIndex, getArg(arguments, a)));
			}
		}
		this->interpretCommand(command, resolvedArguments);
	}
}

void VirtualMachine::executeMethod(int methodIndex) {
	Method* rootMethod = &this->methods[methodIndex];

	#ifdef VIRTUAL_MACHINE_PROFILE
		if (rootMethod->instructionCount < 1) {
			// TODO: Assert that each method ends with a return or jump instruction after compiling
			printText("Cannot call \"", rootMethod->name, "\", because it doesn't have any instructions.\n");
			return;
		}
	#endif

	// Create a new current state
	this->memory->current.methodIndex = methodIndex;
	this->memory->current.programCounter = rootMethod->startAddress;
	for (int t = 0; t < this->machineTypeCount; t++) {
		int framePointer = this->methods[0].count[t];
		this->memory->current.framePointer[t] = framePointer;
		this->memory->current.stackPointer[t] = framePointer + this->methods[methodIndex].count[t];
	}

	#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
		this->debugPrintMemory();
	#endif
	#ifdef VIRTUAL_MACHINE_PROFILE
		printText("Calling \"", rootMethod->name, "\":\n");
		double startTime = time_getSeconds();
	#endif

	// Execute until the program counter is out of bound (-1)
	while (true) {
		int32_t pc = this->memory->current.programCounter;
		if (pc < 0 || pc >= this->machineWords.length()) {
			// Return statements will set the program counter to -1 if there are no more callers saved in the stack
			if (pc != -1) {
				throwError("Unexpected program counter! @", pc, " outside of 0..", (this->machineWords.length() - 1), "\n");
			}
			break;
		}
		MachineWord* word = &this->machineWords[pc];
		#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
			const InsSig* signature = getMachineInstructionFromFunction(word->operation);
			if (signature) {
				printText("Executing @", pc, " ", signature->name, "(");
				for (int a = signature->targetCount; a < word->args.length(); a++) {
					if (a > signature->targetCount) {
						printText(", ");
					}
					debugArgument(word->args[a], this->memory->current.methodIndex, this->memory->current.framePointer, false);
				}
				printText(")");
			}
			word->operation(*this, *(this->memory.get()), word->args);
			if (signature) {
				if (signature->targetCount > 0) {
					printText(" -> ");
					for (int a = 0; a < signature->targetCount; a++) {
						if (a > 0) {
							printText(", ");
						}
						debugArgument(word->args[a], this->memory->current.methodIndex, this->memory->current.framePointer, true);
					}
				}
			}
			printText("\n");
		#else
			word->operation(*this, *(this->memory.get()), word->args);
		#endif
	}
	#ifdef VIRTUAL_MACHINE_PROFILE
		double endTime = time_getSeconds();
		printText("Done calling \"", rootMethod->name, "\" after ", (endTime - startTime) * 1000000.0, " microseconds.\n");
		#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
			printText(" (debug prints are active)\n");
		#endif
	#endif
}
