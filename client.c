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
int                  addr_len;               // Internet address length
int                  udp_s;                  // UDP Client socket descriptor
int                  tcpl_s;                 // TCP Client listening socket descriptor
int                  tcpr_s;                 // TCP Client requesting socket descriptor
struct sockaddr_in   client_addr;            // Internet address struct for user
struct sockaddr_in   tcp_addr;               // TCP address struct used in main connect()
int                  tcp_chat_socket = 0;    // Integer to keep track of current TCP chat socket
int                  end_chat = 0;           // Boolean to indicate ending of a chat session
char                 t_separator[3] = ":";   // Token seperator for packet processing in threads
char                 m_separator[3] = " ";   // Token separator for command handling in main
//==================================================================//

//==========================LocalChat Globals=======================//
user localuser;            //client user
user user_table[30];       //Table to hold online users
int duplicate_count = 0;   //Integer to track number of failed attempts at a user name during intialization
//==================================================================//
//===========Threads================================================//
pthread_t udp_thread;
pthread_t tcp_thread;
pthread_t chatr_thread;
pthread_t chats_thread;
//===================Function prototypes=============================//
void *udpthread(void *arg);     //UDP packet handling thread
void *tcpthread(void *arg);     //TCP listening thread
void *chatrthread(void *arg);   //Chat receive thread
void *chatsthread(void *arg);   //Chat send thread
void exit_LocalChat(void);      //Closing Function
void init_LocalChat(void);      //Initializing function
void show_cmds(void);           //Helper function to show commands
void str_tok(char** array, char* string, char* separator); //tokenizer
//====================================================================//
//=====Main localchat interface==============================================
void main(void)
{
  strcpy(localuser.username, "\n");
  char                 out_buf[BUF_SIZE];   // Output buffer for data
  char                 in_buf[BUF_SIZE];    // Input  buffer for data
  char                 comm_buf[141];       //command line buffer
  int                  retcode;             // Return code
  int                  local_s;             //placeholder in main for tcp_chat_socke
  char                 *tokens[2];          //Array of token pointers 
  user                 tuser;               //Temp user variable 
  //==================================================================//   

  init_LocalChat(); //Set up LocalChat environment
  
  //Begin command loop
  strcpy(comm_buf, "ready");  
  while(strcmp(comm_buf, "quit") != 0)
    { 
      if(tcp_chat_socket) //Check if chat request has been received
	{
	  local_s = tcp_chat_socket;
	  while(1) //Ask for input from user until request is accepted or denied 
	    {
	      if(strcmp(comm_buf, "y") == 0) //Accept request
		{
		  printf("Chat accepted\n");
		  strcpy(out_buf, "CHATY");
		  send(local_s, out_buf, sizeof(out_buf), 0);
		  if(pthread_create(&chatr_thread, NULL, chatrthread, (void *)&local_s)) //Create chat receive thread
		    {
		      printf("Error creating receiving thread\n");
		      abort();
		    }
		  if(pthread_create(&chats_thread, NULL, chatsthread, (void *)&local_s)) //Create chat send thread
		    {
		      printf("Error creating sending thread\n");
		      abort();
		    }
		  if(pthread_join(chats_thread, NULL)) //Join send thread
		    {
		      printf("Error joining sending thread\n");
		      abort();
		    }
		  //Reset global chat booleans nd close connection
		  tcp_chat_socket = 0;
		  end_chat = 0;
		  retcode = close(local_s);
		  if(retcode < 0)
		    {
		      printf("Error closing chat connection\n");
		      abort();
		    }
		  break;
		}
	      else if(strcmp(comm_buf, "n") == 0)
		{
		  printf("Chat declined\n");
		  strcpy(out_buf, "CHATN");
		  send(local_s, out_buf, sizeof(out_buf), 0);
		  memset(comm_buf, 0, sizeof(comm_buf));
		  break;
		}
	      else
		{
		  printf("Invalid response to chat request: %s\n", comm_buf);
		  fgets(comm_buf, 141, stdin);
		  comm_buf[ strlen(comm_buf)-1 ] = '\0';
		}
	    }
	}
      printf("Ready for a command:\n");
      memset(comm_buf, 0, sizeof(comm_buf));
      fgets(comm_buf, 141, stdin);
      comm_buf[ strlen(comm_buf)-1 ] = '\0';
      str_tok(tokens, comm_buf, m_separator);
      if(tokens[0])
	{
	  if(strcmp(tokens[0], "show") == 0)
	    {
	      show_table();
	    } 
	  else if (strcmp(tokens[0], "help") == 0)
	    {
	      show_cmds();
	    }
	  else if(strcmp(tokens[0], "chat") == 0) 
	    {
	      if(tokens[1])
		{
		  tuser = fetch_user_by_name(tokens[1]);
		  if(strcmp(tuser.username, DNE) != 0)
		    {
		      tcpr_s = socket(AF_INET, SOCK_STREAM, 0);
		      if(tcpr_s < 0)
			{
			  printf("Error creating receiving tcp socket\n");
			  exit(-1);
			}
		      tcp_addr.sin_addr = tuser.user_ip_addr;
		      tcp_chat_socket = tcpr_s;
		      errno = 0;
		      retcode = connect(tcpr_s, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr));	   
		      if (retcode < 0)
			{
			  printf("Error with connect\n");
			  char *connecterror = strerror(errno);
			  printf("Error with listening tcp connect: err code %s\n", connecterror);
			  exit(-1);
			}
		      strcpy(out_buf, "CHATREQ");
		      retcode = send(tcpr_s, out_buf, sizeof(out_buf), 0);
		      if (retcode < 0)
			{
			  printf("Error with send\n");
			  exit(-1);
			}
		      retcode = recv(tcpr_s, in_buf, sizeof(in_buf), 0);
		      if (retcode < 0)
			{
			  printf("Error with receive\n");
			  abort();
			}
		      if(strcmp(in_buf, "CHATY") == 0)
			{
			  printf("Chat request accepted by %s\n", tokens[1]);
			  if(pthread_create(&chatr_thread, NULL, chatrthread, (void *)&tcpr_s))
			    {
			      printf("Error creating receiving thread\n");
			      abort();
			    }
			  if(pthread_create(&chats_thread, NULL, chatsthread, (void *)&tcpr_s))
			    {
			      printf("Error creating sending thread\n");
			      abort();
			    }
			  if(pthread_join(chats_thread, NULL))
			    {
			      printf("Error joining sending thread\n");
			    }
			  tcp_chat_socket = 0;
			  end_chat = 0;
			  retcode = close(tcpr_s);
			  if(retcode < 0)
			    {
			      printf("Error closing chat connection\n");
			      abort();
			    }
			}
		      else if(strcmp(in_buf, "CHATN") == 0) 
			{
			  printf("Chat declined by %s\n", tuser.username);
			  tcp_chat_socket = 0;
			}
		    }
		  else
		    {
		      printf("User does not exist in table\n");
		      printf("Please try again\n");
		      show_table();
		    }
		}
	      else
		{
		  printf("No user specified\n");
		  show_cmds();
		}
	    }
	  else if(strcmp(tokens[0], "quit") != 0 && strcmp(tokens[0], "y") != 0 && strcmp(tokens[0], "n") != 0)
	    {
	      printf("Invalid command\n");
	      show_cmds();
	    } 
	}
      fflush(stdout);
    }
  
  exit_LocalChat();   
  exit(0);
   
} // end main

