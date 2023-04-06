
#include "../testTools.h"

START_TEST(Buffer)
	{
		Buffer a; // Empty handle
		Buffer b = buffer_create(0); // Empty buffer
		Buffer c = buffer_create(7); // Buffer
		ASSERT_EQUAL(buffer_exists(a), false);
		ASSERT_EQUAL(buffer_exists(b), true);
		ASSERT_EQUAL(buffer_exists(c), true);
		ASSERT_EQUAL(buffer_getSize(a), 0);
		ASSERT_EQUAL(buffer_getSize(b), 0);
		ASSERT_EQUAL(buffer_getSize(c), 7);
		ASSERT_EQUAL(buffer_getUseCount(a), 0);
		ASSERT_EQUAL(buffer_getUseCount(b), 1);
		ASSERT_EQUAL(buffer_getUseCount(c), 1);
		Buffer d = buffer_clone(a);
		Buffer e = buffer_clone(b); // Empty buffers are reused, which increases the use count.
		Buffer f = buffer_clone(c);
		ASSERT_EQUAL(buffer_getUseCount(a), 0);
		ASSERT_EQUAL(buffer_getUseCount(b), 2);
		ASSERT_EQUAL(buffer_getUseCount(c), 1);
		ASSERT_EQUAL(buffer_getUseCount(d), 0);
		ASSERT_EQUAL(buffer_getUseCount(e), 2);
		ASSERT_EQUAL(buffer_getUseCount(f), 1);
	}
END_TEST
