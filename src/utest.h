/*
	utest - The micro unit test framework for C99
 
	This file provides both the interface and the implementation.

	To provide the implementation for this library, you MUST define
	UTEST_C_IMPLEMENTATION in exactly on source file.

		#define UTEST_C_IMPLEMENTATION
		#include "utest.h"

	USAGE
	-----

	See README.md for details


	LICENSE
	-------

	The MIT License (MIT)

	Copyright (c) 2015 Oli Wilkinson

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
 */

#ifndef UTESTI_INCLUDE_UTEST_H_
#define UTESTI_INCLUDE_UTEST_H_

#include <stdint.h>
#include <setjmp.h> 

#ifndef UTEST_MAX_TESTS_PER_FIXTURE
#define UTEST_MAX_TESTS_PER_FIXTURE	265
#endif

typedef void(*_utest_func)();

typedef struct _utest_entry
{
	// name of the test
	const char* name;

	// Function to run for the actual test
	_utest_func test;

	const char* file;
	const int line;

#ifdef __cplusplus
	_utest_entry() : name(0), test(0), file(0), line(0) {}
	_utest_entry(const char* a, _utest_func b, const char* c, const int d) : name(a), test(b), file(c), line(d) {}
#endif

} utest_entry;

typedef struct _utest_fixture
{
	// name of the fixture
	const char* name;

	// Set up (run before test)
	_utest_func setup;
	// Teardown run after test
	_utest_func teardown;

	utest_entry tests[UTEST_MAX_TESTS_PER_FIXTURE];
} utest_fixture;

typedef struct _utest_state
{
	void*	user_state;

	int run_count;
	int pass_count;
	int fail_count;

	// working state
	utest_entry* current_test;
	jmp_buf	restore_env;

} utest_state;


typedef enum _utest_test_result
{
	TEST_RESULT_SUCCESS = 0,
	TEST_RESULT_FAILED = -1,
} utest_test_result;

#ifdef __cplusplus
extern "C" {
#endif

extern utest_test_result utest_run_fixture(utest_fixture fixture);
extern utest_test_result utest_run_test(utest_entry* test);

// assert stuff
extern void utest_assert_integer_equal(signed expected, signed actual, const char* message);
extern void utest_assert_null(void* actual, char expect_null, const char* message);
extern void utest_assert_float_equal(double expected, double actual, double epsilon, const char* message);
extern void utest_assert_string_equal(const char* expected, const char* actual, const char* message, char ignore_case);
extern void utest_assert_pointer_equal(void* expected, void* actual, const char* message);

extern void utest_fail(const char* format, ...);

extern utest_state g_utest_state;

extern void* utest_get_user();
extern void utest_set_user(void*);

#ifdef __cplusplus
}
#endif

/////////////
// Assert helpers
#define TEST_FAIL_MESSAGE(message)		utest_fail(message);
#define TEST_FAIL						TEST_FAIL_MESSAGE(0)

#define TEST_ASSERT_MESSAGE(expr, message)	if(!(expr)) { TEST_FAIL_MESSAGE(message); }

#define TEST_ASSERT(expr)		TEST_ASSERT_MESSAGE(expr, "Expression is false")

#define TEST_ASSERT_EQUAL_P(expected, actual, message)	utest_assert_pointer_equal(expected, actual, message)
#define TEST_ASSERT_EQUAL_I(type, expected, actual, message)	utest_assert_integer_equal((type)expected, (type)actual, message)
#define TEST_ASSERT_EQUAL_F(type, expected, actual, epsilon, message)	utest_assert_float_equal((type)expected, (type)actual, epsilon, message)

#define TEST_ASSERT_EQUAL_INT(expected, actual)		TEST_ASSERT_EQUAL_I(signed int, expected, actual, 0)
#define TEST_ASSERT_EQUAL_INT8(expected, actual)	TEST_ASSERT_EQUAL_I(int8_t, expected, actual, 0)
#define TEST_ASSERT_EQUAL_INT16(expected, actual)	TEST_ASSERT_EQUAL_I(int16_t, expected, actual, 0)
#define TEST_ASSERT_EQUAL_INT32(expected, actual)	TEST_ASSERT_EQUAL_I(int32_t, expected, actual, 0)
#define TEST_ASSERT_EQUAL_INT64(expected, actual)	TEST_ASSERT_EQUAL_I(int64_t, expected, actual, 0)

#define TEST_ASSERT_EQUAL_UINT(expected, actual)	TEST_ASSERT_EQUAL_I(unsigned int, expected, actual, 0)
#define TEST_ASSERT_EQUAL_UINT8(expected, actual)	TEST_ASSERT_EQUAL_I(uint8_t, expected, actual, 0)
#define TEST_ASSERT_EQUAL_UINT16(expected, actual)	TEST_ASSERT_EQUAL_I(uint16_t, expected, actual, 0)
#define TEST_ASSERT_EQUAL_UINT32(expected, actual)	TEST_ASSERT_EQUAL_I(uint32_t, expected, actual, 0)
#define TEST_ASSERT_EQUAL_UINT64(expected, actual)	TEST_ASSERT_EQUAL_I(uint64_t, expected, actual, 0)

