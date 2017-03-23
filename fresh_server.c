/* Marco Gallegos
 * Remington Bonawitz
 * Luis Alvarez
 * Eastern Washington University 
 */

/* HOW TO RUN
* ./runName port(OPTIONal, default 8888)
*/

/* Notes:
 * Used code from:
 * http://beej.us/guide/bgnet/OUTPUT/html/multipage/advanced.html#select
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
int messageProcessor(int curSocket, int bytesRead, char* INPUT);
void commands(int curSocket);
void broadcast(int curSocket);
void whisper(int curSocket);
void name(int curSocket);
void list(int curSocket, char type);
void switchRoom(int curSocket);
void roomMessage(int socket);
void sendFile(int curSocket, char flag);


/******** Struct for a client ********/

struct client{
    char userName[20];  //username picked by client
    int socket;     //socket client is using
    char ip_addr[20];   //ip address of client
    int connected;  // boolean for connected status
    char room[20];
};

/******** Global Variables ********/

struct client CLIENTLIST[CLIENT_LIMIT]; //list of clients
int socket_dh;  //socket descriptor
int server_Q[3];    //server queue for clients trying to connect
char *ROOMNAMES[] = {"X", "Y", "1", "Waitroom"};   //list of Rooms


/* main() and messageProcessor() */
int fdmax;  //maximum file descriptor number
fd_set master;    // master file descriptor list
int listener;     // listening socket descriptor
char INPUT[MAX_BUFF];    // buffer for client data
char OUTPUT[MAX_BUFF]; //buffer for outgoing data


/* Stuff for messageUnpacker() and sendMessage() */
char CMD;   //command
char OPTION[20];    //OPTION of message
char SIZEARRAY[8];  //SIZE in char
int SIZE;   //SIZE of message 
char PACKDATA[MAX_BUFF]; //array for data
char* INPUTMESSAGEPOINTER; //points to a portion of the INPUT buffer

/***************** SEND A MESSAGE TO A CLIENT *************************
 *********************************************************************/

int sendMessage(int socket, int dataSIZE, char* data)
{
    puts("Send Message");
    
    int i, k, status;
    char stringSIZE[8];
    memset(stringSIZE, 0, sizeof stringSIZE);
    snprintf(stringSIZE, sizeof(stringSIZE), "%d", dataSIZE);
    
    
    /* build the OUTPUT array */
    
    OUTPUT[0] = CMD;
    
    // OPTION - next 20 bytes 
    for (i = 1, k = 0; i < 21; i++, k++)
    {
        OUTPUT[i] = OPTION[k];
    }
    //OPTION[k - 1] = '\0';
    
    // SIZE - next 8 bytes 
    for (i = 21, k = 0; i < 28; i++, k++)
    {
        OUTPUT[i] = stringSIZE[k];
    }
    
    //copy INPUT array into OUTPUT
    for(i = 29, k = 0; k < dataSIZE; i++, k++)
    {
        OUTPUT[i] = data[k];
    }
    
    //send it to the client
    status = send(socket, OUTPUT, dataSIZE + 29, 0);
    
    printf("Message Before send(): %s\n", OUTPUT);
    printf("\nbytes sent to socket %d: %d \n", socket, status);    //debugging purposes
    return status;
}


/***************** MESSAGE UNPACKER *************************
 *********************************************************************/

void messageUnpacker()
{

    int i,k;
    
    // Command - 1st byte 
    CMD = INPUT[0];
    
    // OPTION - next 20 bytes 
    for (i = 1, k = 0; i < 21; i++, k++)
    {
        OPTION[k] = INPUT[i];
    }
    OPTION[k - 1] = '\0';
    
    // SIZE - next 8 bytes 
    for (i = 21, k = 0; i < 28; i++, k++)
    {
        SIZEARRAY[k] = INPUT[i];
    }

    SIZEARRAY[k - 1] = '\0';
    SIZE = atoi(SIZEARRAY);
    
    //set INPUTMESSAGEPOINTER to be the start of the "message" portion of the array
    INPUTMESSAGEPOINTER = &INPUT[29];
    
    printf("Messege unpacked:\nCMD: %c | OPTIONs: %s | SIZEs: %s | %d\n", CMD, OPTION, SIZEARRAY, SIZE);


}//end messageUnpacker


/***************** ADD A USER TO THE LIST *************************
 *********************************************************************/

