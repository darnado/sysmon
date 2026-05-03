/*
 * tests/test_main.c — Test runner entry point
 *
 * Defines the shared counters and calls each test suite.
 */

#include <stdio.h>
#include <stdlib.h>
#include "test_framework.h"

/* Shared counters referenced by all test suites via test_framework.h */
int g_tests_run  = 0;
int g_tests_fail = 0;

int main(void)
{
    suite_cpu();
    suite_mem();

    printf("\n%d/%d tests passed\n", g_tests_run - g_tests_fail, g_tests_run);
    return g_tests_fail > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
