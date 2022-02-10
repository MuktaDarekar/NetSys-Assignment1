/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 * reference: https://pubs.opengroup.org/onlinepubs/009696799/functions/popen.html
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdbool.h>
#include <ctype.h>

#define BUFSIZE 1024
#define MAX_CMD	5
/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
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
  if (cmd_index < 3)
	strcpy(&input[0], argv[1]); 
}


int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* get a message from the user */
    while(1)
    {
		bzero(buf, BUFSIZE);
		printf("Command information:\n");
		printf("1. put <file name> : to send file to server\n");
		printf("2. get <file name> : to get file from server\n");
		printf("3. delete <file name> : delete's file at server\n");
		printf("4. ls : lists folder contents from server side\n");
		printf("5. exit: to exit server and client connection gracefully\n"); 
		printf("Please enter msg to be send from above: ");
		fgets(buf, BUFSIZE, stdin);

		/* send the message to the server */
		serverlen = sizeof(serveraddr);
		n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
		if (n < 0) 
		  error("ERROR in sendto");
		  
		process_command(buf);
			
		switch (cmd_index)
		{
			case 0:
				
			break;
			
			case 1:
			break;
			
			case 2:
				/* print the server's reply */
				n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
				if (n < 0) 
				  error("ERROR in recvfrom");
				printf("deleting file from server: %s\n", buf);
			break;
			
			case 3:
				/* print the server's reply */
				n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
				if (n < 0) 
				  error("ERROR in recvfrom");
				printf("ls data from server: %s\n", buf);
			break;
			
			case 4:				
				/* print the server's reply */
				n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
				if (n < 0) 
				  error("ERROR in recvfrom");
				printf("Exit status from server: %s\n", buf);
		
				close(sockfd);
				closegrace = true;
			break;
			
			default:
				/* print the server's reply */
				n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
				if (n < 0) 
				  error("ERROR in recvfrom");
				printf("Echo from server: %s\n", buf);
			break;
		}
		
		if (closegrace)
		{
			printf("Client exiting\n");
			break;
		}
    }
    return 0;
}
