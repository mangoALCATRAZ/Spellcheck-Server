#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <netdb.h>

#define PORT "3490" // port number

#define BACKLOG 10 // pending connection no.
// DELCARATIONS//
struct node{
    int * socket;
    struct node * next;
}; typedef struct node node;

struct queue{
    node * Front;
    node * Rear;
    int count;
}; typedef struct queue queue;

int new_fd; // new connection, shared among threads

// FUNCTION DECLARATIONS

void QueInit(queue * in);
void enQueue(queue * in, int * socketData);
int * Dequeue(queue * in);
int * socketFactory(int testValue);




int main()
{
    int sockfd; // listening socket
    struct addrinfo hints, *servinfo, *p;

    struct sockaddr_storage out_addr;
    socklen_t sin_size;
    struct sigaction sa;


    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int addrInfoRes;

    if((addrInfoRes = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0){
        fprintf(stderr, "getaddrinfo Error!: %s\n", gai_strerror(addrInfoRes));
        return 1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next){
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            perror("Socket Error!!");
            continue;
        }
        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
            perror("Socket Option Error");
            exit(1);
        }
        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
            close(sockfd);
            perror("Server Bind Error");
            continue;
        }
        break;
    }

    printf("Hello world!\n");

    queue Test;
    QueInit(&Test);

    int * testSock1 = socketFactory(3);
    enQueue(&Test, testSock1);

    int * testSock2 = socketFactory(5);
    enQueue(&Test, testSock2);

    int * testSock3 = socketFactory(2600);
    enQueue(&Test, testSock3);


    printf("%s %d ", "Numbers are: ", *Dequeue(&Test));
    printf("%d ", *Dequeue(&Test));
    printf("%d ", *Dequeue(&Test));

    free(testSock1);
    free(testSock2);
    free(testSock3);
    return 0;


}



int * socketFactory(int testValue){ // generates sockets dynamically
    int * tempSock = malloc(sizeof(int));
    *tempSock = testValue;

    return tempSock;


}





void QueInit(queue * in){
    in->Front = NULL;
    in->Rear = NULL;
    in->count = 0;
}

void enQueue(queue * in, int * socketData){
    node * temp = malloc(sizeof(node));
    temp->socket = socketData;

    if(in->Front == NULL){ // queue is empty
        in->Front = in->Rear = temp;
    }
    else{
        in->Rear->next = temp;
        in->Rear = temp;
    }
}

int * Dequeue(queue * in){
    int * ret = in->Front->socket;
    node * temp;
    temp = in->Front;
    in->Front = in->Front->next;
    free(temp);
    return ret;
}



