
#include "../testTools.h"
#include "../../DFPSR/base/StorableCallback.h"
#include "../../DFPSR/base/TemporaryCallback.h"

int32_t multiplyByTwo(int32_t x) {
	return x * 2;
}

int32_t addOne(int32_t x) {
	return x + 1;
}

int32_t callTwice(const TemporaryCallback<int32_t(int32_t)> &f, int32_t x) {
	return f(f(x));
}

START_TEST(Callback)
	{ // Test that lambdas are working in the selected C++ version.
		// Lambdas are nameless structs containing the captured variables by value or reference.
		// They are called by binding the given function to the () operator in compile time.
		int32_t x = 1;
		int32_t y = 2;
		auto lambdaA = [ x,  y] (int32_t z) -> int32_t { return x + y + z; };
		auto lambdaB = [ x, &y] (int32_t z) -> int32_t { return x + y + z; };
		auto lambdaC = [&x,  y] (int32_t z) -> int32_t { return x + y + z; };
		auto lambdaD = [&x, &y] (int32_t z) -> int32_t { return x + y + z; };
		x = 5;
		y = 4;
		ASSERT_EQUAL(lambdaA(-2), 1); // 1 + 2 - 2 = 1
		ASSERT_EQUAL(lambdaB(-3), 2); // 1 + 4 - 3 = 2
		ASSERT_EQUAL(lambdaC(-4), 3); // 5 + 2 - 4 = 3
		ASSERT_EQUAL(lambdaD(-5), 4); // 5 + 4 - 5 = 4
	}
	// Because each lambda has its own nameless type, they can be difficult to pass to other functions.
	// We might also want to pass function pointers without a closure.
	// So TemporaryCallback can be used passed by const reference to functions without the need for template arguments.
	{ // Test TemporaryCallback constructed using function pointer.
		// Execute the multiply by two function twice.
		//   (3 * 2) * 2 = 12
		ASSERT_EQUAL(callTwice(&multiplyByTwo, 3), 12);
		// The & is optional when creating a function pointer from a function's identifier.
		ASSERT_EQUAL(callTwice(multiplyByTwo, 3), 12);
	}
	{ // Test TemporaryCallback constructed using lambda.
		int32_t y = 2;
		// Create a lambda that multiplies input argument x with captured variable y.
		//   Implicitly cast the lambda into TemporaryCallback allocated in a temporary variable and pass it as const reference to callTwice.
		//   Let callTwice do the multiplication twice:
		//     3 * 2 = 6
		//     6 * 2 = 12
		ASSERT_EQUAL(callTwice([y](int32_t x) -> int32_t { return x * y; }, 3), 12);
		// Updating y, which will later be captured by value when making a new temporary callback.
		y = 4;
		// Let callTwice do the multiplication twice:
		//   1 * 4 = 4
		//   4 * 4 = 16
		ASSERT_EQUAL(callTwice([y](int32_t x) -> int32_t { return x * y; }, 1), 16);
	}
	{ // Test StorableCallback constructed using function pointer.
		// Create a storable callback without any closure, from the function that doubles the integer.
		StorableCallback<int32_t(int32_t)> operation = &multiplyByTwo;
		// 5 * 2 = 10
		ASSERT_EQUAL(operation(5), 10);
		// Override by assignment, with the function that adds one.
		operation = &addOne;
		// 7 + 1 = 8
		ASSERT_EQUAL(operation(7), 8);
	}
	{ // Test StorableCallback constructed using lambda.
		int32_t x = 1;
		int32_t y = 2;
		// Because callbacks do not have default constructors, they should be constructed directly instead of using assignments.
		StorableCallback<int32_t(int32_t)> callbackA([ x,  y] (int32_t z) -> int32_t { return x + y + z; });
		StorableCallback<int32_t(int32_t)> callbackB([ x, &y] (int32_t z) -> int32_t { return x + y + z; });
		StorableCallback<int32_t(int32_t)> callbackC([&x,  y] (int32_t z) -> int32_t { return x + y + z; });
		StorableCallback<int32_t(int32_t)> callbackD([&x, &y] (int32_t z) -> int32_t { return x + y + z; });
		x = 5;
		y = 4;
		ASSERT_EQUAL(callbackA(-2), 1); // 1 + 2 - 2 = 1
		ASSERT_EQUAL(callbackB(-3), 2); // 1 + 4 - 3 = 2
		ASSERT_EQUAL(callbackC(-4), 3); // 5 + 2 - 4 = 3
		ASSERT_EQUAL(callbackD(-5), 4); // 5 + 4 - 5 = 4
	}
END_TEST
