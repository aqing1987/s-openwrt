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
#define DEFAULT_DEBUG_FILE "/proc/kes_debug"

int main(int argc, char *argv[]) {
  
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
    close(sockfd);
    return  -1;
  }

  struct sockaddr_nl dest_addr;
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.nl_family = AF_NETLINK;
  dest_addr.nl_pid = 0; // for linux kernel
  dest_addr.nl_groups = 0; // unicast

  char recvbuf[MAX_PAYLOAD];
  struct nlmsghdr *nlh = NULL;
  nlh = (struct nlmsghdr *)recvbuf;

  struct iovec iov;
  memset(&iov, 0, sizeof(iov));
  iov.iov_base = (void*)nlh;
  iov.iov_len = MAX_PAYLOAD;

  struct msghdr msg;
  memset(&msg, 0, sizeof(msg));
  msg.msg_name = (void*)&dest_addr;
  msg.msg_namelen = sizeof(dest_addr);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  printf("waiting for message from kernel\n");

  int ret = 0;
  int fd = 0;
  fd = open(DEFAULT_DEBUG_FILE, O_WRONLY);
  if (fd < 0) {
    printf("open kes_debug err\n");
    close(sockfd);
    return -1;
  }
  
  while (1) {
    memset(nlh, 0, MAX_PAYLOAD);
    memset(&dest_addr, 0, sizeof(dest_addr));
    ret = recvmsg(sockfd, &msg, 0);
    if (ret < 0) {
      printf("recvmsg err: %s\n", strerror(errno));
      continue;
    }
    //printf("rx[%d]: %s\n", ret, (char*)NLMSG_DATA(nlh));
    write(fd, NLMSG_DATA(nlh), strlen((char*)NLMSG_DATA(nlh)));
  }

  if (sockfd > 0)
    close(sockfd);
  if (fd > 0)
    close(fd);

  return 0;
}

