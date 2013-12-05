/*
 * shm.h
 *
 * Copyright (c) 2014 Virtual Open Systems Sarl.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>

#include "shm.h"
#include "vhost.h"

int shm_fds[VHOST_MEMORY_MAX_NREGIONS];

void* init_shm(const char* path, size_t size, int idx)
{
    int fd = 0;
    void* result = 0;
    char path_idx[PATH_MAX];
    int oflags = 0;

    sprintf(path_idx, "%s%d", path, idx);

    oflags = O_RDWR | O_CREAT;

    fd = shm_open(path_idx, oflags, 0666);
    if (fd == -1) {
        perror("shm_open");
        goto err;
    }

    if (ftruncate(fd, size) != 0) {
        perror("ftruncate");
        goto err;
    }

    result = init_shm_from_fd(fd, size);
    if (!result) {
        goto err;
    }

    shm_fds[idx] = fd;

    return result;

err:
    close(fd);
    return 0;
}

void* init_shm_from_fd(int fd, size_t size) {
    void *result = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (result == MAP_FAILED) {
        perror("mmap");
        result = 0;
    }
    return result;
}

int end_shm(const char* path, void* ptr, size_t size, int idx)
{
    char path_idx[PATH_MAX];

    if (shm_fds[idx] > 0) {
        close(shm_fds[idx]);
    }

    sprintf(path_idx, "%s%d", path, idx);

    if (munmap(ptr, size) != 0) {
        perror("munmap");
        return -1;
    }

    if (shm_unlink(path_idx) != 0) {
        perror("shm_unlink");
        return -1;
    }

    return 0;
}

int sync_shm(void* ptr, size_t size)
{
    return msync(ptr, size, MS_SYNC | MS_INVALIDATE);
}
