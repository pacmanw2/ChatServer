/* Marco Gallegos
 * Remington Bonawitz
 * Luis Alvarez
 * Eastern Washington University 
 */

/* HOW TO RUN
* ./runName port(optional, default 8888)
*/

/* Notes:
 * Used code from:
 * http://beej.us/guide/bgnet/output/html/multipage/advanced.html#select
 * 
 * */
 
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
 #include <sys/sendfile.h>
 #include <fcntl.h>


#define MAX_BUFF 2000000    //2MB buffer :)
#define CLIENT_LIMIT 30     //30 clients for now


/******** Prototypes ********/

void *get_in_addr(struct sockaddr *sa);
int messageProcessor(int curSocket, int bytesRead, char* input);
int sendMessage(int socket, int dataSize, char* data);
void commands(int curSocket);
void broadcast(int curSocket, int bytesRead, char* input);
void whisper(int curSocket, int bytesRead, char* input);
void name(int curSocket, int bytesRead, char* input);
void list(int curSocket, int bytesRead, char* output);
void switchRoom(int curSocket, int bytesRead, char* input);
void sendFile(int curSocket, int bytesRead, char* input);


/******** Struct for a client ********/

struct client{
    char userName[20];  //username picked by client
    int socket;     //socket client is using
    char ip_addr[20];   //ip address of client
    int connected;  // boolean for connected status
    char room[20];
};

/******** Global Variables ********/

struct client clientList[CLIENT_LIMIT]; //list of clients
int socket_dh;  //socket descriptor
int server_Q[3];    //server queue for clients trying to connect
char *roomNames[] = {"X", "Y", "1", "Waitroom"};   //list of Rooms


/* main() and messageProcessor() */
int fdmax;  //maximum file descriptor number
fd_set master;    // master file descriptor list
int listener;     // listening socket descriptor
char input[MAX_BUFF];    // buffer for client data
char output[MAX_BUFF]; //buffer for outgoing data


/* Stuff for messageUnpacker() and sendMessage() */
char cmd;   //command
char option[20];    //option of message
char sizeArray[8];  //size in char
int size;   //size of message 
char packData[MAX_BUFF]; //array for data
char* inputMessagePointer; //points to a portion of the input buffer

/***************** SEND A MESSAGE TO A CLIENT *************************
 *********************************************************************/

int sendMessage(int socket, int dataSize, char* data)
{
    puts("Send Message");
    printf("\n Message Before send(): %s\n", data);
    
    int i, k, status;
    char stringSize[8]; // =  sprintf(size);??
    
    snprintf(stringSize, sizeof(stringSize), "%d", dataSize);
    
    
    /* build the output array */
    
    output[0] = cmd;
    
    // Option - next 20 bytes 
    for (i = 1, k = 0; i < 21; i++, k++)
    {
        output[i] = option[k];
    }
    option[k - 1] = '\0';
    
    // Size - next 8 bytes 
    for (i = 21, k = 0; i < 28; i++, k++)
    {
        output[i] = stringSize[k];
    }
    
    //copy input array into output
    for(i = 29, k = 0; k < size; i++, k++)
    {
        output[i] = data[k];
    }
    
    //send it to the client
    status = send(socket, output, dataSize + 29, 0);
    
    printf("\nbytes sent to socket %d: %d \n", socket, status);    //debugging purposes
    return status;
}


/***************** MESSAGE UNPACKER *************************
 *********************************************************************/

void messageUnpacker()
{

    int i,k;
    
    // Command - 1st byte 
    cmd = input[0];
    
    // Option - next 20 bytes 
    for (i = 1, k = 0; i < 21; i++, k++)
    {
        option[k] = input[i];
    }
    option[k - 1] = '\0';
    
    // Size - next 8 bytes 
    for (i = 21, k = 0; i < 28; i++, k++)
    {
        sizeArray[k] = input[i];
    }

    sizeArray[k - 1] = '\0';
    size = atoi(sizeArray);
    
    //set packData to be the start of the "message" portion of the array
    inputMessagePointer = &input[29];
    
    printf("cmd: %c | options: %s | sizes: %s | %d\n", cmd, option, sizeArray, size);

    printf("%i\n",size);


}//end messageUnpacker


/***************** ADD A USER TO THE LIST *************************
 *********************************************************************/

