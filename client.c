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
  int                  retcode;         // Return code
  int                  iOptVal;         // Socket option value
  int                  iOptLen;         // Socket option length
  struct in_addr       server_ip_addr;  // Server IP Address
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
  server_addr.sin_addr.s_addr = inet_addr(BCAST_IP);

  iOptVal = 1;
  iOptLen = sizeof(int);
  setsockopt(client_s, SOL_SOCKET, SO_BROADCAST, (void*)&iOptVal, iOptLen);
  //=====================================================================//

  //Craft Initial Hello message
 
  strcpy(out_buf, "HELLO::");
  strcat(out_buf, localuser.username);

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
  retcode = sendto(client_s, out_buf, (strlen(out_buf) + 1), 0,
                    (struct sockaddr *)&server_addr, sizeof(server_addr));

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
  user temp;
  int retcode;
  int i = 0;
  for(i = 0; i<5; i++){
    printf("%d loop\n", i);
    show_table();
    char* myIP = malloc(MAXHOSTNAMELEN);
    memset(myIP, 0, MAXHOSTNAMELEN);
    gethostname(myIP, MAXHOSTNAMELEN);
    struct hostent *ip_host = gethostbyname(myIP);
    printf("Waiting for recv()...\n");
    printf("IP: %s\n", inet_ntoa(server_addr.sin_addr));
    retcode = recvfrom(client_s, in_buf, sizeof(in_buf), 0,
		       (struct sockaddr *)&server_addr, &addr_len);
    printf("IP: %s\n", inet_ntoa(server_addr.sin_addr));
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
	    printf("Temp: %s\n", temp.username);
	    printf("Local: %s\n", localuser.username);
	    if(strcmp(temp.username, localuser.username) != 0)
	      {
		printf("Sending an OK to\n");
		printf("IP: %s\n", inet_ntoa(server_addr.sin_addr));
		strcpy(out_buf, "OK::");
		strcat(out_buf, localuser.username);
		strcat(out_buf, "::Y");
		retcode = sendto(client_s, out_buf, (strlen(out_buf) + 1), 0,
				 (struct sockaddr *)&server_addr, sizeof(server_addr));
	      }
	    else 
	      {
		printf("Didn't send OK b/c it's me\n");
	      }
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
	    char* d = (char *)duplicate_count;
	    strcpy(out_buf, "HELLO::");
	    strcat(out_buf, tokens[3]);
	    strcat(out_buf, d);
	    printf("Username was not available, adjusting...\n");
	    server_addr.sin_addr.s_addr = inet_addr(BCAST_IP);
	    printf("IP: %s\n", inet_ntoa(server_addr.sin_addr));
	    sendto(client_s, out_buf, (strlen(out_buf) + 1), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
	    printf("IP: %s\n", inet_ntoa(server_addr.sin_addr));
	  }
	    printf("Received an OK from %s at port %d \n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
      }
    else
      {
	printf("Received a mysteryyyyy packet!");
	printf("%s\n", in_buf);
      }
    fflush(stdout);
    //sleep(0);
  }
}
