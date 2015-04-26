/*
 * vapp_client.c
 *
 * Copyright (c) 2014 Virtual Open Systems Sarl.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "packet.h"
#include "shm.h"
#include "stat.h"
#include "vapp_client.h"

#define VAPP_CLIENT_TEST_MESSAGE        (arp_request)
#define VAPP_CLIENT_TEST_MESSAGE_LEN    (sizeof(arp_request))
#define VAPP_CLIENT_PAGE_SIZE \
            ALIGN(sizeof(struct vhost_vring)+BUFFER_SIZE*VHOST_VRING_SIZE, ONEMEG)

static int _kick_client(FdNode* node);
static int avail_handler_client(void* context, void* buf, size_t size);

VappClient* new_vapp_client(const char* name, const char* path)
{
    VappClient* vapp_client = (VappClient*) calloc(1, sizeof(VappClient));
    int idx = 0;

    //TODO: handle errors here
    vapp_client->client = new_client(name, path);

    vapp_client->page_size = VAPP_CLIENT_PAGE_SIZE;

    // Create and attach shm to memory regions
    vapp_client->memory.nregions = VAPP_CLIENT_VRING_NUM;
    for (idx = 0; idx < vapp_client->memory.nregions; idx++) {
        void* shm = init_shm(vapp_client->client->sock_path,
                vapp_client->page_size, idx);
        if (!shm) {
            fprintf(stderr, "Creatig shm %d failed\n", idx);
            free(vapp_client->client);
            free(vapp_client);
            return 0;
        }
        vapp_client->memory.regions[idx].guest_phys_addr = (uint64_t) shm;
        vapp_client->memory.regions[idx].memory_size = vapp_client->page_size;
        vapp_client->memory.regions[idx].userspace_addr = (uint64_t) shm;
    }

    // init vrings on the shm (through memory regions)
    if (vring_table_from_memory_region(vapp_client->vring_table_shm, VAPP_CLIENT_VRING_NUM,
            &vapp_client->memory) != 0) {
        // TODO: handle error here
    }

    return vapp_client;
}

int init_vapp_client(VappClient* vapp_client)
{
    int idx;

    if (!vapp_client->client)
        return -1;

    if (init_client(vapp_client->client) != 0)
        return -1;

    vhost_ioctl(vapp_client->client, VHOST_USER_SET_OWNER, 0);
    vhost_ioctl(vapp_client->client, VHOST_USER_GET_FEATURES, &vapp_client->features);
    vhost_ioctl(vapp_client->client, VHOST_USER_SET_MEM_TABLE, &vapp_client->memory);

    // push the vring table info to the server
    if (set_host_vring_table(vapp_client->vring_table_shm, VAPP_CLIENT_VRING_NUM,
            vapp_client->client) != 0) {
        // TODO: handle error here
    }

    // VringTable initalization
    vapp_client->vring_table.free_head = 0;
    vapp_client->vring_table.used_head = 0;

    vapp_client->vring_table.last_avail_idx = 0;
    vapp_client->vring_table.last_used_idx = 0;
    vapp_client->vring_table.handler.context = (void*) vapp_client;
    vapp_client->vring_table.handler.avail_handler = avail_handler_client;
    vapp_client->vring_table.handler.map_handler = 0;

    for (idx = 0; idx < VAPP_CLIENT_VRING_NUM; idx++) {
        vapp_client->vring_table.vring[idx].kickfd = vapp_client->vring_table_shm[idx]->kickfd;
        vapp_client->vring_table.vring[idx].callfd = vapp_client->vring_table_shm[idx]->callfd;
        vapp_client->vring_table.vring[idx].desc = vapp_client->vring_table_shm[idx]->desc;
        vapp_client->vring_table.vring[idx].avail = &vapp_client->vring_table_shm[idx]->avail;
        vapp_client->vring_table.vring[idx].used = &vapp_client->vring_table_shm[idx]->used;
    }

    // Add handler for RX kickfd
    add_fd_list(&vapp_client->client->fd_list, FD_READ,
            vapp_client->vring_table.vring[VAPP_CLIENT_VRING_IDX_RX].kickfd,
            (void*) vapp_client, _kick_client);

    return 0;
}

int end_vapp_client(VappClient* vapp_client)
{
    int i = 0;

    vhost_ioctl(vapp_client->client, VHOST_USER_RESET_OWNER, 0);

    // free all shared memory mappings
    for (i = 0; i<vapp_client->memory.nregions; i++)
    {
        end_shm(vapp_client->client->sock_path,
                (void*)vapp_client->memory.regions[i].guest_phys_addr,
                vapp_client->memory.regions[i].memory_size,
                i);
    }

    end_client(vapp_client->client);

    //TODO: should this be here?
    free(vapp_client->client);
    vapp_client->client = 0;

    return 0;
}

static int send_packet(VappClient* vapp_client, void* p, size_t size)
{
    int r = 0;
    uint32_t tx_idx = VAPP_CLIENT_VRING_IDX_TX;

    r = put_vring(&vapp_client->vring_table, tx_idx, p, size);

    if (r != 0)
        return -1;

    return kick(&vapp_client->vring_table, tx_idx);
}

static int avail_handler_client(void* context, void* buf, size_t size)
{

    //VappClient* vapp_client = (VappClient*) context;

    // consume the packet
#if 0
    dump_buffer(buf, size);
#endif

    return 0;
}

static int _kick_client(FdNode* node)
{
    VappClient* vapp_client = (VappClient*) node->context;
    int kickfd = node->fd;
    ssize_t r;
    uint64_t kick_it = 0;

    r = read(kickfd, &kick_it, sizeof(kick_it));

    if (r < 0) {
        perror("recv kick");
    } else if (r == 0) {
        fprintf(stdout, "Kick fd closed\n");
        del_fd_list(&vapp_client->client->fd_list, FD_READ, kickfd);
    } else {
        int idx = VAPP_CLIENT_VRING_IDX_RX;
#if 0
        fprintf(stdout, "Got kick %ld\n", kick_it);
#endif

        process_avail_vring(&vapp_client->vring_table, idx);
    }

    return 0;
}

static int poll_client(void* context)
{
    VappClient* vapp_client = (VappClient*) context;
    uint32_t tx_idx = VAPP_CLIENT_VRING_IDX_TX;

    if (process_used_vring(&vapp_client->vring_table, tx_idx) != 0) {
        fprintf(stderr, "handle_used_vring failed.\n");
        return -1;
    }

    if (send_packet(vapp_client, (void*) VAPP_CLIENT_TEST_MESSAGE,
            VAPP_CLIENT_TEST_MESSAGE_LEN) != 0) {
        fprintf(stdout, "Send packet failed.\n");
        return -1;
    }

    update_stat(&vapp_client->stat,1);
    print_stat(&vapp_client->stat);

    return 0;
}

static AppHandlers vapp_client_handlers =
{
        .context = 0,
        .in_handler = 0,
        .poll_handler = poll_client
};

int run_vapp_client(VappClient* vapp_client)
{
    if (init_vapp_client(vapp_client) != 0)
        return -1;

    vapp_client_handlers.context = vapp_client;
    set_handler_client(vapp_client->client, &vapp_client_handlers);

    start_stat(&vapp_client->stat);
    loop_client(vapp_client->client);
    stop_stat(&vapp_client->stat);

    end_vapp_client(vapp_client);

    return 0;
}
