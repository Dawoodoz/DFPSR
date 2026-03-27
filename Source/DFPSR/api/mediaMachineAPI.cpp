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

#define DSR_INTERNAL_ACCESS

#include "mediaMachineAPI.h"
#include "../implementation/machine/VirtualMachine.h"
#include "../implementation/machine/mediaFilters.h"
#include "../api/imageAPI.h"
#include "../api/drawAPI.h"
#include "../api/filterAPI.h"

namespace dsr {

// Media Machine specification

// Enumerating types
static const DataType DataType_FixedPoint = 0;
static const DataType DataType_ImageU8 = 1;
static const DataType DataType_ImageRgbaU8 = 2;
static ReadableString getMediaTypeName(DataType type) {
	switch(type) {
		case DataType_FixedPoint:  return U"FixedPoint";
		case DataType_ImageU8:     return U"ImageU8";
		case DataType_ImageRgbaU8: return U"ImageRgbaU8";
		default:                   return U"?";
	}
}

// An upper limit on the memory makes sure that the entire application does not crash from getting bottomless recursion in a virtual machine.
static const int32_t MEMORY_LIMIT = 1024;

class MediaMemory : public PlanarMemory<MEDIA_MACHINE_TYPE_COUNT> {
public:
	MemoryPlane<FixedPoint> fixedPointMemory;
	MemoryPlane<AlignedImageU8> alignedImageU8Memory;
	MemoryPlane<OrderedImageRgbaU8> orderedImageRgbaU8Memory;
	MediaMemory() : fixedPointMemory(MEMORY_LIMIT), alignedImageU8Memory(MEMORY_LIMIT), orderedImageRgbaU8Memory(MEMORY_LIMIT) {}
	void store(int32_t targetStackIndex, const VMA& sourceArg, int32_t sourceFramePointer, DataType type) override {
		switch(type) {
			case DataType_FixedPoint:
				this->fixedPointMemory.accessByStackIndex(targetStackIndex) = this->fixedPointMemory.getRead(sourceArg, sourceFramePointer);
			break;
			case DataType_ImageU8:
				this->alignedImageU8Memory.accessByStackIndex(targetStackIndex) = this->alignedImageU8Memory.getRead(sourceArg, sourceFramePointer);
			break;
			case DataType_ImageRgbaU8:
				this->orderedImageRgbaU8Memory.accessByStackIndex(targetStackIndex) = this->orderedImageRgbaU8Memory.getRead(sourceArg, sourceFramePointer);
			break;
			default:
				throwError(U"Storing element of unhandled type!\n");
			break;
		}
	}
	void load(int32_t sourceStackIndex, const VMA& targetArg, int32_t targetFramePointer, DataType type) override {
		// Assuming that targetArg.argType == ArgumentType::Reference.
		int32_t globalIndex = targetArg.index;
		switch(type) {
			case DataType_FixedPoint:
				this->fixedPointMemory.accessByGlobalIndex(globalIndex, targetFramePointer) = this->fixedPointMemory.accessByStackIndex(sourceStackIndex);
			break;
			case DataType_ImageU8:
				this->alignedImageU8Memory.accessByGlobalIndex(globalIndex, targetFramePointer) = this->alignedImageU8Memory.accessByStackIndex(sourceStackIndex);
			break;
			case DataType_ImageRgbaU8:
				this->orderedImageRgbaU8Memory.accessByGlobalIndex(globalIndex, targetFramePointer) = this->orderedImageRgbaU8Memory.accessByStackIndex(sourceStackIndex);
			break;
			default:
				throwError(U"Loading element of unhandled type!\n");
			break;
		}
	}
};

#define MEDIA_MEMORY ((MediaMemory&)memory)

// Type definitions
static const VMTypeDef<MEDIA_MACHINE_TYPE_COUNT> mediaMachineTypes[] = {
	VMTypeDef<MEDIA_MACHINE_TYPE_COUNT>(U"FixedPoint", DataType_FixedPoint, true,
	[](PlanarMemory<MEDIA_MACHINE_TYPE_COUNT>& memory, Variable<MEDIA_MACHINE_TYPE_COUNT>& variable, int32_t globalIndex, const FixedArray<int32_t, MEDIA_MACHINE_TYPE_COUNT>& framePointer, bool fullContent) {
		FixedPoint value = MEDIA_MEMORY.fixedPointMemory.accessByGlobalIndex(globalIndex, framePointer[DataType_FixedPoint]);
		printText(variable.name, U"(", value, U")");
	}),
	VMTypeDef<MEDIA_MACHINE_TYPE_COUNT>(U"ImageU8", DataType_ImageU8, false,
	[](PlanarMemory<MEDIA_MACHINE_TYPE_COUNT>& memory, Variable<MEDIA_MACHINE_TYPE_COUNT>& variable, int32_t globalIndex, const FixedArray<int32_t, MEDIA_MACHINE_TYPE_COUNT>& framePointer, bool fullContent) {
		AlignedImageU8 value = MEDIA_MEMORY.alignedImageU8Memory.accessByGlobalIndex(globalIndex, framePointer[DataType_ImageU8]);
		printText(variable.name, U" ImageU8");
		if (image_exists(value)) {
			if (fullContent) {
				printText(U":\n", image_toAscii(value, U" .:*ixXM"));
			} else {
				printText(U"(", image_getWidth(value), U"x", image_getHeight(value), U")");
			}
		} else {
			printText(U"(nothing)");
		}
	}),
	VMTypeDef<MEDIA_MACHINE_TYPE_COUNT>(U"ImageRgbaU8", DataType_ImageRgbaU8, false,
	[](PlanarMemory<MEDIA_MACHINE_TYPE_COUNT>& memory, Variable<MEDIA_MACHINE_TYPE_COUNT>& variable, int32_t globalIndex, const FixedArray<int32_t, MEDIA_MACHINE_TYPE_COUNT>& framePointer, bool fullContent) {
		OrderedImageRgbaU8 value = MEDIA_MEMORY.orderedImageRgbaU8Memory.accessByGlobalIndex(globalIndex, framePointer[DataType_ImageRgbaU8]);
		printText(variable.name, U" ImageRgbaU8");
		if (image_exists(value)) {
			// TODO: image_toAscii for multi-channel images
			printText(U"(", image_getWidth(value), U"x", image_getHeight(value), U")");
		} else {
			printText(U"(nothing)");
		}
	})
};

#define SCALAR_VALUE(ARG_INDEX) (MEDIA_MEMORY.fixedPointMemory.getRead(args[ARG_INDEX], memory.current.framePointer[DataType_FixedPoint]))
#define INT_VALUE(ARG_INDEX) fixedPoint_round(SCALAR_VALUE(ARG_INDEX))
#define BOOL_VALUE(ARG_INDEX) (SCALAR_VALUE(ARG_INDEX) != FixedPoint::zero())
#define ONE_OR_ZERO FixedPoint::one() : FixedPoint::zero()
#define SCALAR_REF(ARG_INDEX) (MEDIA_MEMORY.fixedPointMemory.getRef(args[ARG_INDEX], memory.current.framePointer[DataType_FixedPoint]))
#define IMAGE_U8_REF(ARG_INDEX) (MEDIA_MEMORY.alignedImageU8Memory.getRef(args[ARG_INDEX], memory.current.framePointer[DataType_ImageU8]))
#define IMAGE_RGBAU8_REF(ARG_INDEX) (MEDIA_MEMORY.orderedImageRgbaU8Memory.getRef(args[ARG_INDEX], memory.current.framePointer[DataType_ImageRgbaU8]))
#define OPERATION [](VirtualMachine<MEDIA_MACHINE_TYPE_COUNT>& machine, PlanarMemory<MEDIA_MACHINE_TYPE_COUNT>& memory, const List<VMA>& args)
#define NEXT_INSTRUCTION memory.current.programCounter++;
static const InsSig<MEDIA_MACHINE_TYPE_COUNT> mediaMachineInstructions[] = {
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"MOVE", 1,
		OPERATION {
			SCALAR_REF(0) = SCALAR_VALUE(1);
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_FixedPoint),
		ArgSig(U"Source", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"MOVE", 1,
		OPERATION {
			IMAGE_U8_REF(0) = IMAGE_U8_REF(1); // TODO: Support immediate constants.
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"Source", true, DataType_ImageU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"MOVE", 1,
		OPERATION {
			IMAGE_RGBAU8_REF(0) = IMAGE_RGBAU8_REF(1); // TODO: Support immediate constants.
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"Source", true, DataType_ImageRgbaU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"RESET", 1,
		OPERATION {
			SCALAR_REF(0) = FixedPoint::zero();
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"RESET", 1,
		OPERATION {
			IMAGE_U8_REF(0) = AlignedImageU8();
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"RESET", 1,
		OPERATION {
			IMAGE_RGBAU8_REF(0) = OrderedImageRgbaU8();
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"JUMP", 0,
		OPERATION {
			int32_t targetAddress = args[0].index;
			// TODO: Assert that the target address is within the same method when running in debug mode.
			//       VirtualMachine.h only asserts that we are within the whole program's instructions.
			#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
				printText(U"Jumping from instruction ", memory.current.programCounter, U" to ", targetAddress, U".\n");
			#endif
			memory.current.programCounter = targetAddress;
		},
		// The argument for labels is matched with the Label type before we know the label's final instruction address,
		//   but label indices it will be converted into DataType_InstructionAddress once all machine instructions are generated.
		ArgSig(U"Address", true, DataType_Label)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"JUMP_IF_TRUE", 0,
		OPERATION {
			if (SCALAR_VALUE(1) != FixedPoint::zero()) {
				// True case
				memory.current.programCounter = args[0].index; // Labels are replaced with absolute instruction addresses after generating the machine instructions.
			} else {
				// False case
				NEXT_INSTRUCTION
			}
		},
		ArgSig(U"Address", true, DataType_Label),
		ArgSig(U"Condition", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"JUMP_IF_FALSE", 0,
		OPERATION {
			if (SCALAR_VALUE(1) == FixedPoint::zero()) {
				// False case
				memory.current.programCounter = args[0].index; // Labels are replaced with absolute instruction addresses after generating the machine instructions.
			} else {
				// True case
				NEXT_INSTRUCTION
			}
		},
		ArgSig(U"Address", true, DataType_Label),
		ArgSig(U"Condition", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"LOGIC_AND", 1,
		OPERATION {
			SCALAR_REF(0) = (BOOL_VALUE(1) && BOOL_VALUE(2)) ? ONE_OR_ZERO;
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_FixedPoint),
		ArgSig(U"LeftSource", true, DataType_FixedPoint),
		ArgSig(U"RightSource", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"LOGIC_OR", 1,
		OPERATION {
			SCALAR_REF(0) = (BOOL_VALUE(1) || BOOL_VALUE(2)) ? ONE_OR_ZERO;
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_FixedPoint),
		ArgSig(U"LeftSource", true, DataType_FixedPoint),
		ArgSig(U"RightSource", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"LOGIC_XOR", 1,
		OPERATION {
			SCALAR_REF(0) = (BOOL_VALUE(1) != BOOL_VALUE(2)) ? ONE_OR_ZERO;
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_FixedPoint),
		ArgSig(U"LeftSource", true, DataType_FixedPoint),
		ArgSig(U"RightSource", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"LOGIC_NOT", 1,
		OPERATION {
			SCALAR_REF(0) = BOOL_VALUE(1) ? ONE_OR_ZERO;
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_FixedPoint),
		ArgSig(U"LeftSource", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"EQUAL", 1,
		OPERATION {
			SCALAR_REF(0) = (SCALAR_VALUE(1) == SCALAR_VALUE(2)) ? ONE_OR_ZERO;
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_FixedPoint),
		ArgSig(U"LeftSource", true, DataType_FixedPoint),
		ArgSig(U"RightSource", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"NOT_EQUAL", 1,
		OPERATION {
			SCALAR_REF(0) = (SCALAR_VALUE(1) != SCALAR_VALUE(2)) ? ONE_OR_ZERO;
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_FixedPoint),
		ArgSig(U"LeftSource", true, DataType_FixedPoint),
		ArgSig(U"RightSource", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"GREATER", 1,
		OPERATION {
			SCALAR_REF(0) = (SCALAR_VALUE(1) > SCALAR_VALUE(2)) ? ONE_OR_ZERO;
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_FixedPoint),
		ArgSig(U"LeftSource", true, DataType_FixedPoint),
		ArgSig(U"RightSource", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"GREATER_EQUAL", 1,
		OPERATION {
			SCALAR_REF(0) = (SCALAR_VALUE(1) >= SCALAR_VALUE(2)) ? ONE_OR_ZERO;
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_FixedPoint),
		ArgSig(U"LeftSource", true, DataType_FixedPoint),
		ArgSig(U"RightSource", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"LESSER", 1,
		OPERATION {
			SCALAR_REF(0) = (SCALAR_VALUE(1) < SCALAR_VALUE(2)) ? ONE_OR_ZERO;
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_FixedPoint),
		ArgSig(U"LeftSource", true, DataType_FixedPoint),
		ArgSig(U"RightSource", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"LESSER_EQUAL", 1,
		OPERATION {
			SCALAR_REF(0) = (SCALAR_VALUE(1) <= SCALAR_VALUE(2)) ? ONE_OR_ZERO;
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_FixedPoint),
		ArgSig(U"LeftSource", true, DataType_FixedPoint),
		ArgSig(U"RightSource", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"ROUND", 1,
		OPERATION {
			SCALAR_REF(0) = FixedPoint::fromWhole(fixedPoint_round(SCALAR_VALUE(1)));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_FixedPoint), // Aliasing is accepted
		ArgSig(U"Source", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"MIN", 1,
		OPERATION {
			SCALAR_REF(0) = fixedPoint_min(SCALAR_VALUE(1), SCALAR_VALUE(2));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_FixedPoint), // Aliasing is accepted
		ArgSig(U"LeftSource", true, DataType_FixedPoint),
		ArgSig(U"RightSource", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"MAX", 1,
		OPERATION {
			SCALAR_REF(0) = fixedPoint_max(SCALAR_VALUE(1), SCALAR_VALUE(2));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_FixedPoint), // Aliasing is accepted
		ArgSig(U"LeftSource", true, DataType_FixedPoint),
		ArgSig(U"RightSource", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"ADD", 1,
		OPERATION {
			SCALAR_REF(0) = SCALAR_VALUE(1) + SCALAR_VALUE(2);
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_FixedPoint), // Aliasing is accepted
		ArgSig(U"LeftSource", true, DataType_FixedPoint),
		ArgSig(U"RightSource", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"ADD", 1,
		OPERATION {
			media_filter_add(IMAGE_U8_REF(0), IMAGE_U8_REF(1), IMAGE_U8_REF(2));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8), // Aliasing is accepted
		ArgSig(U"LeftSource", true, DataType_ImageU8),
		ArgSig(U"RightSource", true, DataType_ImageU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"ADD", 1,
		OPERATION {
			media_filter_add(IMAGE_U8_REF(0), IMAGE_U8_REF(1), SCALAR_VALUE(2));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8), // Aliasing is accepted
		ArgSig(U"LeftSource", true, DataType_ImageU8),
		ArgSig(U"RightSource", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"ADD", 1,
		OPERATION {
			media_filter_add(IMAGE_U8_REF(0), IMAGE_U8_REF(2), SCALAR_VALUE(1));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8), // Aliasing is accepted
		ArgSig(U"LeftSource", true, DataType_FixedPoint),
		ArgSig(U"RightSource", true, DataType_ImageU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"SUB", 1,
		OPERATION {
			SCALAR_REF(0) = SCALAR_VALUE(1) - SCALAR_VALUE(2);
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_FixedPoint), // Aliasing is accepted
		ArgSig(U"PositiveSource", true, DataType_FixedPoint),
		ArgSig(U"NegativeSource", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"SUB", 1,
		OPERATION {
			media_filter_sub(IMAGE_U8_REF(0), IMAGE_U8_REF(1), IMAGE_U8_REF(2));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8), // Aliasing is accepted
		ArgSig(U"PositiveSource", true, DataType_ImageU8),
		ArgSig(U"NegativeSource", true, DataType_ImageU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"SUB", 1,
		OPERATION {
			media_filter_sub(IMAGE_U8_REF(0), IMAGE_U8_REF(1), SCALAR_VALUE(2));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8), // Aliasing is accepted
		ArgSig(U"PositiveSource", true, DataType_ImageU8),
		ArgSig(U"NegativeSource", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"SUB", 1,
		OPERATION {
			media_filter_sub(IMAGE_U8_REF(0), SCALAR_VALUE(2), IMAGE_U8_REF(1));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8), // Aliasing is accepted
		ArgSig(U"PositiveSource", true, DataType_FixedPoint),
		ArgSig(U"NegativeSource", true, DataType_ImageU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"NEG", 1,
		OPERATION {
			SCALAR_REF(0) = -SCALAR_VALUE(1);
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_FixedPoint), // Aliasing is accepted
		ArgSig(U"PositiveSource", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"MUL", 1,
		OPERATION {
			SCALAR_REF(0) = SCALAR_VALUE(1) * SCALAR_VALUE(2);
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_FixedPoint), // Aliasing is accepted
		ArgSig(U"LeftSource", true, DataType_FixedPoint),
		ArgSig(U"RightSource", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"MUL", 1,
		OPERATION {
			media_filter_mul(IMAGE_U8_REF(0), IMAGE_U8_REF(1), SCALAR_VALUE(2));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8), // Aliasing is accepted
		ArgSig(U"LeftSource", true, DataType_ImageU8),
		ArgSig(U"RightSource", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"MUL", 1,
		OPERATION {
			media_filter_mul(IMAGE_U8_REF(0), IMAGE_U8_REF(1), IMAGE_U8_REF(2), SCALAR_VALUE(3));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8), // Aliasing is accepted
		ArgSig(U"FirstSource", true, DataType_ImageU8),
		ArgSig(U"SecondSource", true, DataType_ImageU8),
		ArgSig(U"Scalar", true, DataType_FixedPoint) // Use 1/255 for normalized multiplication
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"CREATE", 1,
		OPERATION {
			int32_t width = INT_VALUE(1);
			int32_t height = INT_VALUE(2);
			if (width < 1 || height < 1) {
				throwError(U"Images must allocate at least one pixel to be created.");
			}
			IMAGE_U8_REF(0) = image_create_U8(width, height);
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"Width", true, DataType_FixedPoint),
		ArgSig(U"Height", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"CREATE", 1,
		OPERATION {
			int32_t width = INT_VALUE(1);
			int32_t height = INT_VALUE(2);
			if (width < 1 || height < 1) {
				throwError(U"Images must allocate at least one pixel to be created.");
			}
			IMAGE_RGBAU8_REF(0) = image_create_RgbaU8(width, height);
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"Width", true, DataType_FixedPoint),
		ArgSig(U"Height", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"EXISTS", 1,
		OPERATION {
			SCALAR_REF(0) = FixedPoint::fromWhole(image_exists(IMAGE_U8_REF(1)) ? 1 : 0);
			NEXT_INSTRUCTION
		},
		ArgSig(U"Truth", false, DataType_FixedPoint), // 1 for existing, 0 for null
		ArgSig(U"Source", true, DataType_ImageU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"EXISTS", 1,
		OPERATION {
			SCALAR_REF(0) = FixedPoint::fromWhole(image_exists(IMAGE_RGBAU8_REF(1)) ? 1 : 0);
			NEXT_INSTRUCTION
		},
		ArgSig(U"Truth", false, DataType_FixedPoint), // 1 for existing, 0 for null
		ArgSig(U"Source", true, DataType_ImageRgbaU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"GET_WIDTH", 1,
		OPERATION {
			SCALAR_REF(0) = FixedPoint::fromWhole(image_getWidth(IMAGE_U8_REF(1)));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Width", false, DataType_FixedPoint),
		ArgSig(U"Source", true, DataType_ImageU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"GET_WIDTH", 1,
		OPERATION {
			SCALAR_REF(0) = FixedPoint::fromWhole(image_getWidth(IMAGE_RGBAU8_REF(1)));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Width", false, DataType_FixedPoint),
		ArgSig(U"Source", true, DataType_ImageRgbaU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"GET_HEIGHT", 1,
		OPERATION {
			SCALAR_REF(0) = FixedPoint::fromWhole(image_getHeight(IMAGE_U8_REF(1)));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Height", false, DataType_FixedPoint),
		ArgSig(U"Source", true, DataType_ImageU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"GET_HEIGHT", 1,
		OPERATION {
			SCALAR_REF(0) = FixedPoint::fromWhole(image_getHeight(IMAGE_RGBAU8_REF(1)));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Height", false, DataType_FixedPoint),
		ArgSig(U"Source", true, DataType_ImageRgbaU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"FILL", 1,
		OPERATION {
			image_fill(IMAGE_U8_REF(0), INT_VALUE(1));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"Luma", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"FILL", 1,
		OPERATION {
			image_fill(
			  IMAGE_RGBAU8_REF(0),
			  ColorRgbaI32(INT_VALUE(1), INT_VALUE(2), INT_VALUE(3), INT_VALUE(4))
			);
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"Red", true, DataType_FixedPoint),
		ArgSig(U"Green", true, DataType_FixedPoint),
		ArgSig(U"Blue", true, DataType_FixedPoint),
		ArgSig(U"Alpha", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"RECTANGLE", 1,
		OPERATION {
			draw_rectangle(
			  IMAGE_U8_REF(0),
			  IRect(INT_VALUE(1), INT_VALUE(2), INT_VALUE(3), INT_VALUE(4)),
			  INT_VALUE(5)
			);
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"Left", true, DataType_FixedPoint),
		ArgSig(U"Top", true, DataType_FixedPoint),
		ArgSig(U"Width", true, DataType_FixedPoint),
		ArgSig(U"Height", true, DataType_FixedPoint),
		ArgSig(U"Luma", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"RECTANGLE", 1,
		OPERATION {
			draw_rectangle(
			  IMAGE_RGBAU8_REF(0),
			  IRect(INT_VALUE(1), INT_VALUE(2), INT_VALUE(3), INT_VALUE(4)),
			  ColorRgbaI32(INT_VALUE(5), INT_VALUE(6), INT_VALUE(7), INT_VALUE(8))
			);
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"Left", true, DataType_FixedPoint),
		ArgSig(U"Top", true, DataType_FixedPoint),
		ArgSig(U"Width", true, DataType_FixedPoint),
		ArgSig(U"Height", true, DataType_FixedPoint),
		ArgSig(U"Red", true, DataType_FixedPoint),
		ArgSig(U"Green", true, DataType_FixedPoint),
		ArgSig(U"Blue", true, DataType_FixedPoint),
		ArgSig(U"Alpha", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"COPY", 1,
		OPERATION {
			draw_copy(IMAGE_U8_REF(0), IMAGE_U8_REF(3), INT_VALUE(1), INT_VALUE(2));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"TargetLeft", true, DataType_FixedPoint),
		ArgSig(U"TargetTop", true, DataType_FixedPoint),
		ArgSig(U"Source", true, DataType_ImageU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"COPY", 1,
		OPERATION {
			draw_copy(IMAGE_RGBAU8_REF(0), IMAGE_RGBAU8_REF(3), INT_VALUE(1), INT_VALUE(2));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"TargetLeft", true, DataType_FixedPoint),
		ArgSig(U"TargetTop", true, DataType_FixedPoint),
		ArgSig(U"Source", true, DataType_ImageRgbaU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"COPY", 1,
		OPERATION {
			draw_copy(
			  IMAGE_U8_REF(0),
			  image_getSubImage(IMAGE_U8_REF(3), IRect(INT_VALUE(4), INT_VALUE(5), INT_VALUE(6), INT_VALUE(7))),
			  INT_VALUE(1), INT_VALUE(2)
			);
			NEXT_INSTRUCTION
		},
		// TODO: Prevent aliasing between IMAGE_U8_REF(0) and IMAGE_U8_REF(3) in compile-time
		//       This will be added as another lambda running safety checks on suggested inputs
		//         The result will either accept, pass on to the next overload or abort compilation
		//         Passing to another overload can be used to fall back on a run-time checked operation
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"TargetLeft", true, DataType_FixedPoint),
		ArgSig(U"TargetTop", true, DataType_FixedPoint),
		ArgSig(U"Source", true, DataType_ImageU8),
		ArgSig(U"SourceLeft", true, DataType_FixedPoint),
		ArgSig(U"SourceTop", true, DataType_FixedPoint),
		ArgSig(U"SourceWidth", true, DataType_FixedPoint),
		ArgSig(U"SourceHeight", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"COPY", 1,
		OPERATION {
			draw_copy(
			  IMAGE_RGBAU8_REF(0),
			  image_getSubImage(IMAGE_RGBAU8_REF(3), IRect(INT_VALUE(4), INT_VALUE(5), INT_VALUE(6), INT_VALUE(7))),
			  INT_VALUE(1), INT_VALUE(2)
			);
			NEXT_INSTRUCTION
		},
		// TODO: Prevent aliasing
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"TargetLeft", true, DataType_FixedPoint),
		ArgSig(U"TargetTop", true, DataType_FixedPoint),
		ArgSig(U"Source", true, DataType_ImageRgbaU8),
		ArgSig(U"SourceLeft", true, DataType_FixedPoint),
		ArgSig(U"SourceTop", true, DataType_FixedPoint),
		ArgSig(U"SourceWidth", true, DataType_FixedPoint),
		ArgSig(U"SourceHeight", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"RESIZE_BILINEAR", 1,
		OPERATION {
			int32_t width = INT_VALUE(1); if (width < 1) width = 1;
			int32_t height = INT_VALUE(2); if (height < 1) height = 1;
			IMAGE_U8_REF(0) = filter_resize(IMAGE_U8_REF(3), Sampler::Linear, width, height);
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"NewWidth", true, DataType_FixedPoint),
		ArgSig(U"NewHeight", true, DataType_FixedPoint),
		ArgSig(U"Source", true, DataType_ImageU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"RESIZE_BILINEAR", 1,
		OPERATION {
			int32_t width = INT_VALUE(1); if (width < 1) width = 1;
			int32_t height = INT_VALUE(2); if (height < 1) height = 1;
			IMAGE_RGBAU8_REF(0) = filter_resize(IMAGE_RGBAU8_REF(3), Sampler::Linear, width, height);
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"NewWidth", true, DataType_FixedPoint),
		ArgSig(U"NewHeight", true, DataType_FixedPoint),
		ArgSig(U"Source", true, DataType_ImageRgbaU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"RESIZE_BILINEAR", 1,
		OPERATION {
			int32_t width = INT_VALUE(1); if (width < 1) width = 1;
			int32_t height = INT_VALUE(2); if (height < 1) height = 1;
			IMAGE_U8_REF(0) = filter_resize(image_getSubImage(IMAGE_U8_REF(3), IRect(INT_VALUE(4), INT_VALUE(5), INT_VALUE(6), INT_VALUE(7))), Sampler::Linear, width, height);
			NEXT_INSTRUCTION
		},
		// TODO: Prevent aliasing
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"NewWidth", true, DataType_FixedPoint),
		ArgSig(U"NewHeight", true, DataType_FixedPoint),
		ArgSig(U"Source", true, DataType_ImageU8),
		ArgSig(U"SourceLeft", true, DataType_FixedPoint),
		ArgSig(U"SourceTop", true, DataType_FixedPoint),
		ArgSig(U"SourceWidth", true, DataType_FixedPoint),
		ArgSig(U"SourceHeight", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"RESIZE_BILINEAR", 1,
		OPERATION {
			int32_t width = INT_VALUE(1); if (width < 1) width = 1;
			int32_t height = INT_VALUE(2); if (height < 1) height = 1;
			IMAGE_RGBAU8_REF(0) = filter_resize(image_getSubImage(IMAGE_RGBAU8_REF(3), IRect(INT_VALUE(4), INT_VALUE(5), INT_VALUE(6), INT_VALUE(7))), Sampler::Linear, width, height);
			NEXT_INSTRUCTION
		},
		// TODO: Prevent aliasing
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"NewWidth", true, DataType_FixedPoint),
		ArgSig(U"NewHeight", true, DataType_FixedPoint),
		ArgSig(U"Source", true, DataType_ImageRgbaU8),
		ArgSig(U"SourceLeft", true, DataType_FixedPoint),
		ArgSig(U"SourceTop", true, DataType_FixedPoint),
		ArgSig(U"SourceWidth", true, DataType_FixedPoint),
		ArgSig(U"SourceHeight", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"GET_RED", 1,
		// Getting red channel of an image.
		OPERATION {
			IMAGE_U8_REF(0) = image_get_red(IMAGE_RGBAU8_REF(1));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"Source", true, DataType_ImageRgbaU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"GET_RED", 1,
		// Getting red channel of a source region in the image.
		OPERATION {
			IMAGE_U8_REF(0) = image_get_red(image_getSubImage(IMAGE_RGBAU8_REF(1), IRect(INT_VALUE(2), INT_VALUE(3), INT_VALUE(4), INT_VALUE(5))));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"Source", true, DataType_ImageRgbaU8),
		ArgSig(U"SourceLeft", true, DataType_FixedPoint),
		ArgSig(U"SourceTop", true, DataType_FixedPoint),
		ArgSig(U"SourceWidth", true, DataType_FixedPoint),
		ArgSig(U"SourceHeight", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"GET_GREEN", 1,
		// Getting green channel of an image.
		OPERATION {
			IMAGE_U8_REF(0) = image_get_green(IMAGE_RGBAU8_REF(1));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"Source", true, DataType_ImageRgbaU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"GET_GREEN", 1,
		// Getting green channel of a source region in the image.
		OPERATION {
			IMAGE_U8_REF(0) = image_get_green(image_getSubImage(IMAGE_RGBAU8_REF(1), IRect(INT_VALUE(2), INT_VALUE(3), INT_VALUE(4), INT_VALUE(5))));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"Source", true, DataType_ImageRgbaU8),
		ArgSig(U"SourceLeft", true, DataType_FixedPoint),
		ArgSig(U"SourceTop", true, DataType_FixedPoint),
		ArgSig(U"SourceWidth", true, DataType_FixedPoint),
		ArgSig(U"SourceHeight", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"GET_BLUE", 1,
		// Getting blue channel of an image.
		OPERATION {
			IMAGE_U8_REF(0) = image_get_blue(IMAGE_RGBAU8_REF(1));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"Source", true, DataType_ImageRgbaU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"GET_BLUE", 1,
		// Getting blue channel of a source region in the image.
		OPERATION {
			IMAGE_U8_REF(0) = image_get_blue(image_getSubImage(IMAGE_RGBAU8_REF(1), IRect(INT_VALUE(2), INT_VALUE(3), INT_VALUE(4), INT_VALUE(5))));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"Source", true, DataType_ImageRgbaU8),
		ArgSig(U"SourceLeft", true, DataType_FixedPoint),
		ArgSig(U"SourceTop", true, DataType_FixedPoint),
		ArgSig(U"SourceWidth", true, DataType_FixedPoint),
		ArgSig(U"SourceHeight", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"GET_ALPHA", 1,
		// Getting alpha channel of an image.
		OPERATION {
			IMAGE_U8_REF(0) = image_get_alpha(IMAGE_RGBAU8_REF(1));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"Source", true, DataType_ImageRgbaU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"GET_ALPHA", 1,
		// Getting alpha channel of a source region in the image.
		OPERATION {
			IMAGE_U8_REF(0) = image_get_alpha(image_getSubImage(IMAGE_RGBAU8_REF(1), IRect(INT_VALUE(2), INT_VALUE(3), INT_VALUE(4), INT_VALUE(5))));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"Source", true, DataType_ImageRgbaU8),
		ArgSig(U"SourceLeft", true, DataType_FixedPoint),
		ArgSig(U"SourceTop", true, DataType_FixedPoint),
		ArgSig(U"SourceWidth", true, DataType_FixedPoint),
		ArgSig(U"SourceHeight", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"PACK_RGBA", 1,
		OPERATION {
			IMAGE_RGBAU8_REF(0) = image_pack(IMAGE_U8_REF(1), INT_VALUE(2), INT_VALUE(3), INT_VALUE(4));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"Red", true, DataType_ImageU8),
		ArgSig(U"Green", true, DataType_FixedPoint),
		ArgSig(U"Blue", true, DataType_FixedPoint),
		ArgSig(U"Alpha", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"PACK_RGBA", 1,
		OPERATION {
			IMAGE_RGBAU8_REF(0) = image_pack(INT_VALUE(1), IMAGE_U8_REF(2), INT_VALUE(3), INT_VALUE(4));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"Red", true, DataType_FixedPoint),
		ArgSig(U"Green", true, DataType_ImageU8),
		ArgSig(U"Blue", true, DataType_FixedPoint),
		ArgSig(U"Alpha", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"PACK_RGBA", 1,
		OPERATION {
			IMAGE_RGBAU8_REF(0) = image_pack(INT_VALUE(1), INT_VALUE(2), IMAGE_U8_REF(3), INT_VALUE(4));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"Red", true, DataType_FixedPoint),
		ArgSig(U"Green", true, DataType_FixedPoint),
		ArgSig(U"Blue", true, DataType_ImageU8),
		ArgSig(U"Alpha", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"PACK_RGBA", 1,
		OPERATION {
			IMAGE_RGBAU8_REF(0) = image_pack(INT_VALUE(1), INT_VALUE(2), INT_VALUE(3), IMAGE_U8_REF(4));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"Red", true, DataType_FixedPoint),
		ArgSig(U"Green", true, DataType_FixedPoint),
		ArgSig(U"Blue", true, DataType_FixedPoint),
		ArgSig(U"Alpha", true, DataType_ImageU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"PACK_RGBA", 1,
		OPERATION {
			IMAGE_RGBAU8_REF(0) = image_pack(IMAGE_U8_REF(1), IMAGE_U8_REF(2), INT_VALUE(3), INT_VALUE(4));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"Red", true, DataType_ImageU8),
		ArgSig(U"Green", true, DataType_ImageU8),
		ArgSig(U"Blue", true, DataType_FixedPoint),
		ArgSig(U"Alpha", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"PACK_RGBA", 1,
		OPERATION {
			IMAGE_RGBAU8_REF(0) = image_pack(IMAGE_U8_REF(1), INT_VALUE(2), IMAGE_U8_REF(3), INT_VALUE(4));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"Red", true, DataType_ImageU8),
		ArgSig(U"Green", true, DataType_FixedPoint),
		ArgSig(U"Blue", true, DataType_ImageU8),
		ArgSig(U"Alpha", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"PACK_RGBA", 1,
		OPERATION {
			IMAGE_RGBAU8_REF(0) = image_pack(IMAGE_U8_REF(1), INT_VALUE(2), INT_VALUE(3), IMAGE_U8_REF(4));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"Red", true, DataType_ImageU8),
		ArgSig(U"Green", true, DataType_FixedPoint),
		ArgSig(U"Blue", true, DataType_FixedPoint),
		ArgSig(U"Alpha", true, DataType_ImageU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"PACK_RGBA", 1,
		OPERATION {
			IMAGE_RGBAU8_REF(0) = image_pack(INT_VALUE(1), IMAGE_U8_REF(2), IMAGE_U8_REF(3), INT_VALUE(4));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"Red", true, DataType_FixedPoint),
		ArgSig(U"Green", true, DataType_ImageU8),
		ArgSig(U"Blue", true, DataType_ImageU8),
		ArgSig(U"Alpha", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"PACK_RGBA", 1,
		OPERATION {
			IMAGE_RGBAU8_REF(0) = image_pack(INT_VALUE(1), IMAGE_U8_REF(2), INT_VALUE(3), IMAGE_U8_REF(4));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"Red", true, DataType_FixedPoint),
		ArgSig(U"Green", true, DataType_ImageU8),
		ArgSig(U"Blue", true, DataType_FixedPoint),
		ArgSig(U"Alpha", true, DataType_ImageU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"PACK_RGBA", 1,
		OPERATION {
			IMAGE_RGBAU8_REF(0) = image_pack(INT_VALUE(1), INT_VALUE(2), IMAGE_U8_REF(3), IMAGE_U8_REF(4));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"Red", true, DataType_FixedPoint),
		ArgSig(U"Green", true, DataType_FixedPoint),
		ArgSig(U"Blue", true, DataType_ImageU8),
		ArgSig(U"Alpha", true, DataType_ImageU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"PACK_RGBA", 1,
		OPERATION {
			IMAGE_RGBAU8_REF(0) = image_pack(INT_VALUE(1), IMAGE_U8_REF(2), IMAGE_U8_REF(3), IMAGE_U8_REF(4));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"Red", true, DataType_FixedPoint),
		ArgSig(U"Green", true, DataType_ImageU8),
		ArgSig(U"Blue", true, DataType_ImageU8),
		ArgSig(U"Alpha", true, DataType_ImageU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"PACK_RGBA", 1,
		OPERATION {
			IMAGE_RGBAU8_REF(0) = image_pack(IMAGE_U8_REF(1), INT_VALUE(2), IMAGE_U8_REF(3), IMAGE_U8_REF(4));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"Red", true, DataType_ImageU8),
		ArgSig(U"Green", true, DataType_FixedPoint),
		ArgSig(U"Blue", true, DataType_ImageU8),
		ArgSig(U"Alpha", true, DataType_ImageU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"PACK_RGBA", 1,
		OPERATION {
			IMAGE_RGBAU8_REF(0) = image_pack(IMAGE_U8_REF(1), IMAGE_U8_REF(2), INT_VALUE(3), IMAGE_U8_REF(4));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"Red", true, DataType_ImageU8),
		ArgSig(U"Green", true, DataType_ImageU8),
		ArgSig(U"Blue", true, DataType_FixedPoint),
		ArgSig(U"Alpha", true, DataType_ImageU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"PACK_RGBA", 1,
		OPERATION {
			IMAGE_RGBAU8_REF(0) = image_pack(IMAGE_U8_REF(1), IMAGE_U8_REF(2), IMAGE_U8_REF(3), INT_VALUE(4));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"Red", true, DataType_ImageU8),
		ArgSig(U"Green", true, DataType_ImageU8),
		ArgSig(U"Blue", true, DataType_ImageU8),
		ArgSig(U"Alpha", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"PACK_RGBA", 1,
		OPERATION {
			IMAGE_RGBAU8_REF(0) = image_pack(IMAGE_U8_REF(1), IMAGE_U8_REF(2), IMAGE_U8_REF(3), IMAGE_U8_REF(4));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"Red", true, DataType_ImageU8),
		ArgSig(U"Green", true, DataType_ImageU8),
		ArgSig(U"Blue", true, DataType_ImageU8),
		ArgSig(U"Alpha", true, DataType_ImageU8)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"LINE", 1,
		OPERATION {
			draw_line(IMAGE_U8_REF(0), INT_VALUE(1), INT_VALUE(2), INT_VALUE(3), INT_VALUE(4), INT_VALUE(5));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"X1", true, DataType_FixedPoint),
		ArgSig(U"Y1", true, DataType_FixedPoint),
		ArgSig(U"X2", true, DataType_FixedPoint),
		ArgSig(U"Y2", true, DataType_FixedPoint),
		ArgSig(U"Luma", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"LINE", 1,
		OPERATION {
			draw_line(
			  IMAGE_RGBAU8_REF(0),
			  INT_VALUE(1), INT_VALUE(2), INT_VALUE(3), INT_VALUE(4),
			  ColorRgbaI32(INT_VALUE(5), INT_VALUE(6), INT_VALUE(7), INT_VALUE(8))
			);
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"X1", true, DataType_FixedPoint),
		ArgSig(U"Y1", true, DataType_FixedPoint),
		ArgSig(U"X2", true, DataType_FixedPoint),
		ArgSig(U"Y2", true, DataType_FixedPoint),
		ArgSig(U"Red", true, DataType_FixedPoint),
		ArgSig(U"Green", true, DataType_FixedPoint),
		ArgSig(U"Blue", true, DataType_FixedPoint),
		ArgSig(U"Alpha", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"FADE_LINEAR", 1,
		OPERATION {
			media_fade_linear(IMAGE_U8_REF(0), SCALAR_VALUE(1), SCALAR_VALUE(2), SCALAR_VALUE(3), SCALAR_VALUE(4), SCALAR_VALUE(5), SCALAR_VALUE(6));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"X1", true, DataType_FixedPoint),
		ArgSig(U"Y1", true, DataType_FixedPoint),
		ArgSig(U"Luma1", true, DataType_FixedPoint), // At x1, y1
		ArgSig(U"X2", true, DataType_FixedPoint),
		ArgSig(U"Y2", true, DataType_FixedPoint),
		ArgSig(U"Luma2", true, DataType_FixedPoint) // At x2, y2
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"FADE_REGION_LINEAR", 1,
		OPERATION {
			media_fade_region_linear(IMAGE_U8_REF(0), IRect(INT_VALUE(1), INT_VALUE(2), INT_VALUE(3), INT_VALUE(4)), SCALAR_VALUE(5), SCALAR_VALUE(6), SCALAR_VALUE(7), SCALAR_VALUE(8), SCALAR_VALUE(9), SCALAR_VALUE(10));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"Left", true, DataType_FixedPoint),
		ArgSig(U"Top", true, DataType_FixedPoint),
		ArgSig(U"Width", true, DataType_FixedPoint),
		ArgSig(U"Height", true, DataType_FixedPoint),
		ArgSig(U"X1", true, DataType_FixedPoint), // Relative to Left
		ArgSig(U"Y1", true, DataType_FixedPoint), // Relative to Top
		ArgSig(U"Luma1", true, DataType_FixedPoint), // At Left + X1, Top + Y1
		ArgSig(U"X2", true, DataType_FixedPoint), // Relative to Left
		ArgSig(U"Y2", true, DataType_FixedPoint), // Relative to Top
		ArgSig(U"Luma2", true, DataType_FixedPoint)  // At Left + X2, Top + Y2
	),
	//void media_fade_radial(ImageU8& targetImage, FixedPoint centerX, FixedPoint centerY, FixedPoint innerRadius, FixedPoint innerLuma, FixedPoint outerRadius, FixedPoint outerLuma);
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"FADE_RADIAL", 1,
		OPERATION {
			media_fade_radial(IMAGE_U8_REF(0), SCALAR_VALUE(1), SCALAR_VALUE(2), SCALAR_VALUE(3), SCALAR_VALUE(4), SCALAR_VALUE(5), SCALAR_VALUE(6));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"CenterX", true, DataType_FixedPoint),
		ArgSig(U"CenterY", true, DataType_FixedPoint),
		ArgSig(U"InnerRadius", true, DataType_FixedPoint),
		ArgSig(U"InnerLuma", true, DataType_FixedPoint),
		ArgSig(U"OuterRadius", true, DataType_FixedPoint),
		ArgSig(U"OuterLuma", true, DataType_FixedPoint)
	),
	// void media_fade_region_radial(ImageU8& targetImage, const IRect& viewport, FixedPoint centerX, FixedPoint centerY, FixedPoint innerRadius, FixedPoint innerLuma, FixedPoint outerRadius, FixedPoint outerLuma);
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"FADE_REGION_RADIAL", 1,
		OPERATION {
			media_fade_region_radial(IMAGE_U8_REF(0), IRect(INT_VALUE(1), INT_VALUE(2), INT_VALUE(3), INT_VALUE(4)), SCALAR_VALUE(5), SCALAR_VALUE(6), SCALAR_VALUE(7), SCALAR_VALUE(8), SCALAR_VALUE(9), SCALAR_VALUE(10));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"Left", true, DataType_FixedPoint),
		ArgSig(U"Top", true, DataType_FixedPoint),
		ArgSig(U"Width", true, DataType_FixedPoint),
		ArgSig(U"Height", true, DataType_FixedPoint),
		ArgSig(U"CenterX", true, DataType_FixedPoint),
		ArgSig(U"CenterY", true, DataType_FixedPoint),
		ArgSig(U"InnerRadius", true, DataType_FixedPoint),
		ArgSig(U"InnerLuma", true, DataType_FixedPoint),
		ArgSig(U"OuterRadius", true, DataType_FixedPoint),
		ArgSig(U"OuterLuma", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"WRITE_PIXEL", 1,
		OPERATION {
			image_writePixel(IMAGE_U8_REF(0), INT_VALUE(1), INT_VALUE(2), INT_VALUE(3));
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageU8),
		ArgSig(U"X", true, DataType_FixedPoint),
		ArgSig(U"Y", true, DataType_FixedPoint),
		ArgSig(U"Luma", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"WRITE_PIXEL", 1,
		OPERATION {
			image_writePixel(
			  IMAGE_RGBAU8_REF(0),
			  INT_VALUE(1), INT_VALUE(2),
			  ColorRgbaI32(INT_VALUE(3), INT_VALUE(4), INT_VALUE(5), INT_VALUE(6))
			);
			NEXT_INSTRUCTION
		},
		ArgSig(U"Target", false, DataType_ImageRgbaU8),
		ArgSig(U"X", true, DataType_FixedPoint),
		ArgSig(U"Y", true, DataType_FixedPoint),
		ArgSig(U"Red", true, DataType_FixedPoint),
		ArgSig(U"Green", true, DataType_FixedPoint),
		ArgSig(U"Blue", true, DataType_FixedPoint),
		ArgSig(U"Alpha", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"READ_PIXEL_BORDER", 1,
		OPERATION {
			SCALAR_REF(0) = FixedPoint::fromWhole(image_readPixel_border(IMAGE_U8_REF(1), INT_VALUE(2), INT_VALUE(3), INT_VALUE(4)));
			NEXT_INSTRUCTION
		},
		ArgSig(U"LumaOutput", false, DataType_FixedPoint),
		ArgSig(U"Source", true, DataType_ImageU8),
		ArgSig(U"X", true, DataType_FixedPoint),
		ArgSig(U"Y", true, DataType_FixedPoint),
		ArgSig(U"LumaBorder", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"READ_PIXEL_BORDER", 4,
		OPERATION {
			ColorRgbaI32 result = image_readPixel_border(
			  IMAGE_RGBAU8_REF(4),
			  INT_VALUE(5), INT_VALUE(6),
			  ColorRgbaI32(INT_VALUE(7), INT_VALUE(8), INT_VALUE(9), INT_VALUE(10))
			);
			SCALAR_REF(0) = FixedPoint::fromWhole(result.red);
			SCALAR_REF(1) = FixedPoint::fromWhole(result.green);
			SCALAR_REF(2) = FixedPoint::fromWhole(result.blue);
			SCALAR_REF(3) = FixedPoint::fromWhole(result.alpha);
			NEXT_INSTRUCTION
		},
		ArgSig(U"RedOutput", false, DataType_FixedPoint),
		ArgSig(U"GreenOutput", false, DataType_FixedPoint),
		ArgSig(U"BlueOutput", false, DataType_FixedPoint),
		ArgSig(U"AlphaOutput", false, DataType_FixedPoint),
		ArgSig(U"Source", true, DataType_ImageRgbaU8),
		ArgSig(U"X", true, DataType_FixedPoint),
		ArgSig(U"Y", true, DataType_FixedPoint),
		ArgSig(U"RedBorder", true, DataType_FixedPoint),
		ArgSig(U"GreenBorder", true, DataType_FixedPoint),
		ArgSig(U"BlueBorder", true, DataType_FixedPoint),
		ArgSig(U"AlphaBorder", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"READ_PIXEL_CLAMP", 1,
		OPERATION {
			SCALAR_REF(0) = FixedPoint::fromWhole(image_readPixel_clamp(IMAGE_U8_REF(1), INT_VALUE(2), INT_VALUE(3)));
			NEXT_INSTRUCTION
		},
		ArgSig(U"LumaOutput", false, DataType_FixedPoint),
		ArgSig(U"Source", true, DataType_ImageU8),
		ArgSig(U"X", true, DataType_FixedPoint),
		ArgSig(U"Y", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"READ_PIXEL_CLAMP", 4,
		OPERATION {
			ColorRgbaI32 result = image_readPixel_clamp(
			  IMAGE_RGBAU8_REF(4),
			  INT_VALUE(5), INT_VALUE(6)
			);
			SCALAR_REF(0) = FixedPoint::fromWhole(result.red);
			SCALAR_REF(1) = FixedPoint::fromWhole(result.green);
			SCALAR_REF(2) = FixedPoint::fromWhole(result.blue);
			SCALAR_REF(3) = FixedPoint::fromWhole(result.alpha);
			NEXT_INSTRUCTION
		},
		ArgSig(U"RedOutput", false, DataType_FixedPoint),
		ArgSig(U"GreenOutput", false, DataType_FixedPoint),
		ArgSig(U"BlueOutput", false, DataType_FixedPoint),
		ArgSig(U"AlphaOutput", false, DataType_FixedPoint),
		ArgSig(U"Source", true, DataType_ImageRgbaU8),
		ArgSig(U"X", true, DataType_FixedPoint),
		ArgSig(U"Y", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"READ_PIXEL_TILE", 1,
		OPERATION {
			SCALAR_REF(0) = FixedPoint::fromWhole(image_readPixel_tile(IMAGE_U8_REF(1), INT_VALUE(2), INT_VALUE(3)));
			NEXT_INSTRUCTION
		},
		ArgSig(U"LumaOutput", false, DataType_FixedPoint),
		ArgSig(U"Source", true, DataType_ImageU8),
		ArgSig(U"X", true, DataType_FixedPoint),
		ArgSig(U"Y", true, DataType_FixedPoint)
	),
	InsSig<MEDIA_MACHINE_TYPE_COUNT>::create(U"READ_PIXEL_TILE", 4,
		OPERATION {
			ColorRgbaI32 result = image_readPixel_tile(
			  IMAGE_RGBAU8_REF(4),
			  INT_VALUE(5), INT_VALUE(6)
			);
			SCALAR_REF(0) = FixedPoint::fromWhole(result.red);
			SCALAR_REF(1) = FixedPoint::fromWhole(result.green);
			SCALAR_REF(2) = FixedPoint::fromWhole(result.blue);
			SCALAR_REF(3) = FixedPoint::fromWhole(result.alpha);
			NEXT_INSTRUCTION
		},
		ArgSig(U"RedOutput", false, DataType_FixedPoint),
		ArgSig(U"GreenOutput", false, DataType_FixedPoint),
		ArgSig(U"BlueOutput", false, DataType_FixedPoint),
		ArgSig(U"AlphaOutput", false, DataType_FixedPoint),
		ArgSig(U"Source", true, DataType_ImageRgbaU8),
		ArgSig(U"X", true, DataType_FixedPoint),
		ArgSig(U"Y", true, DataType_FixedPoint)
	)
};

// API implementation

static void checkMachine(const MediaMachine& machine) {
	if (machine.isNull()) {
		throwError(U"The given media machine does not exist!");
	}
}

static void checkMethodIndex(const MediaMachine& machine, int32_t methodIndex) {
	checkMachine(machine);
	if (methodIndex < 0 || methodIndex >= machine->methods.length()) {
		throwError(U"Invalid method index ", methodIndex, U" of 0..", (machine->methods.length() - 1), U"!");
	}
}

static VMA resolveImmediateConstant(PlanarMemory<MEDIA_MACHINE_TYPE_COUNT> &memory, const ReadableString &argument) {
	MediaMemory &mediaMemory = (MediaMemory&)memory;
	DsrChar firstCharacter = argument[0];
	if (firstCharacter == U'-' || (U'0' <= firstCharacter || firstCharacter <= U'9')) {
		return VMA(ArgumentType::Immediate, DataType_FixedPoint, mediaMemory.fixedPointMemory.allocateImmediate(FixedPoint::fromText(argument)));
	} else {
		throwError(U"The media machine does not recognize the ", argument, U" argument as an immediate value!\n");
		return VMA(ArgumentType::Immediate, -1, -1);
	}
}

MediaMachine mediaMachine_create(const ReadableString& code) {
	Handle<PlanarMemory<MEDIA_MACHINE_TYPE_COUNT>> memory = handle_create<MediaMemory>().setName("MediaMemory");
	static const int32_t mediaMachineInstructionCount = sizeof(mediaMachineInstructions) / sizeof(InsSig<MEDIA_MACHINE_TYPE_COUNT>);
	static const int32_t mediaMachineTypeCount = sizeof(mediaMachineTypes) / sizeof(VMTypeDef<MEDIA_MACHINE_TYPE_COUNT>);
	return MediaMachine(handle_create<VirtualMachine<MEDIA_MACHINE_TYPE_COUNT>>(code, memory, mediaMachineInstructions, mediaMachineInstructionCount, mediaMachineTypes, mediaMachineTypeCount, resolveImmediateConstant).setName("MediaMachine"));
}

void mediaMachine_executeMethod(MediaMachine& machine, int32_t methodIndex) {
	checkMethodIndex(machine, methodIndex);
	machine->executeMethod(methodIndex);
}

template <typename T>
static void setInputByIndex(MemoryPlane<T>& stack, int32_t framePointer, Method<MEDIA_MACHINE_TYPE_COUNT>& method, DataType givenType, int32_t inputIndex, const T& value) {
	if (inputIndex < 0 || inputIndex >= method.inputCount) {
		throwError(U"Invalid input index ", inputIndex, U" of 0..", (method.inputCount - 1), U".");
	}
	Variable<MEDIA_MACHINE_TYPE_COUNT>* variable = &method.locals[inputIndex];
	DataType expected = variable->typeDescription->dataType;
	if (givenType != expected) {
		throwError(U"Cannot assign ", getMediaTypeName(givenType), U" to ", variable->name, U" of ", getMediaTypeName(expected), U".");
	}
	stack.accessByStackIndex(framePointer + variable->typeLocalIndex) = value;
}
template <typename T>
static T& accessOutputByIndex(MemoryPlane<T>& stack, int32_t framePointer, Method<MEDIA_MACHINE_TYPE_COUNT>& method, DataType wantedType, int32_t outputIndex) {
	if (outputIndex < 0 || outputIndex >= method.outputCount) {
		throwError(U"Invalid output index ", outputIndex, U" of 0..", (method.outputCount - 1), U".");
	}
	Variable<MEDIA_MACHINE_TYPE_COUNT>* variable = &method.locals[method.inputCount + outputIndex];
	DataType foundType = variable->typeDescription->dataType;
	if (wantedType != foundType) {
		throwError(U"Cannot get ", variable->name, U" of ", getMediaTypeName(wantedType), U" as ", getMediaTypeName(wantedType), U".");
	}
	return stack.accessByStackIndex(framePointer + variable->typeLocalIndex);
}

// Set input by argument index
//   Indexed arguments are confirmed to be inputs during compilation of the script
void mediaMachine_setInputByIndex(MediaMachine& machine, int32_t methodIndex, int32_t inputIndex, int32_t input) {
	#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
		printText(U"Input ", inputIndex, U" of ", machine->methods[methodIndex].inputCount, U" (", machine->methods[methodIndex].locals[inputIndex].name, U") to ", machine->methods[methodIndex].name, U" = ", input, U"\n");
	#endif
	checkMethodIndex(machine, methodIndex);
	setInputByIndex(((MediaMemory*)machine->memory.getUnsafe())->fixedPointMemory, machine->memory->current.framePointer[DataType_FixedPoint], machine->methods[methodIndex], DataType_FixedPoint, inputIndex, FixedPoint::fromWhole(input));
}
void mediaMachine_setInputByIndex(MediaMachine& machine, int32_t methodIndex, int32_t inputIndex, const FixedPoint& input) {
	#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
		printText(U"Input ", inputIndex, U" of ", machine->methods[methodIndex].inputCount, U" (", machine->methods[methodIndex].locals[inputIndex].name, U") to ", machine->methods[methodIndex].name, U" = ", input, U"\n");
	#endif
	checkMethodIndex(machine, methodIndex);
	setInputByIndex(((MediaMemory*)machine->memory.getUnsafe())->fixedPointMemory, machine->memory->current.framePointer[DataType_FixedPoint], machine->methods[methodIndex], DataType_FixedPoint, inputIndex, input);
}
void mediaMachine_setInputByIndex(MediaMachine& machine, int32_t methodIndex, int32_t inputIndex, const AlignedImageU8& input) {
	#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
		printText(U"Input ", inputIndex, U" of ", machine->methods[methodIndex].inputCount, U" (", machine->methods[methodIndex].locals[inputIndex].name, U") to ", machine->methods[methodIndex].name, U" = monochrome image of ", image_getWidth(input), U"x", image_getHeight(input), U" pixels\n");
	#endif
	checkMethodIndex(machine, methodIndex);
	setInputByIndex(((MediaMemory*)machine->memory.getUnsafe())->alignedImageU8Memory, machine->memory->current.framePointer[DataType_ImageU8], machine->methods[methodIndex], DataType_ImageU8, inputIndex, input);
}
void mediaMachine_setInputByIndex(MediaMachine& machine, int32_t methodIndex, int32_t inputIndex, const OrderedImageRgbaU8& input) {
	#ifdef VIRTUAL_MACHINE_DEBUG_PRINT
		printText(U"Input ", inputIndex, U" of ", machine->methods[methodIndex].inputCount, U" (", machine->methods[methodIndex].locals[inputIndex].name, U") to ", machine->methods[methodIndex].name, U" = rgba image of ", image_getWidth(input), U"x", image_getHeight(input), U" pixels\n");
	#endif
	checkMethodIndex(machine, methodIndex);
	setInputByIndex(((MediaMemory*)machine->memory.getUnsafe())->orderedImageRgbaU8Memory, machine->memory->current.framePointer[DataType_ImageRgbaU8], machine->methods[methodIndex], DataType_ImageRgbaU8, inputIndex, input);
}

// Get output by index
FixedPoint mediaMachine_getFixedPointOutputByIndex(const MediaMachine& machine, int32_t methodIndex, int32_t outputIndex) {
	checkMethodIndex(machine, methodIndex);
	return accessOutputByIndex<FixedPoint>(((MediaMemory*)machine->memory.getUnsafe())->fixedPointMemory, machine->memory->current.framePointer[DataType_FixedPoint], machine->methods[methodIndex], DataType_FixedPoint, outputIndex);
}
AlignedImageU8 mediaMachine_getImageU8OutputByIndex(const MediaMachine& machine, int32_t methodIndex, int32_t outputIndex) {
	checkMethodIndex(machine, methodIndex);
	return accessOutputByIndex<AlignedImageU8>(((MediaMemory*)machine->memory.getUnsafe())->alignedImageU8Memory, machine->memory->current.framePointer[DataType_ImageU8], machine->methods[methodIndex], DataType_ImageU8, outputIndex);
}
OrderedImageRgbaU8 mediaMachine_getImageRgbaU8OutputByIndex(const MediaMachine& machine, int32_t methodIndex, int32_t outputIndex) {
	checkMethodIndex(machine, methodIndex);
	return accessOutputByIndex<OrderedImageRgbaU8>(((MediaMemory*)machine->memory.getUnsafe())->orderedImageRgbaU8Memory, machine->memory->current.framePointer[DataType_ImageRgbaU8], machine->methods[methodIndex], DataType_ImageRgbaU8, outputIndex);
}

bool mediaMachine_exists(const MediaMachine& machine) {
	return machine.isNotNull();
}

int32_t mediaMachine_findMethod(const MediaMachine& machine, const ReadableString& methodName) {
	if (!mediaMachine_exists(machine)) {
		throwError(U"Can not look for ", methodName, U" in a media machine that does not exist!\n");
		return -1;
	} else {
		return machine->findMethod(methodName);
	}
}

MediaMethod mediaMachine_getMethod(MediaMachine& machine, const ReadableString& methodName, int32_t contextIndex, bool mustExist) {
	int32_t methodIndex = mediaMachine_findMethod(machine, methodName);
	if (mustExist && methodIndex == -1) {
		throwError(U"The method name ", methodName, U" could not be found in the media machine!\n");
	}
	return MediaMethod(machine, methodIndex, contextIndex);
}

String mediaMachine_getMethodName(const MediaMachine& machine, int32_t methodIndex) {
	checkMethodIndex(machine, methodIndex);
	return machine->methods[methodIndex].name;
}

int32_t mediaMachine_getMethodCount(const MediaMachine& machine) {
	checkMachine(machine);
	return machine->methods.length();
}

int32_t mediaMachine_getInputCount(const MediaMachine& machine, int32_t methodIndex) {
	checkMethodIndex(machine, methodIndex);
	return machine->methods[methodIndex].inputCount;
}

int32_t mediaMachine_getOutputCount(const MediaMachine& machine, int32_t methodIndex) {
	checkMethodIndex(machine, methodIndex);
	return machine->methods[methodIndex].outputCount;
}

String mediaMachine_getInputName(const MediaMachine& machine, int32_t methodIndex, int32_t inputIndex) {
	checkMethodIndex(machine, methodIndex);
	Method<MEDIA_MACHINE_TYPE_COUNT> *method = &(machine->methods[methodIndex]);
	if (inputIndex < 0 || inputIndex >= method->inputCount) {
		throwError(U"Input index ", inputIndex, U" is out of bound 0..", method->inputCount - 1, U" for method ", mediaMachine_getMethodName(machine, methodIndex), U"!\n");
	}
	return method->locals[inputIndex].name;
}

String mediaMachine_getOutputName(const MediaMachine& machine, int32_t methodIndex, int32_t outputIndex) {
	checkMethodIndex(machine, methodIndex);
	Method<MEDIA_MACHINE_TYPE_COUNT> *method = &(machine->methods[methodIndex]);
	if (outputIndex < 0 || outputIndex >= method->outputCount) {
		throwError(U"Input index ", outputIndex, U" is out of bound 0..", method->outputCount - 1, U" for method ", mediaMachine_getMethodName(machine, methodIndex), U"!\n");
	}
	return method->locals[method->inputCount + outputIndex].name;
}

MediaResult MediaMethod::callUsingKeywords(const TemporaryCallback<void(MediaMachine &machine, int32_t methodIndex, int32_t inputIndex, const ReadableString &argumentName)> &setInputAction) {
	if (this->methodIndex < 0 || this->methodIndex >= this->machine->methods.length()) {
		throwError(U"Method index ", this->methodIndex, U" is out of bound 0..", this->machine->methods.length() - 1, U"\n");
	}
	Method<MEDIA_MACHINE_TYPE_COUNT> *method = &(this->machine->methods[this->methodIndex]);
	int32_t inputCount = method->inputCount;
	// TODO: Make sure that input arguments are assigned default arguments before assigning inputs as keywords.
	for (int32_t i = 0; i < inputCount; i++) {
		setInputAction(this->machine, this->methodIndex, i, method->locals[i].name);
	}
	mediaMachine_executeMethod(this->machine, this->methodIndex);
	return MediaResult(this->machine, this->methodIndex);
}

}
