//LocalChat Client//

#include <stdio.h>        // Needed for printf()
#include <stdlib.h>       // Needed for exit()
#include <string.h>       // Needed for memcpy() and strcpy()
#include <sys/types.h>    // Needed for sockets stuff
#include <netinet/in.h>   // Needed for sockets stuff
#include <sys/socket.h>   // Needed for sockets stuff
#include <arpa/inet.h>    // Needed for sockets stuff
#include <fcntl.h>        // Needed for sockets stuff
#include <netdb.h>        // Needed for sockets stuff
#include "user.h"

//----- Defines ---------------------------------------------------------------
#define  PORT_NUM           6082      // Port number 
#define  BCAST_IP   "192.168.130.255" // Broadcast IP
#define  BUF_SIZE           4096

uint32_t get_user_ip_addr(user U){
  return U.user_ip_addr.s_addr;
}
//===== Main program ==========================================================
void main(void)
{
  //==================Socket Fields=====================================//
  int                  client_s;        // Client socket descriptor
  struct sockaddr_in   server_addr;     // Server Internet address
  struct in_addr       server_ip_addr;  // Server IP Address
  int                  addr_len;        // Internet address length
  char                 out_buf[BUF_SIZE];   // Output buffer for data
  char                 in_buf[BUF_SIZE];    // Input buffer for data
  int                  retcode;         // Return code
  int                  iOptVal;         // Socket option value
  int                  iOptLen;         // Socket option length
  int                  i;               // Loop control variable
  //==================================================================//

  //==============LocalChat User Variables============================//
  user localuser;
  user user_table[30];
  //==================================================================//

  //==============================Create User=========================//
  printf("Welcome to Local Chat! :)\n");
  printf("Please enter a desired username: \n");
  fgets(localuser.username, 31, stdin);
  printf("You will logged on as %s", localuser.username);
  //TODO: Check for usename clashes

  //=======Create Global UDP Socket======================================//
  client_s = socket(AF_INET, SOCK_DGRAM, 0);
  if(client_s < 0){
    printf("Error creating socket");
    exit(-1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT_NUM);
  server_addr.sin_addr.s_addr = inet_addr(BCAST_IP);

  iOptVal = 1;
  iOptLen = sizeof(int);
  setsockopt(client_s, SOL_SOCKET, SO_BROADCAST, (void*)&iOptVal, iOptLen);
  //=====================================================================//

  //Craft Hello message
 
  strcpy(out_buf, "HELLO:");
  strcat(out_buf, localuser.username);

  
}

