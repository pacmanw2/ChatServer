/* Marco Gallegos 
 * Eastern Washington University 
 */

/* HOW TO RUN
* ./runName 
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


int client_socket; 
int socket_dh; //socket descriptor
int ara[3];

//int checkWord(char *msg, char *ePtr);
void *receive_message();
void *send_message();

/*
int checkWord(char *msg, char *ePtr)
{
    int i = 0, key = 0;
    for(i = 0; i < 3; i++){
        if(msg[i] == ePtr[i])
            key++;
    }
    return key;
}
*/

void *receive_message() //void *
{
    char recv_buf[1024];
    while(1){

        //char id[] = "<client>: ";
        int i;
        for(i = 0; i < 2; i++){
            int rec = recv(client_socket[i], recv_buf, 1024, 0);
        }

        //check if connection has been lost
        if(!rec){
            puts("Connection to client was lost!\n");
            puts("Ctrl + C to exit\n");
            close(socket_dh);
            return NULL;
        }

        //strcat(recv_buf, id);
        printf("<client>: %s", recv_buf);
        //recv_buf[0] = '\0';
        memset(recv_buf, 0, sizeof(recv_buf) );
    }
}

void *send_message()
{
    char client_str[1024];
    int i;
    while(1){

        //char id[] = "<server>: ";
        //char tab[] = "\t\t\t";
        //printf("<server> enter message: ");
        fgets(client_str, sizeof(client_str), stdin);
        //strcat(client_str, id);
        //strcat(client_str, tab);
        for(i = 0; i < 3; i++){
            int send_status = send(ara[i], client_str, 1024, 0);    
        }
        
        memset(client_str, 0, sizeof(client_str) );
    }
}

int main(void)
{

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
    server.sin_port = htons( 8888 ); // port 8888 will be used

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
    for(;;){

        // (4) accept connection 
        client_socket = accept(socket_dh, (struct sockaddr *)&client_addr,(socklen_t*)&client_socket); 
        if(client_socket == -1){
            sleep(5);
        }
        else{
            if(i == 2){
                puts("Too many clients!");
                break;
            }
            printf("connection established with client discriptor %d\n", client_socket); 
            ara[i] = client_socket;
            //create Threads
            pthread_create(&client_list[i], NULL, send_message, NULL);
            pthread_create(&client_list[i], NULL, receive_message, NULL);
            i++;
        }

    }

    

    pthread_exit(NULL);

}// end main






