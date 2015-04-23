//LocalChat Client//

#include <unistd.h>
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
#define  UDP_PORT_NUM           7001      // UDP Port number
#define  TCP_PORT_NUM           7002      // TCP Port number 
#define  BCAST_IP   "192.168.130.255"     // Broadcast IP
#define  BUF_SIZE               4096      // Max Buffer Size

//-----Globals-----------------------------------------------------------------

//==========================Socket Fields===========================//
int                  addr_len;        // Internet address length
int                  udp_s;           // UDP Client socket descriptor
int                  tcpl_s;          // TCP Client listening socket descriptor
int                  tcpr_s;          // TCP Client requesting socket descriptor
int                  main_bool = 0;
int                  tcpl_bool = 0;
int                  tcpa_bool = 0;
int                  tcpm_bool = 0;
int                  chat_bool = 0; 
char                 t_separator[4] = "::";
char                 m_separator[3] = " ";
char                 main_buf[BUF_SIZE];
char                 tcpl_buf[BUF_SIZE];
char                 tcpa_buf[BUF_SIZE];
char                 tcpm_buf[BUF_SIZE];
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
void *tcpthreadl(void *arg);
void *tcpthreadr(void *arg);
void *chatsthread(void *arg);
void *chatrthread(void *arg);
void *inputthread(void *arg);
void show_cmds(void);
void str_tok(char** array, char* string, char* separator);
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
  char                 *tokens[2];
  
  //===========Threads================================================//
  pthread_t udp_thread;
  pthread_t tcp_threadl;
  pthread_t tcp_threadr;
  pthread_t input_thread;
  pthread_t chats_thread;
  pthread_t chatr_thread;

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

  if(pthread_create(&input_thread, NULL, inputthread, NULL))
     {
       printf("Error creating input thread");
       abort();
     }
  
  //=======Create Global UDP Socket======================================//
  udp_s = socket(AF_INET, SOCK_DGRAM, 0);  // Create the sockets
  tcpl_s = socket(AF_INET, SOCK_STREAM, 0);
  tcpr_s = socket(AF_INET, SOCK_STREAM, 0);
  
  if(udp_s < 0)
    {
      printf("Error creating udp socket\n");
        exit(-1);
    }
  if(tcpl_s < 0)
    {
      printf("Error creating listening tcp socket\n");
      exit(-1);
    }
  if(tcpr_s < 0)
    {
      printf("Error creating receiving tcp socket\n");
      exit(-1);
    }

  // Set up the client's Internet address
  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(UDP_PORT_NUM);
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
      printf("Error with udp bind: err code %s\n", binderror);
      exit(-1);
    }

  client_addr.sin_port = htons(TCP_PORT_NUM);

  retcode = bind(tcpl_s, (struct sockaddr *)&client_addr, sizeof(client_addr));

   if (retcode < 0)
    {
      char *binderror = strerror(errno);
      printf("Error with listening tcp bind: err code %s\n", binderror);
      exit(-1);
    }
   
   listen(tcpl_s, 100);
   
   // Create the udp thread to begin listening for HELLO's, OK's, BYE's
   if(pthread_create(&udp_thread, NULL, udpthread, NULL))
     {
       printf("Error creating udp thread");
       abort();
     }
   
   if(pthread_create(&tcp_threadl, NULL, tcpthreadl, NULL))
     {
       printf("Error creating tcp thread");
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
   client_addr.sin_port = htons(UDP_PORT_NUM);
   // Send the hello on the broadcast IP
   retcode = sendto(udp_s, out_buf, strlen(out_buf) + 1, 0,
		    (struct sockaddr *)&client_addr, sizeof(client_addr));
   main_bool = 1;
   while(strcmp(comm_buf, "quit") != 0){ 
     pthread_mutex_lock(&lock);
     if(main_bool)
       {
	 printf("Ready for a command: \n");
	 memset(comm_buf, 0, sizeof(comm_buf));
	 if(strlen(main_buf) != 0)
	   {
	     strcpy(comm_buf, main_buf);
	     main_bool = 0;
	   }
       }
     
     comm_buf[ strlen(comm_buf)-1 ] = '\0';
     str_tok(tokens, comm_buf, m_separator);
     if(tokens[0])
       {
	 if (strcmp(tokens[0], "show") == 0)
	   {
	     show_table();
	   } 
	 else if (strcmp(tokens[0], "help") == 0)
	   {
	     show_cmds();
	   }
	 else if(strcmp(tokens[0], "chat") == 0) 
	   {
	     user tuser = fetch_user_by_name(tokens[1]);
	     if(pthread_create(&tcp_threadr, NULL, tcpthreadr, (void *)&tuser))
	       {
		 printf("Error creating tcp thread");
		 abort();
	       }
	   }
	 else if(strcmp(tokens[0], "quit") != 0)
	   {
	     printf("Invalid command\n");
	     show_cmds();
	   }
       }
     pthread_mutex_unlock(&lock);
     usleep(50000);
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
 * TCP thread to handle messaging sessions
 */

void *tcpthreadl(void *args){
  char in_buf[BUF_SIZE];
  char out_buf[BUF_SIZE];
  struct sockaddr_in thread_addr;
  user temp;
  int retcode;
  thread_addr.sin_family = AF_INET;
  thread_addr.sin_port = htons(TCP_PORT_NUM);
  thread_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  char *tokens[2];
  int local_s;
  local_s = accept(tcpl_s, (struct sockaddr *)&thread_addr, &addr_len);
  
  recv(local_s, in_buf, sizeof(in_buf), 0);
  if (retcode < 0)
      {
	printf("Error with recvfrom in tcpthreadl\n");
	exit(-1);
      }

  printf("In_buf: %s\n", in_buf); 

  if(strcmp(in_buf, "CHATREQ") == 0){
    printf("You have received a chat request from %s\n", inet_ntoa(thread_addr.sin_addr));
    
    if(!chat_bool)
      {
	pthread_mutex_lock(&lock);
	tcpl_bool = 1;
	main_bool = 0;
	pthread_mutex_unlock(&unlock);
	while(1)
	  {
	    printf("Would you like to chat with this person? (y or n)\n");
	    memset(out_buf, 0, sizeof(out_buf));
	    pthread_mutex_lock(&lock);
	    strcpy(out_buf, tcpl_buf);
	    printf("%s", out_buf);
	    out_buf[strlen(out_buf) - 1] = '\0';
	    if(strcmp(out_buf, "y") == 0)
	      {
		strcpy(out_buf, "CHATY");
		send(local_s, out_buf, sizeof(out_buf), 0);
		chat_bool = 1;
		tcpm_bool = 1;
		tcpl_bool = 0;
		
		break;
	      }
	    else if(strcmp(out_buf, "n") == 0)
	      {
		strcpy(out_buf, "CHATN");
		send(local_s, out_buf, sizeof(out_buf), 0);
		tcpl_bool = 0;
		main_bool = 1;
		break;
	      }
	    else
	      {
		printf("Invalid response\n");
		printf("Valid responses are (n)o or (y)es\n");
	      }
	    pthread_mutex_unlock(&lock);
	    usleep(50000);
	  }
      }
    else
      {
	strcpy(out_buf, "CHATN");
	send(local_s, out_buf, sizeof(out_buf), 0);
      }
  }
  fflush(stdout);
}

void *tcpthreadr(void *args){
  char in_buf[BUF_SIZE];
  char out_buf[BUF_SIZE];
  struct sockaddr_in thread_addr;
  user temp;
  int retcode;
  user temp_user;
  memcpy(&temp_user, args, sizeof(user));
  thread_addr.sin_family = AF_INET;
  thread_addr.sin_port = htons(TCP_PORT_NUM);
  thread_addr.sin_addr = temp_user.user_ip_addr;
  char *tokens[2];
  retcode = connect(tcpr_s, (struct sockaddr *)&thread_addr, sizeof(thread_addr));
  
  if (retcode < 0)
      {
	printf("Error with connect\n");
	exit(-1);
      }
  strcpy(out_buf, "CHATREQ");
  retcode = send(tcpr_s, out_buf, (strlen(out_buf) + 1), 0);
  if (retcode < 0)
      {
	printf("Error with send in tcpthreadr\n");
	exit(-1);
      }
  
  retcode = recv(tcpr_s, in_buf, sizeof(in_buf), 0);
  if (retcode < 0)
    {
      printf("Error with recv in tcpthreadr\n");
      exit(-1);
    }
  
  pthread_mutex_lock(&lock);
  if(strcmp(in_buf, "CHATY") == 0)
    {
      chat_bool = 1;
      tcpm_bool = 1;
      tcpa_bool = 0;
      if(pthread_create(&chatr_thread, NULL, chatrthread, (void *)tcpr_s))
	{
	  printf("Error creating chat receive thread");
	  abort();
	}
      if(pthread_create(&chats_thread, NULL, chatsthread, (void *)tcpr_s))
	{
	  printf("Error creating chat receive thread");
	  abort();
	}
    }
  else
    {
      main_bool = 1;
      tcpa_bool = 0;
      printf("Client declined chat\n");
    }
  pthread_mutex_unlock(&unlock);
  fflush(stdout);
}

/* 
 * Main TCP Chat send thread
 */

void *chatsthread(void *arg){
  char out_buf[BUF_SIZE];
  int retcode;
  char *tokens[2];
  int local_s = (int)arg;
  
  do
    {
      pthread_mutex_lock(&lock);
      strcpy(out_buf, tcpm_buf);
      retcode = send(local_s, out_buf, sizeof(out_buf), 0);
      if(retcode < 0)
	{
	  printf("Error with chat send\n");
	  exit(-1);
	}
      if(strcmp(out_buf, "ENDCHAT") == 0)
	{
	  printf("Ending chat session\n");
	  close(local_s);
	  main_bool = 1;
	  chat_bool = 0;
	  tcpm_bool = 0;
	}
      pthread_mutex_unlock(&lock);
    }while(strcmp(out_buf, "ENDCHAT") != 0);
}

/*
 * Main TCP Chat receive thread
 */

void *chatrthread(void *arg){
  char out_buf[BUF_SIZE];
  int retcode;
  char *tokens[2];
  int local_s = (int)arg;

  do
    {
      retcode = recv(tcpr_s, in_buf, sizeof(in_buf));
      if(retcode < 0)
	{
	  printf("Error with chat receive\n");
	  exit(-1);
	}
      
      if(strcmp(in_buf, "ENDCHAT") == 0)
	{
	  printf("Chat session has been ended\n");
	  pthread_mutex_lock(&lock);
	  chatbool = 0;
	  tcpm_bool = 0;
	  main_bool = 1;
	  retcode = close(tcpr_s);
	  pthread_mutex_unlock(&lock);
	}
      else
	{
	  str_tok(tokens, in_buf, separator);
	  printf(tokens[1]);
	}
    }while(strcmp(in_buf, "ENDCHAT") != 0);
}

/*
 * UDP thread to handle HELLO, OK, and BYE packet processing
 */

void *udpthread(void *arg) {
  char                 in_buf[BUF_SIZE];    // Input buffer for data
  char                 out_buf[BUF_SIZE];    // Output buffer for data
  struct sockaddr_in   thread_addr;  // Client IP address
  user temp;
  char *tokens[4];
  int retcode;
  thread_addr.sin_family = AF_INET;
  thread_addr.sin_port = htons(UDP_PORT_NUM);
  do {
    thread_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    retcode = recvfrom(udp_s, in_buf, sizeof(in_buf), 0,
		       (struct sockaddr *)&thread_addr, &addr_len);
    if (retcode < 0)
      {
	printf("Error with recvfrom\n");
	exit(-1);
      }
    
    str_tok(tokens, in_buf, t_separator);
    
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

      } 
    else if(strcmp(tokens[0], "BYE") == 0) {
      temp = fetch_user_by_name(tokens[1]);
      remove_user(temp);
    }
    
    fflush(stdout);
  } while (strcmp(tokens[0], "BYE") != 0);
} // end udpthread

void show_cmds(void)
{
  printf("Valid Commands:\n");
  printf(" show:        Shows online users\n");
  printf(" chat [USER]: Request a chat session with an online user\n");
  printf(" help:        Shows this list of commands\n");
  printf(" quit:        Terminates LocalChat :(\n");
}

void str_tok(char** array, char* string, char* seperator){

  int i = 0;
  array[i] = strtok(string, seperator);
  while(array[i] != NULL)
    {
      array[++i] = strtok(NULL, seperator);
    }
}

void *inputthread(void *arg){
  char local_buf[BUF_SIZE];
  while(1){
    memset(local_buf, 0, sizeof(local_buf));
    pthread_mutex_lock(&lock);
    memset(main_buf, 0, sizeof(main_buf));
    memset(tcpl_buf, 0, sizeof(tcpl_buf));
    memset(tcpa_buf, 0, sizeof(tcpa_buf));
    memset(tcpm_buf, 0, sizeof(tcpm_buf));

    fgets(local_buf, BUF_SIZE, stdin);

    if(main_bool){
      strcpy(main_buf, local_buf);
    }else if(tcpl_bool){
      strcpy(tcpl_buf, local_buf);
    }else if(tcpa_bool){
      strcpy(tcpa_buf, local_buf);
    }else if(tcpm_bool){
      strcpy(tcpm_buf, local_buf);
    }
    pthread_mutex_unlock(&lock);
    usleep(50000);
  }
}
