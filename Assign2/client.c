#include "header.h"

int Input_check(char *input) {//检测是否存在非法字符
	int flag = 0, i;
	for (i = 0; i < strlen(input); i++) {
		if (!((input[i] >= '0' && input[i] <= '9') || (input[i] >='a' && input[i] <= 'z') ||\
			input[i]=='_' || (input[i] >= 'A' && input[i] <= 'Z') || s[i]=='/' || s[i]==' ' ||)) {
			flag = 1;
			break;
		}
	}
	return flag;
}

int main(int argc, char *argv[])
{
	
	int socketfd = 0, ret = 0, rv;
	int i = 0;
	struct addrinfo hints, *serverinfo, *pointer;
	char input_command[32], input_buf[MAX_INFO * 2];
	char recv_buf[MAX_INFO / 2];
	char username_client[MAX_INFO / 2], password_client[MAX_INFO / 2];
	char IP_server[MAX_INFO / 2], portnum_server[MAX_INFO / 2];
	char session[MAX_INFO / 2];
	message msg_send, msg_recv;

	int connected = 0, in_session = 0;
	int maxfd;
	
	fd_set read_fds;

	memset(username_client, 0, sizeof(username_client));
	memset(password_client, 0, sizeof(password_client));
	memset(IP_server, 0, sizeof(IP_server));
	memset(portnum_server, 0, sizeof(portnum_server));
	memset(session, 0, sizeof(session));
	
	while(1)
	{
		FD_ZERO(&fds);
		FD_SET(STDIN, &read_fds);
		i = 0;

		if(connect)
		{
			FD_SET(socketfd, &read_fds);
			maxfd = socketfd;
		}

		if(select(maxfd + 1, &read_fds, NULL, NULL, NULL) < 0)
			fprintf(stderr, "Client error: Select does not work.\n");

		if(FD_ISSET(STDIN, &read_fds))
		{
			memset(input_command, 0, sizeof(input_command));
			memset(input_buf, 0, sizeof(input_buf));

			if (fgets(input_buf,sizeof(input_buf),stdin)){
				//input_buf[strcspn(input_buf, "\n")] = '\0';
			}
			else{
				perror("Failed to input");
				continue;
			}

			ret = Input_check(input_buf);
			if(!ret)
			{
				i = 0;
			}
			else
				fprintf("Illegal characters are deteced, please redo your input.\n");

			while(input_buf[i] != ' ' && input_buf[i] != '\n')
			{
				i++;
				input_command[i] = input_buf[i];
			}
			input_command[i] = '\0';

			if(!strcmp(input_command, "/login"))
			{
				if(connected)
				{
					fprintf(stderr, "You have already connected to the server. Please enter other command.\n");
					continue;
				}

				//printf("Please input your information: Username Password Server_IP Server_Portnumber.\n");
				//while(1)
					memset(input_buf, 0, sizeof(input_buf));           // Used for setting zero for the user's basic information
					memset(username_client, 0, sizeof(username_client));
					memset(password_client, 0, sizeof(password_client));
					memset(IP_server, 0, sizeof(IP_server));
					memset(portnum_server, 0, sizeof(portnum_server));				

				// Here we try to pick the information entered by user.


				memset(&hints, 0, sizeof(hints));
				hints.ai_family = AF_UNSPEC;
				hints.ai_socktype = SOCK_STREAM;

				if ((rv = getaddrinfo(/*IP address*/, /*SERVERPORT port number*/, &hints, &serverinfo)) != 0) 
				{
					fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
					continue;
				}
				// 用 循 环 找 出 全 部 的 结 果 , 并 产 生 一 个 socket
				for(pointer = serverinfo; pointer != NULL; pointer = pointer->ai_next) 
				{
					if ((sockfd = socket(pointer->ai_family, pointer->ai_socktype, pointer->ai_protocol)) == -1) 
					{
						perror("Client: socket");
						continue;
					}
					else break;
				} // for
				if(!pointer)
				{
					fprintf(stderr, "Fail to bind socket and connect to any server.\n");
					continue;
				}

				memset(&msg_send, 0, sizeof(msg_send));
				msg_send.type = LOGIN;
				msg_send.size = strlen(password_client);
				strcpy(msg_send.source, username_client);
				strcpy(msg_send.data, password_client);
				
				send(socketfd, &msg_send, sizeof(msg_send), 0);

				memset(recv_buf, 0, sizeof(recv_buf));
				recv(socketfd, recv_buf, sizeof(recv_buf), 0);

				if(atoi(recv_buf) == LOG_ACK)
				{
					connected = 1;
					printf("Connection is established.\n");
				}
				else
				{
					connected = 0;
					fprintf(stderr, "Connection denied.\n");
				}
			} // if

			else if(!strcmp(input_command, "/logout")
			{
				if(!connected)
				{
					fprintf(stderr, "You have not logged in, please log in first before logging out or quit the application.\n");
					continue;
				}

				memset(&msg_send, 0, sizeof(msg_send));
				msg_send.type = LOGOUT;
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
				close(socketfd);
				printf("You have already logged out.\n");
				connected = 0;
			}

			else if(!strcmp(input_command, "/joinsession"))
			{
				if(!connected)
				{
					fprintf(stderr, "You have not logged in, please log in first before joining session or quit the application.\n");
					continue;
				}
				if(!in_session)
				{
					memset(&msg_send, 0, sizeof(msg_send));
					msg_send.type = JOIN;
					msg_send.size = sizeof(session);
					memcpy(msg_send.source, username_client, sizeof(username_client));
					memcpy(msg_send.data, session, sizeof(session));

					if(send(socketfd, &msg_send, sizeof(msg_send), 0) == -1)
					{
						fprintf(stderr, "Session joining fault, please try again.\n");
						continue;
					}

					memset(&msg_recv, 0, sizeof(msg_recv));
					recv(socketfd, &msg_recv, sizeof(msg_recv), 0);
					if(msg_recv.type == JN_ACK)
					{
						printf("Client has successfully joined in session %s.\n", session);
						in_session = 1;
					}
					else if(msg_recv.type == JN_NAK)
					{
						printf("Client is rejected joining in session %s.\n", session);
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
					memset(&msg_send, 0, sizeof(msg_send));
					msg_send.type = LEAVE_SESS;
					msg_send.size = 0;
					memcpy(msg_send.source, username_client, sizeof(username_client));
					memset(msg_send.data, 0, MAX_DATA);
					if(send(socketfd, &msg_send, sizeof(msg_send), 0) == -1)
					{
						fprintf(stderr, "Leaving session fault, please try again.\n");
						continue;
					}

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
					memset(&msg_send, 0, sizeof(msg_send));
					msg_send.type = NEW_SESS;
					msg_send.size = sizeof(session);
					memcpy(msg_send.source, username_client, sizeof(username_client));
					memcpy(msg_send.data, session, sizeof(session));

					if(send(socketfd, &msg_send, sizeof(msg_send), 0) == -1)
					{
						fprintf(stderr, "Creating session fault, please try again.\n");
						continue;
					}


					memset(&msg_recv, 0, sizeof(msg_recv));
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

			else if(!strcmp(input_command, "/list"))
			{
				if(!connected)
				{
					fprintf(stderr, "You have not logged in, please log in first before joining session or quit the application.\n");
					continue;
				}
				// If client is connected, the following code will be carried out.
				memset(&msg_send, 0, sizeof(msg_send));
				msg_send.type = QUERY;
				msg_send.size = 0;
				memcpy(msg_send.source, username_client, sizeof(username_client));
				memset(msg_send.data, 0, sizeof(msg_send));

				if(send(socketfd, &msg_send, sizeof(msg_send), 0) == -1)
				{
					fprintf(stderr, "Listing fault, please try again.\n");
					continue;
				}

				memset(&msg_recv, 0, sizeof(msg_recv));
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
				memset(&msg_send, 0, sizeof(msg_send));
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
				memset(&msg_send, 0, sizeof(msg_send));
				msg_send.type = MESSAGE;
				msg_send.size = sizeof(ms);              //Here we need modification
				memcpy(msg_send.source, username_client, sizeof(username_client));
				memcpy(msg_send.data, 0, sizeof(ms));

				if(send(socketfd, &msg_send, sizeof(msg_send), 0) == -1)
				{
					fprintf(stderr, "Message sending fault, please try again.\n");
					continue;
				}
				
			}
		} // if

		else if(FD_ISSET(sockfd, &readfds) && connceted)
		{
			memset(&msg_recv, 0, sizeof(msg_recv));
			recv(socketfd, &msg_recv, sizeof(msg_recv), 0);
			if(msg_recv.type == MESSAGE)
			{
				if (!in_session) {
                    fprintf(stderr,"Client is not in any session, no message should be sent.");
                    continue;
                }
                printf("Message from client \"%s\":\n\n", msg_recv.source);
                printf("%s\n", packet.data);
			}
			
		}
	} // while
}	
