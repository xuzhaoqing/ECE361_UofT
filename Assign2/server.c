#include "header.h"


void  *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main(int argc, char *argv){
    int port_num;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints, *res;
    int sockfd, new_fd;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC; // IPV4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(NULL, port_num, &hints, &res) == -1){
        perror("Get Address Info failed\n");
        error(1);
    }

    if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
        perror("Socket Creation failed\n");
        error(1);
    }

    if(bind(sockfd,res->ai_addr, res->ai_addrlen) == -1){
        perror("Bind failed\n");
        error(1);
    }

    freeaddrinfo(res);

    if(listen(sockfd, BACKLOG) == -1){
        perror("Listen error\n");
        error(1);
    }

    printf("Start Listening:\n");


    fd_set master;    // master fd
    fd_set read_fds;  // the temporory fds we use for select()
    int fdmax = sockfd;  // fd max
    char remoteIP[INET6_ADDRSTRLEN]

    addr_size = sizeof(their_addr);
    FD_ZERO(&master);
    FD_SET(sockfd, &master);

    message buf;
    while(1){
        read_fds = master;
        if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1 ){  // read_fds will be modified to reflect which of the file discriptors you selected which is ready for reading
            perror("select failed\n");
            exit(1);
        }

        for(i = 0; i < fdmax; i++){  // from all of the file discriptor that has been modified
            if(FD_ISSET(i, &read_fds)) // we find a fds
                if(i == sockfd){ // if it's from the main sockfd
                    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size); // we accept a new sockfd
                    if(new_fd == -1){
                        perror("accept");
                    }
                    else{
                        FD_SET(new_fd, &master); // we add the new_fd into the set;
                        if(new_fd > fdmax){
                            fdmax = new_fd;
                        }
                        printf("Connection accomplished from %s on socket %d\n", 
                                inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr),
                                remoteIP,INET6_ADDRSTRLEN),
                                new_fd);

                    }
                }
                else{
                    if(recv(i, &buf, sizeof(buf), 0) == -1){
                        perror("receive failed\n");
                        exit(0);
                    }
                    do_client_request(buf, i);
                }
        }
    }


    return 0;
}

void do_client_request(buf, socketfd){
    int ret;
    char user_list[MAX_DATA];
    message msg;
    msg.type = 0;
    strcpy(msg.source,"server");     
    memset(&msg.data,0, sizeof(msg.data));

    switch(buf.type){
        case LOGIN:
            if((ret = find_user(buf.source, buf.data)) < 0){
                msg.type = LO_NAK;
                if(ret == PASSWD_FAILED){
                    strcpy(msg.data, "Username doesn't match password\n");
                }
                else if(ret == USER_FAILED){
                    strcpy(msg.data, "No this user in the database\n");
                }
                else{
                    strcpy(msg.data, "Unknown Failure, something wrong with the application\n");
                }
            }
            else{
                msg.type = LO_ACK;
            }
            break;
        
        case EXIT:
            user_logout(buf.source);
            break;

        case JOIN:
            if(user_join_session(buf.source, buf.data) < 0){
                msg.type = JN_NAK;
                sprintf(msg.data, "Join session %s Failed\n", buf.data);
            }
            else{
                msg.type = JN_ACK;
                strcpy(msg.data,buf.data);
            }
            break;
        
        case LEAVE_SESS:
            leave_session(buf.source);
            break;
        
        case NEW_SESS:
            ret = create_session(buf.source)
            msg.type = NS_ACK;
            sprintf(msg.data, "%d", ret);
            break;
        
        case MESSAGE:
            ret = check_session(buf.source)
            send_message(ret, buf.data);
            break;

        case QUERY:
            msg.type = QU_ACK;
            print_users_and_sessions(&user_list);
            strcpy(msg.data, user_list); 
            break;   

        default:
            msg.type = UNKNOWN;
            strcpy(msg.data, "Unknown Control Type\n");
        
    }

    if(msg.type != 0){  // we have to send back something
        if(send(socketfd, &msg, sizeof(msg), 0) == -1){
			perror("send socket %d error", socketfd);
            exit(1);
		}
    }

}