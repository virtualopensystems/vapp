/*
 * fd_list.c
 *
 * Copyright (c) 2014 Virtual Open Systems Sarl.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include <stdio.h>
#include <sys/select.h>

#include "common.h"
#include "fd_list.h"

static int reset_fd_node(FdNode* fd_node);

int init_fd_list(FdList* fd_list, uint32_t ms)
{
    int idx;
    fd_list->nfds = 0;
    fd_list->fdmax = -1;

    for (idx = 0; idx < FD_LIST_SIZE; idx++) {
        reset_fd_node(&(fd_list->read_fds[idx]));
        reset_fd_node(&(fd_list->write_fds[idx]));
    }

    fd_list-> ms = ms;

    return 0;
}

static int reset_fd_node(FdNode* fd_node)
{
    fd_node->fd = -1;
    fd_node->context = 0;
    fd_node->handler = 0;

    return 0;
}

static FdNode* find_fd_node_by_fd(FdNode* fds, int fd)
{
    int idx;

    for (idx = 0; idx < FD_LIST_SIZE; idx++) {
        if (fds[idx].fd == fd)
            return &(fds[idx]);
    }

    return 0;
}

int add_fd_list(FdList* fd_list, FdType type, int fd, void* context,
        FdHandler handler)
{
    FdNode* fds = (type == FD_READ) ? fd_list->read_fds : fd_list->write_fds;
    FdNode* fd_node = find_fd_node_by_fd(fds, -1);

    if (!fd_node) {
        fprintf(stderr, "No space in fd list\n");
        return -1;
    }

    fd_node->fd = fd;
    fd_node->context = context;
    fd_node->handler = handler;

    return 0;
}

int del_fd_list(FdList* fd_list, FdType type, int fd)
{
    FdNode* fds = (type == FD_READ) ? fd_list->read_fds : fd_list->write_fds;
    FdNode* fd_node = find_fd_node_by_fd(fds, fd);

    if (!fd_node) {
        fprintf(stderr, "Fd (%d) not found fd list\n", fd);
        return -1;
    }

    reset_fd_node(fd_node);

    return 0;
}

static int fd_set_from_fd_list(FdList* fd_list, FdType type, fd_set* fdset)
{
    int idx;
    FdNode* fds = (type == FD_READ) ? fd_list->read_fds : fd_list->write_fds;

    FD_ZERO(fdset);

    for (idx = 0; idx < FD_LIST_SIZE; idx++) {
        int fd = fds[idx].fd;
        if (fd != -1) {
            FD_SET(fd,fdset);
            fd_list->fdmax = MAX(fd, fd_list->fdmax);
        }
    }

    return 0;
}

static int process_fd_set(FdList* fd_list, FdType type, fd_set* fdset)
{
    int idx;
    FdNode* fds = (type == FD_READ) ? fd_list->read_fds : fd_list->write_fds;

    for (idx = 0; idx < FD_LIST_SIZE; idx++) {
        FdNode* node = &(fds[idx]);
        if (FD_ISSET(node->fd,fdset)) {
            if (node->handler) {
                node->handler(node);
            }
        }
    }

    return 0;
}

int traverse_fd_list(FdList* fd_list)
{
    fd_set read_fdset, write_fdset;
    struct timeval tv = { .tv_sec = fd_list->ms/1000,
                          .tv_usec = (fd_list->ms%1000)*1000 };
    int r;

    fd_list->fdmax = -1;
    fd_set_from_fd_list(fd_list, FD_READ, &read_fdset);
    fd_set_from_fd_list(fd_list, FD_WRITE, &write_fdset);

    r = select(fd_list->fdmax + 1, &read_fdset, &write_fdset, 0, &tv);

    if (r == -1) {
        perror("select");
    } else if (r == 0) {
        // no ready fds, timeout
    } else {
        int rr = process_fd_set(fd_list, FD_READ, &read_fdset);
        int wr = process_fd_set(fd_list, FD_WRITE, &write_fdset);
        if (r != (rr + wr)) {
            // something wrong
        }
    }

    return 0;
}
