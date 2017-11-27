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
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/msg.h>

#define USERS_NUMBER 10
#define LISTEN_PORT 5000
#define THREADS_N 10
#define HISTORY "History.txt"

void die(char*);
static void newRequest(int*);
void addToUsers(char*);
void connectionServer(int* , char*);
char* getTime(void);
void sendToServer(char* usernameToSend, char* currentTimeToSend, char* messageToSend);
void* createHistory(void* dummy);


typedef enum{
    EMPTY,
    FIFTEENMINSAGO,
    ONLINE
}statusesUser;
typedef enum{
    NEW,
    READ
}statusesMessage;
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
typedef struct
{
    message_t history[200];
    statusesMessage status;
}s_t;

int listenSocket;
user_t users[USERS_NUMBER];
pthread_mutex_t lock;
pthread_t thread_id[THREADS_N];
int historyFD = 0;
s_t* historyVirt; //создание указателя на пользовательскую структуру (область в оперативе)
int historyPoint = 0;

void* createHistory(void* dummy)
{
    /* IPC дескриптор для очереди сообщений */
    int msqid;
    
    /* IPC ключ */
    key_t key = 125;
    
    /* Cчетчик цикла и длина информативной части сообщения */
    long len;
    int maxlen;
    char kostyl[170];

    /* Ниже следует пользовательская структура для сообщения */
    struct mymsgbuf{
        long mtype;
        char username[50];
        char currentTime[20];
        char message[100];
    } mybuf;
    
    /* Пытаемся получить доступ по ключу к очереди сообщений, если она существует,
     или создать ее, если она еще не существует, с правами доступа
     read & write для всех пользователей */
    if((msqid = msgget(key, 0666 | IPC_CREAT)) < 0)
    {
        printf("Can\'t get msqid\n");
        exit(-1);
    }

    while (1)
    {
        maxlen = 170;
        if ((len = msgrcv(msqid, (struct mymsgbuf *)&mybuf, maxlen, 1, 0)) < 0)
        {
            printf("Can\'t receive message from queue\n");
            exit(-1);
        }
        sprintf(kostyl, "%s at %s : %s\n", mybuf.username, mybuf.currentTime, mybuf.message);
        strcpy(historyVirt->history[historyPoint].mess, kostyl);
        msync(&(historyVirt->history[historyPoint]), sizeof(message_t), MS_SYNC);
        historyPoint++;
        
    }
}

char* getTime(void)
{
    time_t now;
    struct tm *ptr;
    static char tbuf[64];
    bzero(tbuf,64);
    time(&now);
    ptr = localtime(&now);
    strftime(tbuf,64, "%Y-%m-%e %H:%M:%S", ptr);
    return tbuf;
}

void sendToServer(char* usernameToSend, char* currentTimeToSend, char* messageToSend)
{
    /* IPC дескриптор для очереди сообщений */
             int msqid;
    
             /* IPC ключ */
             key_t key = 125;
    
             /* Cчетчик цикла и длина информативной части сообщения */
             long len;
    
             /* Ниже следует пользовательская структура для сообщения */
             struct mymsgbuf{
                 long mtype;
                 char username[50];
                 char currentTime[20];
                 char message[100];
                 } mybuf;
    
             /* Пытаемся получить доступ по ключу к очереди сообщений, если она существует,
                         или создать ее, если она еще не существует, с правами доступа
                         read & write для всех пользователей */
             if((msqid = msgget(key, 0666 | IPC_CREAT)) < 0)
             {
                         printf("Can\'t get msqid\n");
                         exit(-1);
             }
    
    /* Сначала заполняем структуру для нашего сообщения и определяем длину информативной части */
    mybuf.mtype = 1;
    strcpy(mybuf.username, usernameToSend);
    strcpy(mybuf.message, messageToSend);
    strcpy(mybuf.currentTime, currentTimeToSend);
    len = 170;
    /* Отсылаем сообщение. В случае ошибки сообщаем об этом */
    if (msgsnd(msqid, (struct mymsgbuf *) &mybuf, len, 0) < 0)
    {
        printf("Can\'t send message to queue\n");
        exit(-1);
        
    }
}

void connectionServer(int* socket, char* username)
{
    char buff[100];
    char currentTime[20];
    recv(*socket, buff, 100, 0);
    strcpy(currentTime, getTime());
    while (1)
    {
        if (strcmp(buff, "exit") == 0)
        {
            close(*socket);
            exit(0);
        }
        sendToServer(username, currentTime, buff);
        recv(*socket, buff, 100, 0);
        strcpy(currentTime, getTime());
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
            printf("[SERVER] Added new user with username %s at %s\n", cp, getTime());
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
            printf("Forcked\n");
            break;
    }
}

int main(int argc, const char * argv[])
{
    char sys[200];
    sprintf(sys, "touch %s && rm %s", HISTORY, HISTORY);
    system(sys); //удаление предыдущей истории истории
    historyFD = open(HISTORY, O_RDWR | O_CREAT, 0666); //открытие файла с правами на чтение и запись и флагом на создание, если его нет
    ftruncate(historyFD, sizeof(s_t)); //зачистка файла с дескриптором history и вторым аргументом новый размер файла
    historyVirt = mmap(NULL, sizeof(s_t), PROT_WRITE | PROT_READ, MAP_SHARED, historyFD, 0);
    
    signal( SIGCHLD, SIG_IGN ); //отсутствие зомби
    
    //создание треда для создания истории
    pthread_t threadID;
    int result;
    result = pthread_create(&threadID, NULL, createHistory, NULL);
    if (result)
    {
        printf("Can't create thread, returned value = %d\n", result);
        exit(-1);
    }
    
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
