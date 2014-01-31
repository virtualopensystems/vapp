/*
 * server.c
 *
 * Copyright (c) 2014 Virtual Open Systems Sarl.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "vhost_client.h"
#include "vhost_server.h"

static struct sigaction sigact;
int app_running = 0;

static void signal_handler(int);
static void init_signals(void);
static void cleanup(void);

int main(int argc, char* argv[])
{
    int opt = 0;

    VhostClient *vhost_master = 0;
    VhostServer *vhost_slave = 0;

    atexit(cleanup);
    init_signals();

    while ((opt = getopt(argc, argv, "q:s:c:")) != -1) {

        switch (opt) {
        case 'q':
            vhost_master = new_vhost_client(optarg);
            break;
        case 's':
            vhost_slave = new_vhost_server(optarg, 1 /*is_listen*/);
            break;
        case 'c':
            vhost_slave = new_vhost_server(optarg, 0 /*is_listen*/);
            break;
        default:
            break;
        }

        if (vhost_master || vhost_slave)
            break;
    }

    if (vhost_slave) {
        run_vhost_server(vhost_slave);
        end_vhost_server(vhost_slave);
        free(vhost_slave);
    } else if (vhost_master) {
        run_vhost_client(vhost_master);
        free(vhost_master);
    } else {
        fprintf(stderr, "Usage: %s [-q path | -s path | -c path]\n", argv[0]);
        fprintf(stderr, "\t-q - act as master\n");
        fprintf(stderr, "\t-s - act as slave server\n");
        fprintf(stderr, "\t-c - act as slave client\n");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;

}

static void signal_handler(int sig){
    switch(sig)
    {
    case SIGINT:
    case SIGKILL:
    case SIGTERM:
        app_running = 0;
        break;
    default:
        break;
    }
}

static void init_signals(void){
    sigact.sa_handler = signal_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, (struct sigaction *)NULL);
}

static void cleanup(void){
    sigemptyset(&sigact.sa_mask);
    app_running = 0;
}
