//
//  main.c
//  server
//
//  Created by Владислав Агафонов on 26.11.2017.
//  Copyright © 2017 Владислав Агафонов. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //sleep
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

#define USERS_NUMBER 10
#define LISTEN_PORT 5000
#define THREADS_N 10

void die(char*);
static void newRequest(int*);
void addToUsers(char*);
void connectionServer(int* , char*);

typedef enum{
    EMPTY,
    FIFTEENMINSAGO,
    ONLINE
}statusesUser;
typedef struct{
    char username[20];
    int password;
    statusesUser status;
    pthread_t worker;
    int connectionSocket;
}user_t;
typedef struct{
    char mess[100];
}message_t;

int listenSocket;
user_t users[USERS_NUMBER];
pthread_mutex_t lock;
pthread_t thread_id[THREADS_N];
FILE* history = NULL;

void connectionServer(int* socket, char* username)
{
    char buff[100];
    recv(*socket, buff, 100, 0);
    while (1)
    {
        if (strcmp(buff, "exit") == 0)
        {
            //close(*socket);
            exit(0);
        }
        fprintf(history, "%s\n", buff);
        recv(*socket, buff, 100, 0);
    }
}

void addToUsers(char* cp)
{
    int flag = 1;
    for (int i = 0; i < USERS_NUMBER; i++)
    {
        if (users[i].status == EMPTY)
        {
            users[i].status = FIFTEENMINSAGO;
            sprintf(users[i].username, "%s", cp);
            printf("[SERVER] Added new user with username %s\n", cp);
            flag = 0;
            break;
        }
    }
    if (flag == 1)
    {
        die("users overflow");
    }
}

void die(char* msg)
{
    printf("error: %s, %s\n", msg, strerror(errno));
    exit(1);
}

static void newRequest(int* connfd)
{
    char buff[100];
    int flag = 0;
    printf("Waiting for the username ...\n");
    recv(*connfd, buff, 100, 0);
    printf("[SERVER] New request from %s\n\n", buff);
    for (int i = 0; i < USERS_NUMBER; i++)
    {
        if (strcmp(users[i].username, buff)==0)
        {
            flag = 1;
            break;
        }
    }
    if (flag == 0)
    {
        addToUsers(buff);
    }
    
    printf("Begin forking for creating new connect-Server...\n");
    int pid = fork();
    printf("Forcked\n");
    switch( pid )
    {
        case 0:
            //дочерний процесс
            connectionServer(connfd, buff);
            printf("Connection ended with %s\n", buff);
            break;
        case -1:
            printf("Fail: unable to fork\n");
            break;
        default:
            //родительский процесс
            break;
    }
}

int main(int argc, const char * argv[])
{
    history = fopen("HISTORY.txt", "a");
    signal( SIGCHLD, SIG_IGN );
    //инициализация массива
    for (int i = 0; i < USERS_NUMBER; i++)
    {
        users[i].status = EMPTY;
    }
    //создание слушающего сокета
    struct sockaddr_in serv_addr;
    if ((listenSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        die("can't create socket");
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(LISTEN_PORT);
    
    if (bind(listenSocket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        die("can't bind");
    }
    if (listen(listenSocket, 10) < 0) {
        die("can't listen");
    }
    printf("[SERVER] Main server starts...\n");
    
    int connfd;

    while (1)
    {
        if ((connfd = accept(listenSocket, (struct sockaddr*)NULL, NULL)) < 0)
        {
            die("can't accept");
        }
        printf("\n[SERVER] Got new connection\n");
        
        newRequest(&connfd);
    }
    return 0;
}
