/* Marco Gallegos 
 * Eastern Washington University 
 */

/* HOW TO RUN
* ./runName PORT(optional)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

//#define *recvPtr
//#define *clientPtr

#define MAX_BUFF 2000000    //2MB buffer :)


/* Prototypes */

void *receive_message();
void *send_message();
int messageProcessor(int bytesRead, char* input);


/* Structs */

/* Struct for a client */

struct client{
    char userName[20];
    int socket;
    char ip_addr[20];
};

/* Struct for a room */

struct room{
    char name[20];  //name of Room
    char userList[20][20];  //an array of userNames in the room
};

/* Global Variables */

struct client clientList[3]; //list of clients
struct room roomList[3];    //list of rooms
int socket_dh;  //socket descriptor
int server_Q[3];    //server queue for clients trying to connect


/******************************** RECEIVE ****************************/
/*********************************************************************/

void *receive_message() 
{
    char recv_buf[MAX_BUFF];
    int rec;
    while(1){

        //char id[] = "<client>: ";
        int i, bytesRead;
        bytesRead = 0;
        for(i = 0; i < 2; i++){
            rec = recv(clientList[i].socket, recv_buf, MAX_BUFF, 0);
            //rec = read(clientList[i].socket, recv_buf - bytesRead, MAX_BUFF - bytesRead);
            printf(">>> %i\n", rec);
            
            //check if connection has been lost
            if(rec == 0){
                
                puts("Connection to client was lost!\n");
                close(socket_dh);
                return NULL;    
            }
            
            bytesRead += rec;
        }

        
        /* THere should be a # bytesRead checker here to make sure its over 30 or something */ 
        
        messageProcessor(bytesRead, recv_buf);
        
        
        
        
        /* This is the old receive message block.  Keeping it just in case *
        //strcat(recv_buf, id);
        printf("<client>: %s", recv_buf);
        //recv_buf[0] = '\0';
        memset(recv_buf, 0, sizeof(recv_buf) );
        * */
    }
}


/******************************** SEND *******************************/
/*********************************************************************/
/* might not even need a separate SEND thread */

void *send_message()
{
    char client_str[MAX_BUFF];
    int i, bytesRead;
    bytesRead = 0;
    while(1){

        //char id[] = "<server>: ";
        //char tab[] = "\t\t\t";
        //printf("<server> enter message: ");
        fgets(client_str, sizeof(client_str), stdin);
        //strcat(client_str, id);
        //strcat(client_str, tab);
        for(i = 0; i < 3; i++){
            int send_status = send(server_Q[i], client_str - bytesRead, MAX_BUFF - bytesRead, 0);    
        }
        
        memset(client_str, 0, sizeof(client_str) );
    }
}


/******************************** MAIN *******************************/
/*********************************************************************/

int main(int argc, char *argv[])
{
    int port = 8888;
    
    /* Checks for an argument to be used as a port - uses 8888 by default */
    if (argc == 2)
    {
        int port = atoi(argv[1]);
        printf("Setting port to: %d\n", port);
    }
    
    
    //int socket_dh; // the socket discriptor
    struct sockaddr_in server; // struct of type sockaddr_in 

    //create threads
    pthread_t thread_send, thread_recv;
    pthread_t client_list[3];

    // (1) create socket
    socket_dh = socket(AF_INET, SOCK_STREAM, 0);
    //printf("Sokcet descriptor is %d\n", socket_dh);	

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET; // using inet type of family address

    /* INADDR_ANY means that we will use any ip address local to this computer for example
     * localhost, loopback or 127.0.0.1. (0.0.0.0) == anyadress */
    server.sin_addr.s_addr = INADDR_ANY; 
    server.sin_port = htons(port);

    // (2) bind
    int stat;
    stat = bind(socket_dh, (struct sockaddr *)&server, sizeof(server));
    printf("Bind Status %d\n", stat);
    if(stat < 0){
        printf("bad bind status!\n");
        exit(1);
    }
    
    // (3) listen for connections
    listen(socket_dh, 2);

    puts("Wating for client. . .");
    struct sockaddr_in client_addr;

    int i = 0;
    do{

        // (4) accept connection 
        clientList[i].socket = accept(socket_dh, (struct sockaddr *)&client_addr,(socklen_t*)&(clientList[i].socket)); 
        if(clientList[i].socket == -1){
            sleep(5);
        }
        else{
            if(i == 2){
                puts("Too many clients!");
                break;
            }
            else{
                printf("connection established with client discriptor %d\n", clientList[i].socket); 
                server_Q[i] = clientList[i].socket;
                //create Threads
                pthread_create(&client_list[i], NULL, send_message, NULL);
                pthread_create(&client_list[i], NULL, receive_message, NULL);
                i++;  
                
                /* Also we should populate the rest of the client struct in the clientlist,
                 * with stuff like IP address and a random username maybe. */
                 
                 /* Maybe shove them into a default chat room as well */
                  
            }
        }
    }while(i < 2);

    

    pthread_exit(NULL);

}// end main



/******************** Process Data from Clients ***********************/

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
    
    //need to figure that one out.  A different array to dump the buffer into?
    
    switch(cmd)
    {
        case 't':
            printf("TEST Successful. Option: %s", option);
            break;
        
        
        default:
            printf("ERROR: COMMAND NOT RECOGNIZED.");
            break;
    }

    
    
    return 0;
}



