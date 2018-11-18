

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


