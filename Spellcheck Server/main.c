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

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define PORT "3490" // port number

#define BACKLOG 10 // pending connection no.
#define MAX 5
// DELCARATIONS//
struct node{
    void * socket; // takes either a string or a socket. so  i dont have to recode the log queue
    struct node * next;
}; typedef struct node node;



struct queue{
    node * Front;
    node * Rear;
    int count;
}; typedef struct queue queue;

int new_fd, sockfd; // new connection, shared among threads
struct sockaddr_storage their_addr;
socklen_t sin_size;

queue Test;
queue Log;

FILE *dict;
FILE *logFile;


pthread_cond_t emp = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t logEmp = PTHREAD_COND_INITIALIZER;
pthread_cond_t logFill = PTHREAD_COND_INITIALIZER;
pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

int done = 0;


// FUNCTION DECLARATIONS
int checkWord(char * phraseIn, FILE * dict);
void QueInit(queue * in);
void enQueue(queue * in, void * socketData);
void * Dequeue(queue * in);
int * socketFactory();
void * producer(void * arg);
void * consumer(void * arg);
void * consumLog(void * arg);
void prodLog(char * put);
char * wordTrimmer(char * in);




int main()
{
    QueInit(&Test);
    QueInit(&Log);

    char buff[255];
    dict = fopen("dictionary.txt", "r");
    if(dict == NULL){
        printf("\n\nFile Open Error\n\n");
        exit(1);
    }

    logFile = fopen("log2txt", "w");

     // listening socket
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

        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
            close(sockfd);
            perror("Server Bind Error");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);
    if(p == NULL){
        fprintf(stderr, "Server: failed to bind \n");
    }
    // end of socket initialization logic


    // BEGIN //

    printf("Hello world!\n");

    pthread_t t1, t2, t3, t4, t5, t6, t7;
    pthread_create(&t1, NULL, producer, NULL);
    pthread_create(&t2, NULL, consumer, NULL);
    pthread_create(&t3, NULL, consumer, NULL);
    pthread_create(&t4, NULL, consumer, NULL);
    pthread_create(&t5, NULL, consumer, NULL);
    pthread_create(&t6, NULL, consumer, NULL);
    pthread_create(&t7, NULL, consumLog, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);
    pthread_join(t5, NULL);
    pthread_join(t6, NULL);
    pthread_join(t7, NULL);


    return 0;

    // END
}



int * socketFactory(){ // generates sockets dynamically
    int * tempSock = malloc(sizeof(int));

    return tempSock;


}

void prodLog(char * put){
    pthread_mutex_lock(&logMutex);
    while(Log.count == MAX){
        pthread_cond_wait(&logEmp, &logMutex);
    }
    enQueue(&Log, put);
    pthread_cond_signal(&logFill);
    pthread_mutex_unlock(&logMutex);
    return;

}
void * consumLog(void * arg){
    pthread_mutex_lock(&logMutex);
    while(Log.count == 0){
        pthread_cond_wait(&logFill, &logMutex);
    }
    char * put = Dequeue(&Log);
    int result = fputs(put, logFile);
    fputs("\n", logFile);
    pthread_cond_signal(&logEmp);
    pthread_mutex_unlock(&logMutex);
    return 0;
}

int checkWord(char * phraseIn, FILE * dict){
    char * inTrimmed = wordTrimmer(phraseIn);
    char dictBuff[100];
    if(strcmp(inTrimmed, "quit") == 0){
        free(inTrimmed);
        return 1;
    }


    else{
        memcpy(dictBuff, "\0", sizeof(dictBuff)); // clears buffer for word
        while(fgets(dictBuff, sizeof(dictBuff), dict) != NULL){

            char * dictTrimmed = wordTrimmer(dictBuff);
            if(strcmp(inTrimmed, dictTrimmed) == 0){
                free(inTrimmed);
                free(dictTrimmed);
                fseek(dict, 0, SEEK_SET); // resets file pointer to beginning
                return 2; // word found
            }
            else{
                memcpy(dictBuff, "\0", sizeof(dictBuff)); // clears buffer for word
                free(dictTrimmed);
            }
        }
        fseek(dict, 0, SEEK_SET);
        free(inTrimmed);

        return -1; // word not found
    }
}

char * wordTrimmer(char * in){
    for(int i = 0; i < strlen(in); i++){
        if(in[i] == '\n' || in[i] == '\r'){
            in[i] = '\0';
        }
    }
    char * out = malloc(strlen(in));
    strcpy(out, in);
    return out;
}

void * producer(void * arg){
    while(1){

        listen(sockfd, BACKLOG);
        int * new_fd = socketFactory();
        *new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        char *msg = ("Please wait to be serviced...");
        int len, bytes_sent;
        len = strlen(msg);
        bytes_sent = send(*new_fd, msg, len, 0);


        pthread_mutex_lock(&mutex);
        while(Test.count == MAX){
            pthread_cond_wait(&emp, &mutex);
        }

        enQueue(&Test, new_fd);
        pthread_cond_signal(&fill);
        pthread_mutex_unlock(&mutex);

    }
    done = 1;

    return 0;
}

void * get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void * consumer(void * arg){

    while(done == 0){
        int * new_fd;
        pthread_mutex_lock(&mutex);
        while(Test.count == 0){
            pthread_cond_wait(&fill, &mutex);
        }
        new_fd = Dequeue(&Test);
        pthread_cond_signal(&emp);
        pthread_mutex_unlock(&mutex);

        char ip[INET6_ADDRSTRLEN];
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), ip, sizeof(ip));

        char out[INET6_ADDRSTRLEN + 20];
        strcpy(out, ip);
        strcat(out, " ");
        strcat(out, "Connected.");

        prodLog(out);

        while(1){
            char * sendVal = ("\n\nPlease input a word to check. Or, type 'quit' to close the connection.\n>");
            int valLen = strlen(sendVal);
            send(*new_fd, sendVal, valLen, 0);
            char recieved[100];
            if(recv(*new_fd, recieved, sizeof(recieved), 0) != -1){
                int res = checkWord(recieved, dict);
                if(res == 1){
                    // quit condition
                    char * quitVal = ("\n\nThank you, goodbye");
                    send(*new_fd, quitVal, sizeof(quitVal), 0);
                    break;
                }
                else{
                    char correctStatus[50];
                    char * word = wordTrimmer(recieved);
                    int correctly = 0; // 0 for false, 1 for true
                    strcpy(correctStatus, word);
                    if(res == 2){ // word found
                        strcat(correctStatus, " CORRECT");
                    }
                    else if(res == -1){
                        strcat(correctStatus, " INCORRECT");
                    }
                    send(*new_fd, correctStatus, sizeof(correctStatus), 0);
                    prodLog(correctStatus);
                }
            }
            else{
                char * error = ("\n\nError in recieving");
                printf("\n\n%s", error);
                send(*new_fd, error, sizeof(error), 0);
            }
        }



        close(*new_fd);
        free(new_fd);

    }
    return 0;
}



void QueInit(queue * in){
    in->Front = NULL;
    in->Rear = NULL;
    in->count = 0;
}

void enQueue(queue * in, void * socketData){
    node * temp = malloc(sizeof(node));
    temp->socket = socketData;

    if(in->Front == NULL){ // queue is empty
        in->Front = in->Rear = temp;
    }
    else{
        in->Rear->next = temp;
        in->Rear = temp;
    }

    in->count++;

    return;
}

void * Dequeue(queue * in){
    int * ret = in->Front->socket;
    node * temp;
    temp = in->Front;
    in->Front = in->Front->next;
    free(temp);

    in->count--;

    return ret;
}