int addUser(char *ipaddr, int newSocket)
{
    int i;
    for (i = 0; i < CLIENT_LIMIT; i++)
    {
        // go to the first client in the list that's not connected
        if (!CLIENTLIST[i].connected)
        {
            strcpy(CLIENTLIST[i].userName, "NEWGUY\n");
            CLIENTLIST[i].socket = newSocket;
            strcpy(CLIENTLIST[i].ip_addr, ipaddr);// = inet_ntop(remoteaddr.ss_family,
                                //get_in_addr((struct sockaddr*)&remoteaddr),
                               // remoteIP, INET6_ADDRSTRLEN);
            CLIENTLIST[i].connected = 1;
            strcpy(CLIENTLIST[i].room, "Waitroom");
            return 0;
        }
    }
    
    return 0;
}

/***************** DISCONNECT A USER *************************
 *********************************************************************/
 
int disconnectClient(int discSocket)
{
    FD_CLR(discSocket, &master); // remove from master set
    
	int i;
	for (i = 0; i < CLIENT_LIMIT; i++){
		if (CLIENTLIST[i].socket == discSocket){
			CLIENTLIST[i].connected = 0;
			return 0;
		}
	}
    close(discSocket);
	
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
                        addUser(remoteIP, newfd); //add user to CLIENTLIST
                    }
                }
                else {
                    // handle data from a client
                    if ((nbytes = recv(i, INPUT, sizeof INPUT, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        }
                        else {
                            perror("recv");
                        }
                        disconnectClient(i); // bye!
                    }
                    else {
                        // we got some data from a client, sending to processor
                        printf("\nFROM CLIENT: %s\n NUMBER OF BYTES: %i\n", INPUT, nbytes);
                        messageProcessor(i, nbytes, INPUT);
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!

}// end main



/******************** Process Data from Clients **********************/
/*********************************************************************/

int messageProcessor(int curSocket, int bytesRead, char* INPUT)
{
    /* Call messageUnpacker() to extract contents of inbound message
    * stuff will be extracted into the global variables */
    messageUnpacker();

    /* Message | File - up to the next 100KB */
    
    switch(CMD)
    {
        /* BROADCAST *  goes through CLIENTLIST, retransmits buffer to everyone who is connected  */
        case 'b':
            puts("BROADCAST MESSAGE RECEIVED");
            broadcast(curSocket);
            break;
            
        /* COMMANDS */
        case 'c':
            commands(curSocket);
            break;
            
        /* DISCONNECT */
        case 'd':
            disconnectClient(curSocket);
            break;
        
        /* TESTING PURPOSES */
        case 't':
            printf("TEST Successful. OPTION: %s", OPTION);
            break;
        
        /* GLOBAL FILE */
        case 'e':
            sendFile(curSocket, 'e');
            break;
            
        /* USER FILE */
        case 'f':
            sendFile(curSocket, 'f');
            break;
            
        /* ROOM FILE */
        case 'g':
            sendFile(curSocket, 'g');
            break;
            
        /* CURRENT ROOM USER LIST */
        case 'h':
            list(curSocket, 'r');
            break;
            
        /* GLOBAL USER LIST */
        case 'l':
            list(curSocket, 'a');
            break;
        
        /* NAME */
        case 'n':
            name(curSocket);
            break;
            
        /* ROOM MESSAGE */
        case 'r':
            roomMessage(curSocket);
            break;
        
        /* SWITCH ROOMS */
        case 's':
            switchRoom(curSocket);
            break;
            
        /* WHISPER */
        case 'w':
            whisper(curSocket);
            break;
        
        /* DEFAULT */
        default:
            printf("ERROR: COMMAND NOT RECOGNIZED.");
            break;
    }//end switch

    memset(INPUT, '\0', sizeof(INPUT));    //zero out the INPUT buffer
    memset(OUTPUT, '\0', sizeof(OUTPUT));    //zero out the OUTPUT buffer that was sent to client
    memset(SIZEARRAY, '\0', sizeof(SIZEARRAY));
    memset(OPTION, '\0', sizeof(OPTION));
    memset(PACKDATA, '\0', sizeof(PACKDATA));
    SIZE = 0;
    //memset(client_str, 0, sizeof(client_str) );
    
    return 0;
}
   

void broadcast(int curSocket)
{
    //'bytesRead' is basically the end of message (EOM) index
    //'INPUT' is the pointer to the char[] buffer

    printf("CMD: %c | OPTIONs: %s | SIZEs: %s | %d\n", CMD, OPTION, SIZEARRAY, SIZE);
    
    int j;

    //for socket in the FD list
    for(j = 0; j <= fdmax; j++){
        if (FD_ISSET(j, &master)){
            // except the server
            if (j != listener){
                if (sendMessage(j, SIZE, INPUTMESSAGEPOINTER) == -1){
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
    
    //INPUTMESSAGEPOINTER = list;
    SIZE = strlen(list);
    
    sendMessage(curSocket, SIZE, list);

}


/* SEND A FILE **********
 * So, this just sets the outgoing message CMD to 'f' and then
 * uses whisper(), broadcast(), or roomMessage to send the file.
 * */
 
void sendFile(int curSocket, char flag)
{
    CMD = 'f';
    
    switch(flag)
    {
        case 'e':  //file for everyone
            broadcast(curSocket);
            break;
        case 'f':   //file for one person
            whisper(curSocket);
            break;
        case 'g':    //file for room
            roomMessage(curSocket);
            break;
        default:
            printf("file message improperly formatted");
    }
}

/*
'n': NAME: ***Server receives packet [n][newName][][], 
then checks to see if the name is already in use by looking 
through the client list -> then if it's not in use, updates the client's name, 
and sends back [][SERVER][sizeof(success)]["name change success!"]
***Client receives this packet back, and displays it however they want.
*/

void name(int curSocket)
{
    //make the string for a possible success
    char namechange[] = "Name change success";
    CMD = 'w';
    int i;
    
    //go through whole client list, find socket == socket
    for (i = 0; i < CLIENT_LIMIT; i++){
        if (CLIENTLIST[i].connected && CLIENTLIST[i].socket == curSocket){
            strcpy(CLIENTLIST[i].userName, OPTION);
            //whisper user confirming name change
            sendMessage(curSocket, strlen(namechange), namechange);
            break;
        }
    }
}

/*
  'l' and 'h': LIST ALL CLIENTS EVERYWHERE ON THE SERVER
  * AND also handles room list (here? command)
   just send the list of ALL users connected to the server.
   * Takes a socket() and the type, if type is 'r' it is a room list
*/

void list(int curSocket, char type)
{
    char holder[20 * CLIENT_LIMIT]; //holds list of users (built by list())
    char comma[] = ", ";
    
    int i; 
    for(i = 0; i < CLIENT_LIMIT; i++)
    {
        if(CLIENTLIST[i].connected != 0)    //if connected
        {
            if (type == 'r' )   //room list
            {
                if (strcmp(CLIENTLIST[i].room, OPTION) )
                {
                    strncat(holder, CLIENTLIST[i].userName, 20);
                    strncat(holder, comma, 2);
                }
            }
            else //add everyone
            {
                strncat(holder, CLIENTLIST[i].userName, 20);
                strcat(holder, comma);
            }
        }
    }   
    
    sendMessage(curSocket, strlen(holder), holder);
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

void switchRoom(int curSocket)
{
    int i;
    char flag = 'f';
    //check if the room name is valid
    for (i = 0; i < 4; i++)
    {
        if (strcmp(ROOMNAMES[i], OPTION))   //if OPTION == roomName from the list
        {
            //flag = true
            flag = 't';
        }
    }
    
    //if room not found in list, tell user, then ditch out
    if (flag == 'f')
    {
        char message[] = "Invalid Room";
        sendMessage(curSocket, strlen(message), message);
        return;
    }
    
    //change the user's room field in their struct ("move" into the room)
    for (i = 0; i < CLIENT_LIMIT; i++)
    {
        if (CLIENTLIST[i].socket == curSocket)
        {
            strcpy(CLIENTLIST[i].room, OPTION);
        }
    }
}


/* ROOM MESSAGE
 * 
 * Sends a message to a room
 * 
 */

void roomMessage(int socket)
{
    int i;

    for (i = 0; i < CLIENT_LIMIT; i++)  //go through the CLIENTLIST
    {
        if (strcmp(CLIENTLIST[i].room, OPTION)) //if the room is equal to room in OPTION
        {
            sendMessage(CLIENTLIST[i].socket, SIZE, INPUTMESSAGEPOINTER);   //mail the bastard
        }
    }
}



/*
"whisper" send a message to a specified user.
Example usage: /w starkiller45 hey man how's it going?
Example packet: [w][starkiller45][sizeof(message)][Message]
*/
//snprintf(SIZE,9,"%d",100000); //the sizeof(the remaining message the user sent)
void whisper(int curSock)
{
    //SIZE of message alredy set in global from messageUnpacker()
    
    int i;
    
    //copy the "from" socket userName
    char from[20];
    for (i = 0; i < CLIENT_LIMIT; i++)
    {
        if (CLIENTLIST[i].socket == curSock)
        {
            strcpy(from, CLIENTLIST[i].userName);
        }
    }
    
    //cycle through clients, find target socket
    int targetSocket;
    for (i = 0; i < CLIENT_LIMIT; i++)
    {
        if (strcmp(OPTION, CLIENTLIST[i].userName))
        {
            targetSocket = CLIENTLIST[i].socket;
            strcpy(OPTION, from);
            break;
        }
    }
    
    sendMessage(targetSocket, SIZE, INPUTMESSAGEPOINTER); //relay message to targetSocket
}


