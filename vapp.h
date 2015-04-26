/*
 * vapp.h
 *
 * Copyright (c) 2014 Virtual Open Systems Sarl.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef VAPP_H_
#define VAPP_H_

#include <stddef.h>
#include <stdint.h>

typedef struct VhostUserMemoryRegion {
    uint64_t guest_phys_addr;
    uint64_t memory_size;
    uint64_t userspace_addr;
} VhostUserMemoryRegion;

enum {
    VHOST_MEMORY_MAX_NREGIONS = 8
};

typedef struct VhostUserMemory {
    uint32_t nregions;
    VhostUserMemoryRegion regions[VHOST_MEMORY_MAX_NREGIONS];
} VhostUserMemory;

// Structures imported from the Linux headers.
struct vhost_vring_state { unsigned int index, num; };
struct vhost_vring_file { unsigned int index; int fd; };
struct vhost_vring_addr {
  unsigned int index, flags;
  uint64_t desc_user_addr, used_user_addr, avail_user_addr, log_guest_addr;
};

/*--------------------------------------------------------*/

#define VHOST_USER_MSG_MEM_PADDING   \
        (offsetof(struct vhost_memory, regions) + \
        VHOST_MEMORY_MAX_NREGIONS*sizeof(struct vhost_memory_region))

typedef enum VhostUserRequest {
    VHOST_USER_NONE = 0,
    VHOST_USER_GET_FEATURES = 1,
    VHOST_USER_SET_FEATURES = 2,
    VHOST_USER_SET_OWNER = 3,
    VHOST_USER_RESET_OWNER = 4,
    VHOST_USER_SET_MEM_TABLE = 5,
    VHOST_USER_SET_LOG_BASE = 6,
    VHOST_USER_SET_LOG_FD = 7,
    VHOST_USER_SET_VRING_NUM = 8,
    VHOST_USER_SET_VRING_ADDR = 9,
    VHOST_USER_SET_VRING_BASE = 10,
    VHOST_USER_GET_VRING_BASE = 11,
    VHOST_USER_SET_VRING_KICK = 12,
    VHOST_USER_SET_VRING_CALL = 13,
    VHOST_USER_SET_VRING_ERR = 14,
    VHOST_USER_NET_SET_BACKEND = 15,
    VHOST_USER_MAX
} VhostUserRequest;

typedef struct VhostUserMsg {
    VhostUserRequest request;

    int flags;
    union {
        uint64_t    u64;
        int         fd;
        struct vhost_vring_state state;
        struct vhost_vring_addr addr;
        struct vhost_vring_file file;

        VhostUserMemory memory;
    };
} VhostUserMsg;

const char* cmd_from_vappmsg(const VhostUserMsg* msg);
void dump_vappmsg(const VhostUserMsg* msg);

struct vring_desc;
struct vring_avail;
struct vring_used;
struct vhost_vring;

void dump_buffer(uint8_t* p, size_t len);
void dump_vring(struct vring_desc* desc, struct vring_avail* avail,struct vring_used* used);
void dump_vhost_vring(struct vhost_vring* vring);

int vhost_user_send_fds(int fd, const VhostUserMsg *msg, int *fds, size_t fd_num);
int vhost_user_recv_fds(int fd, const VhostUserMsg *msg, int *fds, size_t *fd_num);

#endif /* VAPP_H_ */
