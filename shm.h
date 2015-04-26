/*
 * shm.h
 *
 * Copyright (c) 2014 Virtual Open Systems Sarl.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef SHM_H_
#define SHM_H_

extern int shm_fds[];

void* init_shm(const char* path, size_t size, int idx);
void* init_shm_from_fd(int fd, size_t size);
int end_shm(const char* path, void* ptr, size_t size, int idx);

int sync_shm(void* ptr, size_t size);

#endif /* SHM_H_ */
