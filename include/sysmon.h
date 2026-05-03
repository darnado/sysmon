#ifndef SYSMON_H
#define SYSMON_H

#include <stdint.h>
#include <sys/types.h>  /* pid_t */
#include <unistd.h>     /* sysconf */

/* --------------------------------------------------------------------------
 * sysmon.h — public API for the sysmon library
 *
 * All data is read from the Linux /proc virtual filesystem, which exposes
 * kernel statistics to userspace without requiring elevated privileges.
 * -------------------------------------------------------------------------- */

/* Maximum number of processes we will track in one snapshot */
#define SYSMON_MAX_PROCS 512

/* Length limit for process name strings */
#define SYSMON_PROC_NAME_LEN 64


/* -- CPU ------------------------------------------------------------------ */

/**
 * struct cpu_stat - Raw CPU time counters from /proc/stat.
 *
 * All values are in units of USER_HZ (typically 1/100 s).
 * See `man 5 proc` for the full description of each field.
 */
typedef struct {
    uint64_t user;      /* Normal user-mode time            */
    uint64_t nice;      /* Low-priority user-mode time      */
    uint64_t system;    /* Kernel-mode time                 */
    uint64_t idle;      /* Idle time                        */
    uint64_t iowait;    /* Waiting for I/O                  */
    uint64_t irq;       /* Servicing hardware interrupts    */
    uint64_t softirq;   /* Servicing software interrupts    */
} cpu_stat_t;

/**
 * cpu_stat_read() - Populate *stat with the current aggregate CPU counters.
 *
 * Returns 0 on success, -1 on error (errno is set).
 */
int cpu_stat_read(cpu_stat_t *stat);

/**
 * cpu_usage_percent() - Compute CPU usage between two snapshots.
 *
 * @prev: snapshot taken at time T0
 * @curr: snapshot taken at time T1  (T1 > T0)
 *
 * Returns a value in [0.0, 100.0], or -1.0 on invalid input.
 */
double cpu_usage_percent(const cpu_stat_t *prev, const cpu_stat_t *curr);


/* -- Memory --------------------------------------------------------------- */

/**
 * struct mem_info - System memory summary from /proc/meminfo (kilobytes).
 */
typedef struct {
    uint64_t total;     /* Total usable RAM                 */
    uint64_t free;      /* Free (unallocated) RAM           */
    uint64_t available; /* Estimate of reclaimable memory   */
    uint64_t buffers;   /* Kernel buffer cache              */
    uint64_t cached;    /* Page cache                       */
    uint64_t swap_total;
    uint64_t swap_free;
} mem_info_t;

/**
 * mem_info_read() - Populate *info with current memory counters.
 *
 * Returns 0 on success, -1 on error.
 */
int mem_info_read(mem_info_t *info);

/** Convenience: percentage of RAM currently in use (0–100). */
double mem_used_percent(const mem_info_t *info);


/* -- Process list --------------------------------------------------------- */

/**
 * struct proc_entry - Snapshot of a single process.
 */
typedef struct {
    pid_t    pid;
    char     name[SYSMON_PROC_NAME_LEN];
    char     state;     /* R=running S=sleeping D=disk-wait Z=zombie … */
    uint64_t vsize_kb;  /* Virtual memory size in kilobytes             */
    uint64_t rss_kb;    /* Resident set size in kilobytes               */
    long     priority;
    long     nice;
} proc_entry_t;

/**
 * proc_list_read() - Collect process entries from /proc/<pid>/stat.
 *
 * @entries: caller-supplied array of at least SYSMON_MAX_PROCS elements.
 * @count:   out-parameter; set to the number of entries written.
 *
 * Returns 0 on success, -1 on error.
 */
int proc_list_read(proc_entry_t *entries, int *count);


/* -- Formatted output ----------------------------------------------------- */

/** Print a one-line CPU usage bar to stdout. */
void print_cpu_bar(double usage_pct);

/** Print a human-readable memory summary to stdout. */
void print_mem_info(const mem_info_t *info);

/** Print a sorted process table (top N by RSS) to stdout. */
void print_proc_table(const proc_entry_t *entries, int count, int top_n);

#endif /* SYSMON_H */
