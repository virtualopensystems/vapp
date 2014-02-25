/*
 * common.c
 *
 * Copyright (c) 2014 Virtual Open Systems Sarl.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "common.h"
#include "vring.h"
#include "vhost_user.h"

const char* cmd_from_vhostmsg(const VhostUserMsg* msg)
{
    switch (msg->request) {
    case VHOST_USER_NONE:
        return "VHOST_USER_NONE";
    case VHOST_USER_GET_FEATURES:
        return "VHOST_USER_GET_FEATURES";
    case VHOST_USER_SET_FEATURES:
        return "VHOST_USER_SET_FEATURES";
    case VHOST_USER_SET_OWNER:
        return "VHOST_USER_SET_OWNER";
    case VHOST_USER_RESET_OWNER:
        return "VHOST_USER_RESET_OWNER";
    case VHOST_USER_SET_MEM_TABLE:
        return "VHOST_USER_SET_MEM_TABLE";
    case VHOST_USER_SET_LOG_BASE:
        return "VHOST_USER_SET_LOG_BASE";
    case VHOST_USER_SET_LOG_FD:
        return "VHOST_USER_SET_LOG_FD";
    case VHOST_USER_SET_VRING_NUM:
        return "VHOST_USER_SET_VRING_NUM";
    case VHOST_USER_SET_VRING_ADDR:
        return "VHOST_USER_SET_VRING_ADDR";
    case VHOST_USER_SET_VRING_BASE:
        return "VHOST_USER_SET_VRING_BASE";
    case VHOST_USER_GET_VRING_BASE:
        return "VHOST_USER_GET_VRING_BASE";
    case VHOST_USER_SET_VRING_KICK:
        return "VHOST_USER_SET_VRING_KICK";
    case VHOST_USER_SET_VRING_CALL:
        return "VHOST_USER_SET_VRING_CALL";
    case VHOST_USER_SET_VRING_ERR:
        return "VHOST_USER_SET_VRING_ERR";
    case VHOST_USER_MAX:
        return "VHOST_USER_MAX";
    }

    return "UNDEFINED";
}

void dump_vhostmsg(const VhostUserMsg* msg)
{
    int i = 0;
    fprintf(stdout, "Cmd: %s (0x%x)\n", cmd_from_vhostmsg(msg), msg->request);
    fprintf(stdout, "Flags: 0x%x\n", msg->flags);

    // command specific `dumps`
    switch (msg->request) {
    case VHOST_USER_GET_FEATURES:
        fprintf(stdout, "u64: 0x%"PRIx64"\n", msg->u64);
        break;
    case VHOST_USER_SET_FEATURES:
        fprintf(stdout, "u64: 0x%"PRIx64"\n", msg->u64);
        break;
    case VHOST_USER_SET_OWNER:
        break;
    case VHOST_USER_RESET_OWNER:
        break;
    case VHOST_USER_SET_MEM_TABLE:
        fprintf(stdout, "nregions: %d\n", msg->memory.nregions);
        for (i = 0; i < msg->memory.nregions; i++) {
            fprintf(stdout,
                    "region: \n\tgpa = 0x%"PRIX64"\n\tsize = %"PRId64"\n\tua = 0x%"PRIx64"\n",
                    msg->memory.regions[i].guest_phys_addr,
                    msg->memory.regions[i].memory_size,
                    msg->memory.regions[i].userspace_addr);
        }
        break;
    case VHOST_USER_SET_LOG_BASE:
        fprintf(stdout, "u64: 0x%"PRIx64"\n", msg->u64);
        break;
    case VHOST_USER_SET_LOG_FD:
        break;
    case VHOST_USER_SET_VRING_NUM:
        fprintf(stdout, "state: %d %d\n", msg->state.index, msg->state.num);
        break;
    case VHOST_USER_SET_VRING_ADDR:
        fprintf(stdout, "addr:\n\tidx = %d\n\tflags = 0x%x\n"
                "\tdua = 0x%"PRIx64"\n"
                "\tuua = 0x%"PRIx64"\n"
                "\taua = 0x%"PRIx64"\n"
                "\tlga = 0x%"PRIx64"\n", msg->addr.index, msg->addr.flags,
                msg->addr.desc_user_addr, msg->addr.used_user_addr,
                msg->addr.avail_user_addr, msg->addr.log_guest_addr);
        break;
    case VHOST_USER_SET_VRING_BASE:
        fprintf(stdout, "state: %d %d\n", msg->state.index, msg->state.num);
        break;
    case VHOST_USER_GET_VRING_BASE:
        fprintf(stdout, "state: %d %d\n", msg->state.index, msg->state.num);
        break;
    case VHOST_USER_SET_VRING_KICK:
    case VHOST_USER_SET_VRING_CALL:
    case VHOST_USER_SET_VRING_ERR:
        fprintf(stdout, "u64: 0x%"PRIx64"\n", msg->u64);
        break;
    case VHOST_USER_NONE:
    case VHOST_USER_MAX:
        break;
    }

    fprintf(stdout,
            "................................................................................\n");
}

void dump_buffer(uint8_t* p, size_t len)
{
    int i;
    fprintf(stdout,
            "................................................................................");
    for(i=0;i<len;i++) {
        if(i%16 == 0)fprintf(stdout,"\n");
        fprintf(stdout,"%.2x ",p[i]);
    }
    fprintf(stdout,"\n");
}

void dump_vring(struct vring_desc* desc, struct vring_avail* avail,struct vring_used* used)
{
    int idx;

    fprintf(stdout,"desc:\n");
    for(idx=0;idx<VHOST_VRING_SIZE;idx++){
        fprintf(stdout, "%d: 0x%"PRIx64" %d 0x%x %d\n",
                idx,
                desc[idx].addr, desc[idx].len,
                desc[idx].flags, desc[idx].next);
    }

    fprintf(stdout,"avail:\n");
    for(idx=0;idx<VHOST_VRING_SIZE;idx++){
       int desc_idx = avail->ring[idx];
       fprintf(stdout, "%d: %d\n",idx, desc_idx);

       //dump_buffer((uint8_t*)desc[desc_idx].addr, desc[desc_idx].len);
    }
}

void dump_vhost_vring(struct vhost_vring* vring)
{
    fprintf(stdout, "kickfd: 0x%x, callfd: 0x%x\n", vring->kickfd, vring->callfd);
    dump_vring(vring->desc, &vring->avail, &vring->used);
}

int vhost_user_send_fds(int fd, const VhostUserMsg *msg, int *fds,
        size_t fd_num)
{
    int ret;

    struct msghdr msgh;
    struct iovec iov[1];

    size_t fd_size = fd_num * sizeof(int);
    char control[CMSG_SPACE(fd_size)];
    struct cmsghdr *cmsg;

    memset(&msgh, 0, sizeof(msgh));
    memset(control, 0, sizeof(control));

    /* set the payload */
    iov[0].iov_base = (void *) msg;
    iov[0].iov_len = VHOST_USER_HDR_SIZE + msg->size;

    msgh.msg_iov = iov;
    msgh.msg_iovlen = 1;

    if (fd_num) {
        msgh.msg_control = control;
        msgh.msg_controllen = sizeof(control);

        cmsg = CMSG_FIRSTHDR(&msgh);

        cmsg->cmsg_len = CMSG_LEN(fd_size);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        memcpy(CMSG_DATA(cmsg), fds, fd_size);
    } else {
        msgh.msg_control = 0;
        msgh.msg_controllen = 0;
    }

    do {
        ret = sendmsg(fd, &msgh, 0);
    } while (ret < 0 && errno == EINTR);

    if (ret < 0) {
        fprintf(stderr, "Failed to send msg, reason: %s\n", strerror(errno));
    }

    return ret;
}

