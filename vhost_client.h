/*
 * vhost_client.h
 *
 * Copyright (c) 2014 Virtual Open Systems Sarl.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef VHOST_CLIENT_H_
#define VHOST_CLIENT_H_

#include "client.h"
#include "stat.h"
#include "vring.h"
#include "vhost_user.h"

typedef struct VhostClient {
    Client* client;
    VhostUserMemory memory;
    uint64_t features;           // features negotiated with the server

    struct vhost_vring* vring_table_shm[VHOST_CLIENT_VRING_NUM];

    VringTable vring_table;
    size_t  page_size;

    Stat stat;
} VhostClient;

VhostClient* new_vhost_client(const char* path);

int init_vhost_client(VhostClient* vhost_client);
int end_vhost_client(VhostClient* vhost_client);

// Test-only procedure
int run_vhost_client(VhostClient* vhost_client);

#endif /* VHOST_CLIENT_H_ */
