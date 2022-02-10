/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define BUFSIZE 1024
#define MAX_CMD	5

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

bool closegrace = false;
char cmd_table[MAX_CMD][12] = { "put", "get", "delete", "ls", "exit"};
char cmd_size[MAX_CMD] = { 3, 3, 6, 2, 4};
char cmd_arg[MAX_CMD] = { 2, 2, 2, 1, 1};
int cmd_index=0;

/*
 * Process a received command string, and send response.
 *
 * Parameters:
 *   input   The null-terminated string as typed by the user
 */
void process_command(char *input)
{
  char *p = input;
  char *end;

  // find end of string
  for (end = input; *end != '\0'; end++)
    ;

  // Lexical analysis: Tokenize input in place
  bool in_token = false;
  char *argv[10];
  int argc = 0;
  memset(argv, 0, sizeof(argv));
  for (p = input; p < end; p++) {
    if (in_token && isspace(*p)) {
      *p = '\0';
      in_token = false;
    } else if (!in_token && !isspace(*p)) {
      argv[argc++] = p;
      in_token = true;
      if (argc == sizeof(argv)/sizeof(char*) - 1)
        // too many arguments! drop remainder
        break;
    }
  }
  argv[argc] = NULL;
  if (argc == 0)   // no command
    return;

  // Dispatch command to command handler
  for (cmd_index = 0; cmd_index<MAX_CMD; cmd_index++)
	{
		if((strncasecmp(argv[0], &cmd_table[cmd_index][0], cmd_size[cmd_index])) == 0)
		{
			if (argc != cmd_arg[cmd_index])
				cmd_index = MAX_CMD;
			break;
		}
	}
	
  if (cmd_index == 2)
	{
		strncpy(input, "rm ", 3);  
		strncpy(input+3, argv[1], strlen(argv[1]));  
	}
  else if(cmd_index < 2)
	{
		strcpy(&input[0], argv[1]); 
	}
}

int main(int argc, char **argv) {
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */
	FILE *fp = NULL;
  /* 
   * check command line arguments 
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* 
   * socket: create the parent socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* 
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,
		 (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");

    /* 
     * gethostbyaddr: determine who sent the datagram
     */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n", 
	   hostp->h_name, hostaddrp);
    printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    
    process_command(buf);
    
    switch (cmd_index)
    {
    	case 0:
    		
    	break;
    	
    	case 1:
    		
    	break;
    	
    	case 2:
			printf("%s\n", buf);
    		system(buf);
    	
			n = sendto(sockfd, buf, strlen(buf), 0, 
				   (struct sockaddr *) &clientaddr, clientlen);
			if (n < 0) 
			  error("ERROR in sendto");
    	break;
    	
    	case 3:
    		fp = popen("ls *", "r");
			if (fp == NULL)
			  error("ERROR in popen");
			  			
   			fread(buf, 1, BUFSIZE, fp);
   			printf("%s\n", buf);
			n = sendto(sockfd, buf, strlen(buf), 0, 
				   (struct sockaddr *) &clientaddr, clientlen);
			if (n < 0) 
			  error("ERROR in sendto");
   			fclose(fp);
    	break;
    	
    	case 4:
			/* 
			 * sendto: echo the input back to the client 
			 */
			n = sendto(sockfd, "Server exiting", 15, 0, 
				   (struct sockaddr *) &clientaddr, clientlen);
			if (n < 0) 
			  error("ERROR in sendto");
			close(sockfd);
			closegrace = true;
    	break;
    	
    	default:
			/* 
			 * sendto: echo the input back to the client 
			 */
			n = sendto(sockfd, buf, strlen(buf), 0, 
				   (struct sockaddr *) &clientaddr, clientlen);
			if (n < 0) 
			  error("ERROR in sendto");
    	break;
    }
    
    if (closegrace)
    	break;
  }
}