int vhost_user_recv_fds(int fd, const VhostUserMsg *msg, int *fds,
        size_t *fd_num)
{
    int ret;

    struct msghdr msgh;
    struct iovec iov[1];

    size_t fd_size = (*fd_num) * sizeof(int);
    char control[CMSG_SPACE(fd_size)];
    struct cmsghdr *cmsg;

    memset(&msgh, 0, sizeof(msgh));
    memset(control, 0, sizeof(control));
    *fd_num = 0;

    /* set the payload */
    iov[0].iov_base = (void *) msg;
    iov[0].iov_len = VHOST_USER_HDR_SIZE;

    msgh.msg_iov = iov;
    msgh.msg_iovlen = 1;
    msgh.msg_control = control;
    msgh.msg_controllen = sizeof(control);

    ret = recvmsg(fd, &msgh, 0);
    if (ret > 0) {
        if (msgh.msg_flags & (MSG_TRUNC | MSG_CTRUNC)) {
            ret = -1;
        } else {
            cmsg = CMSG_FIRSTHDR(&msgh);
            if (cmsg && cmsg->cmsg_len > 0&&
                cmsg->cmsg_level == SOL_SOCKET &&
                cmsg->cmsg_type == SCM_RIGHTS) {
                if (fd_size >= cmsg->cmsg_len - CMSG_LEN(0)) {
                    fd_size = cmsg->cmsg_len - CMSG_LEN(0);
                    memcpy(fds, CMSG_DATA(cmsg), fd_size);
                    *fd_num = fd_size / sizeof(int);
                }
            }
        }
    }

    if (ret < 0) {
        fprintf(stderr, "Failed recvmsg, reason: %s\n", strerror(errno));
    } else {
        read(fd, ((char*)msg) + ret, msg->size);
    }

    return ret;
}
