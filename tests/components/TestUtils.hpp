#ifndef TESTUTILS_HPP
#define TESTUTILS_HPP

#include <iostream>
#include <string>

extern int	g_testsPassed;
extern int	g_testsFailed;

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
			<< " Expected [" << expected << "] but got [" \
			<< actual << "]" << std::endl; \
		} \
	} while (0)
#endif

#define RUN_TEST(test_fn) \
	do { \
		try { \
			test_fn(); \
		} catch (const std::exception &e) { \
			g_testsFailed++; \
			std::cerr << "FAIL: " << #test_fn << " threw exception: " << e.what() << std::endl; \
		} catch (...) { \
			g_testsFailed++; \
			std::cerr << "FAIL: " << #test_fn << " threw unknown exception: " << std::endl; \
		} \
	} while (0)
