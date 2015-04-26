SOURCES=main.c
SOURCES+=shm.c fd_list.c stat.c vapp.c vring.c
SOURCES+=server.c vapp_server.c
SOURCES+=client.c vapp_client.c

HEADERS=common.h
HEADERS+=shm.h fd_list.h stat.h vapp.h vring.h
HEADERS+=server.h vapp_server.h
HEADERS+=client.h vapp_client.h
HEADERS+=packet.h

BIN=vapp
CFLAGS += -Wall -Werror
CFLAGS += -ggdb3 -O0
LFLAGS = -lrt

all: ${BIN}

${BIN}: ${SOURCES} ${HEADERS}
		${CC} ${CFLAGS} ${SOURCES} -o $@ ${LFLAGS}

clean:
		rm -rf ${BIN}

