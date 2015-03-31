#include "user.h"

#ifndef _USER_TABLE_H_
#define _USER_TABLE_H_

#define DNE "Does_Not_Exist"

extern user user_table[30];
extern int user_count;

user fetch_user_by_name(char* username);

user fetch_user_by_ip(char* username);

void add_user(user u);

void remove_user(user u);

#endif
