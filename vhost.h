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

#endif /* VHOST_H_ */
