#include "userTable.h"

int user_count = 0;

user user_table[30];

void add_user(user u){
  if(user_count < 30){
    user_table[user_count] = u;
    user_count++;
  }else{
    printf("User Table full! \n");
  }
}

void remove_user(user u){
  user temp;
  if(user_count > 0){
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
    printf("User not found! \n");
  }else{
    printf("No other users online! \n");
  }
}

user fetch_user_by_name(char* username){
  int i;
  user temp;
  if(user_count == 0){
    printf("No other users online! \n");
    strcpy(temp.username, EMPTY);
    return temp;
  }else{
    for(i = 0; i<user_count; i++){
      if(strcmp(user_table[i].username, username) == 0){
	  return user_table[i];
      } 
    }
    strcpy(temp.username, DNE);
    return temp;
  }
}

user fetch_user_by_ip(uint32_t ip){
  int i;
  user temp;
  if(user_count == 0){
    printf("No other users online! \n");
    strcpy(temp.username, EMPTY);
    return temp;
  }else{
    for(i = 0; i<user_count; i++){
      if(get_user_ip_addr(user_table[i]) == ip){
	return user_table[i];
      }
    }
    strcpy(temp.username, DNE);
    return temp;
  }
}
