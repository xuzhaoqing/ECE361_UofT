#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#define MAX_INFO 128
#define MAX_DATA 1024
#define STDIN 0

#define LOGIN 		10
#define LO_ACK 		11
#define LO_NAK 		12

#define EXIT 		20

#define JOIN 		30
#define JN_ACK 		31
#define JN_NAK 		32
#define LEAVE_SESS	33
#define NEW_SESS	34
#define NS_ACK		35

#define MESSAGE 	40

#define QUERY		50
#define Q_ACK		51

#define INVITE		60
#define ACCEPT		61
#define rejected	62

typedef struct message
{
	unsigned int type;
	unsigned int size;
	unsigned char source[MAX_INFO];
	unsigned char data[MAX_DATA];
}message;

int main(int argc, char *argv[])
{
	
	int socketfd = 0, ret = 0, rv;
	int i = 0, j = 0;
	struct addrinfo hints, *serverinfo, *pointer;
	char input_command[32], input_buf[MAX_DATA];
	char recv_buf[MAX_INFO / 2];
	char username_client[MAX_INFO / 2], password_client[MAX_INFO / 2];
	char IP_server[MAX_INFO / 2], portnum_server[MAX_INFO / 2];
	char session[MAX_INFO / 2], invite_client[MAX_INFO / 2];
	message msg_send, msg_recv;

	int connected = 0, in_session = 0;
	int maxfd;
	
	fd_set read_fds;
	printf("Welcome to the Conference Application.\n");
	
	while(1)
	{
		FD_ZERO(&read_fds);
		FD_SET(STDIN, &read_fds);
		i = 0; j = 0;

		if(connected)
		{
			FD_SET(socketfd, &read_fds);
			maxfd = socketfd;
		}

		if(select(maxfd + 1, &read_fds, NULL, NULL, NULL) < 0)
			fprintf(stderr, "Client error: Select does not work.\n");

		memset(input_buf, 0, sizeof(input_buf));
		memset(&msg_send, 0, sizeof(msg_send));
		memset(&recv_buf, 0, sizeof(msg_recv));

		if(FD_ISSET(STDIN, &read_fds))
		{
			memset(input_command, 0, sizeof(input_command));

			if (!fgets(input_buf,sizeof(input_buf),stdin)){ // We check if the input is accepted.

				perror("Failed to input");
				continue;
			}

			i = 0;
			while(input_buf[i] != ' ' && input_buf[i] != '\n')
			{
				input_command[i] = input_buf[i];
				i++;
			}
			input_command[i] = '\0';
			while(input_buf[i] == ' ')
				i++;

			if(!strcmp(input_command, "/login"))
			{
				if(connected)
				{
					fprintf(stderr, "You have already connected to the server. Please enter other command.\n");
					continue;
				}

				//printf("Please input your information: Username Password Server_IP Server_Portnumber.\n");
					memset(username_client, 0, sizeof(username_client));
					memset(password_client, 0, sizeof(password_client));
					memset(IP_server, 0, sizeof(IP_server));
					memset(portnum_server, 0, sizeof(portnum_server));
					j = 0;				

				// Here we try to pick the information entered by user.
				while(input_buf[i] != ' ' && input_buf[i] != '\n' && input_buf[i] != '\t' && input_buf[i] != '\0')
				{
					username_client[j] = input_buf[i];
					i++;               // This is used for storing username
					j++;
				}
				username_client[j] = '\0';
				while(input_buf[i] == ' ')
					i++;
				j = 0;

				while(input_buf[i] != ' ' && input_buf[i] != '\n' && input_buf[i] != '\t' && input_buf[i] != '\0')
				{
					password_client[j] = input_buf[i];
					i++;               // This is used for storing password
					j++;
				}
				password_client[j] = '\0';
				while(input_buf[i] == ' ')
					i++;
				j = 0;

				while(input_buf[i] != ' ' && input_buf[i] != '\n' && input_buf[i] != '\t' && input_buf[i] != '\0')
				{
					IP_server[j] = input_buf[i];
					i++;               // This is used for storing Server IP.
					j++;
				}
				IP_server[j] = '\0';
				while(input_buf[i] == ' ')
					i++;
				j = 0;

				while(input_buf[i] != ' ' && input_buf[i] != '\n' && input_buf[i] != '\t' && input_buf[i] != '\0')
				{
					portnum_server[j] = input_buf[i];
					i++;               // This is used for storing port number
					j++;
				}
				portnum_server[j] = '\0';
				j = 0;

				if(strlen(username_client) <= 0 || strlen(password_client) <= 0 || strlen(IP_server) <= 0 || strlen(portnum_server) <= 0)
				{
					fprintf(stderr, "The num of parameters you have entered is wrong when logging in, please retry.\n");
					continue;
				}

				memset(&hints, 0, sizeof(hints));
				hints.ai_family = AF_UNSPEC;
				hints.ai_socktype = SOCK_STREAM;    // TCP

				if ((rv = getaddrinfo(IP_server, portnum_server, &hints, &serverinfo)) != 0) 
				{
					fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
					continue;
				}

				// 用 循 环 找 出 全 部 的 结 果 , 并 产 生 一 个 socket
				for(pointer = serverinfo; pointer != NULL; pointer = pointer->ai_next) 
				{
					if ((socketfd = socket(pointer->ai_family, pointer->ai_socktype, pointer->ai_protocol)) == -1) 
					{
						printf("Client: socket");
						continue;
					}
					if ((rv = connect(socketfd, pointer->ai_addr, pointer->ai_addrlen)) == -1) 
					{
                        fprintf(stderr, "Client cannot connect to the server.\n");
						continue;		
					}
					else	break;
				} // for
				if(!pointer)
				{
					fprintf(stderr, "Fail to bind socket and connect to any server.\n");
					continue;
				}

				msg_send.type = LOGIN;
				msg_send.size = strlen(password_client);
				strcpy(msg_send.source, username_client);
				strcpy(msg_send.data, password_client);

				if(send(socketfd, &msg_send, sizeof(msg_send), 0) == -1)
				{
					fprintf(stderr, "Session joining fault, please try again.\n");
					continue;
				}

				recv(socketfd, &msg_recv, sizeof(msg_recv), 0);
				if(msg_recv.type == LO_ACK)
				{
					connected = 1;
					printf("Connection is established.\n");
				}
				else
				{
					connected = 0;
					fprintf(stderr, "Connection is denied based on following reason.\n");
					fprintf(stderr, "%s", msg_recv.data);
				}
			} // if

			else if(!strcmp(input_command, "/logout"))
			{
				if(!connected)                 // if not connected
				{
					fprintf(stderr, "You have not logged in, please log in first before logging out or quit the application.\n");
					continue;
				}

				msg_send.type = EXIT;
				msg_send.size = 0;
				memcpy(msg_send.source, username_client, sizeof(username_client));
				memset(msg_send.data, 0, MAX_DATA);
				if(send(socketfd, &msg_send, sizeof(msg_send), 0) == -1)
				{
					fprintf(stderr, "Logout fault, please try again.\n");
					continue;
				}
				
				memset(input_buf, 0, sizeof(input_buf));           // Make these memory clear.
				memset(username_client, 0, sizeof(username_client));
				memset(password_client, 0, sizeof(password_client));
				memset(IP_server, 0, sizeof(IP_server));
				memset(portnum_server, 0, sizeof(portnum_server));
				memset(session, 0, sizeof(session));
				
				printf("You have already logged out.\n");
				connected = 0;
				in_session = 0;
			}

			else if(!strcmp(input_command, "/joinsession"))
			{
				if(!connected)
				{
					fprintf(stderr, "You have not logged in, please log in first before joining session or quit the application.\n");
					continue;
				}
				if(!in_session)                        // if not in session, we can join one.
				{
					memset(session, 0, sizeof(session));
					while(input_buf[i] != ' ' && input_buf[i] != '\n' && input_buf[i] != '\t' && input_buf[i] != '\0')
					{
						session[j] = input_buf[i];
						i++;               
						j++;
					}
					session[j] = '\0';
					j = 0;

					msg_send.type = JOIN;
					msg_send.size = sizeof(session);
					memcpy(msg_send.source, username_client, sizeof(username_client));
					memcpy(msg_send.data, session, sizeof(session));

					if(send(socketfd, &msg_send, sizeof(msg_send), 0) == -1)
					{
						fprintf(stderr, "Session joining fault, please try again.\n");
						continue;
					}

					recv(socketfd, &msg_recv, sizeof(msg_recv), 0);
					if(msg_recv.type == JN_ACK)
					{
						printf("Client has successfully joined in session %s.\n", session);
						in_session = 1;
					}
					else if(msg_recv.type == JN_NAK)
					{
						fprintf(stderr, "Client is rejected joining in session %s based on following reason.\n", session);
						fprintf(stderr, "%s", msg_recv.data);
						in_session = 0;
					}
				}
				else      // If in session, the client cannot join another seesion at present.
				{
					fprintf(stderr, "You have already been in session.\n");
					continue;
				}
				
			}

			else if(!strcmp(input_command, "/leavesession"))
			{
				if(!connected)
				{
					fprintf(stderr, "You have not logged in, please log in first before joining session or quit the application.\n");
					continue;
				}
				if(in_session)
				{
					msg_send.type = LEAVE_SESS;
					msg_send.size = 0;
					memcpy(msg_send.source, username_client, sizeof(username_client));
					memset(msg_send.data, 0, MAX_DATA);
					if(send(socketfd, &msg_send, sizeof(msg_send), 0) == -1)
					{
						fprintf(stderr, "Leaving session fault, please try again.\n");
						continue;
					}
printf("Now You have already left the session.\n");
					in_session = 0;
				}
				else   // If not in session, this is illegal operation.
				{
					fprintf(stderr, "You are not in session right now.\n");
					continue;
				}
			}

			else if(!strcmp(input_command, "/createsession"))
			{
				if(!connected)
				{
					fprintf(stderr, "You have not logged in, please log in first before joining session or quit the application.\n");
					continue;
				}
				if(in_session)
				{
					fprintf(stderr, "You are in session now, please leave the current session and then create new one.\n");
					continue;
				}
				else   // If not in session
				{
					memset(session, 0, sizeof(session));
					while(input_buf[i] != ' ' && input_buf[i] != '\n' && input_buf[i] != '\t' && input_buf[i] != '\0')
					{
						session[j] = input_buf[i];
						i++;               
						j++;
					}
					session[j] = '\0';
					j = 0;

					msg_send.type = NEW_SESS;
					msg_send.size = sizeof(session);
					memcpy(msg_send.source, username_client, sizeof(username_client));
					memcpy(msg_send.data, session, sizeof(session));

					if(send(socketfd, &msg_send, sizeof(msg_send), 0) == -1)
					{
						fprintf(stderr, "Creating session fault, please try again.\n");
						continue;
					}

					recv(socketfd, &msg_recv, sizeof(msg_recv), 0);
					if(msg_recv.type == NS_ACK)
					{
						printf("New session %s has been created, and you have been enrolled in this session now.\n", msg_recv.data);
						in_session = 1;
					}
					else
					{
						fprintf(stderr, "New session creation fails.\n");
						fprintf(stderr, "%s\n", msg_recv.data);
						in_session = 0;
					}
				}
			}

			else if(!strcmp(input_command, "/list"))
			{
				if(!connected)
				{
					fprintf(stderr, "You have not logged in, please log in first before joining session or quit the application.\n");
					continue;
				}                         // If client is connected, the following code will be carried out.
				msg_send.type = QUERY;
				msg_send.size = 0;
				memcpy(msg_send.source, username_client, sizeof(username_client));
				memset(msg_send.data, 0, sizeof(msg_send));

				if(send(socketfd, &msg_send, sizeof(msg_send), 0) == -1)
				{
					fprintf(stderr, "Listing fault, please try again.\n");
					continue;
				}

				recv(socketfd, &msg_recv, sizeof(msg_recv), 0);
				if(msg_recv.type = Q_ACK)
				{
					printf("These are clients who are online now:\n");
					printf("%s\n", msg_recv.data);
				}
				else
				{
					fprintf(stderr, "Query is denied by the server.\n");
					continue;
				}
			}

			else if(!strcmp(input_command, "/quit"))
			{

				msg_send.type = EXIT;
				msg_send.size = 0;
				memcpy(msg_send.source, username_client, sizeof(username_client));
				memset(msg_send.data, 0, sizeof(msg_send));

				if(send(socketfd, &msg_send, sizeof(msg_send), 0) == -1)
				{
					fprintf(stderr, "Quiting fault, please try again.\n");
					continue;
				}

				close(socketfd);
				socketfd = 0;
				printf("Quiting the conference app.\n");
				return(1);
			}

			/*else if(!strcmp(input_command, "/invite"))
			{
				if(!connected)
				{
					fprintf(stderr, "You have not logged in, please log in first before joining session or quit the application.\n");
					continue;
				}
				else   // If not in connected
				{
					memset(invite_client, 0, sizeof(invite_client));
					while(input_buf[i] != ' ' && input_buf[i] != '\n' && input_buf[i] != '\t' && input_buf[i] != '\0')
					{
						invite_client[j] = input_buf[i];
						i++;               
						j++;
					}
					invite_client[j] = '\0';
					j = 0;

					msg_send.type = INVITE;
					msg_send.size = sizeof(session);
					memcpy(msg_send.source, username_client, sizeof(username_client));
					memcpy(msg_send.data, invite_client, sizeof(invite_client));

					if(send(socketfd, &msg_send, sizeof(msg_send), 0) == -1)
					{
						fprintf(stderr, "Inviting client %s fault, please try again.\n", invite_client);
						continue;
					}

					recv(socketfd, &msg_recv, sizeof(msg_recv), 0);
					if(msg_recv.type == NS_ACK)
					{
						printf("New session %s has been create, and you have been enrolled in this session now.\n");
						in_session = 1;
					}
					else
					{
						fprintf(stderr, "New session creation fails.\n");
						in_session = 0;
					}
				}
			}

			else if(!strcmp(input_command, "/accept"))
			{
				if(!connected)
				{
					fprintf(stderr, "You have not logged in, please log in first before joining session or quit the application.\n");
					continue;
				}
				if(!invited)
				{
					fprintf(stderr, "You are not invited, please try other instructions.\n")
				}
				else   // If invited
				{
					msg_send.type = ACCEPT;
					msg_send.size = 0;
					memcpy(msg_send.source, username_client, sizeof(username_client));
					memset(msg_send.data, 0, sizeof(MAX_DATA));

					if(send(socketfd, &msg_send, sizeof(msg_send), 0) == -1)
					{
						fprintf(stderr, "Inviting client %s fault, please try again.\n", invite_client);
						continue;
					}
					invited = 0;            //After dealing with the invitation.
				}
			}
			
			else if(!strcmp(input_command, "/reject"))
			{
				if(!connected)
				{
					fprintf(stderr, "You have not logged in, please log in first before joining session or quit the application.\n");
					continue;
				}
				if(!invited)
				{
					fprintf(stderr, "You are not invited, please try other instructions.\n")
				}
				else   // If invited
				{
					msg_send.type = REJECT;
					msg_send.size = 0;
					memcpy(msg_send.source, username_client, sizeof(username_client));
					memset(msg_send.data, 0, sizeof(MAX_DATA));

					if(send(socketfd, &msg_send, sizeof(msg_send), 0) == -1)
					{
						fprintf(stderr, "Inviting client %s fault, please try again.\n", invite_client);
						continue;
					}
					invited = 0;              //After dealing with the invitation.
				}
			}*/

			else  // For message transferring.
			{
				if(!connected)
				{
					fprintf(stderr, "You have not logged in, please log in first before joining session or quit the application.\n");
					continue;
				}
				if(!in_session)
				{
					fprintf(stderr, "You are not in any session now, you can not send any message.\n");
					continue;
				}
				msg_send.type = MESSAGE;
				msg_send.size = sizeof(input_buf);              //Here we need modification
				memcpy(msg_send.source, username_client, sizeof(username_client));
				memcpy(msg_send.data, input_buf, sizeof(input_buf));

				if(send(socketfd, &msg_send, sizeof(msg_send), 0) == -1)
				{
					fprintf(stderr, "Message sending fault, please try again.\n");
					continue;
				}
				
			}
		} // if

		else if(FD_ISSET(socketfd, &read_fds) && connected)
		{
			memset(&msg_recv, 0, sizeof(msg_recv));
			if(recv(socketfd, &msg_recv, sizeof(msg_recv), 0) == -1)
				fprintf(stderr, "Something wrong when receiving.\n");
			if(msg_recv.type == MESSAGE)
			{
				if (!in_session) {
                    fprintf(stderr,"Client is not in any session, no message should be sent.");
                    continue;
                }
                printf("Message from client \"%s\":\n", msg_recv.source);
                printf("%s", msg_recv.data);
			}
			/* else if(msg_recv.type == INVITE)
			{
				invited = 1;
				printf("Client %s invite you to be enrolled in the session.\n", msg_recv.source);
			}
			else if(msg_recv.type == ACCEPT)
			{
				printf("Client %s accepts your invitation.\n", msg_recv.source);
			}
			/login xuzhaoqing buaa 128.100.13.233 8080
			/login shenrunnan runnan 128.100.13.233 8080
			/createsession con1
			/joinsession con1
			else if
			{
				printf("Client %s turns down your invitation.\n", msg_recv.source);
			}*/
		}
	} // while
}	
