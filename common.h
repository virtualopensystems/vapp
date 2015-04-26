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

#define NAMELEN     32

#define INSTANCE_CREATED        1
#define INSTANCE_INITIALIZED    2
#define INSTANCE_END            3

#define ONEMEG                  (1024*1024)

#define ETH_PACKET_SIZE         (1518)
#define BUFFER_SIZE             ETH_PACKET_SIZE
#define BUFFER_ALIGNMENT        (8)         // alignment in bytes
#define VAPP_SOCK_NAME          "vapp.sock" // "/var/run/vapp.sock"

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

#endif /* COMMON_H_ */
