#ifndef __ETCP_H__
#define __ETCP_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include "skel.h"

#define NLISTEN 128
void set_address(char *host, char *port,
                 struct sockaddr_in *sap, char *protocol);
SOCKET tcp_server(char *host, char *port);
SOCKET tcp_client(char *host, char *port);
SOCKET udp_server(char *host, char *port);
SOCKET udp_client(char *host, char *port,
                  struct sockaddr_in *sap);
int readn(SOCKET fd, char *buf, size_t len);
int readvrec(SOCKET fd, char *buf, size_t len);
int readline(SOCKET fd, char *buf, size_t len);

#endif
