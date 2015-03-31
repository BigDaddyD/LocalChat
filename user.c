#include "user.h"

uint32_t get_user_ip_addr(user u){
  return u.user_ip_addr.s_addr;
}

