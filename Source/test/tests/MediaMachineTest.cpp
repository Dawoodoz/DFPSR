
#include "../testTools.h"
#include "../../DFPSR/api/mediaMachineAPI.h"

START_TEST(CompilerFront)
	{
		MediaMachine testMachine = mediaMachine_create(
			U"HIDDEN: ImageU8, firstGlobalVariable\n"
			U"HIDDEN: ImageRgbaU8, secondGlobalVariable\n"
			U"BEGIN: addFixedPoint\n"
			U"	INPUT: FixedPoint, left\n"
			U"	INPUT: FixedPoint, right\n"
			U"	OUTPUT: FixedPoint, result\n"
			U"	ADD: result, left, right\n"
			U"END:\n"
			U"HIDDEN: FixedPoint, someGlobalVariable\n"
			U"BEGIN: addEight\n"
			U"	INPUT: FixedPoint, x\n"
			U"	OUTPUT: FixedPoint, result\n"
			U"	CALL: addFixedPoint, result, x, 8\n"
			U"	JUMP: myLabel\n"
			U"		MOVE: result, 10\n" // Dead code to jump past
			U"	LABEL: myLabel\n"
			U"	TEMP: FixedPoint, aTemporaryVariable\n"
			U"END:\n"
			U"HIDDEN: ImageU8, anotherGlobalVariable\n"
		);
		ASSERT(mediaMachine_exists(testMachine));
		{
			int32_t addMethod = mediaMachine_findMethod(testMachine, U"addFixedPoint");
			ASSERT_NOT_EQUAL(addMethod, -1);
			mediaMachine_setInputByIndex(testMachine, addMethod, 0, FixedPoint::fromWhole(1200));
			mediaMachine_setInputByIndex(testMachine, addMethod, 1, FixedPoint::fromWhole(34));
			mediaMachine_executeMethod(testMachine, addMethod);
			ASSERT_EQUAL(mediaMachine_getFixedPointOutputByIndex(testMachine, addMethod, 0), FixedPoint::fromWhole(1234));
		}
		{
			MediaMethod addMethod = mediaMachine_getMethod(testMachine, U"addFixedPoint", -1);
			FixedPoint result;
			addMethod(FixedPoint::fromWhole(1200), FixedPoint::fromWhole(34))(result);
			ASSERT_EQUAL(result, FixedPoint::fromWhole(1234));
		}
		{
			int32_t addEightMethod = mediaMachine_findMethod(testMachine, U"addEight");
			ASSERT_NOT_EQUAL(addEightMethod, -1);
			mediaMachine_setInputByIndex(testMachine, addEightMethod, 0, FixedPoint::fromWhole(120));
			mediaMachine_executeMethod(testMachine, addEightMethod);
			ASSERT_EQUAL(mediaMachine_getFixedPointOutputByIndex(testMachine, addEightMethod, 0), FixedPoint::fromWhole(128));
		}
	}
	{
		// A virtual machine containing the Fibonacci function.
		// Implemented using FixedPoint because MediaMachine does not have a pure integer type, so giving it fractions as input will give incorrect results.
		MediaMachine fibonacciMachine = mediaMachine_create(
			U"BEGIN: fibonacci\n"
			U"	INPUT: FixedPoint, n\n"
			U"	OUTPUT: FixedPoint, result\n"
			U"	TEMP: FixedPoint, n1, n2, r1, r2, condition\n"
			U"	LESSER_EQUAL: condition, n, 0\n"           // In case of negative input, case zero accepts n <= 0.
			U"		JUMP_IF_TRUE: case_zero, condition\n"
			U"	LESSER_EQUAL: condition, n, 1\n"           // In case of giving inputs between 0 and 1, case one accepts 0 < n <= 1.
			U"		JUMP_IF_TRUE: case_one, condition\n"
			U"	JUMP: case_n\n"
			U"	LABEL: case_zero\n"                        // If n <= 0
			U"		MOVE: result, 0\n"                     // result = 0
			U"	JUMP: done\n"
			U"	LABEL: case_one\n"                         // If n == 1
			U"		MOVE: result, 1\n"                     // result = 1
			U"	JUMP: done\n"
			U"	LABEL: case_n\n"                           // If n > 1
			U"		SUB: n2, n, 2\n"
			U"		SUB: n1, n, 1\n"
			U"		CALL: fibonacci, r2, n2\n"
			U"		CALL: fibonacci, r1, n1\n"
			U"		ADD: result, r2, r1\n"                 // result = fibonacci(n - 2) + fibonacci(n - 1)
			U"	LABEL: done\n"
			U"END:\n"
		);
		ASSERT(mediaMachine_exists(fibonacciMachine));
		int32_t fibonacciFunction = mediaMachine_findMethod(fibonacciMachine, U"fibonacci");
		ASSERT_NOT_EQUAL(fibonacciFunction, -1);
		mediaMachine_setInputByIndex(fibonacciMachine, fibonacciFunction, 0, FixedPoint::fromWhole(6));
		mediaMachine_executeMethod(fibonacciMachine, fibonacciFunction);
		FixedPoint result = mediaMachine_getFixedPointOutputByIndex(fibonacciMachine, fibonacciFunction, 0);
		// Input     : 0 1 2 3 4 5 6...
		// Fibonacci : 0 1 1 2 3 5 8...
		ASSERT_EQUAL(result, FixedPoint::fromWhole(8));
	}
END_TEST
