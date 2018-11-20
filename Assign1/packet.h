#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <regex.h>
#define TEMP_SIZE 50
#define PACKET_SIZE 1200
#define BUF_SIZE 1100

typedef struct packet { 
	unsigned int total_frag; 
	unsigned int frag_no; 
	unsigned int size; 
	char* filename; 
	char filedata[1000]; 
} Packet;

/* It encapsulates packet into a string */
void Packet2String(Packet *packet, char* str){
	char temp[TEMP_SIZE] = {'\0'};
	int cur_len = 0;
	sprintf(str,"%d:%d:%d:%s:",packet->total_frag, \
				    packet->frag_no,    \
				    packet->size,       \
				    packet->filename );
	// To store binary data, we should use memcpy here.
	cur_len = strlen(str);
	memcpy(str+cur_len, packet->filedata, packet->size);
}


/* It breaks encapsulution of the string into a packet */
void String2Packet(char* str, Packet *packet){
	int i = 0, count = 0, start = 0;
	char temp[TEMP_SIZE] = {'\0'};
	
	while(str[i]!='\0'){
		if(str[i] == ':' && count < 4){
			count += 1;
			memset(temp, '\0', TEMP_SIZE-1);
			memcpy(temp,str + start,i - start);
			start = i + 1;	
			switch(count){
				case 1:
					packet->total_frag = atoi(temp);
					break;
				case 2: 
					packet->frag_no = atoi(temp);
					break;
				case 3:
					packet->size = atoi(temp);
					break;
				case 4:
					packet->filename = (char*)malloc(sizeof(char)*(strlen(temp)+1));
					strcpy(packet->filename,temp);
					break;
				default:
					printf("Error on the packet!\n");
					exit(1);
			}
		}
		i++;
	}
	memcpy(packet->filedata,str + start,packet->size);
	//printf("%s\n",packet->filedata);
	//printf("%d\n",packet->total_frag);
	//printf("%d\n",packet->frag_no);
	//printf("%s\n",packet->filename);
	
}

