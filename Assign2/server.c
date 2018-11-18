
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

#define BUF_SIZE 1200
#define BACKLOG 10

#define MAX_INFO 128
#define MAX_DATA 1024
#define STDIN 0

#define USER_FAILED       -1
#define PASSWD_FAILED 	  -2
#define USER_ALREADY_CONN -3

#define SESSION_FULL -4
#define SESSION_INVALID -5

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

#define UNKNOWN     60


#define MAX_USER 50
#define MAX_SESSION 10


typedef struct message
{
	unsigned int type;
	unsigned int size;
	unsigned char source[MAX_INFO];
	unsigned char data[MAX_DATA];
}message;


void  *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



typedef struct user{
    char name[MAX_INFO];
    char passwd[MAX_DATA];
    char session_id[MAX_INFO];
    int  sockfd;
    struct user *next;
}user;

user *user_list_init(){
    user *temp = NULL;
    user *head = NULL;
    FILE *fp = fopen("userinfo.txt","r");
    int ret;
    if(fp != NULL){
        while(1){
            temp = (user*)malloc(sizeof(user));
            if((ret = fscanf(fp, "%s %s",temp->name, temp->passwd)) == EOF){
                break;
            }
            
            temp->next = head;
            head = temp;
        }
    }
    else{
        perror("No userlist.txt existed\n");
        exit(1);
    }
    free(temp);
    fclose(fp);
    return head;
}

user *user_list_head = NULL;
user *curr_user_list = NULL;


int add_user_to_list(char *user_name, char *user_passwd, int sockfd){
    user *temp = curr_user_list;
    while(temp != NULL){
        if(temp->name == user_name){
            return USER_ALREADY_CONN;
        }
        else{
            temp = temp->next;
        }
    }

    temp = (user*)malloc(sizeof(user));
    strcpy(temp->name, user_name);
    strcpy(temp->passwd, user_passwd);
    temp->sockfd = sockfd;
    temp->next = curr_user_list;
    curr_user_list = temp;
    temp = NULL;
    return 1;
}


int user_login(char* user_name, char* user_passwd, int sockfd){
    if(user_list_head == NULL){
        user_list_head = user_list_init();
    }

    user *temp = user_list_head;
    while(temp != NULL){
        if(!strcmp(temp->name,user_name)){
            if(!strcmp(temp->passwd,user_passwd)){
                return  add_user_to_list(user_name, user_passwd, sockfd);
            }
            else{
                return PASSWD_FAILED;
            }
        }
        else{
            temp = temp->next;
        }
    }
    return USER_FAILED;
}

void user_logout(int sockfd){
    user *temp = curr_user_list;
    if(curr_user_list->sockfd == sockfd){
        curr_user_list = curr_user_list->next;
        printf("User %s has logged out\n", temp->name);
        free(temp);
        return ;
    }

    temp = temp->next;
    user *temp_prev = curr_user_list;
    while(temp != NULL){
        if(temp->sockfd == sockfd){
            temp_prev->next = temp->next;
            printf("User %s has logged out\n", temp->name);
            free(temp);
            return ;
        }
        temp = temp->next;
        temp_prev = temp_prev->next;
    }

    printf("Didn't find available user to log out\n");
}

user* find_user(int sockfd){
    user *my_user = curr_user_list;
    while(my_user != NULL){
        if(my_user->sockfd == sockfd){
            return my_user;
        }
    }

    // we shouldn't get here
    printf("Didn't find my user\n");
}

typedef struct session{
    char session_id[MAX_INFO];
    int user_num;
    int user_sockfd[MAX_USER];
}session;

session *session_list[MAX_SESSION];

int create_session(int sockfd, char* session_id){
    for(int i = 0; i < MAX_SESSION; i++){
        if(session_list[i] == NULL){ // find a valid session 
            session_list[i] = (session*)malloc(sizeof(session));
            strcpy(session_list[i]->session_id, session_id);
            memset(session_list[i]->user_sockfd, -1, sizeof(int)* MAX_USER);
            session_list[i]->user_sockfd[0] = sockfd;
            user *my_user = find_user(sockfd);
            strcpy(my_user->session_id, session_id);
            session_list[i]->user_num++ ;
            printf("Session %s has been created by %s\n", session_id, my_user->name);
            return 1;
        }
    }
    printf("Session is full, can't create new session\n");
    return 0;
}


int user_join_session(int sockfd, char *session_id){
    for(int j = 0; j < MAX_SESSION; j++){
        if(session_list[j] != NULL){
            if(!strcmp(session_list[j]->session_id,session_id)){
                for(int i = 0; i < MAX_USER; i++){
                    if(session_list[j]->user_sockfd[i] < 0){
                        session_list[j]->user_sockfd[i] = sockfd;
                        return 1;
                    }
                }
                printf("Session %s is full\n", session_id);
                return SESSION_FULL;
            }      
        }   
    }
    printf("Session Name Invalid\n");
    return SESSION_INVALID;

}

void leave_session(int sockfd, int being_exited){
    user* my_user = find_user(sockfd);
    
    for(int i = 0; i < MAX_SESSION; i++){
        if(session_list[i] != NULL && !strcmp(session_list[i]->session_id, my_user->session_id)){
                for(int j = 0; j < MAX_USER; j++){
                    if(session_list[i]->user_sockfd[j] == my_user->sockfd){
                        session_list[i]->user_num--;
                        session_list[i]->user_sockfd[j] = -1;
                        memset(my_user->session_id,0,sizeof(char)* MAX_INFO);
                    
                        if(session_list[i]->user_num == 0){
                            free(session_list[i]);
                            session_list[i] = NULL;
                        }
                        printf("%s has left the session %s\n", my_user->name, my_user->session_id);
                        return;
                    }
                }
        }

    }
    if(!being_exited){
        printf("Leave session error: You are not at any session!\n");
    }
}

