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
#include <sys/stat.h>
#include <fcntl.h>

#define BUFSIZE 1024
#define MAX_CMD	5
/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

char cmd_table[MAX_CMD][12] = { "put", "get", "delete", "ls", "exit"};
char cmd_size[MAX_CMD] = { 3, 3, 6, 2, 4};
char cmd_arg[MAX_CMD] = { 2, 2, 2, 1, 1};
uint8_t cmd_index=0;
char filename[200] = {0};
int fd = 0;
int sockfd, n;
int serverlen;
struct sockaddr_in serveraddr;
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
  {
	strcpy(&input[0], argv[1]); 
    bzero(filename, 200);
    strcpy(filename, argv[1]);
    printf("%s %s\n", filename, argv[1]);
  }
}

uint32_t get_parity(char *pbuf, int bytes)
{
	uint32_t parity = 0;
	
	while(bytes)
	{
		while(*pbuf)
		{
			parity++;
			*pbuf = *pbuf & ((*pbuf) - 1);
		}
		*pbuf++;
		bytes--;		
	}
	
	return parity;
}

void reliable_put()
{	
    char *str = NULL;
    char databuf[105] = {0};
    int ret = 0;
	char packet = '0';
	int prv_bytes = 0;
    
	printf("Sending file %s\n", &filename[0]);
	
	fd = open(&filename[0], O_RDWR, 0764);
	if (fd == -1)
		{
        	error("ERROR opening file");
		 	return;
		}
			
	while(1)
	{
		bzero(databuf, 105);
		str = &databuf[0];
		// first 4 bytes packet number
		*str++ = packet;
		*str++; 
		// Read max 100 bytes from file
		ret = read(fd, str, 100);
		if(ret == -1)
		{
        	error("ERROR reading file");
        	//add packet check
        	break;
		} 
		if(ret == 0)
		{
			printf("done sending\n", ret);
			n = sendto(sockfd, "done", 5, 0, &serveraddr, serverlen);
			if (n < 0) 
			  error("ERROR in sendto");
			break;
		}
		databuf[1] = ret;
		
		printf("%s\n", databuf);
		
		n = sendto(sockfd, databuf, ret+2, 0, &serveraddr, serverlen);
		if (n < 0) 
		  error("ERROR in sendto");
		
		printf("Packet %c sent\n", packet);
		
		bzero(databuf, 105);
		/* print the server's reply */
		n = recvfrom(sockfd, databuf, 6, 0, &serveraddr, &serverlen);
		if (n < 0) 
		  error("ERROR in recvfrom");	
		  		  
		if(databuf[1] == 0xFF)
		{
			printf("Packet %c resend\n", databuf[0]);
			//add lseek to resend packet
			printf("lseek = %d\n", lseek(fd, prv_bytes, SEEK_SET));
		}
		else if (packet == databuf[0])
		{
			printf("Packet %c recieved successfully\n", databuf[0]);
			prv_bytes += ret;
			packet++;
			if(packet > '9')
				packet = '0';
		}
		else
		{
			printf("Packet %c resend\n", databuf[0]);
			//add lseek to resend packet
			printf("lseek = %d\n", lseek(fd, prv_bytes, SEEK_SET));
		}
	}
	close(fd);
	printf("Sent file %s successfully\n", &filename[0]);
}

void reliable_get()
{	
    char *str = NULL;
    char databuf[105] = {0};
    int bytes = 0;
	char packet = 0;
	uint32_t parity = 0;
		
	printf("Receiving file %s\n", &filename[0]);
	
	fd = open(&filename[0], O_CREAT | O_APPEND | O_RDWR, 0764);
	if (fd == -1)
		{
			error("ERROR opening file");
		 	return;
		}
		
	while(1)
	{
		bzero(databuf, 105);
		str = &databuf[0];
		
		n = recvfrom(sockfd, databuf, 105, 0,
		 	(struct sockaddr *) &serveraddr, &serverlen);
		if (n < 0)
		{
		  error("ERROR in recvfrom");
		}
		  
		if(strncmp(databuf, "done", 4) == 0)
		{
			break;
		}
		// first 4 bytes packet number
		packet = databuf[0];
		bytes = databuf[1];
		
		// number of bytes
		*str++;
		*str++;
		printf("n = %d bytes = %d  packet = %c data = %s\n", n, bytes, packet, str);
		
		if ((n <= 1) || (bytes != n-2))
		{
			databuf[0] = packet;
			databuf[1] = 0xFF;
			n = sendto(sockfd, databuf, strlen(databuf), 0, &serveraddr, serverlen);
			if (n < 0) 
			  error("ERROR in sendto");
			printf("Packet %c resend\n", packet);
			continue;
		}
		// write max 100 bytes to file
		n = write(fd, str, bytes);
		if(n == -1)
		{
        	error("ERROR writing file");
        	break;
		} 
		
		bzero(databuf, 105);
		// first 4 bytes packet number
		databuf[0] = packet;
		databuf[1] = 0x01;
		n = sendto(sockfd, databuf, strlen(databuf), 0, &serveraddr, serverlen);
		if (n < 0) 
		  error("ERROR in sendto");
		
		printf("Packet %c received successfully\n", packet);
	}
	close(fd);
	printf("Received file %s successfully\n", &filename[0]);
}

int main(int argc, char **argv) {
    //int sockfd, portno, n;
    //int serverlen;
    //struct sockaddr_in serveraddr;
    int portno;
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
		
		printf("%s\n", buf);
		/* send the message to the server */
		serverlen = sizeof(serveraddr);
		n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
		if (n < 0) 
		  error("ERROR in sendto");
		  
		process_command(buf);
			
		/* print the server's reply */
		n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
		if (n < 0) 
		  error("ERROR in recvfrom");
				
		switch (cmd_index)
		{
			case 0:
				printf("Put file response from server: %s\n", buf);
				
				if((strncasecmp("File created", buf, strlen(buf))) != 0)
					{
		  				error("ERROR in Put file response from server");
		  				break;
					}
					
				reliable_put();				
			break;
			
			case 1:
				printf("Get file response from server: %s\n", buf);
				
				if((strncasecmp("File found", buf, strlen(buf))) != 0)
					{
		  				printf("ERROR in Put file response from server");
		  				break;
					}
				
				reliable_get();	
			break;
			
			case 2:
				printf("deleting file from server: %s\n", buf);
			break;
			
			case 3:
				printf("ls data from server: %s\n", buf);
			break;
			
			case 4:				
				printf("Exit status from server: %s\n", buf);
			break;
			
			default:
				printf("Echo from server: %s\n", buf);
			break;
		}
    }
    return 0;
}




