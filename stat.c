/*
 * stat.c
 *
 * Copyright (c) 2014 Virtual Open Systems Sarl.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include <inttypes.h>
#include <stdio.h>

#include "stat.h"

#define STAT_PRINT_INTERVAL (3) // in ms
int init_stat(Stat* stat)
{
    clock_gettime(CLOCK_MONOTONIC, &stat->start);
    clock_gettime(CLOCK_MONOTONIC, &stat->stop);
    stat->count = 0;
    stat->diff = 0;
    return 0;
}

int start_stat(Stat* stat)
{
    clock_gettime(CLOCK_MONOTONIC, &stat->start);
    return 0;
}

int stop_stat(Stat* stat)
{
    clock_gettime(CLOCK_MONOTONIC, &stat->stop);
    return 0;
}

int update_stat(Stat* stat, uint32_t count)
{
    stat->count += count;
    return 0;
}

int print_stat(Stat* stat)
{
    struct timespec now;
    uint64_t diff;

    clock_gettime(CLOCK_MONOTONIC, &now);

    diff = (now.tv_sec - stat->start.tv_sec)
            + (now.tv_nsec - stat->start.tv_nsec) / 1000000000;

    if (diff > stat->diff) {
        if (diff % STAT_PRINT_INTERVAL == 0) {
            fprintf(stdout,"%10"PRId64"\r", stat->count / diff);fflush(stdout);
        }
        stat->diff = diff;
    }

    return 0;
}
