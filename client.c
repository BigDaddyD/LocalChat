//=========================================== file = udpClientBroadcast.c =====
//=  A message "client" program to demonstrate sockets programming            =
//=   - This is udpClient.c modified to use broadcast                         =
//=============================================================================
//=  Notes:                                                                   =
//=       This program needs udpServer to be running on another host.         =
//=       Program udpServer must be started first.                            =
//=---------------------------------------------------------------------------=
//=  Example execution: (udpServer and udpClientBroadcast on host 127.0.0.1)  =
//=    Received from server: This is a reply message from SERVER to CLIENT    =
//=---------------------------------------------------------------------------=
//=  Compile:                                                                 =
//=         Unix: gcc -o udpClientBroadcast udpClientBroadcast.c              =
//=---------------------------------------------------------------------------=
//=  Execute: udpClientBroadcast                                              =
//=============================================================================

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
//===== Main program ==========================================================
void main(void)
{
  //==================Socket Fields=====================================//
  int                  client_s;        // Client socket descriptor
  struct sockaddr_in   server_addr;     // Server Internet address
  struct in_addr       server_ip_addr;  // Server IP Address
  int                  addr_len;        // Internet address length
  char                 out_buf[4096];   // Output buffer for data
  char                 in_buf[4096];    // Input buffer for data
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

  
}

