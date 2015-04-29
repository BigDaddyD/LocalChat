#include "user.h"

uint32_t get_user_ip_addr(user u){
  return u.user_ip_addr.s_addr;
}

user create_user(struct in_addr ip, char* name){
  user temp;
  temp.user_ip_addr = ip;
  strcpy(temp.username, name);
  return temp;
}

char* get_username(user u){
  char* username = u.username;
  return username;
}

void print_user(user u){
  printf("Name: %s IP: %s\n", u.username, inet_ntoa(u.user_ip_addr));
}
