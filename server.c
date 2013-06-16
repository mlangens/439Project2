#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <sys/types.h>
#include <netdb.h>
#include <sys/mman.h>
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */

#include "msg.h"

#define MAXPENDING 5    /* Maximum outstanding connection requests */

void DieWithError(char *errorMessage); /* Error handling function */
void HandleTCPClient(int clntSocket, int clientId, struct queue *q); /* TCP client handling function */

ssize_t getClientMessage(int clntSocket, ClientMessage *data) {
	ssize_t numBytesRcvd;
	numBytesRcvd = recv(clntSocket, data, sizeof(ClientMessage), 0);
	data->request_Type = ntohl(data->request_Type);
	data->SenderId = ntohl(data->SenderId);
	data->RecipientId = ntohl(data->RecipientId);
	return numBytesRcvd;
}

ssize_t sendServerMessage(int clntSocket, ServerMessage *data) {
	ssize_t numBytesSent;
	data->messageType = htonl(data->messageType);
	data->SenderId = htonl(data->SenderId);
	data->RecipientId = htonl(data->RecipientId);
	numBytesSent = send(clntSocket, data, sizeof(ServerMessage), 0);
	return numBytesSent;
}

int SetupTCPServerSocket(const char *service) {
	// Construct the server address structure
	struct addrinfo addrCriteria;                  // Criteria for address match
	struct addrinfo *addr;
	memset(&addrCriteria, 0, sizeof(addrCriteria));
	// Zero out structure
	addrCriteria.ai_family = AF_UNSPEC;             // Any address family
	addrCriteria.ai_flags = AI_PASSIVE;            // Accept on any address/port
	addrCriteria.ai_socktype = SOCK_STREAM;         // Only stream sockets
	addrCriteria.ai_protocol = IPPROTO_TCP;         // Only TCP protocol

	struct addrinfo *servAddr; // List of server addresses
	int rtnVal = getaddrinfo(NULL, service, &addrCriteria, &servAddr);
	if (rtnVal != 0)
		DieWithError("getaddrinfo() failed");

	int servSock = -1;
	for (addr = servAddr; addr != NULL ; addr = addr->ai_next) {
		// Create a TCP socket
		servSock = socket(addr->ai_family, addr->ai_socktype,
				addr->ai_protocol);
		if (servSock < 0)
			continue;       // Socket creation failed; try next address

		// Bind to the local address and set socket to listen
		if ((bind(servSock, addr->ai_addr, addr->ai_addrlen) == 0)
				&& (listen(servSock, MAXPENDING) == 0)) {
			// Print local address of socket
			struct sockaddr_storage localAddr;
			socklen_t addrSize = sizeof(localAddr);
			if (getsockname(servSock, (struct sockaddr *) &localAddr, &addrSize)
					< 0)
				DieWithError("getsockname() failed");
			fputs("Binding to ", stdout);
			//PrintSocketAddress((struct sockaddr *) &localAddr, stdout);
			fputc('\n', stdout);
			break;       // Bind and listen successful
		}

		close(servSock);  // Close and try again
		servSock = -1;
	}

	// Free address list allocated by getaddrinfo()
	freeaddrinfo(servAddr);

	return servSock;
}

int AcceptTCPConnection(int servSock) {
	struct sockaddr_storage clntAddr; // Client address
	// Set length of client address structure (in-out parameter)
	socklen_t clntAddrLen = sizeof(clntAddr);

	// Wait for a client to connect
	int clntSock = accept(servSock, (struct sockaddr *) &clntAddr,
			&clntAddrLen);
	if (clntSock < 0)
		DieWithError("accept() failed");

	// clntSock is connected to a client!

	fputs("Handling client ", stdout);
	//PrintSocketAddress((struct sockaddr *) &clntAddr, stdout);
	fputc('\n', stdout);

	return clntSock;
}

void HandleTCPClient(int clntSocket, int clientId, struct queue *q) {
	ClientMessage incomingMessage;
	struct queue *queue;
	ServerMessage outgoingMessage;
	int i;
	ssize_t numBytesRcvd;
	// Receive message from client
	// Initial handshake msg
	outgoingMessage.messageType = Server_Handshake;
	outgoingMessage.SenderId = clientId;
	outgoingMessage.RecipientId = 0xDEADBEEF;
	sendServerMessage(clntSocket, &outgoingMessage);
	printf("sent handshake\n");
	//ack from client
	getClientMessage(clntSocket, &incomingMessage);
	if (incomingMessage.request_Type != Client_Handshake
			|| incomingMessage.RecipientId != clientId
			|| incomingMessage.SenderId != 0xDEADBEEF)
		DieWithError("handshake failed");
	printf("handshake completed for client %d\n", clientId);
	for (;;) {
		numBytesRcvd = getClientMessage(clntSocket, &incomingMessage);
		if (numBytesRcvd < 0)
			DieWithError("recv() failed");
		if (incomingMessage.request_Type == Send) {
			queue = &q[incomingMessage.RecipientId];
			printf("Storing %d (%s)\n", incomingMessage.RecipientId,
					incomingMessage.message);
			strncpy(queue->m[queue->elements].msg, incomingMessage.message,
					100);
			queue->m[queue->elements].type = New;
			//wrap around queue
			queue->elements = (queue->elements + 1) % 5;
		} else {
			queue = &q[incomingMessage.SenderId];
			for (i = 0; i < queue->elements; ++i) {
				outgoingMessage.messageType = queue->m[i].type;
				outgoingMessage.RecipientId = incomingMessage.SenderId;
				outgoingMessage.SenderId = incomingMessage.RecipientId;
				strncpy(outgoingMessage.message, queue->m[i].msg, 100);
				sendServerMessage(clntSocket, &outgoingMessage);
			}
			outgoingMessage.messageType = No_Message;
			outgoingMessage.RecipientId = incomingMessage.SenderId;
			outgoingMessage.SenderId = incomingMessage.RecipientId;
			sendServerMessage(clntSocket, &outgoingMessage);
			queue->elements = 0;
		}
	}
	close(clntSocket); // Close client socket
}

int main(int argc, char *argv[]) {
	struct queue *q;
	if (argc != 2) // Test for correct number of arguments
		DieWithError("Parameter(s)" "<Server Port/Service>");

	char *service = argv[1]; // First arg:  local port/service
	int servSock = SetupTCPServerSocket(service);
	if (servSock < 0)
		DieWithError("SetupTCPServerSocket() failed");
	q = mmap(NULL, sizeof(struct queue) * 32, PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANON, -1, 0);
	unsigned int childProcCount = 0; // Number of child processes
	for (;;) { // Run forever
		// New connection creates a client socket
		int clntSock = AcceptTCPConnection(servSock);
		// Fork child process and report any errors
		pid_t processID = fork();
		if (processID < 0)
			DieWithError("fork() failed");
		else if (processID == 0) { // If this is the child process
			close(servSock);         // Child closes parent socket
			HandleTCPClient(clntSock, childProcCount, q);
			exit(0);                 // Child process terminates
		}

		printf("with child process: %d\n", processID);
		close(clntSock);  // Parent closes child socket descriptor
		childProcCount++; // Increment number of child processes

		while (childProcCount) { // Clean up all zombies
			processID = waitpid((pid_t) -1, NULL, WNOHANG); // Non-blocking wait
			if (processID < 0) // waitpid() error?
				DieWithError("waitpid() failed");
			else if (processID == 0) // No zombie to wait on
				break;
			else
				childProcCount--; // Cleaned up after a child
		}
	}
}