int send_message(int sockfd, char* msg_text){
    user *my_user = find_user(sockfd);
    int curr_sockfd = -1;
    for(int i = 0; i < MAX_SESSION; i++){
        if(session_list[i] != NULL){
            if(!strcmp(session_list[i]->session_id, my_user->session_id)){
                for(int j = 0; j < MAX_USER; j++){
                    curr_sockfd = session_list[i]->user_sockfd[j];
                    if( curr_sockfd != -1 && curr_sockfd != sockfd){ // if the user exists
                            if(send(curr_sockfd, msg_text, sizeof(msg_text), 0) == -1){
                                perror("send error in send_message()\n");
                            }
                               
                    }
                }
                printf("user %s has sent a message to %s\n",my_user->name, my_user->session_id); 
                break;
            }
        }
    }
}

void print_users_and_sessions(char* user_list){
    user *temp = curr_user_list;
    int cnt = 0;
    sprintf(user_list, "User\tSession\n");
    cnt = sizeof(user_list);
    while(temp != NULL){
        sprintf(user_list + cnt, "%s\t%s\n", temp->name,temp->session_id);
        cnt = sizeof(user_list);
    }
}

void do_client_request(message buf, int sockfd, fd_set* master){
    int ret;
    char user_list[MAX_DATA];
    message msg;
    msg.type = 0;
    strcpy(msg.source,"server");     
    memset(&msg.data,0, sizeof(msg.data));
    memset(&user_list, 0, sizeof(char) * MAX_DATA);

    switch(buf.type){
        case LOGIN:
            if((ret = user_login(buf.source, buf.data, sockfd)) < 0){
                msg.type = LO_NAK;
                if(ret == PASSWD_FAILED){
                    strcpy(msg.data, "Username doesn't match password\n");
                }
                else if(ret == USER_FAILED){
                    strcpy(msg.data, "No this user in the database\n");
                }
                else if(ret == USER_ALREADY_CONN){
                    strcpy(msg.data, "User already connected\n");
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
            leave_session(sockfd,1);
            user_logout(sockfd);
            FD_CLR(sockfd, master);
            close(sockfd);
            break;

        case JOIN:
            if((ret = user_join_session(sockfd, buf.data)) < 0){
                msg.type = JN_NAK;
                if(ret == SESSION_INVALID){
                    sprintf(msg.data, "No session %s exists", buf.data);
                }
                else if(ret == SESSION_FULL){
                    sprintf(msg.data, "The session %s is full", buf.data);
                }
            }
            else{
                msg.type = JN_ACK;
                strcpy(msg.data,buf.data);
            }
            break;
        
        case LEAVE_SESS:
            leave_session(sockfd,0);
            break;
        
        case NEW_SESS:
            ret = create_session(sockfd, buf.data);
            if(ret == 1){
                msg.type = NS_ACK;
                strcpy(msg.data, buf.data);
            }
            break;
        
        case MESSAGE:
            send_message(sockfd, buf.data);
            break;

        case QUERY:
            msg.type = Q_ACK;
            print_users_and_sessions(user_list);
            strcpy(msg.data, user_list); 
            break;   

        default:
            msg.type = UNKNOWN;
            strcpy(msg.data, "Unknown Control Type\n");
        
    }

    if(msg.type != 0){  // we have to send back something
        if(send(sockfd, &msg, sizeof(msg), 0) == -1){
			printf("send socket %d error", sockfd);
            exit(1);
		}
    }

}



int main(int argc, char *argv[]){
    int port_num;
    struct sockaddr_storage their_addr;
    int addr_size;
    struct addrinfo hints, *res, *p;
    int sockfd, new_fd;
    int yes = 1;

    if(argc != 2){
        printf("server usage: server [port_number]\n");
        exit(1);
    }

   // port_num = atoi(*argv[1]); // assign the value to port_num
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC; // IPV4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(NULL, argv[1], &hints, &res) == -1){
        perror("Get Address Info failed\n");
        exit(1);
    }

    for(p =  res; p != NULL; p = p->ai_next){
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(sockfd < 0){
            continue;
        }

    
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));  // address already in use

        if(bind(sockfd,p->ai_addr, p->ai_addrlen) < 0){
            close(sockfd);
            continue;
        }
        break;
    }

    if(p == NULL){ // bind failed
        fprintf(stderr,"selectserver: failed to bind\n");
        exit(1);
    }

    freeaddrinfo(res);

    if(listen(sockfd, BACKLOG) == -1){
        perror("Listen error\n");
        exit(1);
    }

    printf("Start Listening:\n");


    fd_set master;    // master fd
    fd_set read_fds;  // the temporory fds we use for select()
    int fdmax = sockfd;  // fd max
    char remoteIP[INET6_ADDRSTRLEN];

    addr_size = sizeof(their_addr);
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &master);

    message buf;
    while(1){
        read_fds = master;
        if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1 ){  // read_fds will be modified to reflect which of the file discriptors you selected which is ready for reading
            perror("select failed\n");
            exit(1);
        }

        for(int i = 0; i < fdmax + 1; i++){  // from all of the file discriptor that has been modified
            if(FD_ISSET(i, &read_fds)){ // we find a fds
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
                    do_client_request(buf, i, &master);
                }
            }
        }
    }


    return 0;
}

