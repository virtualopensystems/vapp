/*
 * vring.c
 *
 * Copyright (c) 2014 Virtual Open Systems Sarl.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef VRING_C_
#define VRING_C_

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/eventfd.h>

#include "vring.h"
#include "shm.h"

#define VRING_IDX_NONE          ((uint16_t)-1)

static struct vhost_vring* new_vring(void* vring_base)
{
    struct vhost_vring* vring = (struct vhost_vring*) vring_base;
    int i = 0;
    uintptr_t ptr = (uintptr_t) ((char*)vring + sizeof(struct vhost_vring));
    size_t initialized_size = 0;

    // Layout the descriptor table
    for (i = 0; i < VHOST_VRING_SIZE; i++) {
        // align the pointer
        ptr = ALIGN(ptr, BUFFER_ALIGNMENT);

        vring->desc[i].addr = ptr;
        vring->desc[i].len = BUFFER_SIZE;
        vring->desc[i].flags = VIRTIO_DESC_F_WRITE;
        vring->desc[i].next = i+1;

        ptr += vring->desc[i].len;
    }

    initialized_size = ptr - (uintptr_t)vring_base;

    vring->desc[VHOST_VRING_SIZE-1].next = VRING_IDX_NONE;

    vring->avail.idx = 0;
    vring->used.idx =  0;


    sync_shm(vring_base, initialized_size);

    return vring;
}

int vring_table_from_memory_region(struct vhost_vring* vring_table[], size_t vring_table_num,
        VhostUserMemory *memory)
{
    int i = 0;

    /* TODO: here we assume we're putting each vring in a separate
     * memory region from the memory map.
     * In reality this probably is not like that
     */
    assert(vring_table_num == memory->nregions);

    for (i = 0; i < vring_table_num; i++) {
        struct vhost_vring* vring = new_vring((void*)memory->regions[i].guest_phys_addr);
        if (!vring) {
            fprintf(stderr, "Unable to create vring %d.\n", i);
            return -1;
        }
        vring_table[i] = vring;
    }

    return 0;
}

int set_host_vring(Client* client, struct vhost_vring *vring, int index)
{
    vring->kickfd = eventfd(0, EFD_NONBLOCK);
    vring->callfd = eventfd(0, EFD_NONBLOCK);
    assert(vring->kickfd >= 0);
    assert(vring->callfd >= 0);

    struct vhost_vring_state num = { .index = index, .num = VHOST_VRING_SIZE };
    struct vhost_vring_state base = { .index = index, .num = 0 };
    struct vhost_vring_file kick = { .index = index, .fd = vring->kickfd };
    struct vhost_vring_file call = { .index = index, .fd = vring->callfd };
    struct vhost_vring_addr addr = { .index = index,
            .desc_user_addr = (uint64_t) &vring->desc,
            .avail_user_addr = (uint64_t) &vring->avail,
            .used_user_addr = (uint64_t) &vring->used,
            .log_guest_addr = (uint64_t) NULL,
            .flags = 0 };

    if (vhost_ioctl(client, VHOST_USER_SET_VRING_NUM, &num) != 0)
        return -1;
    if (vhost_ioctl(client, VHOST_USER_SET_VRING_BASE, &base) != 0)
        return -1;
    if (vhost_ioctl(client, VHOST_USER_SET_VRING_KICK, &kick) != 0)
        return -1;
    if (vhost_ioctl(client, VHOST_USER_SET_VRING_CALL, &call) != 0)
        return -1;
    if (vhost_ioctl(client, VHOST_USER_SET_VRING_ADDR, &addr) != 0)
        return -1;

    return 0;
}

int set_host_vring_table(struct vhost_vring* vring_table[], size_t vring_table_num,
        Client* client)
{
    int i = 0;

    for (i = 0; i < vring_table_num; i++) {
        if (set_host_vring(client, vring_table[i], i) != 0) {
            fprintf(stderr, "Unable to init vring %d.\n", i);
            return -1;
        }
    }
    return 0;
}

int put_vring(VringTable* vring_table, uint32_t v_idx, void* buf, size_t size)
{
    struct vring_desc* desc = vring_table->vring[v_idx].desc;
    struct vring_avail* avail = vring_table->vring[v_idx].avail;
    ProcessHandler* handler = &vring_table->handler;

    uint16_t f_idx = vring_table->free_head;
    void* dest_buf = 0;

    if (f_idx == VRING_IDX_NONE) {
        //fprintf(stderr, "Vring is full - no room for new packes.\n");
        return -1;
    }

    assert(f_idx>=0 && f_idx<VHOST_VRING_SIZE);
    if (size > desc[f_idx].len) {
        return -1;
    }

    // move free head
    vring_table->free_head = desc[f_idx].next;

    // map the address
    if (handler && handler->map_handler) {
        dest_buf = (void*)handler->map_handler(handler->context, desc[f_idx].addr);
    } else {
        dest_buf = (void*)desc[f_idx].addr;
    }


    // We support only single buffer per packet
    memcpy(dest_buf,buf,size);
    desc[f_idx].len = size;
    desc[f_idx].flags = 0;
    desc[f_idx].next = VRING_IDX_NONE;

    // add to avail
    avail->ring[avail->idx] = f_idx;

    // move/warp avail.idx to next
    avail->idx = (avail->idx+1)%VHOST_VRING_SIZE;

    sync_shm(dest_buf, size);
    sync_shm((void*)&(avail), sizeof(struct vring_avail));

    return 0;
}

