/*************************************************************************
 * Copyright (c) 2016, NVIDIA CORPORATION. All rights reserved.
 *
 * See LICENSE.txt for license information
 ************************************************************************/

#ifndef NCCL_SOCKET_H_
#define NCCL_SOCKET_H_

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <netdb.h>
#include <ifaddrs.h>

typedef enum {
  GETIP_ENV = 1,
  GETIP_IB = 2,
  GETIP_NO_LO = 3,
  GETIP_WITH_LO = 4
}getIpMode_t;

static ncclResult_t createListenSocket(int *fd, struct in_addr addr, uint16_t *port) {
  /* Create socket and bind it to a port */
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    WARN("Socket creation failed");
    return ncclSystemError;
  }
  struct sockaddr_in sa_in = { AF_INET, 0 /* Any port */, addr };
  SYSCHECK(bind(sockfd, (struct sockaddr*)&sa_in, sizeof(sa_in)), "bind");

  /* Get Port */
  socklen_t size = sizeof(struct sockaddr_in);
  SYSCHECK(getsockname(sockfd, (struct sockaddr*)&sa_in, &size), "getsockname");
  *port = sa_in.sin_port;
  //INFO("Listening on port %d", *port);

  /* Put the socket in listen mode */
  SYSCHECK(listen(sockfd, 128), "listen");
  *fd = sockfd;
  return ncclSuccess;
}

struct socketAddress {
  struct in_addr ip_addr;
  uint16_t port;
};

static ncclResult_t connectAddress(struct socketAddress* address, int* fd) {
  /* Connect to a hostname / port */
  *fd = socket(AF_INET, SOCK_STREAM, 0);
  if (*fd == -1) {
    WARN("Socket creation failed");
    return ncclSystemError;
  }
  const int one = 1;
  SYSCHECK(setsockopt(*fd, IPPROTO_TCP, TCP_NODELAY, (char*)&one, sizeof(int)), "setsockopt");
/*  const int bufsize = 128*1024;
  SYSCHECK(setsockopt(*fd, SOL_SOCKET, SO_SNDBUF, (char*)&bufsize, sizeof(int)), "setsockopt");
  SYSCHECK(setsockopt(*fd, SOL_SOCKET, SO_RCVBUF, (char*)&bufsize, sizeof(int)), "setsockopt");*/
  struct sockaddr_in remote;
  remote.sin_family = AF_INET;
  remote.sin_addr = address->ip_addr;
  remote.sin_port = address->port;
  SYSCHECK(connect(*fd, (struct sockaddr*)&remote, sizeof(remote)), "connect");
  return ncclSuccess;
}

static ncclResult_t socketReceive(int fd, void* ptr, int size) {
  char* data = (char*)ptr;
  int offset = 0;
  while (offset < size) {
    int recvsize;
    SYSCHECKVAL(recv(fd, data, size-offset, 0), "recv", recvsize);
    if (recvsize == 0) {
      WARN("Connection closed by remote peer");
      return ncclSystemError;
    }
    if (recvsize == -1) {
      INFO("Recv : got retcode %d, retrying", errno);
      continue;
    }
    data += recvsize;
    offset += recvsize;
  }
  return ncclSuccess;
}

static ncclResult_t socketSend(int fd, void* ptr, int size) {
  char* data = (char*)ptr;
  int offset = 0;
  while (offset < size) {
    int sendsize;
    SYSCHECKVAL(write(fd, data, size-offset), "write", sendsize);
    if (sendsize == -1) {
      INFO("Send : got retcode %d, retrying", errno);
      continue;
    }
    data += sendsize;
    offset += sendsize;
  }
  return ncclSuccess;
}

#endif
