#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>

#ifndef _USER_H_
#define _USER_H_

typedef struct users{
  struct in_addr user_ip_addr;
  char username[33];  
}user;

struct in_addr get_user_ip_addr(user);

#endif
