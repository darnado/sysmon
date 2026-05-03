/*
 * cpu.c — CPU usage calculation via /proc/stat
 *
 * /proc/stat exposes cumulative CPU time counters since boot.  To compute
 * usage over a period we take two snapshots and calculate:
 *
 *   usage% = 100 * (Δactive / Δtotal)
 *
 * where Δactive = Δuser + Δnice + Δsystem + Δirq + Δsoftirq
 *       Δtotal  = Δactive + Δidle + Δiowait
 *
 * iowait is intentionally excluded from the "active" bucket: a CPU that is
 * stalled waiting for disk I/O is technically idle from the scheduler's
 * point of view, but many tools include it to highlight I/O saturation.
 * We keep the two separate so callers can decide.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "sysmon.h"

#define PROC_STAT_PATH "/proc/stat"

int cpu_stat_read(cpu_stat_t *stat)
{
    FILE *fp;
    int   rc;
    char  label[8];

    if (!stat) {
        errno = EINVAL;
        return -1;
    }

    fp = fopen(PROC_STAT_PATH, "r");
    if (!fp)
        return -1;   /* errno set by fopen */

    /*
     * The first line of /proc/stat looks like:
     *   cpu  <user> <nice> <system> <idle> <iowait> <irq> <softirq> ...
     * There may be additional fields (steal, guest, …) which we ignore.
     */
    rc = fscanf(fp, "%7s %llu %llu %llu %llu %llu %llu %llu",
                label,
                (unsigned long long *)&stat->user,
                (unsigned long long *)&stat->nice,
                (unsigned long long *)&stat->system,
                (unsigned long long *)&stat->idle,
                (unsigned long long *)&stat->iowait,
                (unsigned long long *)&stat->irq,
                (unsigned long long *)&stat->softirq);

    fclose(fp);

    if (rc != 8) {
        /* Unexpected format */
        errno = EIO;
        return -1;
    }

    return 0;
}

double cpu_usage_percent(const cpu_stat_t *prev, const cpu_stat_t *curr)
{
    uint64_t prev_active, curr_active;
    uint64_t prev_total,  curr_total;
    uint64_t delta_active, delta_total;

    if (!prev || !curr)
        return -1.0;

    prev_active = prev->user + prev->nice + prev->system
                + prev->irq  + prev->softirq;
    curr_active = curr->user + curr->nice + curr->system
                + curr->irq  + curr->softirq;

    prev_total  = prev_active + prev->idle + prev->iowait;
    curr_total  = curr_active + curr->idle + curr->iowait;

    delta_total  = curr_total  - prev_total;
    delta_active = curr_active - prev_active;

    if (delta_total == 0)
        return 0.0;   /* No time elapsed — avoid division by zero */

    return 100.0 * (double)delta_active / (double)delta_total;
}
