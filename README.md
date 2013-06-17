439Project2
===========
Max Langensiepen

Borrowed Sample Code From:
	-http://cs.ecs.baylor.edu/~donahoo/practical/CSockets2/code/code/TCPEchoServer-Fork.c
	-Also referenced the TCPEchoServer / Client from the Sample Code File
			
//Compile with:			
	//Client must be linked with readline (was very frustrated with fgets)
	gcc -o client client.c DieWithError.c  -lreadline
		
	gcc -o server server.c DieWithError.c

//Start server with
./server 3333, or any port number

//Run any client with three arguments. Port defaults to 3333 if none is specified. 
//The first argument is the recipient ID. 
//The IDs increment starting from 0 on each instance of the server. 
//New messages are retrieved in each client after they finish sending a message (the input blocks retrieval until a message is entered)
//The client sits in a loop asking for new messages, each instance of the client sends to its indicated recipient  
//quit as a message quits the program

./client 1 127.0.0.1 3333

Changes and updates can be tracked at https://github.com/mlangens/439Project2



