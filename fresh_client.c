/* Marco Gallegos 
 * Eastern Washington University 
 */

/* HOW TO RUN
* ./runName local_IP 8888
* use port 8888
*/


#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ctype.h>
#include <netinet/in.h>
#include <pthread.h>

typedef int bool;
#define true 1
#define false 0

void *receive_message();
void *send_message();

int sock, sk;
int list[3];

//inspired by StackOverflow
void *receive_message() // void *receive_message
{
    char recv_buff[1024];
    while(1){

        //int recv_status = recv(sock, recv_buff, 2000, 0);
        int rec = recv(sock, recv_buff, 2000, 0);

        //quant = read (sockfd, inbuffer, sizeof(inbuffer));
        //printf ("Received: %*.*s\n", quant, quant, inbuffer);

        //check if connection has been lost
        if(!rec){
            puts("Connection to server was lost!\n");
            puts("Ctrl + C to exit\n");
            close(sk);
            return NULL;
        }

        printf("<server>: %s", recv_buff);
        //recv_buff[0] = '\0';
        memset(recv_buff, 0, sizeof(recv_buff) );
    }
}

//inspired by StackOverflow
void *send_message()// *send
{
    char message[1024];
    while(1){

        fgets(message, sizeof(message), stdin);
        int send_status = send(sock, message, 2000, 0);
        memset(message, 0, sizeof(message) );
    }
}

int main(int argc , char *argv[])
{
    /* get the ip and port from argv */
    char *server_ip = argv[1]; 
    int port = atoi(argv[2]);
    //printf("argc %d\n", argc);
    //int sock;
    struct sockaddr_in server;
    //char message[1000] , server_reply[2000];
    char end[] = "bye";

    //create threads
    pthread_t thread_send, thread_recv;

    // (1) Create socket
    //int sk;
    sk = socket(AF_INET , SOCK_STREAM , 0); //crash 1-22-17
    sock = sk;

    if (sk == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
    server.sin_addr.s_addr = inet_addr(server_ip);
    server.sin_family = AF_INET;
    server.sin_port = htons( port );

    // (2) Connect to remote server
    if (connect(sk , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        exit(1);
    }
    puts("connections success");

    pthread_create(&thread_send, NULL, send_message, NULL);
    pthread_create(&thread_recv, NULL, receive_message, NULL);

    while(1);	
 
    printf("Goodbye. . .\n");
    close(sk); 
    pthread_exit(NULL);
}
