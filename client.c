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

//==========================Socket Fields===========================//
int                  addr_len;        // Internet address length
int                  udp_s;        // Client socket descriptor
//==================================================================//

//==========================LocalChat Globals=======================//
user localuser;
user user_table[30];
int duplicate_count = 0;
//==================================================================//

//==========================Thread Globals==========================//
pthread_mutex_t lock;
//==================================================================//

//===================Function prototypes=============================//
void *udpthread(void *arg);
void *tcpthread(void *arg);
void show_cmds(void);
//====================================================================//

//=====Main localchat interface==============================================
void main(void)
{
  struct sockaddr_in   client_addr;         // Server Internet address
  char                 out_buf[BUF_SIZE];   // Output buffer for data
  char                 comm_buf[141];       //command line buffer
  int                  retcode;             // Return code
  int                  iOptVal;             // Socket option value
  int                  iOptLen;             // Socket option length
  struct in_addr       my_ip;               // Server IP Address       
  int                  i;                   // Loop control variable
  
  //===========Threads================================================//
  pthread_t udp_thread;
  pthread_t tcp_thread;
  
  pthread_mutex_init(&lock, NULL);    // Create the mutex
  //==================================================================//
  
  //==============================Create User=========================//
  printf("Welcome to Local Chat! :)\n");
  show_cmds();
  printf("Please enter a desired username: \n");
  
  fgets(localuser.username, 31, stdin);
  
  // Delete new line from end of entered username
  localuser.username[strlen(localuser.username)-1] = '\0';
  
  printf("You will logged on as %s\n", localuser.username);
  
  //=======Create Global UDP Socket======================================//
  udp_s = socket(AF_INET, SOCK_DGRAM, 0);  // Create the socket
  
  if(udp_s < 0)
    {
      printf("Error creating socket");
        exit(-1);
    }

  // Set up the client's Internet address
  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(PORT_NUM);
  client_addr.sin_addr.s_addr = htonl(INADDR_ANY);    // Send and receive on
  // any IP
  
  iOptVal = 1;
  iOptLen = sizeof(int);
  setsockopt(udp_s, SOL_SOCKET, SO_BROADCAST, (void*)&iOptVal, iOptLen);
  //=====================================================================//
  
  // Bind socket to the client's Internet Address
  retcode = bind(udp_s, (struct sockaddr *)&client_addr, sizeof(client_addr));
  
  if (retcode < 0)
    {
      char *binderror = strerror(errno);
      printf("Error with bind: err code %s\n", binderror);
      exit(-1);
    }
  
  // Create the udp thread to begin listening for HELLO's, OK's, BYE's
  if(pthread_create(&udp_thread, NULL, udpthread, NULL))
    {
      printf("Error creating thread");
      abort();
    }
  
  // Now we're ready to send a HELLO message to other peers
  printf("Sending HELLO message...\n");
  
  //Craft Initial Hello message
  strcpy(out_buf, "HELLO::");
  strcat(out_buf, localuser.username);
  strcpy(comm_buf, out_buf);
  
  // Change the client address to the broadcast IP
  client_addr.sin_addr.s_addr = inet_addr(BCAST_IP);
  
  // Send the hello on the broadcast IP
  retcode = sendto(udp_s, out_buf, strlen(out_buf) + 1, 0,
		   (struct sockaddr *)&client_addr, sizeof(client_addr));
  
  while(strcmp(comm_buf, "quit") != 0){
    printf("Ready for a command: \n");
    memset(comm_buf, 0, sizeof(comm_buf));
    fgets(comm_buf, 141, stdin);
    comm_buf[ strlen(comm_buf)-1 ] = '\0';
    if (strcmp(comm_buf, "show") == 0)
      {
        show_table();
      } else if (strcmp(comm_buf, "help") == 0)
      {
        show_cmds();
      }
    else
      {
        printf("Invalid command\n");
        show_cmds();
      }
  }
  
    client_addr.sin_addr.s_addr = inet_addr(BCAST_IP);
    strcpy(out_buf, "BYE::");
    strcat(out_buf, localuser.username);
    retcode = sendto(udp_s, out_buf, strlen(out_buf) + 1, 0,
		     (struct sockaddr *)&client_addr, sizeof(client_addr));
    
    printf("Terminating...\n");
    sleep(1);
    
    if (retcode < 0) {
      printf("Sending bye didn't work. retcode = %d\n", retcode);
    }
    
    pthread_kill(udp_thread, 15);
    
    
    pthread_mutex_destroy(&lock);
    printf("Exiting...\n");
    exit(0);
    
} // end main