#define TEST_ASSERT_EQUAL_PTR(expected, actual)				TEST_ASSERT_EQUAL_P(expected, actual, 0)
#define TEST_ASSERT_EQUAL_PTR_MESSAGE(expected, actual, message)	TEST_ASSERT_EQUAL_P(expected, actual, message)

#define TEST_ASSERT_NULL(actual)		utest_assert_null(actual, 1, 0)
#define TEST_ASSERT_NULL_MESSAGE(actual, message)		utest_assert_null(actual, 1, message)

#define TEST_ASSERT_NOT_NULL(actual)		utest_assert_null(actual, 0, 0)
#define TEST_ASSERT_NOT_NULL_MESSAGE(actual, message)		utest_assert_null(actual, 0, message)

#define TEST_ASSERT_EQUAL_FLOAT(expected, actual, epsilon)	TEST_ASSERT_EQUAL_F(float, expected, actual, epsilon, 0)
#define TEST_ASSERT_EQUAL_DOUBLE(expected, actual, epsilon)	TEST_ASSERT_EQUAL_F(double, expected, actual, epsilon, 0)

#define TEST_ASSERT_EQUAL_INT_MESSAGE(expected, actual, message)	TEST_ASSERT_EQUAL_I(signed int, expected, actual, message)
#define TEST_ASSERT_EQUAL_INT8_MESSAGE(expected, actual, message)	TEST_ASSERT_EQUAL_I(int8_t, expected, actual, message)
#define TEST_ASSERT_EQUAL_INT16_MESSAGE(expected, actual, message)	TEST_ASSERT_EQUAL_I(int16_t, expected, actual, message)
#define TEST_ASSERT_EQUAL_INT32_MESSAGE(expected, actual, message)	TEST_ASSERT_EQUAL_I(int32_t, expected, actual, message)
#define TEST_ASSERT_EQUAL_INT64_MESSAGE(expected, actual, message)	TEST_ASSERT_EQUAL_I(int64_t, expected, actual, message)

#define TEST_ASSERT_EQUAL_UINT_MESSAGE(expected, actual, message)	TEST_ASSERT_EQUAL_I(unsigned int, expected, actual, message)
#define TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected, actual, message)	TEST_ASSERT_EQUAL_I(uint8_t, expected, actual, message)
#define TEST_ASSERT_EQUAL_UINT16_MESSAGE(expected, actual, message)	TEST_ASSERT_EQUAL_I(uint16_t, expected, actual, message)
#define TEST_ASSERT_EQUAL_UINT32_MESSAGE(expected, actual, message)	TEST_ASSERT_EQUAL_I(uint32_t, expected, actual, message)
#define TEST_ASSERT_EQUAL_UINT64_MESSAGE(expected, actual, message)	TEST_ASSERT_EQUAL_I(uint64_t, expected, actual, message)

#define TEST_ASSERT_EQUAL_FLOAT_MESSAGE(expected, actual, epsilon, message)	TEST_ASSERT_EQUAL_F(float, expected, actual, epsilon, message)
#define TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(expected, actual, epsilon, message)	TEST_ASSERT_EQUAL_F(double, expected, actual, epsilon, message)

#define TEST_ASSERT_EQUAL_NOCASE_STRING_MESSAGE(expected, actual, message)		utest_assert_string_equal(expected, actual, message, 1)
#define TEST_ASSERT_EQUAL_NOCASE_STRING(expected, actual)		TEST_ASSERT_EQUAL_NOCASE_STRING_MESSAGE(expected, actual, 0)

#define TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, actual, message)		utest_assert_string_equal(expected, actual, message, 0)
#define TEST_ASSERT_EQUAL_STRING(expected, actual)		TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, actual, 0)

//////////////
// Test specs
#define TEST(test_name)	\
	void test_name()

#define DECLARE_TEST(test_name)	\
	void test_name();

#define TEST_FIXTURE_BEGIN_F(fixture_name, setup, teardown)	\
static utest_fixture fixture_name = { #fixture_name, setup, teardown, {

#define TEST_FIXTURE_BEGIN(fixture_name)	TEST_FIXTURE_BEGIN_F(fixture_name, 0, 0)

// Adds a test to a fixture
#define TEST_FIXTURE_TEST(test_name)	\
			{ #test_name, test_name, __FILE__, __LINE__ },


#define TEST_FIXTURE_END()	\
			{ 0, 0, 0, 0 } \
		} \
	};

#endif

#ifdef UTEST_C_IMPLEMENTATION

