#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <readline/readline.h>
#include <readline/history.h>

#include "msg.h"

#define RCVBUFSIZE 32   /* Size of receive buffer */

void DieWithError(char *errorMessage); /* Error handling function */

ssize_t getServerMessage(int clntSocket, ServerMessage *data) {
	ssize_t numBytesRcvd;
	numBytesRcvd = recv(clntSocket, data, sizeof(ServerMessage), 0);
	data->messageType = ntohl(data->messageType);
	data->SenderId = ntohl(data->SenderId);
	data->RecipientId = ntohl(data->RecipientId);
	return numBytesRcvd;
}

ssize_t sendClientMessage(int clntSocket, ClientMessage *data) {
	ssize_t numBytesSent;
	data->request_Type = htonl(data->request_Type);
	data->SenderId = htonl(data->SenderId);
	data->RecipientId = htonl(data->RecipientId);
	numBytesSent = send(clntSocket, data, sizeof(ClientMessage), 0);
	return numBytesSent;
}

int do_retreive_messages() {
	return 0;
}

int main(int argc, char *argv[]) {
	int sock; /* Socket descriptor */
	int recipientId;
	char ch;
	char* buffer = NULL;
	struct sockaddr_in chatServAddr; /* Echo server address */
	unsigned short chatServPort; /* Echo server port */
	char *servIP; /* Server IP address (dotted quad) */
	struct timeval timeout;
	fd_set readfds;
	uint32_t clientId;
	ClientMessage outgoingMessage;
	ServerMessage incomingMessage;
	int bytesRcvd, totalBytesRcvd; /* Bytes read in single recv()
	 and total bytes read */

	if ((argc < 3) || (argc > 4)) { /* Test for correct number of arguments */
		fprintf(stderr, "Usage: %s <User ID> <Server IP> [<Echo Port>]\n",
				argv[0]);
		exit(1);
	}

	servIP = argv[2]; /* First arg: server IP address (dotted quad) */
	recipientId = atoi(argv[1]);

	if (argc == 4)
		chatServPort = atoi(argv[3]); /* Use given port, if any */
	else
		chatServPort = 3333;

	/* Create a reliable, stream socket using TCP */
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		DieWithError("socket() failed");

	/* Construct the server address structure */
	memset(&chatServAddr, 0, sizeof(chatServAddr));
	/* Zero out structure */
	chatServAddr.sin_family = AF_INET; /* Internet address family */
	chatServAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
	chatServAddr.sin_port = htons(chatServPort); /* Server port */

	/* Establish the connection to the echo server */
	if (connect(sock, (struct sockaddr *) &chatServAddr, sizeof(chatServAddr))
			< 0)
		DieWithError("connect() failed");
	getServerMessage(sock, &incomingMessage);
	printf("got handshake %d (%d) %x\n", incomingMessage.messageType,
			Server_Handshake, incomingMessage.RecipientId);
	if (incomingMessage.messageType != Server_Handshake
			|| incomingMessage.RecipientId != 0xDEADBEEF)
		DieWithError("bad handshake");
	outgoingMessage.RecipientId = clientId = incomingMessage.SenderId;
	outgoingMessage.request_Type = Client_Handshake;
	outgoingMessage.SenderId = 0xDEADBEEF;
	sendClientMessage(sock, &outgoingMessage);
	printf("my Id is %d\n", clientId);

	//rl_set_keyboard_input_timeout(10000);
	//rl_event_hook = do_retreive_messages;
	for (;;) {
		free(buffer);
		buffer = readline("Type your message: ");
		if (buffer == NULL || *buffer == '\0')
			continue;
		if (strcmp(buffer, "quit") == 0)
			break;
		add_history(buffer);
		outgoingMessage.RecipientId = recipientId;
		outgoingMessage.SenderId = clientId;
		outgoingMessage.request_Type = Send;
		strncpy(outgoingMessage.message, buffer, 100);
		sendClientMessage(sock, &outgoingMessage);

		outgoingMessage.RecipientId = recipientId;
		outgoingMessage.SenderId = clientId;
		outgoingMessage.request_Type = Retrieve;
		sendClientMessage(sock, &outgoingMessage);

		do {
			getServerMessage(sock, &incomingMessage);
			if (incomingMessage.messageType != No_Message) {
				printf("%d: %s\n", recipientId, incomingMessage.message);
			}
		} while (incomingMessage.messageType != No_Message);
	}
	free(buffer);
	close(sock);
	exit(0);
}