int addUser(char *ipaddr, int newSocket)
{
    int i;
    for (i = 0; i < CLIENT_LIMIT; i++)
    {
        // go to the first client in the list that's not connected
        if (!clientList[i].connected)
        {
            strcpy(clientList[i].userName, "NEWGUY");
            clientList[i].socket = newSocket;
            strcpy(clientList[i].ip_addr, ipaddr);// = inet_ntop(remoteaddr.ss_family,
                                //get_in_addr((struct sockaddr*)&remoteaddr),
                               // remoteIP, INET6_ADDRSTRLEN);
            clientList[i].connected = 1;
            return 0;
        }
    }
    
    return 0;
}


int disconnectClient(int discSocket)
{
	int i;
	for (i = 0; i < CLIENT_LIMIT; i++){
		if (clientList[i].socket == discSocket){
			clientList[i].connected = 0;
			return 0;
		}
	}
	
	return 1;
}


/***************** get sockaddr, IPv4 or IPv6: *************************
 *********************************************************************/

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}




/******************************** MAIN *******************************/
/*********************************************************************/

int main(int argc, char *argv[])
{
    char port[6] = "8888";
    
    /* Checks for an argument to be used as a port - uses 8888 by default */
    if (argc == 2)
    {
        strcpy(port, argv[1]);
        printf("Setting port to: %s\n", port);
    }
    
    /*************beej************/
    
    
    fd_set read_fds;  // temp file descriptor list for select()
    fd_set wait_fds; // waiting room file descriptor

    
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, port, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    }
                    else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                        addUser(remoteIP, newfd); //add user to clientList
                    }
                }
                else {
                    // handle data from a client
                    if ((nbytes = recv(i, input, sizeof input, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        }
                        else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                        disconnectClient(i);
                    }
                    else {
                        // we got some data from a client, sending to processor
                        printf("\nFROM CLIENT: %s\n NUMBER OF BYTES: %i\n", input, nbytes);
                        messageProcessor(i, nbytes, input);
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!

}// end main



/******************** Process Data from Clients **********************/
/*********************************************************************/

int messageProcessor(int curSocket, int bytesRead, char* input)
{
    /* Call messageUnpacker() to extract contents of inbound message
    * stuff will be extracted into the global variables */
    messageUnpacker();
    
    cmd = input[0];

    /* Message | File - up to the next 100KB */
    
    // need to figure that one out.  A different array to dump the buffer into?
    // also just putting stuff in the switch statement for now, might
    // have to break it up into more functions
    
    int j;  //index for loops
    
    switch(cmd)
    {
        /* BROADCAST *  goes through clientList, retransmits buffer to everyone who is connected  */
        case 'b':
            puts("BROADCAST MESSAGE RECEIVED");
            broadcast(curSocket, bytesRead, input);
            /*
            for (i = 0; i < CLIENT_LIMIT; i++){
                if (clientList[i].connected){
                    sendMessage(clientList[i].socket, bytesRead, input);
                    puts("------------------\n");
                }
            }
            */
            break;
            
        /* COMMANDS */
        case 'c':
            commands(curSocket);
            break;
            
        /* DISCONNECT */
        case 'd':
            // this command disconnects the client from the server.
            break;
        
        /* TESTING PURPOSES */
        case 't':
            printf("TEST Successful. Option: %s", option);
            break;
        
        /* GLOBAL FILE */
        case 'e':
            // this command sends a file to everyone on the server
            break;
            
        /* USER FILE */
        case 'f':
            sendFile(curSocket, bytesRead, output);
            break;
            
        /* CURRENT ROOM USER LIST */
        case 'h':
            break;
            
        /* GLOBAL USER LIST */
        case 'l':
            list(curSocket, bytesRead, output);
            
            //sendMessage(curSocket, sizeof(output), output);
            //printf("List of users sent. . . \n");
            break;
        
        /* NAME */
        case 'n':
            //name(curSocket, sizeof(output), output);
            /*
            for (i = 0; i < CLIENT_LIMIT; i++){
                if (clientList[i].connected && clientList[i].socket == curSocket){
                    strcpy(clientList[i].userName, option);
                    //whisper user confirming name change?
                    break;
                }
            }*/
            break;
            
        /* ROOM MESSAGE */
        case 'r':
            break;
        
        /* SWITCH ROOMS */
        case 's':
            break;
            
        /* WHISPER */
        case 'w':
            //rebuild outgoing message
            //<w><from><size><blaaaah>
            whisper(curSocket, bytesRead, input);
            /*
            output[0] = cmd;
            //strcat(output, fromUser);
            strcat(output, sizeArray);
            strcat(output, input);
            */
            
            //sendMessage(otherGuysSocket, bytesRead, output)
            break;
        
        
        /* DEFAULT */
        default:
            printf("ERROR: COMMAND NOT RECOGNIZED.");
            break;
    }

    memset(input, '\0', bytesRead);    //zero out the input buffer
    memset(output, '\0', sizeof(output));    //zero out the output buffer that was sent to client
    //memset(client_str, 0, sizeof(client_str) );
    
    return 0;
}
   

void broadcast(int curSocket, int bytesRead, char* input)
{
    //'bytesRead' is basically the end of message (EOM) index
    //'input' is the pointer to the char[] buffer

    printf("cmd: %c | options: %s | sizes: %s | %d\n", cmd, option, sizeArray, size);
    printf("%i\n",bytesRead);
    
    int j;

    //for socket in the FD list
    for(j = 0; j <= fdmax; j++){
        if (FD_ISSET(j, &master)){
            // except the server
            if (j != listener){
                if (sendMessage(clientList[j].socket, size, input) == -1){
                    perror("send");
                }
            }
        }
    }//end for
}// end of broadcast



/*
'c': COMMANDS: ***Server receives a packet [c][][][], and goes to the switch, 
finds c: and then sends a list of commands/functions to the client who sent it: 
[][SERVER][sizeof(commandList)]["a:aDesc,b:bDesc,c:cDesc,e:eDesc,etc"].  
It is then up to the client to parse the command list and display it as desired.

i.e. SERVER: a -> description of command a \n

b -> description of command b \n
*/

void commands(int curSocket)
{
    char list[] = "\n\t\tb: BROADCAST\n\
               c: COMMANDS\n\
               d: DISCONNECT\n\
               e: FILE FOR EVERYONE\n\
               f: FILE FOR ONE PERSON\n\
               g: FILE FOR ROOM\n\
               h: HERE?- List of users in Room\n\
               l: LIST ALL CLIENTS ON SERVER\n\
               n: NAME - check if username is available\n\
               r: ROOM MESSAGE - broadcast to current room\n\
               s: SWITCH ROOMS\n\
               w: WHISPER - private message\n";
    
    //inputMessagePointer = list;
    size = strlen(list);
    
    sendMessage(curSocket, size, list);

}

void sendFile(int curSocket, int bytesRead, char* input)
{
    
}

/*
'n': NAME: ***Server receives packet [n][newName][][], 
then checks to see if the name is already in use by looking 
through the client list -> then if it's not in use, updates the client's name, 
and sends back [][SERVER][sizeof(success)]["name change success!"]
***Client receives this packet back, and displays it however they want.
*/

void name(int curSocket, int bytesRead, char* input)
{
    
}

/*
  'l': LIST ALL CLIENTS EVERYWHERE ON THE SERVER: same as how HERE? works - 
   just send the list of ALL users connected to the server.
*/

void list(int curSocket, int bytesRead, char* input)
{

    int i; 
    for(i = 0; i < CLIENT_LIMIT; i++)
    {
        if(clientList[i].userName != NULL)
            printf("%s\n",clientList[i].userName);
    }   

}


/*
's':SWITCH ROOMS: ***Server receives a packet [s][][][] - and sends back 
[][SERVER][sizeof(roomList)]["room1,room2,room3"], so the client can see valid rooms. 
OR ***Server receives packet 
[s][room1][][] 
and changes the client's current room to "room1" 
IF the room is valid - if valid, send back a success message 
([][SERVER][sizeof(success)]["successful room change to room1"]), 
if invalid, send an error message [][SERVER][sizeof(failure)]["failed to enter room, valid rooms are: room1,room2,room3]
*/

void switchRoom(int curSocket, int bytesRead, char* input)
{

}


/*
"whisper" send a message to a specified user.
Example usage: /w starkiller45 hey man how's it going?
Example packet: [w][starkiller45][sizeof(message)][Message]
*/
//snprintf(size,9,"%d",100000); //the sizeof(the remaining message the user sent)
void whisper(int curSock, int bytesRead, char* input)
{
    //size of message alredy set in global from messageUnpacker()
    
    int i;
    
    //copy the "from" socket userName
    char from[20];
    for (i = 0; i < CLIENT_LIMIT; i++)
    {
        if (clientList[i].socket == curSock)
        {
            strcpy(from, clientList[i].userName);
        }
    }
    
    //cycle through clients, find target socket
    int targetSocket;
    for (i = 0; i < CLIENT_LIMIT; i++)
    {
        if (strcmp(option, clientList[i].userName))
        {
            targetSocket = clientList[i].socket;
            break;
        }
    }
    
    sendMessage(targetSocket, size, input); //relay message to targetSocket
}


