/*
 * main.c — sysmon entry point
 *
 * Usage:
 *   sysmon [interval_seconds] [top_n_procs]
 *
 * Defaults:
 *   interval  = 2 seconds
 *   top_n     = 10 processes
 *
 * The program runs in a loop, clearing the terminal each cycle and
 * redrawing the CPU bar, memory summary, and process table.
 * Press Ctrl-C to quit.
 *
 * CPU usage is computed by diffing two /proc/stat snapshots separated by
 * `interval` seconds.  The first iteration always shows 0% CPU because
 * there is no previous snapshot yet.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include "sysmon.h"

/* Set to 1 by the SIGINT/SIGTERM handler to request a clean exit */
static volatile sig_atomic_t g_quit = 0;

static void handle_signal(int sig)
{
    (void)sig;
    g_quit = 1;
}

/* Install a handler for both Ctrl-C and kill */
static void setup_signals(void)
{
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/* Print a timestamp header: "=== sysmon  2024-01-15 12:34:56 ===" */
static void print_header(void)
{
    time_t    now = time(NULL);
    struct tm tm_info;
    char      buf[32];

    localtime_r(&now, &tm_info);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);
    printf("\033[1m=== sysmon  %s ===\033[0m\n\n", buf);
}

int main(int argc, char *argv[])
{
    int interval_s = 2;
    int top_n      = 10;

    cpu_stat_t   cpu_prev = {0};
    cpu_stat_t   cpu_curr = {0};
    mem_info_t   mem      = {0};
    proc_entry_t procs[SYSMON_MAX_PROCS];
    int          proc_count = 0;
    double       cpu_pct;

    /* --- Parse optional arguments --- */
    if (argc >= 2) {
        interval_s = atoi(argv[1]);
        if (interval_s < 1) interval_s = 1;
    }
    if (argc >= 3) {
        top_n = atoi(argv[2]);
        if (top_n < 1) top_n = 1;
    }

    setup_signals();

    /* Prime the first CPU snapshot so the second iteration has a baseline */
    if (cpu_stat_read(&cpu_prev) != 0) {
        perror("sysmon: cannot read /proc/stat");
        return EXIT_FAILURE;
    }

    printf("sysmon started — interval %ds, showing top %d processes. "
           "Press Ctrl-C to quit.\n\n", interval_s, top_n);

    while (!g_quit) {
        sleep((unsigned int)interval_s);
        if (g_quit) break;

        /* --- Gather data --- */
        if (cpu_stat_read(&cpu_curr) != 0) {
            perror("sysmon: cpu_stat_read");
            break;
        }

        if (mem_info_read(&mem) != 0) {
            perror("sysmon: mem_info_read");
            break;
        }

        if (proc_list_read(procs, &proc_count) != 0) {
            perror("sysmon: proc_list_read");
            break;
        }

        cpu_pct = cpu_usage_percent(&cpu_prev, &cpu_curr);
        cpu_prev = cpu_curr;   /* slide the window */

        /* --- Render --- */
        /* \033[H\033[2J: move cursor to top-left, clear screen */
        printf("\033[H\033[2J");
        print_header();
        print_cpu_bar(cpu_pct);
        print_mem_info(&mem);
        print_proc_table(procs, proc_count, top_n);
        fflush(stdout);
    }

    printf("\nsysmon: exiting.\n");
    return EXIT_SUCCESS;
}
