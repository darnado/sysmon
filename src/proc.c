/*
 * proc.c — Process enumeration via /proc/<pid>/stat
 *
 * For each numeric directory under /proc we read the single-line
 * /proc/<pid>/stat file.  Its format is documented in `man 5 proc`:
 *
 *   pid (comm) state ppid pgroup session tty_nr ...
 *
 * The comm field is wrapped in parentheses and may itself contain spaces
 * and parentheses, so we locate the last ')' before parsing the rest of
 * the fields.  This is the standard defensive approach used by tools like
 * procps (ps, top).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include "sysmon.h"

/* Page size in kilobytes — used to convert RSS pages → kB */
static long g_page_size_kb = 0;

static long get_page_size_kb(void)
{
    if (g_page_size_kb == 0) {
        long ps = sysconf(_SC_PAGESIZE);   /* bytes */
        g_page_size_kb = (ps > 0) ? ps / 1024 : 4;
    }
    return g_page_size_kb;
}

/*
 * parse_stat_line() - Extract fields we care about from /proc/<pid>/stat.
 *
 * Returns 0 on success, -1 if the line cannot be parsed.
 */
static int parse_stat_line(const char *line, proc_entry_t *entry)
{
    const char *p;
    const char *name_start;
    const char *name_end;
    size_t      name_len;

    unsigned long long utime, stime;   /* consumed but not stored */
    unsigned long long vsize;          /* bytes */
    long long          rss;            /* pages */
    long               priority, nice;
    int                pid;
    char               state;

    /* --- pid --- */
    if (sscanf(line, "%d", &pid) != 1)
        return -1;
    entry->pid = (pid_t)pid;

    /* --- comm: find the outermost parentheses --- */
    name_start = strchr(line, '(');
    if (!name_start)
        return -1;
    name_start++;                       /* skip '(' */

    name_end = strrchr(line, ')');      /* last ')' handles embedded parens */
    if (!name_end || name_end <= name_start)
        return -1;

    name_len = (size_t)(name_end - name_start);
    if (name_len >= SYSMON_PROC_NAME_LEN)
        name_len = SYSMON_PROC_NAME_LEN - 1;

    memcpy(entry->name, name_start, name_len);
    entry->name[name_len] = '\0';

    /* --- fields after ')': state ppid pgrp session tty_nr tpgid flags
     *     minflt cminflt majflt cmajflt utime stime cutime cstime
     *     priority nice num_threads ... vsize rss --- */
    p = name_end + 1;   /* character after ')' */

    if (sscanf(p,
               " %c"          /*  3 state        */
               " %*d"         /*  4 ppid         */
               " %*d"         /*  5 pgrp         */
               " %*d"         /*  6 session      */
               " %*d"         /*  7 tty_nr       */
               " %*d"         /*  8 tpgid        */
               " %*u"         /*  9 flags        */
               " %*u %*u %*u %*u"  /* 10-13 faults */
               " %llu %llu"   /* 14-15 utime, stime */
               " %*d %*d"     /* 16-17 cutime, cstime */
               " %ld"         /* 18 priority     */
               " %ld"         /* 19 nice         */
               " %*d"         /* 20 num_threads  */
               " %*d"         /* 21 itrealvalue  */
               " %*llu"       /* 22 starttime    */
               " %llu"        /* 23 vsize (bytes)*/
               " %lld",       /* 24 rss (pages)  */
               &state,
               &utime, &stime,
               &priority, &nice,
               &vsize,
               &rss) != 7)
    {
        return -1;
    }

    entry->state    = state;
    entry->priority = priority;
    entry->nice     = nice;
    entry->vsize_kb = vsize / 1024;
    entry->rss_kb   = (rss > 0) ? (uint64_t)rss * get_page_size_kb() : 0;

    (void)utime;   /* available for future CPU-per-process accounting */
    (void)stime;

    return 0;
}

int proc_list_read(proc_entry_t *entries, int *count)
{
    DIR           *proc_dir;
    struct dirent *dent;
    int            n = 0;
    char           path[64];
    char           line[512];
    FILE          *fp;

    if (!entries || !count) {
        errno = EINVAL;
        return -1;
    }

    *count = 0;

    proc_dir = opendir("/proc");
    if (!proc_dir)
        return -1;

    while ((dent = readdir(proc_dir)) != NULL && n < SYSMON_MAX_PROCS) {
        /* We only care about numeric directory names (PIDs) */
        if (!isdigit((unsigned char)dent->d_name[0]))
            continue;

        snprintf(path, sizeof(path), "/proc/%s/stat", dent->d_name);

        fp = fopen(path, "r");
        if (!fp)
            continue;   /* process may have exited — skip silently */

        if (fgets(line, sizeof(line), fp)) {
            if (parse_stat_line(line, &entries[n]) == 0)
                n++;
        }

        fclose(fp);
    }

    closedir(proc_dir);
    *count = n;
    return 0;
}
