/*
 * Copyright 2014
 *
 * Author: 		Dinux
 * Description:		Simple chatroom in C
 * Version:		1.0
 *
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENTS	100

#define SERVER_PORT (5000)

static unsigned int cli_count = 0;
static int uid = 10;

/* Client structure */
typedef struct {
	struct sockaddr_in addr;	/* Client remote address */
	int connfd;			/* Connection file descriptor */
	int uid;			/* Client unique identifier */
	char name[32];			/* Client name */
} client_t;

client_t *clients[MAX_CLIENTS];

/* Add client to queue */
void queue_add(client_t *cl){
	int i;
	for(i=0;i<MAX_CLIENTS;i++){
		if(!clients[i]){
			clients[i] = cl;
			return;
		}
	}
}

/* Delete client from queue */
void queue_delete(int uid){
	int i;
	for(i=0;i<MAX_CLIENTS;i++){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				return;
			}
		}
	}
}

/* Send message to all clients but the sender */
void send_message(char *s, int uid){
	int i;
	for(i=0;i<MAX_CLIENTS;i++){
		if(clients[i]){
			if(clients[i]->uid != uid){
				write(clients[i]->connfd, s, strlen(s));
			}
		}
	}
}

/* Send message to all clients */
void send_message_all(char *s){
	int i;
	for(i=0;i<MAX_CLIENTS;i++){
		if(clients[i]){
			write(clients[i]->connfd, s, strlen(s));
		}
	}
}

/* Send message to sender */
void send_message_self(const char *s, int connfd){
	write(connfd, s, strlen(s));
}

/* Send message to client */
void send_message_client(char *s, int uid){
	int i;
	for(i=0;i<MAX_CLIENTS;i++){
		if(clients[i]){
			if(clients[i]->uid == uid){
				write(clients[i]->connfd, s, strlen(s));
			}
		}
	}
}

/* Send list of active clients */
void send_active_clients(int connfd){
	int i;
	char s[64];
	for(i=0;i<MAX_CLIENTS;i++){
		if(clients[i]){
			sprintf(s, "<<CLIENT %d | %s\r\n", clients[i]->uid, clients[i]->name);
			send_message_self(s, connfd);
		}
	}
}

/* Strip CRLF */
void strip_newline(char *s){
	while(*s != '\0'){
		if(*s == '\r' || *s == '\n'){
			*s = '\0';
		}
		s++;
	}
}

/* Print ip address */
void print_client_addr(struct sockaddr_in addr){
	printf("%d.%d.%d.%d",
		addr.sin_addr.s_addr & 0xFF,
		(addr.sin_addr.s_addr & 0xFF00)>>8,
		(addr.sin_addr.s_addr & 0xFF0000)>>16,
		(addr.sin_addr.s_addr & 0xFF000000)>>24);
}

/* Get 2 byte word data */
int get_word_data(unsigned char *buff)
{
	//int res = buff[0] + (buff[1] << 8);
	int res = (buff[0] << 8) + buff[1];
	return res;
}

void set_word_data(unsigned char *buff, int data)
{
	buff[0] = (unsigned char) ((data >> 8) & 0xFF);
	buff[1] = (unsigned char) (data & 0xFF);
}


/* Handle all communication with the client */
void *handle_client(void *arg){
	unsigned char buff_out[1024];
	unsigned char buff_in[1024];
	int rlen;

	cli_count++;
	client_t *cli = (client_t *)arg;

	printf("<<ACCEPT ");
	print_client_addr(cli->addr);
	printf(" REFERENCED BY %d\n", cli->uid);

	//sprintf(buff_out, "<<JOIN, HELLO %s\r\n", cli->name);
	//send_message_all(buff_out);

	/* Receive input from client */
	while((rlen = read(cli->connfd, buff_in, sizeof(buff_in)-1)) > 0)
	{
		/*
	    buff_in[rlen] = '\0';
	    buff_out[0] = '\0';

		strip_newline(buff_in);

		// Ignore empty buffer
		if(!strlen(buff_in)){
			continue;
		}
	
		// Special options
		if(buff_in[0] == '\\'){
			char *command, *param;
			command = strtok(buff_in," ");
			if(!strcmp(command, "\\QUIT")){
				break;
			}else{
				send_message_self("<<UNKOWN COMMAND\r\n", cli->connfd);
			}
		}else{
			// Send message 
			sprintf(buff_out, "[%s] %s\r\n", cli->name, buff_in);
			send_message(buff_out, cli->uid);
		}
		*/

		printf(">> %d bytes received \n", rlen);

		// wbkim
		if (rlen >= 12)
		{
			int transaction_id	= get_word_data(&buff_in[0]);		// increased by 1
			int protocol_id		= get_word_data(&buff_in[2]);		// 0x0000 (Fixed)
			int length			= get_word_data(&buff_in[4]);		// data_length after me (0x0006)
			int unit_id			= buff_in[6];						// 0x01 (default)	
			int function_code	= buff_in[7];						// 0x04 (Read input command)
			int ref_number		= get_word_data(&buff_in[8]);		// 0x0000 (Register ?)
			int word_count		= get_word_data(&buff_in[10]);		// 0x???? (multibyte ?)

			int data_len = word_count * 2 + 3;
			int packet_len = data_len + 6;
			
			if (word_count > 0 && word_count < 128)
			{
				buff_out[0] = buff_in[0];
				buff_out[1] = buff_in[1];
				buff_out[2] = buff_in[2];
				buff_out[3] = buff_in[3];

				set_word_data(&buff_out[4], data_len);

				buff_out[6] = unit_id;
				buff_out[7] = function_code;
				buff_out[8] = word_count * 2;

				for (int i=0; i<word_count; i++)
				{
					set_word_data(&buff_out[i*2+9], (0xAA00 + i));
				}

				// Send message 
				printf("<< sending %d bytes \n", packet_len);
				//send_message(buff_out, cli->uid);
				//send_data(buff_out, packet_len, cli->uid);
				write(cli->connfd, buff_out, packet_len);
			}
		}
	}

	/* Close connection */
	close(cli->connfd);
	//sprintf(buff_out, "<<LEAVE, BYE %s\r\n", cli->name);
	//send_message_all(buff_out);

	/* Delete client from queue and yeild thread */
	queue_delete(cli->uid);
	printf("<<LEAVE ");
	print_client_addr(cli->addr);
	printf(" REFERENCED BY %d\n", cli->uid);
	free(cli);
	cli_count--;
	pthread_detach(pthread_self());
	
	return NULL;
}

int main(int argc, char *argv[]){
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	pthread_t tid;

	/* Socket settings */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(SERVER_PORT); 

	/* Ignore pipe signals */
	signal(SIGPIPE, SIG_IGN);
	
	/* Bind */
	if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		perror("Socket binding failed");
		return 1;
	}

	/* Listen */
	if(listen(listenfd, 10) < 0){
		perror("Socket listening failed");
		return 1;
	}

	printf("<[SERVER STARTED]>\n");

	/* Accept clients */
	while(1){
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		/* Check if max clients is reached */
		if((cli_count+1) == MAX_CLIENTS){
			printf("<<MAX CLIENTS REACHED\n");
			printf("<<REJECT ");
			print_client_addr(cli_addr);
			printf("\n");
			close(connfd);
			continue;
		}

		/* Client settings */
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->addr = cli_addr;
		cli->connfd = connfd;
		cli->uid = uid++;
		sprintf(cli->name, "%d", cli->uid);

		/* Add client to the queue and fork thread */
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

		/* Reduce CPU usage */
		sleep(1);
	}
}
