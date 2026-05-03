/*
 * display.c — Terminal output helpers
 *
 * Keeps all I/O in one place so the rest of the library stays pure logic.
 * The print functions write to stdout and use only ANSI escape codes that
 * are widely supported (VT100 compatible terminals, xterm, most Linux TTYs).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysmon.h"

/* Bar width in characters for the ASCII usage bar */
#define BAR_WIDTH 40

/* ANSI colour codes */
#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_RED     "\033[31m"
#define ANSI_CYAN    "\033[36m"

/* Choose a colour based on percentage: green < 60, yellow < 85, red >= 85 */
static const char *usage_color(double pct)
{
    if (pct < 60.0) return ANSI_GREEN;
    if (pct < 85.0) return ANSI_YELLOW;
    return ANSI_RED;
}

void print_cpu_bar(double usage_pct)
{
    int filled;
    int i;

    if (usage_pct < 0.0)  usage_pct = 0.0;
    if (usage_pct > 100.0) usage_pct = 100.0;

    filled = (int)(BAR_WIDTH * usage_pct / 100.0);

    printf(ANSI_BOLD "CPU  [" ANSI_RESET);
    printf("%s", usage_color(usage_pct));

    for (i = 0; i < BAR_WIDTH; i++)
        putchar(i < filled ? '#' : '.');

    printf(ANSI_RESET ANSI_BOLD "] %5.1f%%\n" ANSI_RESET, usage_pct);
}

void print_mem_info(const mem_info_t *info)
{
    double used_pct;
    uint64_t used_kb;

    if (!info)
        return;

    used_kb  = info->total - info->available;
    used_pct = mem_used_percent(info);

    printf(ANSI_BOLD "MEM  [" ANSI_RESET);
    printf("%s", usage_color(used_pct));

    {
        int filled = (int)(BAR_WIDTH * used_pct / 100.0);
        int i;
        for (i = 0; i < BAR_WIDTH; i++)
            putchar(i < filled ? '#' : '.');
    }

    printf(ANSI_RESET ANSI_BOLD "] %5.1f%%  "
           ANSI_RESET ANSI_CYAN "%lu MiB used / %lu MiB total\n" ANSI_RESET,
           used_pct,
           (unsigned long)(used_kb   / 1024),
           (unsigned long)(info->total / 1024));

    if (info->swap_total > 0) {
        double swap_pct = 100.0 * (double)(info->swap_total - info->swap_free)
                                / (double)info->swap_total;
        printf(ANSI_BOLD "SWAP [" ANSI_RESET);
        printf("%s", usage_color(swap_pct));
        {
            int filled = (int)(BAR_WIDTH * swap_pct / 100.0);
            int i;
            for (i = 0; i < BAR_WIDTH; i++)
                putchar(i < filled ? '#' : '.');
        }
        printf(ANSI_RESET ANSI_BOLD "] %5.1f%%  "
               ANSI_RESET ANSI_CYAN "%lu MiB used / %lu MiB total\n" ANSI_RESET,
               swap_pct,
               (unsigned long)((info->swap_total - info->swap_free) / 1024),
               (unsigned long)(info->swap_total / 1024));
    }
}

/* Comparison function for qsort: sort proc_entry_t by rss_kb descending */
static int cmp_rss_desc(const void *a, const void *b)
{
    const proc_entry_t *pa = (const proc_entry_t *)a;
    const proc_entry_t *pb = (const proc_entry_t *)b;

    if (pb->rss_kb > pa->rss_kb) return  1;
    if (pb->rss_kb < pa->rss_kb) return -1;
    return 0;
}

void print_proc_table(const proc_entry_t *entries, int count, int top_n)
{
    /* We need a mutable copy to sort */
    proc_entry_t *sorted;
    int i, limit;

    if (!entries || count <= 0)
        return;

    sorted = malloc((size_t)count * sizeof(proc_entry_t));
    if (!sorted) {
        fprintf(stderr, "sysmon: out of memory in print_proc_table\n");
        return;
    }

    memcpy(sorted, entries, (size_t)count * sizeof(proc_entry_t));
    qsort(sorted, (size_t)count, sizeof(proc_entry_t), cmp_rss_desc);

    limit = (top_n > 0 && top_n < count) ? top_n : count;

    printf(ANSI_BOLD
           "\n  %-7s %-20s %4s  %9s  %9s  %4s  %4s\n"
           ANSI_RESET,
           "PID", "NAME", "ST", "VIRT(MiB)", "RSS(MiB)", "PRI", "NI");

    printf("  %-7s %-20s %4s  %9s  %9s  %4s  %4s\n",
           "-------", "--------------------", "----",
           "---------", "---------", "----", "----");

    for (i = 0; i < limit; i++) {
        const proc_entry_t *p = &sorted[i];
        printf("  %-7d %-20s %4c  %9.1f  %9.1f  %4ld  %4ld\n",
               p->pid,
               p->name,
               p->state,
               (double)p->vsize_kb / 1024.0,
               (double)p->rss_kb   / 1024.0,
               p->priority,
               p->nice);
    }

    free(sorted);
}
