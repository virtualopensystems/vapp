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

#include "vapp_client.h"
#include "vapp_server.h"

static struct sigaction sigact;
int app_running = 0;

static void signal_handler(int);
static void init_signals(void);
static void cleanup(void);

int main(int argc, char* argv[])
{
    int opt = 0;

    VappClient *vapp_client = 0;
    VappServer *server = 0;

    atexit(cleanup);
    init_signals();

    while ((opt = getopt(argc, argv, "qs:")) != -1) {

        switch (opt) {
        case 'q':
            vapp_client = new_vapp_client("qemu", /*optarg*/NULL );
            break;
        case 's':
            server = new_vapp_server("switch", optarg);
            break;
        default:
            break;
        }

        if (vapp_client || server)
            break;
    }

    if (server) {
        run_vapp_server(server);
        free(server);
    } else if (vapp_client) {
        run_vapp_client(vapp_client);
        free(vapp_client);
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
