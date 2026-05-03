/*
 * tests/test_cpu.c — Unit tests for CPU usage calculation
 *
 * We test cpu_usage_percent() in isolation: the function is pure logic
 * operating on two cpu_stat_t structs, so it runs on any OS without /proc.
 */

#include "test_framework.h"
#include "../include/sysmon.h"

void suite_cpu(void)
{
    printf("=== CPU tests ===\n");

    /* 50%: active and idle counters grow by equal amounts */
    {
        cpu_stat_t prev = { .user=1000, .idle=1000 };
        cpu_stat_t curr = { .user=1500, .idle=1500 };
        ASSERT_NEAR(cpu_usage_percent(&prev, &curr), 50.0, 0.01,
                    "cpu_usage: 50% busy");
    }

    /* 100%: idle counter stays put, all time is active */
    {
        cpu_stat_t prev = { .user=0,    .idle=1000 };
        cpu_stat_t curr = { .user=1000, .idle=1000 };
        ASSERT_NEAR(cpu_usage_percent(&prev, &curr), 100.0, 0.01,
                    "cpu_usage: 100% busy");
    }

    /* 0%: only idle advances */
    {
        cpu_stat_t prev = { .user=0, .idle=0    };
        cpu_stat_t curr = { .user=0, .idle=1000 };
        ASSERT_NEAR(cpu_usage_percent(&prev, &curr), 0.0, 0.01,
                    "cpu_usage: 0% busy");
    }

    /* Identical snapshots: no time elapsed, must return 0 not NaN */
    {
        cpu_stat_t snap = { .user=500, .idle=500 };
        ASSERT_NEAR(cpu_usage_percent(&snap, &snap), 0.0, 0.01,
                    "cpu_usage: no time elapsed → 0%");
    }

    /* NULL arguments must return -1.0 */
    {
        cpu_stat_t snap = {0};
        ASSERT_NEAR(cpu_usage_percent(NULL, &snap), -1.0, 0.001,
                    "cpu_usage: NULL prev → -1.0");
        ASSERT_NEAR(cpu_usage_percent(&snap, NULL), -1.0, 0.001,
                    "cpu_usage: NULL curr → -1.0");
    }
}
