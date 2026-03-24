
#include "../testTools.h"
#include "../../DFPSR/api/mediaMachineAPI.h"

START_TEST(CompilerFront)
	{
		MediaMachine testMachine = machine_create(
			U"BEGIN: addFixedPoint\n"
			U"	INPUT: FixedPoint, left\n"
			U"	INPUT: FixedPoint, right\n"
			U"	OUTPUT: FixedPoint, result\n"
			U"	ADD: result, left, right\n"
			U"END:\n"
			U"BEGIN: addEight\n"
			U"	INPUT: FixedPoint, x\n"
			U"	OUTPUT: FixedPoint, result\n"
			U"	CALL: addFixedPoint, result, x, 8\n"
			U"END:\n"
		);
		ASSERT(machine_exists(testMachine));
		{
			int32_t addMethod = machine_findMethod(testMachine, U"addFixedPoint");
			ASSERT_NOT_EQUAL(addMethod, -1);
			machine_setInputByIndex(testMachine, addMethod, 0, FixedPoint::fromWhole(1200));
			machine_setInputByIndex(testMachine, addMethod, 1, FixedPoint::fromWhole(34));
			machine_executeMethod(testMachine, addMethod);
			ASSERT_EQUAL(machine_getFixedPointOutputByIndex(testMachine, addMethod, 0), FixedPoint::fromWhole(1234));
		}
		{
			MediaMethod addMethod = machine_getMethod(testMachine, U"addFixedPoint", -1);
			FixedPoint result;
			addMethod(FixedPoint::fromWhole(1200), FixedPoint::fromWhole(34))(result);
			ASSERT_EQUAL(result, FixedPoint::fromWhole(1234));
		}
		{
			int32_t addEightMethod = machine_findMethod(testMachine, U"addEight");
			ASSERT_NOT_EQUAL(addEightMethod, -1);
			machine_setInputByIndex(testMachine, addEightMethod, 0, FixedPoint::fromWhole(120));
			machine_executeMethod(testMachine, addEightMethod);
			ASSERT_EQUAL(machine_getFixedPointOutputByIndex(testMachine, addEightMethod, 0), FixedPoint::fromWhole(128));
		}
	}
END_TEST
