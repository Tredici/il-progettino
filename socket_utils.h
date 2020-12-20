/**
 * Funzioni generiche per creare socket per client e server
 */

#ifndef SOCKET_UTILS
#define SOCKET_UTILS

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int initUDPSocket(int);
int getSocketPort(int);

#endif