void init_LocalChat(void)
{
  int                  iOptVal;             // Socket option value
  int                  iOptLen;             // Socket option length
  int                  retcode;             // Return code
  char                 out_buf[BUF_SIZE];   // Sending buffer           
  //==============================Create User=========================//
  printf("Welcome to Local Chat! :)\n");
  show_cmds();
  while(strcmp(localuser.username, "\n") == 0)
    {
      printf("Please enter a desired username: \n");
      fgets(localuser.username, 31, stdin);
    }
  // Delete new line from end of entered username
  localuser.username[strlen(localuser.username)-1] = '\0';
  
  //=======Create Global Sockets======================================//
  udp_s = socket(AF_INET, SOCK_DGRAM, 0);  // Create the sockets
  tcpl_s = socket(AF_INET, SOCK_STREAM, 0);
  
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

  //Set up the client's Internet address
  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(UDP_PORT_NUM);
  client_addr.sin_addr.s_addr = htonl(INADDR_ANY);    //Send and receive on any IP

  //Set up TCP Internet address for use in main
  tcp_addr.sin_family = AF_INET;
  tcp_addr.sin_port = htons(TCP_PORT_NUM);
  
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
   
   if(pthread_create(&tcp_thread, NULL, tcpthread, NULL))
     {
       printf("Error creating tcp thread");
       abort();
     }

   strcpy(out_buf, "HELLO:");
   strcat(out_buf, localuser.username);
   // Change the client address to the broadcast IP
   client_addr.sin_addr.s_addr = inet_addr(BCAST_IP);
   client_addr.sin_port = htons(UDP_PORT_NUM);
   // Send the hello on the broadcast IP
   retcode = sendto(udp_s, out_buf, strlen(out_buf) + 1, 0,
		    (struct sockaddr *)&client_addr, sizeof(client_addr));
   return;
}

