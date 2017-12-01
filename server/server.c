//
//  main.c
//  server
//
//  Created by Владислав Агафонов on 26.11.2017.
//  Copyright © 2017 Владислав Агафонов. All rights reserved.
//

//ipcrm -Q 125 ОЧИЩАЙ ОЧЕРЕДЬ СООБЩЕНИЙ
//ipcrm -Q 126 ОЧИЩАЙ ОЧЕРЕДЬ СООБЩЕНИЙ
//MSG_PEEK -- флаг на чтение из сокет-канала без удаления сообщения

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
#include "parcer.h"

#define USERS_NUMBER 10
#define LISTEN_PORT 5000
#define THREADS_N 10
#define HISTORY_FILE "History.txt"
#define HISTORY_N 200

void die(char*);
static void newRequest(int*);
int addToUsers(char*);
void connectionServer(int* , char*);
char* getTime(void);
void sendToHistoryServer(char* usernameToSend, char* currentTimeToSend, char* messageToSend);
void* HistoryServer(void* dummy);
void* SyncHistoryWithConnectionServers(void* dummy);
void* sendUpdate(void* copy);


typedef enum{
    EMPTY,
    OFFLINE,
    ONLINE
}statusesUser;
typedef enum{
    NOTHING,
    READ,
    NEW
}statusesMessage;
typedef struct{
    char username[20];
    int password;
    statusesUser status;
    pid_t connectionServer;
    int connectionSocket;
}user_t;
typedef struct{
    char usernameHis[50];
    char between1[5];
    char currentTimeHis[20];
    char between2[5];
    char messageHis[200];
    char end[5];
    statusesMessage status;
}message_t;
typedef struct
{
    message_t history[HISTORY_N];
}s_t;

int listenSocket;
user_t users[USERS_NUMBER];
pthread_t thread_id[THREADS_N];
int historyFD = 0;
s_t* historyVirt; //создание указателя на пользовательскую структуру (область в оперативе)
int historyPoint = 0;

void* SyncHistoryWithConnectionServers(void* dummy)
{
    /* IPC дескриптор для очереди сообщений */
    int msqid;
    
    /* IPC ключ */
    key_t key = 126;
    
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
    
    while (1)
    {
        for (int i = 0; i < HISTORY_N; i++)
        {
            if (historyVirt->history[i].status == NEW)
            {
                historyVirt->history[i].status = READ;
                len = 170;
                for (int j = 0; j < USERS_NUMBER; j++)
                {
                    //if ( (users[i].status == ONLINE) && (strcmp(users[i].username, historyVirt->history[i].usernameHis) != 0) )
                    if (users[j].status == ONLINE)
                    {
                        mybuf.mtype = (int)users[j].connectionServer;
                        strcpy(mybuf.username, historyVirt->history[i].usernameHis);
                        strcpy(mybuf.message, historyVirt->history[i].messageHis);
                        strcpy(mybuf.currentTime, historyVirt->history[i].currentTimeHis);
                        /* Отсылаем сообщение. В случае ошибки сообщаем об этом */
                        if (msgsnd(msqid, (struct mymsgbuf *) &mybuf, len, 0) < 0)
                        {
                            printf("Can\'t send message to queue\n");
                            exit(-1);
                        }
                    }
                }
            }
        }
    }
}

void* HistoryServer(void* dummy)
{
    /* IPC дескриптор для очереди сообщений */
    int msqid;
    
    /* IPC ключ */
    key_t key = 125;
    
    /* Cчетчик цикла и длина информативной части сообщения */
    long len;
    int maxlen;
    //char kostyl[170];

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
        char kostyl[100];
        strcpy(kostyl, fromWordToEnd(mybuf.message, 1));
        strcpy(historyVirt->history[historyPoint].usernameHis, mybuf.username);
        strcpy(historyVirt->history[historyPoint].between1, " at ");
        strcpy(historyVirt->history[historyPoint].currentTimeHis, mybuf.currentTime);
        strcpy(historyVirt->history[historyPoint].between2, " : ");
        strcpy(historyVirt->history[historyPoint].messageHis,kostyl);
        strcpy(historyVirt->history[historyPoint].end, "\n");
        
        //ВРЕМЕННАЯ МЕРА 
        if (strcmp(kostyl, "[SYSTEM] DISCONNECTED") == 0)
        {
            for (int i = 0; i < USERS_NUMBER; i++)
            {
                if (strcmp(users[i].username, mybuf.username) == 0)
                {
                    users[i].status = OFFLINE;
                }
            }
        }
        
        historyVirt->history[historyPoint].status = NEW; //в этот момент синхронизатор подхватит это сообщение всем на пересылку
        msync(&(historyVirt->history[historyPoint]), sizeof(message_t), MS_SYNC); //принудительная синхронизация истории и виртуальной истории
        historyPoint++;
    }
}

void sendToHistoryServer(char* usernameToSend, char* currentTimeToSend, char* messageToSend)
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

