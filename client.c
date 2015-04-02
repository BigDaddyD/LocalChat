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
int duplicate_count = 0;
//==================================================================//
//==========================LocalChat Globals=======================//
user localuser;
user user_table[30];

//==================================================================//

//===== Main program ==========================================================
void main(void)
{

  char                 out_buf[BUF_SIZE];   // Output buffer for data
  char                 comm_buf[141];   //command line buffer
  int                  retcode;         // Return code
  int                  iOptVal;         // Socket option value
  int                  iOptLen;         // Socket option length
  struct in_addr       my_ip;  // Server IP Address       
  int                  i;               // Loop control variable
  //===================Function prototypes=============================//
  void *udpthreadr(void *arg);
  //====================================================================//

  //===========Threads================================================//
  pthread_t recv_thread;
  pthread_t send_thread;
  //==================================================================//

  //==============================Create User=========================//
  printf("Welcome to Local Chat! :)\n");
  printf("Please enter a desired username: \n");
  fgets(localuser.username, 31, stdin);
  localuser.username[strlen(localuser.username)-1] = '\0';
  printf("You will logged on as %s\n", localuser.username);
  //TODO: Check for usename clashes

  //=======Create Global UDP Socket======================================//
  client_s = socket(AF_INET, SOCK_DGRAM, 0);
  if(client_s < 0){
    printf("Error creating socket");
    exit(-1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT_NUM);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  iOptVal = 1;
  iOptLen = sizeof(int);
  setsockopt(client_s, SOL_SOCKET, SO_BROADCAST, (void*)&iOptVal, iOptLen);
  //=====================================================================//

  //Craft Initial Hello message
 
  strcpy(out_buf, "HELLO::");
  strcat(out_buf, localuser.username);
  strcpy(comm_buf, out_buf);
    

  retcode = bind(client_s, (struct sockaddr *)&server_addr, sizeof(server_addr));

  if (retcode < 0)
  {
    char *binderror = strerror(errno);
    printf("Error with bind: err code %s\n", binderror);
    exit(-1);
  }

  if(pthread_create(&recv_thread, NULL, udpthreadr, NULL)){
    printf("Error creating thread");
    abort();
  }

  printf("Sending message...\n");
  server_addr.sin_addr.s_addr = inet_addr(BCAST_IP);
  retcode = sendto(client_s, out_buf, (strlen(out_buf) + 1), 0,
                    (struct sockaddr *)&server_addr, sizeof(server_addr));
  while(strcmp(comm_buf, "quit" != 0)){
    memset(comm_buf, 0, sizeof(comm_buf));
    fgets(comm_buf, 141, stdin);
    comm_buf[ strlen(comm_buf)-1] = '\0'; 
  } 
  
  server_addr.sin_addr.s_addr = inet_addr(BCAST_IP);
  retcode = sendto(client_s, out_buf, (strlen(out_buf) + 1) ,0, (struct *)&server_a
  exit(0);
}



void *udpthreadr(void *arg){
  char                 in_buf[BUF_SIZE];    // Input buffer for data
  char                 out_buf[BUF_SIZE];    // Output buffer for data
  struct in_addr       client_ip_addr;  // Client IP address
  user temp;
  int retcode;
  int i = 0;
  for(i = 0; i<5; i++){
    printf("%d loop\n", i);
    show_table();
    printf("Waiting for recv()...\n");
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    retcode = recvfrom(client_s, in_buf, sizeof(in_buf), 0,
		       (struct sockaddr *)&server_addr, &addr_len);
    if (retcode < 0)
      {
	printf("Error with recvfrom\n");
	exit(-1);
      }
    
    int tnum = 0;
    char *tokens[4];
    char separator[4] = "::";
    int j = 0;
    tokens[j] = strtok(in_buf, separator);
    while(tokens[j] != NULL)
      {
	tokens[++j] = strtok(NULL, separator);
      }
    
    if (strcmp(tokens[0], "HELLO") == 0)
      {
	printf("Received a HELLO from %s at port %d \n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
        temp = fetch_user_by_name(tokens[1]);
	if(strcmp(temp.username, DNE) == 0)
	  {
	    strcpy(temp.username, tokens[1]);
	    temp.user_ip_addr = server_addr.sin_addr;
	    add_user(temp);
	    show_table();
	    if(strcmp(temp.username, localuser.username) != 0)
	      {
		printf("Sending an yes OK to\n");
		printf("IP: %s\n", inet_ntoa(server_addr.sin_addr));
                memset(out_buf, 0, sizeof(out_buf));
		strcpy(out_buf, "OK::");
		strcat(out_buf, localuser.username);
		strcat(out_buf, "::Y");
		retcode = sendto(client_s, out_buf, (strlen(out_buf) + 1), 0,
				 (struct sockaddr *)&server_addr, sizeof(server_addr));
		printf("%s\n", inet_ntoa(server_addr.sin_addr));
	      }
	    else 
	      {
		printf("Didn't send OK b/c it's me\n");
	      }
	  }
	else
	   {
	      printf("Sending an no OK to\n");
	      printf("IP: %s\n", inet_ntoa(server_addr.sin_addr));
	      memset(out_buf, 0, sizeof(out_buf));
	      strcpy(out_buf, "OK::");
	      strcat(out_buf, localuser.username);
	      strcat(out_buf, "::N::");
	      strcat(out_buf, tokens[1]);
	      retcode = sendto(client_s, out_buf, (strlen(out_buf) + 1), 0,
				 (struct sockaddr *)&server_addr, sizeof(server_addr));
	      printf("%s\n", inet_ntoa(server_addr.sin_addr));	   
           }
      }
    else if(strcmp(tokens[0], "OK") == 0)
      {
	printf("Received an OK...\n");
	if(strcmp(tokens[2], "Y") == 0)
	  {
	    printf("Username is available\n");
	    printf("IP: %s\n", inet_ntoa(server_addr.sin_addr));
	    temp = create_user(server_addr.sin_addr, tokens[1]);
	    add_user(temp);
	    show_table();
	    duplicate_count = 0;
	  }
	else
	  {
	    duplicate_count++;
	    char d[5];
            itoa(duplicate_count, d, 10);
	    memset(out_buf, 0, sizeof(out_buf));
	    strcpy(out_buf, "HELLO::");
	    strcat(out_buf, tokens[3]);
	    strcat(out_buf, d);
	    printf("Username was not available, adjusting...\n");
	    server_addr.sin_addr.s_addr = inet_addr(BCAST_IP);
	    sendto(client_s, out_buf, (strlen(out_buf) + 1), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
	  }
	    printf("Received an OK from %s at port %d \n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
      }
    else
	 temp = fetch_user_by_name(tokens[1]);
         remove_user(temp); 
      }
    fflush(stdout);
    //sleep(0);
  }
}
