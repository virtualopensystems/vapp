/*
 * fd_list.h
 *
 * Copyright (c) 2014 Virtual Open Systems Sarl.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef FD_H_
#define FD_H_

#include <stddef.h>
#include <stdint.h>

#define FD_LIST_SIZE    10

struct FdNodeStruct;

typedef int (*FdHandler)(struct FdNodeStruct* node);

struct FdNodeStruct {
    int fd;
    void* context;
    FdHandler handler;
};

typedef struct FdNodeStruct FdNode;

typedef struct {
    size_t nfds;
    int fdmax;
    FdNode  read_fds[FD_LIST_SIZE];
    FdNode  write_fds[FD_LIST_SIZE];
    uint32_t    ms;
} FdList;

typedef enum {
    FD_READ, FD_WRITE
} FdType;

#define FD_LIST_SELECT_POLL     (0)     // poll and exit
#define FD_LIST_SELECT_5        (200)   // 5 times per sec

int init_fd_list(FdList* fd_list, uint32_t ms);
int add_fd_list(FdList* fd_list, FdType type, int fd, void* context, FdHandler handler);
int del_fd_list(FdList* fd_list, FdType type, int fd);
int traverse_fd_list(FdList* fd_list);

#endif /* FD_H_ */
