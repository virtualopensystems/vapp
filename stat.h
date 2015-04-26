/*
 * stat.h
 *
 * Copyright (c) 2014 Virtual Open Systems Sarl.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef STAT_H_
#define STAT_H_

#include <stdint.h>
#include <time.h>

typedef struct {
    struct timespec start, stop;
    uint64_t diff;
    uint64_t count;
} Stat;

int init_stat(Stat* stat);
int start_stat(Stat* stat);
int update_stat(Stat* stat, uint32_t count);
int stop_stat(Stat* stat);
int print_stat(Stat* stat);

#endif /* STAT_H_ */
