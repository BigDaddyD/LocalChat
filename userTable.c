#include "userTable.h"

int user_count = 0;

user user_table[30];

void show_table(){
  int i;
  for(i = 0; i<user_count; i++){
    printf("User %d- ", i);
    print_user(user_table[i]);
  }
}

void add_user(user u){
  //printf("Added user!\n");
  user_table[user_count] = u;
  user_count++;
  show_table();
}

void remove_user(user u){
  user temp;
  int i;
  for(i = 0; i<user_count; i++){
    if(memcmp(&u, &user_table[i], sizeof(user)) == 0){
      int j;
      for(j = i; j<user_count; j++){
	user_table[j] = user_table[j+1];
      }
      user_count--;
      return;
    }
  }
  show_table();
  //printf("User not found! \n");
}

user fetch_user_by_name(char* username){
  int i;
  user temp;
  for(i = 0; i<user_count; i++){
    if(strcmp(user_table[i].username, username) == 0){
      return user_table[i];
    }
  }
  strcpy(temp.username, DNE);
  return temp;
}

user fetch_user_by_ip(uint32_t ip){
  int i;
  user temp;
  for(i = 0; i<user_count; i++){
    if(get_user_ip_addr(user_table[i]) == ip){
      return user_table[i];
    }
  }
  strcpy(temp.username, DNE);
  return temp;
}

void clear_table() {
    memset(user_table, 0, sizeof(user) * 30); 
    user_count = 0;
}
