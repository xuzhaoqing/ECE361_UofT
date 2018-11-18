#include "header.h"
#include "session.h"


typedef struct user{
    char name[MAX_INFO];
    char passwd[MAX_DATA];
    char sess_id[MAX_INFO];
    int  sockfd;
    struct user *next;
}user;

user *user_list_init(){
    user *temp = NULL;
    user *head = NULL;
    FILE *fp = open("userlist.txt","r");
    int ret;
    if(fp != NULL){
        while(1){
            temp = (user*)malloc(sizeof(user));
            if((ret = fscanf("%s %s\n",temp->name, temp->passwd)) == EOF){
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
    return head;
}

extern user *user_list_head = user_list_init()  // we have a user_list
extern user *curr_user_list = NULL;


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
    user *temp = user_list_head;
    while(temp != NULL){
        if(strcmp(temp->name,user_name)){
            if(strcmp(temp->passwd,user_passwd)){
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
        return
    }

    temp = temp->next;
    user *temp_prev = curr_user_list;
    while(temp != NULL){
        if(temp->sockfd == sockfd){
            temp_prev->next = temp->next;
            printf("User %s has logged out\n", temp->name);
            free(temp);
            return
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


int user_join_session(int sockfd, char* session_id){
    user *my_user = find_user(sockfd);
    return session_add_user(my_user,session_id);
}

