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
    FD_SET(sockfd, &master);

    message buf;
    while(1){
        read_fds = master;
        if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1 ){
            perror("select failed\n");
            exit(1);
        }

        for(i = 0; i < fdmax; i++){
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
                else{
                    if(recv(i,buf, sizeof(buf), 0) == -1){
                        perror("receive failed");
                        exit(0);
                    }

                }
                }
        }
    }


    return 0;
}