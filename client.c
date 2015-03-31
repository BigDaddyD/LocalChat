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
#include <errno.h>        // Needed for debugging bind errors
#include "user.h"
#include "userTable.h"
//----- Defines ---------------------------------------------------------------
#define  PORT_NUM           6071      // Port number 
#define  BCAST_IP   "192.168.130.255" // Broadcast IP
#define  BUF_SIZE           4096

//-----Globals-----------------------------------------------------------------
//==================Socket Fields=====================================//
struct sockaddr_in   server_addr;     // Server Internet address
int                  addr_len;        // Internet address length
int                  client_s;        // Client socket descriptor
//==================================================================//

//===== Main program ==========================================================
void main(void)
{

  char                 out_buf[BUF_SIZE];   // Output buffer for data
  int                  retcode;         // Return code
  int                  iOptVal;         // Socket option value
  int                  iOptLen;         // Socket option length
  struct in_addr       server_ip_addr;  // Server IP Address
  int                  i;               // Loop control variable
  //===================Function prototypes=============================//
  void *udpthreadr(void *arg);
  //====================================================================//

  //==============LocalChat User Variables============================//
  user localuser;
  user user_table[30];
  //==================================================================//

  //===========Threads================================================//
  pthread_t recv_thread;
  pthread_t send_thread;
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

  retcode = bind(client_s, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (retcode < 0)
  {
    char *binderror = strerror(errno);
    printf("Error with bind: err code %s\n", binderror);
    exit(-1);
  }

  printf("Sending message...\n");
  retcode = sendto(client_s, out_buf, (strlen(out_buf) + 1), 0,
                    (struct sockaddr *)&server_addr, sizeof(server_addr));

  if(pthread_create(&recv_thread, NULL, udpthreadr, NULL)){
    printf("Error creating thread");
    abort();
  }


  if(pthread_join(recv_thread, NULL)){
    printf("Error joining thread");
    abort();
  } 
  
  exit(0);
}

void *udpthreadr(void *arg){
  char                 in_buf[BUF_SIZE];    // Input buffer for data
  char                 out_buf[BUF_SIZE];    // Output buffer for data
  struct in_addr       client_ip_addr;  // Client IP address
  int retcode = 0;
  int i = 0;
  for(i = 0; i<5; i++){
    printf("%dth loop\n", i);
    printf("Waiting for recv()...\n");
    //retcode = bind(client_s, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (retcode < 0)
    {
        printf("Error with bind\n");
        exit(-1);
    }
    retcode = recvfrom(client_s, in_buf, sizeof(in_buf), 0,
                      (struct sockaddr *)&server_addr, &addr_len);

    if (retcode < 0)
    {
        printf("Error with recvfrom\n");
    }

    //strtok stuff
    char *token[4];
    token[0] = "HELLO";
    
    if (strcmp(token[0], "HELLO") == 0)
    {
        memcpy(&client_ip_addr, &server_addr.sin_addr.s_addr, 4);
        printf("received a hello\n");
        printf("from %s at port %d \n", inet_ntoa(client_ip_addr), ntohs(server_addr.sin_port));
        // if username is unique...
        // TODO: print client_ip_addr in a couple of places to see how it's being changed 
        retcode = sendto(client_s, out_buf, (strlen(out_buf) + 1), 0,
                    (struct sockaddr *)&server_addr, sizeof(server_addr));
    }

    printf("%s\n", in_buf);
    fflush(stdout);
    //sleep(0);
  }
}
