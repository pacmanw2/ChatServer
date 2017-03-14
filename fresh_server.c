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
#define MAX_BUFF 1024


int client_socket[3]; 
int socket_dh; //socket descriptor
int server_Q[3]; //server queue 


void *receive_message();
void *send_message();


void *receive_message() 
{
    char recv_buf[MAX_BUFF];
    int rec;
    while(1){

        //char id[] = "<client>: ";
        int i, bytesRead;
        bytesRead = 0;
        for(i = 0; i < 2; i++){
            rec = recv(client_socket[i], recv_buf, MAX_BUFF, 0);
            //rec = read(client_socket[i], recv_buf - bytesRead, MAX_BUFF - bytesRead);
            printf(">>> %i\n", rec);

            //check if connection has been lost
            if(rec == 0){
                
                puts("Connection to client was lost!\n");
                close(socket_dh);
                return NULL;    
            }
            bytesRead += rec;
        }

        

        //strcat(recv_buf, id);
        printf("<client>: %s", recv_buf);
        //recv_buf[0] = '\0';
        memset(recv_buf, 0, sizeof(recv_buf) );
    }
}

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
    do{

        // (4) accept connection 
        client_socket[i] = accept(socket_dh, (struct sockaddr *)&client_addr,(socklen_t*)&client_socket[i]); 
        if(client_socket[i] == -1){
            sleep(5);
        }
        else{
            if(i == 2){
                puts("Too many clients!");
                break;
            }
            else{
                printf("connection established with client discriptor %d\n", client_socket[i]); 
                server_Q[i] = client_socket[i];
                //create Threads
                pthread_create(&client_list[i], NULL, send_message, NULL);
                pthread_create(&client_list[i], NULL, receive_message, NULL);
                i++;    
            }
        }
    }while(i < 2);

    

    pthread_exit(NULL);

}// end main






