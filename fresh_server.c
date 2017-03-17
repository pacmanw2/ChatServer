/* Marco Gallegos
 * Remington Bonawitz
 * Luis Alvarez
 * Eastern Washington University 
 */

/* HOW TO RUN
* ./runName port(optional)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


//#define *recvPtr
//#define *clientPtr

#define MAX_BUFF 2000000    //2MB buffer :)
#define CLIENT_LIMIT 10     //10 clients for now


/******** Prototypes ********/

void *get_in_addr(struct sockaddr *sa);
int messageProcessor(int bytesRead, char* input);
int sendMessage(int socket, int bytesRead, char* input);


/******** Structs ********/

/* Struct for a client */

struct client{
    char userName[20];  //username picked by client
    int socket;     //socket client is using
    char ip_addr[20];   //ip address of client
    int connected;  // boolean for connected status
};

/* Struct for a room */

struct room{
    char name[20];  //name of Room
    char userList[CLIENT_LIMIT][20];  //an array of userNames in the room
    int maxOccupancy;
};

/******** Global Variables ********/

struct client clientList[CLIENT_LIMIT]; //list of clients
struct room roomList[3];    //list of rooms
int socket_dh;  //socket descriptor
int server_Q[3];    //server queue for clients trying to connect




/***************** SEND A MESSAGE TO A CLIENT *************************
 *********************************************************************/

int sendMessage(int socket, int bytesRead, char* input)
{
    int status;
    status = send(socket, input, bytesRead, 0);
    printf("bytes sent: %d \n", status);    //debugging purposes
    return 0;
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
    
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buf[256];    // buffer for client data
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

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
                    }
                }
                else {
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    }
                    else {
                        // we got some data from a client
                        for(j = 0; j <= fdmax; j++) {
                            // send to everyone!
                            if (FD_ISSET(j, &master)) {
                                // except the listener and ourselves
                                if (j != listener && j != i) {
                                    if (send(j, buf, nbytes, 0) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        }
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!

}// end main



/******************** Process Data from Clients **********************/
/*********************************************************************/

int messageProcessor(int bytesRead, char* input)
{
    //'bytesRead' is basically the end of message (EOM) index
    //'input' is the pointer to the char[] buffer
    
    char cmd;
    char option[25];
    char sizeArray[10];
    int size;
    
    
    /* Command - 1st byte */
    cmd = input[0];
    
    /* Option - next 20 bytes */
    int i, k;
    for (i = 1, k = 0; i < 21; i++, k++)
    {
        option[k] = input[i];
    }
    option[k] = '\0';
    
    /* Size - next 8 bytes */
    for (i = 21, k = 0; i < 28; i++, k++)
    {
        sizeArray[k] = input[i];
    }
    sizeArray[k] = '\0';
    size = atoi(sizeArray);
    
    
    /* Message | File - up to the next 100KB */
    
    // need to figure that one out.  A different array to dump the buffer into?
    // also just putting stuff in the switch statement for now, might
    // have to break it up into more functions
    
    
    switch(cmd)
    {
        /* BROADCAST *  goes through clientList, retransmits buffer to everyone who is connected  */
        case 'b':
            puts("BROADCAST MESSAGE RECEIVED");
            for (i = 0; i < CLIENT_LIMIT; i++)
            {
                if (clientList[i].connected != 0)
                {
                    sendMessage(clientList[i].socket, bytesRead, input);
                }
            }
            
            break;
            
        /* COMMANDS */
        case 'c':
            //no idea how this will work, whos it comming from?!
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
            break;
            
        /* CURRENT ROOM USER LIST */
        case 'h':
            break;
            
        /* GLOBAL USER LIST */
        case 'l':
            break;
        
        /* NAME */
        case 'n':
            break;
            
        /* ROOM MESSAGE */
        case 'r':
            break;
        
        /* SWITCH ROOMS */
        case 's':
            break;
            
        /* WHISPER */
        case 'w':
            break;
        
        
        /* DEFAULT */
        default:
            printf("ERROR: COMMAND NOT RECOGNIZED.");
            break;
    }

    bzero(input, bytesRead);    //zero out the buffer that was used (switch to memset)
    //memset(client_str, 0, sizeof(client_str) );
    
    return 0;
}