/*
 * UDP thread to handle HELLO, OK, and BYE packet processing
 */

void *udpthread(void *arg) {
  char                 in_buf[BUF_SIZE];    // Input buffer for data
  char                 out_buf[BUF_SIZE];    // Output buffer for data
  struct sockaddr_in   thread_addr;  // Client IP address
  user temp;
  int retcode;
  thread_addr.sin_family = AF_INET;
  thread_addr.sin_port = htons(PORT_NUM);
  char *tokens[4];
  char separator[4] = "::";
  do {
    thread_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    retcode = recvfrom(udp_s, in_buf, sizeof(in_buf), 0,
		       (struct sockaddr *)&thread_addr, &addr_len);
    if (retcode < 0)
      {
	printf("Error with recvfrom\n");
	exit(-1);
      }
    
    
    int tnum = 0;
    int j = 0;
    tokens[j] = strtok(in_buf, separator);
    while(tokens[j] != NULL)
      {
	tokens[++j] = strtok(NULL, separator);
      }
    
    if (strcmp(tokens[0], "HELLO") == 0)
      {
	temp = fetch_user_by_name(tokens[1]);
	if(strcmp(temp.username, DNE) == 0)
	  {
	    strcpy(temp.username, tokens[1]);
	    temp.user_ip_addr = thread_addr.sin_addr;
	    add_user(temp);
	    if(strcmp(temp.username, localuser.username) != 0)
	      {
		memset(out_buf, 0, sizeof(out_buf));
		strcpy(out_buf, "OK::");
		strcat(out_buf, localuser.username);
		strcat(out_buf, "::Y");
		retcode = sendto(udp_s, out_buf, (strlen(out_buf) + 1), 0,
				 (struct sockaddr *)&thread_addr, sizeof(thread_addr));
	      }
	  }
	else
	  {
	    memset(out_buf, 0, sizeof(out_buf));
	    strcpy(out_buf, "OK::");
	    strcat(out_buf, localuser.username);
	    strcat(out_buf, "::N::");
	    strcat(out_buf, tokens[1]);
	    retcode = sendto(udp_s, out_buf, (strlen(out_buf) + 1), 0,
			     (struct sockaddr *)&thread_addr, sizeof(thread_addr));
	  }
      }
    else if(strcmp(tokens[0], "OK") == 0)
      {
	if(strcmp(tokens[2], "Y") == 0)
	  {
	    printf("Username is available\n");
	    temp = create_user(thread_addr.sin_addr, tokens[1]);
	    add_user(temp);
	    duplicate_count = 0;
	  }
	else
	  {
	    duplicate_count++;
	    char d[5];
	    snprintf(d, 5, "%d", duplicate_count);
	    memset(out_buf, 0, sizeof(out_buf));
	    strcpy(out_buf, "HELLO::");
	    strcat(out_buf, tokens[3]);
	    strcat(out_buf, d);
	    printf("Username was not available, adjusting...\n");
	    clear_table();
	    strcpy(localuser.username, tokens[3]);
	    strcat(localuser.username, d);
	    thread_addr.sin_addr.s_addr = inet_addr(BCAST_IP);
	    sendto(udp_s, out_buf, (strlen(out_buf) + 1), 0, 
		   (struct sockaddr *)&thread_addr, sizeof(thread_addr));
	  }
	printf("Received an OK from %s at port %d \n", 
	       inet_ntoa(thread_addr.sin_addr), 
	       ntohs(thread_addr.sin_port));
      } 
    else if(strcmp(tokens[0], "BYE") == 0) {
      temp = fetch_user_by_name(tokens[1]);
      remove_user(temp);
    }
    
    fflush(stdout);
  } while (strcmp(tokens[0], "BYE") != 0);
} // end udpthread

void *tcpthread(void *args){
  
}

void show_cmds(void)
{
  printf("Valid Commands:\n");
  printf(" show: Shows online users\n");
  printf(" help: Shows this list of commands\n");
  printf(" quit: Terminates LocalChat :(\n");
}
