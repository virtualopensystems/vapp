/*
 * vring.h
 *
 * Copyright (c) 2014 Virtual Open Systems Sarl.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef VRING_H_
#define VRING_H_

#include "client.h"

// Number of vring structures used in Linux vhost. Max 32768.
enum { VHOST_VRING_SIZE = 32*1024 };

// vring_desc I/O buffer descriptor
struct vring_desc {
  uint64_t addr;  // packet data buffer address
  uint32_t len;   // packet data buffer size
  uint16_t flags; // (see below)
  uint16_t next;  // optional index next descriptor in chain
};

// Available vring_desc.flags
enum {
  VIRTIO_DESC_F_NEXT = 1,    // Descriptor continues via 'next' field
  VIRTIO_DESC_F_WRITE = 2,   // Write-only descriptor (otherwise read-only)
  VIRTIO_DESC_F_INDIRECT = 4 // Buffer contains a list of descriptors
};

enum { // flags for avail and used rings
  VRING_F_NO_INTERRUPT  = 1,  // Hint: don't bother to call process
  VRING_F_NO_NOTIFY     = 1,  // Hint: don't bother to kick kernel
  VRING_F_INDIRECT_DESC = 28, // Indirect descriptors are supported
  VRING_F_EVENT_IDX     = 29  // (Some boring complicated interrupt behavior..)
};

// ring of descriptors that are available to be processed
struct vring_avail {
  uint16_t flags;
  uint16_t idx;
  uint16_t ring[VHOST_VRING_SIZE];
};

// ring of descriptors that have already been processed
struct vring_used {
  uint16_t flags;
  uint16_t idx;
  struct vring_used_elem { uint32_t id; uint32_t len; } ring[VHOST_VRING_SIZE];
};

struct vhost_vring {
  int kickfd, callfd;
  struct vring_desc desc[VHOST_VRING_SIZE] __attribute__((aligned(4)));
  struct vring_avail avail                 __attribute__((aligned(2)));
  struct vring_used used                   __attribute__((aligned(4096)));
};

typedef int (*AvailHandler)(void* context, void* buf, size_t size);
typedef uintptr_t (*MapHandler)(void* context, uint64_t addr);

typedef struct {
    void* context;
    AvailHandler avail_handler;
    MapHandler map_handler;
} ProcessHandler;

typedef struct {
  int kickfd, callfd;
  struct vring_desc* desc;
  struct vring_avail* avail;
  struct vring_used* used;
  unsigned int num;
  uint16_t last_avail_idx;
  uint16_t last_used_idx;
} Vring;

struct VhostUserMemory;

// TODO: these are NET specific
#define VHOST_CLIENT_VRING_IDX_RX   0
#define VHOST_CLIENT_VRING_IDX_TX   1
#define VHOST_CLIENT_VRING_NUM      2


int vring_table_from_memory_region(struct vhost_vring* vring_table[], size_t vring_table_num,
        struct VhostUserMemory *memory);

int set_host_vring(Client* client, struct vhost_vring *vring, int index);

int set_host_vring_table(struct vhost_vring* vring_table[], size_t vring_table_num, Client* client);

typedef struct {
    ProcessHandler handler;
    Vring vring[VHOST_CLIENT_VRING_NUM];
} VringTable;

int put_vring(VringTable* vring_table, uint32_t v_idx, void* buf, size_t size);
int process_used_vring(VringTable* vring_table, uint32_t v_idx);
int process_avail_vring(VringTable* vring_table, uint32_t v_idx);

int kick(VringTable* vring_table, uint32_t v_idx);

#endif /* VRING_H_ */