char* getTime(void)
{
    time_t now;
    struct tm *ptr;
    static char tbuf[64];
    bzero(tbuf,64);
    time(&now);
    ptr = localtime(&now);
    strftime(tbuf,64, "%e-%m-%Y %H:%M:%S", ptr);
    return tbuf;
}

void* sendUpdate(void* copy)
{
    int* socket = (int*)copy;
    int MyPID = (int)getpid();
    /* IPC дескриптор для очереди сообщений */
    int msqid;
    long maxlen;
    /* IPC ключ */
    key_t key = 126;
    
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
    
    maxlen = 170;
    while (1)
    {
        if ((len = msgrcv(msqid, (struct mymsgbuf *)&mybuf, maxlen, MyPID, 0)) < 0)
        {
            printf("Can\'t receive message from queue\n");
            exit(-1);
        }
        char kostyl[200];
        sprintf(kostyl, "update %s at %s : %s\n", mybuf.username, mybuf.currentTime, mybuf.message);
        if (send(*socket, kostyl, 1 + strlen(kostyl), 0) < 0)
        {
            die("can't send message");
        }
    }
}

void connectionServer(int* socket, char* username)
{
    //создание треда для создания истории
    pthread_t threadIdUpdate;
    int result;
    result = pthread_create(&threadIdUpdate, NULL, sendUpdate, (void*)socket);
    if (result) {
        printf("Can't create thread, returned value = %d\n", result);
        exit(-1);
    }
    char buff[100];
    char currentTime[20];
    sendToHistoryServer(username, getTime(), "message [SYSTEM] NEW USER CONNECTED");
    recv(*socket, buff, 100, MSG_PEEK);
    strcpy(currentTime, getTime());
    while (1)
    {
        if (strcmp(brkFind(buff, 1), "exit") == 0)
        {
            close(*socket);
            //тут нельзя поставить отключенному пользователю ОФЛАЙН, так как оригинал базы лежит на MainServer, а здесь только копия
            //Решение: все сервера обработчики через mmap коннектятся с файлу-базе пользователей
            sendToHistoryServer(username, getTime(), "message [SYSTEM] DISCONNECTED");
            exit(0);
        }
        if (strcmp(brkFind(buff, 1), "message") == 0)
        {
            recv(*socket, buff, 100, 0);
            sendToHistoryServer(username, currentTime, buff);
        }
        usleep(100);
        recv(*socket, buff, 100, MSG_PEEK);
        strcpy(currentTime, getTime());
    }
}

int addToUsers(char* cp)
{
    int flag = 1;
    int index = 0;
    for (int i = 0; i < USERS_NUMBER; i++)
    {
        if (users[i].status == EMPTY)
        {
            users[i].status = ONLINE;
            index = i;
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
    return index;
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
    int index = 0;
    printf("Waiting for the username ...\n");
    recv(*connfd, buff, 100, 0);
    printf("[SERVER] New request from %s\n\n", buff);
    for (int i = 0; i < USERS_NUMBER; i++)
    {
        if (strcmp(users[i].username, buff)==0)
        {
            users[i].status = ONLINE;
            index = i;
            flag = 1;
            break;
        }
    }
    if (flag == 0)
    {
        index = addToUsers(buff);
    }
    
    printf("Begin forking for creating new connect-Server...\n");
    pid_t pid = fork();
    switch( (int)pid )
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
            users[index].connectionServer = pid;
            printf("Forked\n");
            break;
    }
}

int main(int argc, const char * argv[])
{
    char sys[100];
    sprintf(sys, "touch %s && rm %s", HISTORY_FILE, HISTORY_FILE);
    system(sys); //удаление предыдущей истории истории
    sprintf(sys, "ipcrm -Q 125 && ipcrm -Q 126");
    system(sys); //удаление старых очередей сообщений
    historyFD = open(HISTORY_FILE, O_RDWR | O_CREAT, 0666); //открытие файла с правами на чтение и запись и флагом на создание, если его нет
    ftruncate(historyFD, sizeof(s_t)); //зачистка файла с дескриптором history и вторым аргументом новый размер файла
    historyVirt = mmap(NULL, sizeof(s_t), PROT_WRITE | PROT_READ, MAP_SHARED, historyFD, 0);
    
    signal( SIGCHLD, SIG_IGN ); //отсутствие зомби
    
    //создание треда для поддержания истории
    pthread_t threadIDHistory;
    int result;
    result = pthread_create(&threadIDHistory, NULL, HistoryServer, NULL);
    if (result)
    {
        printf("Can't create thread, returned value = %d\n", result);
        exit(-1);
    }
    
    //создание треда для синхронизации истории с серверами
    pthread_t threadIDSync;
    int res;
    res = pthread_create(&threadIDSync, NULL, SyncHistoryWithConnectionServers, NULL);
    if (res)
    {
        printf("Can't create thread, returned value = %d\n", result);
        exit(-1);
    }
    
    //инициализация массивов
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
