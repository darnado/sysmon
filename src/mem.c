/*
 * mem.c — Memory statistics via /proc/meminfo
 *
 * /proc/meminfo lists memory counters as "Key: value kB" lines.
 * We use a simple key-match loop rather than positional parsing so that
 * the code is robust to kernel versions that add or reorder fields.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "sysmon.h"

#define PROC_MEMINFO_PATH "/proc/meminfo"
#define LINE_BUF_SIZE     128

/*
 * Helper: scan one "Key: value kB" line and store value if the key matches.
 * Returns 1 if the key matched, 0 otherwise.
 */
static int match_meminfo_field(const char *line,
                               const char *key,
                               uint64_t   *dest)
{
    char     found_key[64];
    uint64_t value;

    if (sscanf(line, "%63s %llu", found_key, (unsigned long long *)&value) != 2)
        return 0;

    /* Keys in /proc/meminfo include the trailing colon, e.g. "MemTotal:" */
    if (strcmp(found_key, key) != 0)
        return 0;

    *dest = value;
    return 1;
}

int mem_info_read(mem_info_t *info)
{
    FILE *fp;
    char  line[LINE_BUF_SIZE];
    int   found = 0;     /* bit-mask: track which fields we have read */

    /* Bit positions for each required field */
    enum {
        F_TOTAL     = 1 << 0,
        F_FREE      = 1 << 1,
        F_AVAIL     = 1 << 2,
        F_BUFFERS   = 1 << 3,
        F_CACHED    = 1 << 4,
        F_SWAPTOTAL = 1 << 5,
        F_SWAPFREE  = 1 << 6,
        F_ALL       = (1 << 7) - 1
    };

    if (!info) {
        errno = EINVAL;
        return -1;
    }

    memset(info, 0, sizeof(*info));

    fp = fopen(PROC_MEMINFO_PATH, "r");
    if (!fp)
        return -1;

    while (fgets(line, sizeof(line), fp) && found != F_ALL) {
        if (match_meminfo_field(line, "MemTotal:",     &info->total))     found |= F_TOTAL;
        if (match_meminfo_field(line, "MemFree:",      &info->free))      found |= F_FREE;
        if (match_meminfo_field(line, "MemAvailable:", &info->available)) found |= F_AVAIL;
        if (match_meminfo_field(line, "Buffers:",      &info->buffers))   found |= F_BUFFERS;
        if (match_meminfo_field(line, "Cached:",       &info->cached))    found |= F_CACHED;
        if (match_meminfo_field(line, "SwapTotal:",    &info->swap_total)) found |= F_SWAPTOTAL;
        if (match_meminfo_field(line, "SwapFree:",     &info->swap_free))  found |= F_SWAPFREE;
    }

    fclose(fp);

    if (found != F_ALL) {
        errno = EIO;
        return -1;
    }

    return 0;
}

double mem_used_percent(const mem_info_t *info)
{
    if (!info || info->total == 0)
        return 0.0;

    /* "used" = total minus what is genuinely available to applications */
    uint64_t used = info->total - info->available;
    return 100.0 * (double)used / (double)info->total;
}
