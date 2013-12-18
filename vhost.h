/*
 * vhost.h
 *
 * Copyright (c) 2014 Virtual Open Systems Sarl.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef VHOST_H_
#define VHOST_H_

enum {
    VHOST_MEMORY_MAX_NREGIONS = 8
};

// Structures imported from the Linux headers.
struct vhost_vring_state { unsigned int index, num; };
struct vhost_vring_file { unsigned int index; int fd; };
struct vhost_vring_addr {
  unsigned int index, flags;
  uint64_t desc_user_addr, used_user_addr, avail_user_addr, log_guest_addr;
};

struct virtio_net_hdr
{
#define VIRTIO_NET_HDR_F_NEEDS_CSUM     1       // Use csum_start, csum_offset
#define VIRTIO_NET_HDR_F_DATA_VALID    2       // Csum is valid
    uint8_t flags;
#define VIRTIO_NET_HDR_GSO_NONE         0       // Not a GSO frame
#define VIRTIO_NET_HDR_GSO_TCPV4        1       // GSO frame, IPv4 TCP (TSO)
#define VIRTIO_NET_HDR_GSO_UDP          3       // GSO frame, IPv4 UDP (UFO)
#define VIRTIO_NET_HDR_GSO_TCPV6        4       // GSO frame, IPv6 TCP
#define VIRTIO_NET_HDR_GSO_ECN          0x80    // TCP has ECN set
    uint8_t gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
};

#endif /* VHOST_H_ */
