#ifndef TESTUTILS_HPP
#define TESTUTILS_HPP

#include <iostream>
#include <string>

static int	g_testsPassed = 0;
static int	g_testsFailed = 0;

#define ASSERT_TRUE(condition) \
	do { \
		if (condition) \
			g_testsPassed++; \
		else { \
			g_testsFailed++; \
			std::cerr << "FAIL: " << __FILE__ << ":" << __LINE__ \
			<< " --> " << #condition << std::endl; \
		} \
	} while (0)

#define ASSERT_EQ(expected, actual) \
	do { \
		if ((expected) == (actual)) \
			g_testsPassed++; \
		else { \
			g_testsFailed++; \
			std::cerr << "FAIL: " << __FILE__ << ":" << __LINE__ \
			<< "Expected [" << expected << "] but got [" \
			<< actual << "]" << std::endl \
		} \
	} while (0)

#endif
