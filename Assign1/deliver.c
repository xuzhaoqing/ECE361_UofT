#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#define MAX_PACK_LEN 1000

int main(int argc, char* argv[])
{
	int i = 0, socketfd, rv;
	char buffer[128], file_name[128], protocol_name[10], input[128];
	char packet_send[MAX_PACK_LEN + 128];
	
	// Checking if the input satisfy the requirement 
	if(argc != 3)
	{
		perror("Wrong input!");
		exit(1);
	 }
	  
	struct addrinfo hints, *serverinfo;
	int time_send, time_recv, time_round;
	
	// Initialization of hints
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	
	// Using function getaddrinfo to get basic information of server
	if((rv = getaddrinfo(argv[1], argv[2], &hints, &serverinfo)) == -1)
	{
		perror("failed to get the information from address!");
		exit(1);
	}
	
	// Setting the socket
	if((socketfd = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol)) == -1)
	{
		perror("Error occurs in client: Socket error");
		exit(1);
	}
	
	// After finishing the settings, we can accept the input of user
	memset(buffer, 0, 128);
	memset(protocol_name, 0, 10);
	memset(input, 0, 128);
	printf("Please input the requirement following the given format: Protocol file_name\n");
	
	

	while(1){
		if (fgets(input,sizeof(input),stdin)){
			input[strcspn(input, "\n")] = '\0';
		}
		else{
			perror("failed to input");
			exit(1);
		}
		// Tackling with the input
		while(input[i] != ' ' && input[i] != '\n' && input[i] != '\t'){
			protocol_name[i] = input[i];
			i++;               // This is for Protocol for file name won't consist of this
		}
		protocol_name[i] = '\0';
		while(input[i] == ' '){
			i++;              
			} // In case of more than one space
		strcpy(file_name, input + i);
		// We need to know if the file exist.
		if(access(file_name, F_OK) == -1){
			printf("File %s does not exist! Please re-enter it\n", file_name);
			i = 0;
		}
		else{
			printf("File exists.\n");
			break;
		}
	}
	
	
	time_send = clock();
	// Sending the message to server.	
	if(sendto(socketfd, protocol_name, strlen(protocol_name), 0, serverinfo->ai_addr, serverinfo->ai_addrlen) == -1)
	{
		perror("Error occurs in client: Sending!");
		exit(1);
	}
	
	// We expect to receive "Yes"
	if(recvfrom(socketfd, buffer, 128, 0, serverinfo->ai_addr, &serverinfo->ai_addrlen) == -1)
	{
		perror("Error occurs in client: Receiving!");
		exit(1);
	}
	else
	{
		char *judge = "yes";
		if(!strcmp(buffer, judge))
			printf("A file transfer can start.\n");
		else
		{
			perror("File transfer is denied.");
			exit(1);
		}
		
	}
	
	//Now we calculate the round trip time.
	struct timeval tm_out;
	time_recv = clock();
	time_round = time_recv - time_send;
	printf("The round-trip time between client and server is %d ms.\n", time_round);
	
	//Set for timeout
	tm_out.tv_sec = 1;
	tm_out.tv_usec = 100000;
	if(setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, &tm_out, sizeof(tm_out)) < 0)
	{
		printf("Failure occurs when setting timeout. Quit.\n");
		exit(1);
	}
	
	//Open the file and get the basic information of it
	FILE *file_pointer = fopen(file_name, "r");
	fseek(file_pointer, 0L, SEEK_SET);
	fseek(file_pointer, 0L, SEEK_END);
	int file_size = ftell(file_pointer);                           //filesize_count is used in circulation
	int frag_total = (file_size % MAX_PACK_LEN)?((file_size / MAX_PACK_LEN) + 1) : (file_size / MAX_PACK_LEN);
	int packet_size, frag_no = 1, header;
	fseek(file_pointer, 0L, SEEK_SET);
	int filesize_count = file_size;

	int timeout_counter = 0, retran_frag = 0;	
	while(frag_no <= frag_total)
	{
		//memset(packet_send, 0, MAX_PACK_LEN + 128);  //initialize the packet_send

		packet_size = ((filesize_count - 1000) >= 0)? 1000: filesize_count;
		//printf("file_size %d\n", packet_size);
		
		filesize_count -= packet_size;              //record the change of file size
		sprintf(packet_send, "%d:%d:%d:%s:", frag_total, frag_no, packet_size, file_name);               
		//write the packet information into memory
		header = strlen(packet_send);
		fread((void*)(packet_send + header), sizeof(char), packet_size, file_pointer);


		// This is for sending part. We are sending our packets to the server.
		if (sendto(socketfd, packet_send, (header + packet_size), 0, serverinfo->ai_addr, serverinfo->ai_addrlen) == -1)
		{
			printf("File sending fails at packet %d.\n", frag_no);
			exit(1);
		}

		// This is for receiving part. We need to know what happens during the transmission.
		// If error occurs, we just quit directly.
		if((rv = recvfrom(socketfd, buffer, 128, 0, serverinfo->ai_addr, &serverinfo->ai_addrlen)) < 0)
		{
			if(errno = EAGAIN)		
			{
				if(timeout_counter >= 5)
				{
					perror("Too many timeout on the same packet, terminating the process.\n");
					exit(-1);
				}				
				if(retran_frag == 0)
					retran_frag = frag_no;
				if(retran_frag == frag_no)
					timeout_counter++;
				else 
				{
					timeout_counter = 0;
					retran_frag = frag_no;
				}
				// The if_else statements we set here are used to detect same num of retransmission.
				// if the total same num retransmission exceeds 5, we will terminate the programme.

				printf("Time out at packet %d occurs, we will start retransmission.\n", frag_no);
				printf("Time %d for packet %d timeout.\n\n", timeout_counter, frag_no);
				filesize_count += packet_size;
				fseek(file_pointer, packet_size * (-1), SEEK_CUR);      
				// Set the pointer of file back to the position before transferring this packet.
				continue;                                         	
			}    //好啊，這個不是重傳，只是將指針回指，等到新的循環重傳

			else if(rv == -1)
			{
				perror("Issues occur during file transfering or receiving errors occur.\n");
				exit(1);
			}
		}

		char *check_ACK = "NACK";
            //this is just for debugging, no actual meanings
		if(!strcmp(buffer, check_ACK))
		{
			printf("Have not received ACK%d from server.\nRetransmit the packet%d.\n", (frag_no + 1), frag_no);
			filesize_count += packet_size;
			fseek(file_pointer, packet_size * (-1), SEEK_CUR);
			continue;                                         
		}    //好啊，這個不是重傳，只是將指針回指，等到新的循環重傳
		
		frag_no += 1;
	}
	printf("File %s transmit succeed.\n", file_name);
	
	freeaddrinfo(serverinfo);
	close(socketfd);
	fclose(file_pointer);
 } 

