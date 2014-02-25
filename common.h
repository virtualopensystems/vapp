/*
 * common.h
 *
 * Copyright (c) 2014 Virtual Open Systems Sarl.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#define INSTANCE_CREATED        1
#define INSTANCE_INITIALIZED    2
#define INSTANCE_END            3

#define ONEMEG                  (1024*1024)

#define ETH_PACKET_SIZE         (1518)
#define BUFFER_SIZE             (sizeof(struct virtio_net_hdr) + ETH_PACKET_SIZE)
#define BUFFER_ALIGNMENT        (8)         // alignment in bytes
#define VHOST_SOCK_NAME         "vhost.sock"

// align a value on a boundary
#define ALIGN(v,b)   (((long int)v + (long int)b - 1)&(-(long int)b))

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define DUMP_PACKETS

struct ServerMsg;

typedef int (*InMsgHandler)(void* context, struct ServerMsg* msg);
typedef int (*PollHandler)(void* context);

struct AppHandlers {
    void* context;
    InMsgHandler in_handler;
    PollHandler poll_handler;
};
typedef struct AppHandlers AppHandlers;

struct VhostUserMsg;

const char* cmd_from_vhostmsg(const struct VhostUserMsg* msg);
void dump_vhostmsg(const struct VhostUserMsg* msg);

struct vring_desc;
struct vring_avail;
struct vring_used;
struct vhost_vring;

void dump_buffer(uint8_t* p, size_t len);
void dump_vring(struct vring_desc* desc, struct vring_avail* avail,struct vring_used* used);
void dump_vhost_vring(struct vhost_vring* vring);

int vhost_user_send_fds(int fd, const struct VhostUserMsg *msg, int *fds, size_t fd_num);
int vhost_user_recv_fds(int fd, const struct VhostUserMsg *msg, int *fds, size_t *fd_num);

#endif /* COMMON_H_ */
