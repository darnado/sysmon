/*
 * tests/test_framework.h — Minimal test harness shared across test files.
 *
 * Each test file includes this header.  The counters are defined once in
 * test_main.c, which also drives the overall test run.
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>

#define T_PASS "\033[32mPASS\033[0m"
#define T_FAIL "\033[31mFAIL\033[0m"

/* Counters incremented by ASSERT_NEAR */
extern int g_tests_run;
extern int g_tests_fail;

static inline void _assert_near(double got, double want, double tol,
                                 const char *msg)
{
    double diff = got - want;
    g_tests_run++;
    if (diff < -tol || diff > tol) {
        printf("[" T_FAIL "] %s  (got %.4f, want %.4f)\n", msg, got, want);
        g_tests_fail++;
    } else {
        printf("[" T_PASS "] %s\n", msg);
    }
}

static inline void _assert_true(int cond, const char *msg)
{
    g_tests_run++;
    if (!cond) {
        printf("[" T_FAIL "] %s\n", msg);
        g_tests_fail++;
    } else {
        printf("[" T_PASS "] %s\n", msg);
    }
}

#define ASSERT_NEAR(got, want, tol, msg) \
    _assert_near((double)(got), (double)(want), (double)(tol), (msg))

#define ASSERT_TRUE(cond, msg) \
    _assert_true(!!(cond), (msg))

#define ASSERT_EQ(got, want, msg) \
    _assert_near((double)(got), (double)(want), 0.0, (msg))

/* Suites declare themselves via this signature */
void suite_cpu(void);
void suite_mem(void);

#endif /* TEST_FRAMEWORK_H */
