#ifndef _MSG_H
#define _MSG_H

#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <stdint.h>

void DieWithError(char *errorMessage); /* Error handling function */
void HandleTCPClient(int clntSocket, struct queue *q); /* TCP client handling function */
int CreateTCPServerSocket(unsigned short port); /* Create TCP server socket */
int AcceptTCPConnection(int servSock); /* Accept TCP connection request */

struct message {
	char msg[100];
	uint32_t type;
};

struct queue {
	int elements;
	struct message m[5];
};

typedef struct {
	enum {
		Send, Retrieve
	} request_Type; /* same size as an unsigned int */
	unsigned int SenderId; /* unique client identifier */
	unsigned int RecipientId; /* unique client identifier */
	char message[100];
} ClientMessage; /* an unsigned int is 32 bits = 4 bytes */

typedef struct {
	enum {
		New, Old, No_Message
	} messageType; /* same size as an unsigned int */
	unsigned int SenderId; /* unique client identifier */
	unsigned int RecipientId; /* unique client identifier */
	char message[100];
} ServerMessage; /* an unsigned int is 32 bits = 4 bytes */
#endif //_MSG_H
