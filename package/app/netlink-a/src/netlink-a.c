#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define NETLINK_USER 11
#define MAX_PAYLOAD 1024
#define NETLINK_ASSOC_DEBUG_PORT_ID  0x10

int main(int argc, char *argv[]) {
  
  // man 7 netlink
  // netlink - communication between kernel and user space (AF_NETLINK)
  // netlink_socket = socket(AF_NETLINK, socket_type, netlink_family);
  // Netlink  is  used to transfer information between kernel and user-space
  // processes.
  // Netlink  is  a datagram-oriented service.
  // Both SOCK_RAW and SOCK_DGRAM are valid values for socket_type.
  // However, the netlink  protocol  does not distinguish between
  // datagram and raw sockets.
  int sockfd = 0;
  sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_USER);
  if (sockfd < 0) {
    printf("err socket\n");
    return -1;
  }

  struct sockaddr_nl src_addr;
  memset(&src_addr, 0, sizeof(src_addr));
  src_addr.nl_family = AF_NETLINK;
  src_addr.nl_pid = NETLINK_ASSOC_DEBUG_PORT_ID;
  src_addr.nl_groups = 0;

  if (bind(sockfd, (struct sockaddr*)&src_addr, sizeof(src_addr)) != 0) {
    printf("bind err\n");
    goto out_free;
  }

  struct sockaddr_nl dest_addr;
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.nl_family = AF_NETLINK;
  dest_addr.nl_pid = 0; // for linux kernel
  dest_addr.nl_groups = 0; // unicast

  struct nlmsghdr *nlh = NULL;
  nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
  memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
  nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
  nlh->nlmsg_pid = NETLINK_ASSOC_DEBUG_PORT_ID;
  nlh->nlmsg_flags = 0;

  strcpy(NLMSG_DATA(nlh), "hello");

  struct iovec iov;
  memset(&iov, 0, sizeof(iov));
  iov.iov_base = (void*)nlh;
  iov.iov_len = nlh->nlmsg_len;

  struct msghdr msg;
  memset(&msg, 0, sizeof(msg));
  msg.msg_name = (void*)&dest_addr;
  msg.msg_namelen = sizeof(dest_addr);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  printf("sending message to kernel\n");
  ssize_t tlen = 0;
  tlen = sendmsg(sockfd, &msg, 0);
  if (tlen < 0) {
    printf("sendmsg err: %s\n", strerror(errno));
    goto out_free;
  }

  printf("waiting for message from kernel\n");

  while (1) {
    tlen = recvmsg(sockfd, &msg, 0);
    if (tlen < 0) {
      printf("recvmsg err: %s\n", strerror(errno));
    }
    printf("rx[%d]: %s\n", tlen, (char*)NLMSG_DATA(nlh));
  }

out_free:
  if (sockfd > 0)
    close(sockfd);

  if (nlh) {
    free(nlh);
    nlh = NULL;
  }

  return 0;
}

