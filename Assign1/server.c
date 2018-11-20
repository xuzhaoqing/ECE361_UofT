
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "packet.h"
#define BACKLOG 10 // how many pending connections queue do we need?
#define BUFSIZE 1128




int main(int argc, char* argv[])
{
	int port_num;
	port_num = atoi(argv[1]); // assign the value to port_num
	int socketfd, i, r;   
	struct addrinfo hints, *serverinfo;    
	char buf[BUFSIZE]={0}; 

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	if((r = getaddrinfo(NULL, argv[1], &hints, &serverinfo)) == -1)
	{
		perror("Error occurs in server: getaddrinfo");
		exit(1);
	}

	socketfd = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol); // socket file descriptor
	if(socketfd == -1)
	{	
		perror("socket error\n");
		exit(1);
	}

	// create a socket 
	/* memset(&s_in, 0, s_in_len);	
	s_in.sin_family = AF_INET; //  use ipv4 here
	s_in.sin_port = htons(port_num); //  use Network Byter Order here
	s_in.sin_addr.s_addr = INADDR_ANY; // bind the socket to all available interfaces 
	memset(&s_in.sin_zero,0,sizeof(s_in.sin_zero)); // initialize sin_zero */

int check_fwrite;
	
	if(bind(socketfd, serverinfo->ai_addr, serverinfo->ai_addrlen) == -1) // bind the socket 
	{
		perror("bind error\n");
		exit(1);
	}
	
	printf("Start Listening the port\n");	
	if(recvfrom(socketfd,buf,BUFSIZE,0,serverinfo->ai_addr, &serverinfo->ai_addrlen) == -1) 
	// receive the message from client  (Here the s_out_len must be a pointer)
	{
		perror("receive error\n");
		exit(1);
	}		
	else{
		if(strcmp(buf,"ftp") == 0){
			if(sendto(socketfd, "yes",4, 0, serverinfo->ai_addr, serverinfo->ai_addrlen) == -1){
				perror("send error\n");
				exit(1);
			}		
		}
		else{
			if(sendto(socketfd, "no", 3, 0, serverinfo->ai_addr, serverinfo->ai_addrlen) == -1){
				perror("send error\n");
				exit(1);
			}
		}		
	}
	Packet packet;
	FILE* ptr = NULL;
	while(1){
		// receive data
		if(recvfrom(socketfd, buf , BUFSIZE, 0, serverinfo->ai_addr, &serverinfo->ai_addrlen) == -1){
			perror("receive error\n");
			exit(1);
		}
		// transform the string to Packet
		String2Packet(buf,&packet);
		// if it's the first time to transport data
		if(packet.frag_no == 1){
			ptr  = fopen(packet.filename,"wb+");
			if(ptr == NULL){
				printf("Error open file %s\n",packet.filename);
				exit(1);
			}
			fseek(ptr, 0L, SEEK_SET);
		}
		if((check_fwrite = fwrite(packet.filedata,sizeof(char),packet.size, ptr)) != packet.size){
			printf("Failure in writing %s", packet.filename);
			strcpy(packet.filedata,"NACK");
		}
		else{
			// sends ACK back
			strcpy(packet.filedata,"ACK");
		}

		Packet2String(&packet,buf);
		if(sendto(socketfd, buf, BUFSIZE, 0, serverinfo->ai_addr, serverinfo->ai_addrlen) == -1){
			printf("send error in packet fragment %d\n", packet.frag_no);
			exit(1);
		}
		// if it's EOF file
		if(packet.frag_no == packet.total_frag){
			fclose(ptr);
			printf("File %s has transfered successfully\n",packet.filename);
			break;
		}
	}		
	close(r);
	return 0;
}