void exit_LocalChat(void)
{
  char out_buf[BUF_SIZE];  //Buffer
  int retcode;             //Return code

  client_addr.sin_addr.s_addr = inet_addr(BCAST_IP);
  strcpy(out_buf, "BYE:");
  strcat(out_buf, localuser.username);
  retcode = sendto(udp_s, out_buf, strlen(out_buf) + 1, 0,
		   (struct sockaddr *)&client_addr, sizeof(client_addr));

  if (retcode < 0) 
    {
      printf("Sending bye didn't work. retcode = %d\n", retcode);
    }
  printf("Terminating...\n");
  sleep(1);
 
  pthread_cancel(udp_thread);   
  pthread_cancel(tcp_thread);
  printf("Exiting\n");
  return;
}

/*
 * TCP thread to handle incoming connection requests
 */

void *tcpthread(void *args)
{
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
  while(1)
    {
      local_s = accept(tcpl_s, (struct sockaddr *)&thread_addr, &addr_len);
      recv(local_s, in_buf, sizeof(in_buf), 0);
      if (retcode < 0)
	{
	  printf("Error with recv in tcpthread\n");
	  exit(-1);
	}
      if(strcmp(in_buf, "CHATREQ") == 0)
	{
	  printf("You have received a chat request\n");
	  if(tcp_chat_socket)
	    {
	      strcpy(out_buf, "CHATN");
	      send(local_s, out_buf, sizeof(out_buf), 0);
	    }
	  else
	    {
	      printf("Would you like to accept?(y or n)\n");
	      tcp_chat_socket = local_s;
	    }
	}
      fflush(stdout);
    }
}



/* 
 * Main TCP Chat send thread
 */

void *chatsthread(void *arg)
{
  char out_buf[BUF_SIZE];
  char in_buf[BUF_SIZE];
  int retcode;
  char *tokens[2];
  int local_s = *((int *)arg);
  printf("Type 'end' to end chat session\n");
  while(!end_chat)
    {
      memset(out_buf, 0, sizeof(out_buf));
      memset(in_buf, 0, sizeof(in_buf));
      fgets(in_buf, 141, stdin);
      in_buf[ strlen(in_buf)-1 ] = '\0';
      if(strcmp(in_buf, "end") == 0)
	{
	  printf("Ending chat session\n");
	  strcpy(out_buf, "ENDCHAT");
	  end_chat = 1;
	  pthread_cancel(chatr_thread);
	}
      else
	{
	  if(strcmp(out_buf, "\n") != 0)
	    {
	      strcpy(out_buf, "MSG:");
	      strcat(out_buf, in_buf);
	    }
	  else
	    {
	      printf("Blank message\n");
	    }
	}
      if(strcmp(out_buf, "\n") != 0)
	{
	  retcode = send(local_s, out_buf, sizeof(out_buf), 0);
	}
      if(retcode < 0)
	{
	  printf("Error with chat send\n");
	  exit(-1);
	}
    }
}
      

