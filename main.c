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

    VhostClient *vhost_client = 0;
    VhostServer *vhost_server = 0;

    atexit(cleanup);
    init_signals();

    while ((opt = getopt(argc, argv, "qs:")) != -1) {

        switch (opt) {
        case 'q':
            vhost_client = new_vhost_client(/*optarg*/NULL );
            break;
        case 's':
            vhost_server = new_vhost_server(optarg);
            break;
        default:
            break;
        }

        if (vhost_client || vhost_server)
            break;
    }

    if (vhost_server) {
        run_vhost_server(vhost_server);
        end_vhost_server(vhost_server);
        free(vhost_server);
    } else if (vhost_client) {
        run_vhost_client(vhost_client);
        free(vhost_client);
    } else {
        fprintf(stderr, "Usage: %s [-q path | -s path]\n", argv[0]);
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
