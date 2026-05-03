# sysmon

A lightweight Linux system monitor written in C.

`sysmon` reads directly from the Linux `/proc` virtual filesystem to display
real-time CPU usage, memory consumption, and a sorted process table. No
external libraries required.

```
=== sysmon  2024-05-10 14:32:07 ===

CPU  [################........................]  40.2%
MEM  [############################............]  70.5%  5734 MiB used / 8192 MiB total

  PID     NAME                 ST    VIRT(MiB)   RSS(MiB)   PRI    NI
  ------- -------------------- ----  ---------   ---------  ----  ----
  1234    firefox              S       2048.0       512.3     20     0
  5678    code                 S        900.1       320.7     20     0
  ...
```

## Quick start

Create executable

```bash
make && ./sysmon
```
Run executable

```bash
./sysmon
```
## Why this project exists

Built as a learning exercise to understand how the Linux kernel exposes
process and resource information through `/proc`. The code deliberately
avoids external libraries so every line is easy to trace back to a specific
`man 5 proc` entry.

## Building

### Native (Linux only — requires `/proc`)

```bash
make          # builds ./sysmon
make test     # compiles and runs unit tests
make install  # installs to /usr/local/bin
```

Requires: `gcc` (or `clang` via `CC=clang make`), `make`.

## Usage

```
sysmon [interval_seconds] [top_n_procs]
```

| Argument           | Default | Description                 |
| ------------------ | ------- | --------------------------- |
| `interval_seconds` | `2`     | Seconds between refreshes   |
| `top_n_procs`      | `10`    | Number of processes to show |

Press **Ctrl-C** to quit.

## Project structure

```
sysmon/
├── include/
│   └── sysmon.h        Public API — all types and function declarations
├── src/
│   ├── cpu.c           /proc/stat parsing and CPU usage calculation
│   ├── mem.c           /proc/meminfo parsing
│   ├── proc.c          /proc/<pid>/stat enumeration
│   ├── display.c       ANSI terminal output helpers
│   └── main.c          Entry point, signal handling, main loop
├── tests/
│   ├── test_framework.h  Minimal test harness (ASSERT macros, pass/fail counters)
│   ├── test_main.c       Test runner entry point — calls all suites
│   ├── test_cpu.c        Unit tests for CPU usage calculation
│   └── test_mem.c        Unit tests for memory helpers
├── Makefile
└── README.md
```

## How it works

### CPU usage

The kernel maintains cumulative CPU time counters in `/proc/stat`. `sysmon`
takes two snapshots separated by `interval` seconds and computes:

```
usage% = 100 × Δactive / Δtotal
```

where `Δactive = Δuser + Δnice + Δsystem + Δirq + Δsoftirq`
and `Δtotal  = Δactive + Δidle + Δiowait`.

### Memory

`/proc/meminfo` exposes named counters. The "used" value is
`MemTotal − MemAvailable` rather than `MemTotal − MemFree`,
because `MemAvailable` accounts for reclaimable page cache.

### Process list

For each numeric entry in `/proc`, `sysmon` reads `/proc/<pid>/stat`.
The process name (`comm`) is wrapped in parentheses and may itself contain
parentheses, so parsing locates the _last_ `)` before reading subsequent
fields — the same approach used by `ps` and `top`.

## Developer Information

This project was developed by **David Arnado**. You can also check out my personal website [caledrosforge.com](https://caledrosforge.com/).
