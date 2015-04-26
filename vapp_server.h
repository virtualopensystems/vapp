/*
 * vapp_server.h
 *
 * Copyright (c) 2014 Virtual Open Systems Sarl.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef VAPP_SERVER_H_
#define VAPP_SERVER_H_

#include "server.h"
#include "vring.h"
#include "stat.h"

struct VappServerMemoryRegion {
    uint64_t guest_phys_addr;
    uint64_t memory_size;
    uint64_t userspace_addr;
    uint64_t mmap_addr;
};

typedef struct VappServerMemoryRegion VappServerMemoryRegion;

struct VappServerMemory {
    uint32_t nregions;
    VappServerMemoryRegion regions[VHOST_MEMORY_MAX_NREGIONS];
};

typedef struct VappServerMemory VappServerMemory;

struct VappServerStruct {
    Server* server;
    VappServerMemory memory;
    VringTable vring_table;

    uint8_t buffer[BUFFER_SIZE];
    uint32_t buffer_size;
    Stat stat;
    unsigned int vring_base[VAPP_CLIENT_VRING_NUM];
};

typedef struct VappServerStruct VappServer;

VappServer* new_vapp_server(const char* name, const char* path);
int run_vapp_server(VappServer* vapp_server);

#endif /* VAPP_SERVER_H_ */