/*
 * Main TCP Chat receive thread
 */

void *chatrthread(void *arg)
{
  char in_buf[BUF_SIZE];
  int retcode = 0;
  char temp;
  int count;
  char string[BUF_SIZE];
  int local_s = *((int *)arg);
  while(!end_chat)
    {
      memset(in_buf, 0, sizeof(in_buf));
      memset(string, 0, sizeof(string));
      count = 0;
      temp = 0;
      retcode = recv(local_s, in_buf, sizeof(in_buf), 0);
      if(retcode > 0)
	{ 
	  if(strcmp(in_buf, "ENDCHAT") == 0)
	    {
	      printf("Chat session has been ended\n");
	      end_chat = 1;
	      pthread_cancel(chats_thread);
	    }
	  else
	    {
	      while(temp != ':' && count < strlen(in_buf))
		{
		  if(in_buf[count])
		    {
		      temp = in_buf[count++];
		    }
		}
	      if(in_buf[count])
		{
		  strcpy(string, (in_buf + count));
		}
	      printf(">>> %s\n", string);
	    }
	}
      else if(retcode < 0)
	{
	  printf("Error with chat receive\n");
	  exit(-1);
	}
    }
}

/*
 * UDP thread to handle HELLO, OK, and BYE packet processing
 */

void *udpthread(void *arg) 
{
  char                 in_buf[BUF_SIZE];    // Input buffer for data
  char                 out_buf[BUF_SIZE];    // Output buffer for data
  struct sockaddr_in   thread_addr;  // Client IP address
  user temp;
  char *tokens[4];
  int retcode;
  thread_addr.sin_family = AF_INET;
  thread_addr.sin_port = htons(UDP_PORT_NUM);
  while(1) 
    {
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
		  strcpy(out_buf, "OK:");
		  strcat(out_buf, localuser.username);
		  strcat(out_buf, ":Y");
		  retcode = sendto(udp_s, out_buf, (strlen(out_buf) + 1), 0,
				   (struct sockaddr *)&thread_addr, sizeof(thread_addr));
		}
	    }
	  else
	    {
	      memset(out_buf, 0, sizeof(out_buf));
	      strcpy(out_buf, "OK:");
	      strcat(out_buf, localuser.username);
	      strcat(out_buf, ":N:");
	      strcat(out_buf, tokens[1]);
	      retcode = sendto(udp_s, out_buf, (strlen(out_buf) + 1), 0,
			       (struct sockaddr *)&thread_addr, sizeof(thread_addr));
	    }
	}
      else if(strcmp(tokens[0], "OK") == 0)
	{
	  if(strcmp(tokens[2], "Y") == 0)
	    {
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
	      strcpy(out_buf, "HELLO:");
	      strcat(out_buf, tokens[3]);
	      strcat(out_buf, d);
	      printf("Username was not available, adjusting...\n");
	      clear_table();
	      strcpy(localuser.username, tokens[3]);
	      strcat(localuser.username, d);
	      printf("Trying %s...\n", localuser.username);
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
    }
} // end udpthread

void show_cmds(void)
{
  printf("Valid Commands:\n");
  printf(" show:        Shows online users\n");
  printf(" chat [USER]: Request a chat session with an online user\n");
  printf(" help:        Shows this list of commands\n");
  printf(" quit:        Terminates LocalChat :(\n");
}

void str_tok(char** array, char* string, char* seperator)
{
  int i = 0;
  array[i] = strtok(string, seperator);
  while(array[i] != NULL)
    {
      array[++i] = strtok(NULL, seperator);
    }
}