#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h> 
#include <string.h>

#define __UTEST_FAIL_TEST_EXIT()		\
	longjmp(g_utest_state.restore_env, 1); \
	return;

#ifndef UTEST_MSG_BUFFER_SIZE
#define UTEST_MSG_BUFFER_SIZE	512
#endif

// Global state
static utest_state g_utest_state;

void utest_print(const char* message)
{
	printf("%s\r\n", message);
}

utest_test_result utest_run_fixture(utest_fixture fixture)
{
	utest_entry* test = fixture.tests;
	g_utest_state.run_count = 0;
	g_utest_state.pass_count = 0;
	g_utest_state.fail_count = 0;

	while (test->test)
	{
		g_utest_state.current_test = test;
		g_utest_state.run_count++;

		if (fixture.setup)
		{
			(*fixture.setup)();
		}

		utest_test_result result = utest_run_test(test++);

		if (fixture.teardown)
		{
			(*fixture.teardown)();
		}

		if (!result)
		{
			g_utest_state.pass_count++;
		}
		else
		{
			g_utest_state.fail_count++;
		}
	}

	return g_utest_state.fail_count ? TEST_RESULT_FAILED : TEST_RESULT_SUCCESS;
}

utest_test_result utest_run_test(utest_entry* test)
{
	// Grab current environment
	int result = setjmp(g_utest_state.restore_env);

	if (result == 0)
	{
		// Run the test
		(*test->test)();			
		return TEST_RESULT_SUCCESS;
	}
	else
	{		
		return TEST_RESULT_FAILED;
	}
}

//////////////////////////
// Assert implementations
//
void utest_assert_integer_equal(signed expected, signed actual, const char* message)
{
	if (expected == actual)
	{
		return;
	}
	if (message)
	{
		utest_fail("Values not equal. Expected [%i], Actual [%i] - %s", expected, actual, message);
	}
	else
	{
		utest_fail("Values not equal. Expected [%i], Actual [%i]", expected, actual);
	}
}

void utest_assert_null(void* actual, char expect_null, const char* message)
{
	if (expect_null)
	{
		if(!actual)
		{
			return;
		}

		if (message)
		{
			utest_fail("Value non-null. Expected null. %s", message);
		}
		else
		{
			utest_fail("Value non-null. Expected null.");
		}
	}

	// otherwise we're expecting null
	if(actual)
	{
		return;
	}

	if (message)
	{
		utest_fail("Value null. Expected non-null. %s", message);
	}
	else
	{
		utest_fail("Value null. Expected non-null.");
	}
	
}

void utest_assert_float_equal(double expected, double actual, double epsilon, const char* message)
{
	if (fabs(expected - actual) < epsilon)
	{
		return;
	}

	if (message)
	{
		utest_fail("Values not equal. Expected [%lf], Actual [%lf] - %s", expected, actual, message);
	}
	else
	{
		utest_fail("Values not equal. Expected [%lf], Actual [%lf]", expected, actual);
	}
}

void utest_assert_string_equal(const char* expected, const char* actual, const char* message, char ignore_case)
{
	if (ignore_case)
	{
		if (_stricmp(expected, actual) == 0)
		{
			return;
		}
	}
	else
	{
		if (strcmp(expected, actual) == 0)
		{
			return;
		}
	}	

	if (message)
	{
		utest_fail("Strings not equal. Expected [%s], Actual [%s] - %s", expected, actual, message);
	}
	else
	{
		utest_fail("Strings not equal. Expected [%s], Actual [%s]", expected, actual);
	}
}

void utest_assert_pointer_equal(void* expected, void* actual, const char* message)
{
	if (expected == actual)
	{
		return;
	}

	if (message)
	{
		utest_fail("Pointers not equal. Expected [0x%016x], Actual [0x%016x] - %s", expected, actual, message);
	}
	else
	{
		utest_fail("Pointers not equal. Expected [0x%016x], Actual [0x%016x]", expected, actual);
	}
}

void utest_fail(const char* format, ...)
{
	char msgbuffer[UTEST_MSG_BUFFER_SIZE] = { 0 };

	va_list args;
	va_start(args, format);
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
	vsnprintf_s(msgbuffer, UTEST_MSG_BUFFER_SIZE, UTEST_MSG_BUFFER_SIZE, format, args);
#else
	vsprintf(msgbuffer, format, args);
#endif		
	va_end(args);

	printf("%s(%i): Test failed: '%s': %s\r\n", 
		g_utest_state.current_test->file, g_utest_state.current_test->line,
		g_utest_state.current_test->name, msgbuffer);

	__UTEST_FAIL_TEST_EXIT();
}

void* utest_get_user()
{
	return g_utest_state.user_state;
}

void utest_set_user(void* state)
{
	g_utest_state.user_state = state;
}
#endif // UTEST_C_IMPLEMENTATION