static int free_vring(VringTable* vring_table, uint32_t v_idx, uint32_t d_idx)
{
    struct vring_desc* desc = vring_table->vring[v_idx].desc;
    uint16_t f_idx = vring_table->free_head;

    assert(d_idx>=0 && d_idx<VHOST_VRING_SIZE);

    // return the descriptor back to the free list
    desc[d_idx].len = BUFFER_SIZE;
    desc[d_idx].flags |= VIRTIO_DESC_F_WRITE;
    desc[d_idx].next = f_idx;
    vring_table->free_head = d_idx;

    return 0;
}

int process_used_vring(VringTable* vring_table, uint32_t v_idx)
{
    struct vring_used* used = vring_table->vring[v_idx].used;
    uint16_t u_idx = vring_table->used_head;

    for (; u_idx != used->idx; u_idx = (u_idx + 1) % VHOST_VRING_SIZE) {
        free_vring(vring_table, v_idx, used->ring[u_idx].id);
    }

    vring_table->used_head = u_idx;

    return 0;
}

static int process_desc(VringTable* vring_table, uint32_t v_idx, uint32_t a_idx)
{
    struct vring_desc* desc = vring_table->vring[v_idx].desc;
    struct vring_avail* avail = vring_table->vring[v_idx].avail;
    struct vring_used* used = vring_table->vring[v_idx].used;
    ProcessHandler* handler = &vring_table->handler;
    uint16_t u_idx = vring_table->last_used_idx;
    uint16_t d_idx = avail->ring[a_idx];
    uint32_t len = 0;
    size_t buf_size = ETH_PACKET_SIZE;
    uint8_t buf[buf_size];

#ifdef DUMP_PACKETS
    fprintf(stdout, "chunks: ");
#endif

    for (;;) {
        void* cur = 0;
        uint32_t cur_len = desc[d_idx].len;

        // map the address
        if (handler && handler->map_handler) {
            cur = (void*)handler->map_handler(handler->context, desc[d_idx].addr);
        } else {
            cur = (void*)desc[d_idx].addr;
        }

        if (len + cur_len < buf_size) {
            memcpy(buf + len, cur, cur_len);
#ifdef DUMP_PACKETS
            fprintf(stdout, "%d ", cur_len);
#endif
        } else {
            break;
        }

        len += cur_len;

        // add it to the used ring
        used->ring[u_idx].id = d_idx;
        used->ring[u_idx].len = cur_len;
        u_idx = (u_idx + 1) % VHOST_VRING_SIZE;

        if (desc[d_idx].flags & VIRTIO_DESC_F_NEXT) {
            d_idx = desc[d_idx].next;
        } else {
            break;
        }
    }

#ifdef DUMP_PACKETS
    fprintf(stdout, "\n");
#endif

    // consume the packet
    if (handler && handler->avail_handler) {
        if (handler->avail_handler(handler->context, buf, len) != 0) {
            // error handling current packet
            // TODO: we basically drop it here
        }
    }

    return u_idx;
}

int process_avail_vring(VringTable* vring_table, uint32_t v_idx)
{
    struct vring_avail* avail = vring_table->vring[v_idx].avail;
    struct vring_used* used = vring_table->vring[v_idx].used;

    uint32_t count = 0;
    uint16_t a_idx = vring_table->last_avail_idx;
    uint16_t u_idx = vring_table->last_used_idx;

    // Loop all avail descriptors
    for (; a_idx != avail->idx; a_idx = (a_idx + 1) % VHOST_VRING_SIZE) {
        u_idx = process_desc(vring_table, v_idx, a_idx);
        count++;
    }

    vring_table->last_avail_idx = a_idx;
    vring_table->last_used_idx = u_idx;
    used->idx = u_idx;

    return count;
}

int kick(VringTable* vring_table, uint32_t v_idx)
{
    uint64_t kick_it = 1;
    int kickfd = vring_table->vring[v_idx].kickfd;

    write(kickfd, &kick_it, sizeof(kick_it));
    fsync(kickfd);

    return 0;
}

#endif /* VRING_C_ */
