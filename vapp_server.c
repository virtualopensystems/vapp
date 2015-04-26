/*
 * vapp_server.c
 *
 * Copyright (c) 2014 Virtual Open Systems Sarl.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fd_list.h"
#include "shm.h"
#include "vapp.h"
#include "vapp_server.h"
#include "vring.h"

typedef int (*MsgHandler)(VappServer* vapp_server, ServerMsg* msg);

static int avail_handler_server(void* context, void* buf, size_t size);
static uint64_t map_handler(void* context, uint64_t addr);

VappServer* new_vapp_server(const char* name, const char* path)
{
    VappServer* vapp_server = (VappServer*) calloc(1, sizeof(VappServer));
    int idx;

    //TODO: handle errors here

    vapp_server->server = new_server(name, path);
    vapp_server->memory.nregions = 0;

    // VringTable initalization
    vapp_server->vring_table.free_head = 0;
    vapp_server->vring_table.used_head = 0;

    vapp_server->vring_table.last_avail_idx = 0;
    vapp_server->vring_table.last_used_idx = 0;
    vapp_server->vring_table.handler.context = (void*) vapp_server;
    vapp_server->vring_table.handler.avail_handler = avail_handler_server;
    vapp_server->vring_table.handler.map_handler = map_handler;

    for (idx = 0; idx < VAPP_CLIENT_VRING_NUM; idx++) {
        vapp_server->vring_table.vring[idx].kickfd = -1;
        vapp_server->vring_table.vring[idx].callfd = -1;
        vapp_server->vring_table.vring[idx].desc = 0;
        vapp_server->vring_table.vring[idx].avail = 0;
        vapp_server->vring_table.vring[idx].used = 0;
    }

    init_stat(&vapp_server->stat);

    return vapp_server;
}

static int end_vapp_server(VappServer* vapp_server)
{
    int idx;

    for (idx = 0; idx < vapp_server->memory.nregions; idx++) {
        VappServerMemoryRegion *region = &vapp_server->memory.regions[idx];
        end_shm(vapp_server->server->path, (void*) region->userspace_addr,
                region->memory_size, idx);
    }

    return 0;
}

static int64_t _map_guest_addr(VappServer* vapp_server, uint64_t addr)
{
    uint64_t result = 0;
    int idx;

    for (idx = 0; idx < vapp_server->memory.nregions; idx++) {
        VappServerMemoryRegion *region = &vapp_server->memory.regions[idx];

        if (region->guest_phys_addr <= addr
                && addr < (region->guest_phys_addr + region->memory_size)) {
            result = region->mmap_addr + addr - region->guest_phys_addr;
            break;
        }
    }

    return result;
}

static int64_t _map_user_addr(VappServer* vapp_server, uint64_t addr)
{
    uint64_t result = 0;
    int idx;

    for (idx = 0; idx < vapp_server->memory.nregions; idx++) {
        VappServerMemoryRegion *region = &vapp_server->memory.regions[idx];

        if (region->userspace_addr <= addr
                && addr < (region->userspace_addr + region->memory_size)) {
            result = region->mmap_addr + addr - region->userspace_addr;
            break;
        }
    }

    return result;
}

static int _get_features(VappServer* vapp_server, ServerMsg* msg)
{
    fprintf(stdout, "%s\n", __FUNCTION__);

    msg->msg.u64 = 0; // no features

    return 1; // should reply back
}

static int _set_features(VappServer* vapp_server, ServerMsg* msg)
{
    fprintf(stdout, "%s\n", __FUNCTION__);
    return 0;
}

static int _set_owner(VappServer* vapp_server, ServerMsg* msg)
{
    fprintf(stdout, "%s\n", __FUNCTION__);
    return 0;
}

static int _reset_owner(VappServer* vapp_server, ServerMsg* msg)
{
    fprintf(stdout, "%s\n", __FUNCTION__);
    return 0;
}

static int _set_mem_table(VappServer* vapp_server, ServerMsg* msg)
{
    int idx;
    fprintf(stdout, "%s\n", __FUNCTION__);

    vapp_server->memory.nregions = 0;

    for (idx = 0; idx < msg->msg.memory.nregions; idx++) {
        if (msg->fds[idx] > 0) {
            VappServerMemoryRegion *region = &vapp_server->memory.regions[idx];

            region->guest_phys_addr = msg->msg.memory.regions[idx].guest_phys_addr;
            region->memory_size = msg->msg.memory.regions[idx].memory_size;
            region->userspace_addr = msg->msg.memory.regions[idx].userspace_addr;

            assert(idx < msg->fd_num);
            assert(msg->fds[idx] > 0);

            region->mmap_addr =
                    (uint64_t) init_shm_from_fd(msg->fds[idx], region->memory_size);

            vapp_server->memory.nregions++;
        }
    }

    fprintf(stdout, "Got memory.nregions %d\n", vapp_server->memory.nregions);

    return 0;
}

static int _set_log_base(VappServer* vapp_server, ServerMsg* msg)
{
    fprintf(stdout, "%s\n", __FUNCTION__);
    return 0;
}

static int _set_log_fd(VappServer* vapp_server, ServerMsg* msg)
{
    fprintf(stdout, "%s\n", __FUNCTION__);
    return 0;
}

static int _set_vring_num(VappServer* vapp_server, ServerMsg* msg)
{
    fprintf(stdout, "%s\n", __FUNCTION__);
    return 0;
}

static int _set_vring_addr(VappServer* vapp_server, ServerMsg* msg)
{
    fprintf(stdout, "%s\n", __FUNCTION__);

    int idx = msg->msg.addr.index;

    assert(idx<VAPP_CLIENT_VRING_NUM);

    vapp_server->vring_table.vring[idx].desc =
            (struct vring_desc*) _map_user_addr(vapp_server,
                    msg->msg.addr.desc_user_addr);
    vapp_server->vring_table.vring[idx].avail =
            (struct vring_avail*) _map_user_addr(vapp_server,
                    msg->msg.addr.avail_user_addr);
    vapp_server->vring_table.vring[idx].used =
            (struct vring_used*) _map_user_addr(vapp_server,
                    msg->msg.addr.used_user_addr);
    return 0;
}

static int _set_vring_base(VappServer* vapp_server, ServerMsg* msg)
{
    fprintf(stdout, "%s\n", __FUNCTION__);

    int idx = msg->msg.state.index;

    assert(idx<VAPP_CLIENT_VRING_NUM);

    vapp_server->vring_base[idx] = msg->msg.state.num;

    return 0;
}

static int _get_vring_base(VappServer* vapp_server, ServerMsg* msg)
{
    fprintf(stdout, "%s\n", __FUNCTION__);

    int idx = msg->msg.state.index;

    assert(idx<VAPP_CLIENT_VRING_NUM);

    msg->msg.state.num = vapp_server->vring_base[idx];

    return 1; // should reply back
}

static int avail_handler_server(void* context, void* buf, size_t size)
{
    VappServer* vapp_server = (VappServer*) context;

    // copy the packet to our private buffer
    memcpy(vapp_server->buffer, buf, size);
    vapp_server->buffer_size = size;

#ifdef DUMP_PACKETS
    dump_buffer(buf, size);
#endif

    return 0;
}

static uint64_t map_handler(void* context, uint64_t addr)
{
    VappServer* vapp_server = (VappServer*) context;
    return _map_guest_addr(vapp_server, addr);
}

static int _kick_server(FdNode* node)
{
    VappServer* vapp_server = (VappServer*) node->context;
    int kickfd = node->fd;
    ssize_t r;
    uint64_t kick_it = 0;

    r = read(kickfd, &kick_it, sizeof(kick_it));

    if (r < 0) {
        perror("recv kick");
    } else if (r == 0) {
        fprintf(stdout, "Kick fd closed\n");
        del_fd_list(&vapp_server->server->fd_list, FD_READ, kickfd);
    } else {
        int idx = VAPP_CLIENT_VRING_IDX_TX;
#if 0
        fprintf(stdout, "Got kick %ld\n", kick_it);
#endif
        // if vring is already set, process the vring
        if (vapp_server->vring_table.vring[idx].desc) {
            uint32_t count = process_avail_vring(&vapp_server->vring_table, idx);
#ifdef DUMP_PACKETS
            (void)count;
#else
            update_stat(&vapp_server->stat, count);
            print_stat(&vapp_server->stat);
#endif
        }
    }

    return 0;
}

static int _set_vring_kick(VappServer* vapp_server, ServerMsg* msg)
{
    fprintf(stdout, "%s\n", __FUNCTION__);

    int idx = msg->msg.file.index;

    assert(idx<VAPP_CLIENT_VRING_NUM);
    assert(msg->fd_num == 1);

    vapp_server->vring_table.vring[idx].kickfd = msg->fds[0];

    fprintf(stdout, "Got kickfd 0x%x\n", vapp_server->vring_table.vring[idx].kickfd);

    if (idx == VAPP_CLIENT_VRING_IDX_TX) {
        add_fd_list(&vapp_server->server->fd_list, FD_READ,
                vapp_server->vring_table.vring[idx].kickfd,
                (void*) vapp_server, _kick_server);
        fprintf(stdout, "Listening for kicks on 0x%x\n", vapp_server->vring_table.vring[idx].kickfd);
    }

    return 0;
}

static int _set_vring_call(VappServer* vapp_server, ServerMsg* msg)
{
    fprintf(stdout, "%s\n", __FUNCTION__);

    int idx = msg->msg.file.index;

    assert(idx<VAPP_CLIENT_VRING_NUM);
    assert(msg->fd_num == 1);

    vapp_server->vring_table.vring[idx].callfd = msg->fds[0];

    fprintf(stdout, "Got callfd 0x%x\n", vapp_server->vring_table.vring[idx].callfd);

    return 0;
}

static int _set_vring_err(VappServer* vapp_server, ServerMsg* msg)
{
    fprintf(stdout, "%s\n", __FUNCTION__);
    return 0;
}

static MsgHandler msg_handlers[VHOST_USER_MAX] = {
        0,                  // VHOST_USER_NONE
        _get_features,      // VHOST_USER_GET_FEATURES
        _set_features,      // VHOST_USER_SET_FEATURES
        _set_owner,         // VHOST_USER_SET_OWNER
        _reset_owner,       // VHOST_USER_RESET_OWNER
        _set_mem_table,     // VHOST_USER_SET_MEM_TABLE
        _set_log_base,      // VHOST_USER_SET_LOG_BASE
        _set_log_fd,        // VHOST_USER_SET_LOG_FD
        _set_vring_num,     // VHOST_USER_SET_VRING_NUM
        _set_vring_addr,    // VHOST_USER_SET_VRING_ADDR
        _set_vring_base,    // VHOST_USER_SET_VRING_BASE
        _get_vring_base,    // VHOST_USER_GET_VRING_BASE
        _set_vring_kick,    // VHOST_USER_SET_VRING_KICK
        _set_vring_call,    // VHOST_USER_SET_VRING_CALL
        _set_vring_err,     // VHOST_USER_SET_VRING_ERR
        0                   // VHOST_USER_NET_SET_BACKEND
        };

static int in_msg_server(void* context, ServerMsg* msg)
{
    VappServer* vapp_server = (VappServer*) context;
    int result = 0;

    fprintf(stdout, "Processing message: %s\n", cmd_from_vappmsg(&msg->msg));

    assert(msg->msg.request > VHOST_USER_NONE && msg->msg.request < VHOST_USER_MAX);

    if (msg_handlers[msg->msg.request]) {
        result = msg_handlers[msg->msg.request](vapp_server, msg);
    }

    return result;
}

static int poll_server(void* context)
{
    VappServer* vapp_server = (VappServer*) context;
    int rx_idx = VAPP_CLIENT_VRING_IDX_RX;

    if (vapp_server->vring_table.vring[rx_idx].desc) {
        if (vapp_server->buffer_size) {
            // send a packet from the buffer
            put_vring(&vapp_server->vring_table, rx_idx,
                      vapp_server->buffer, vapp_server->buffer_size);

            // signal the client
            kick(&vapp_server->vring_table, rx_idx);

            // mark the buffer empty
            vapp_server->buffer_size = 0;
        }
    }

    return 0;
}

static AppHandlers vapp_server_handlers =
{
        .context = 0,
        .in_handler = in_msg_server,
        .poll_handler = poll_server
};

int run_vapp_server(VappServer* vapp_server)
{
    vapp_server_handlers.context = vapp_server;
    set_handler_server(vapp_server->server, &vapp_server_handlers);

    start_stat(&vapp_server->stat);
    run_server(vapp_server->server);
    stop_stat(&vapp_server->stat);

    end_vapp_server(vapp_server);

    return 0;
}
