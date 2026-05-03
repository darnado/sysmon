/*
 * tests/test_mem.c — Unit tests for memory helpers
 */

#include "test_framework.h"
#include "../include/sysmon.h"

void suite_mem(void)
{
    printf("\n=== Memory tests ===\n");

    /* Half of total RAM is available → 50% used */
    {
        mem_info_t info = {
            .total     = 8 * 1024 * 1024,
            .available = 4 * 1024 * 1024,
        };
        ASSERT_NEAR(mem_used_percent(&info), 50.0, 0.01, "mem_used: 50%");
    }

    /* Nothing available → 100% used */
    {
        mem_info_t info = { .total = 1024, .available = 0 };
        ASSERT_NEAR(mem_used_percent(&info), 100.0, 0.01, "mem_used: 100%");
    }

    /* Total is zero → should return 0, not crash or NaN */
    {
        mem_info_t info = { .total = 0, .available = 0 };
        ASSERT_NEAR(mem_used_percent(&info), 0.0, 0.01,
                    "mem_used: zero total → 0%");
    }
}
