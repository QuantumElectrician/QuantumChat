//
//  main.c
//  client
//
//  Created by Владислав Агафонов on 26.11.2017.
//  Copyright © 2017 Владислав Агафонов. All rights reserved.
//

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "parcer.h"

#define HISTORY_N 200

void die(char*);
void connectToServer(int);
void* updateHistory(void*);
void printHistory(void);
int sendAll(int s, char *buf, int len, int flags);

typedef struct{
    char total[200];
}message_t;

message_t localHistory[HISTORY_N];
int historyPoint = 0;

int sendAll(int s, char *buf, int len, int flags)
{
    int total = 0;
    int n = 0;
    
    while(total < len)
    {
        n = send(s, buf+total, len-total, flags);
        if(n == -1) { break; }
        total += n;
    }
    return (n==-1 ? -1 : total);
}

int recvAll(int s, char *buf, int len, int flags)
{
    int total = 0;
    int n = 0;
    
    while(total < len)
    {
        n = recv(s, buf+total, len-total, flags);
        if(n == -1) { break; }
        total += n;
    }
    return (n==-1 ? -1 : total);
}

void die(char* msg)
{
    printf("error: %s, %s\n", msg, strerror(errno));
    exit(1);
}

void printHistory()
{
    for (int i = 0; i < historyPoint; i++)
    {
        printf("%s", localHistory[i].total);
    }
}

void* updateHistory(void* copy)
{
    int* connectSocket = (int*)copy;
    char buff[200];
    strcpy(buff, " "); //инициализация буфера
    int i = 0;
    while (1)
    {
        if ( (i = (int)recvAll(*connectSocket, buff, 200, MSG_PEEK) ) > 0) //ВОТ ЗДЕСЬ ИЗ СОКЕТА ДОСТАЁТСЯ ХУЁВЫЙ РЕСИВ
        {
            if (strcmp(brkFind(buff, 1), "update") == 0)
            {
                //printf("%s", buff);
                recvAll(*connectSocket, buff, 200, 0);
                //printf("NEW UPDATE ADDED TO HISTORY\n");
                strcpy(localHistory[historyPoint].total, fromWordToEnd(buff, 1));
                historyPoint++;
                usleep(2000);
                strcpy(buff, " "); //очистка буфера
            }
        }
    }
}


void connectToServer(int sockfd) {
    char buff[200];
    int flag = 1;
    printf("Enter ur command:");
    strcpy(buff, inputString());
    while (buff[0] == ' ')
    {
        int i = 1;
        while (buff[i] != '\0')
        {
            buff[i-1] = buff[i];
            i++;
        }
        buff[i-1] = buff[i];
    }
    if (strcmp(brkFind(buff, 1), "help") == 0)
    {
        printf("Available commands:\n help\n update\n history\n send\n exit\n");
        flag = 0;
    }
    if (strcmp(brkFind(buff, 1), "update") == 0)
    {
        flag = 0;
    }
    if (strcmp(brkFind(buff, 1), "history") == 0)
    {
        printHistory();
        flag = 0;
    }
    if (strcmp(brkFind(buff, 1), "send") == 0)
    {
        flag = 0;
        printf("Enter ur message:");
        strcpy(buff, inputString());
        char buffToSend[200];
        sprintf(buffToSend, "message %s", buff);
        //if (sendAll(sockfd, buffToSend, 1 + strlen(buffToSend), 0) < 0)
        if (sendAll(sockfd, buffToSend, 200, 0) < 0)
        {
            die("can't send message");
        }
    }

    if (strcmp(buff, "exit") == 0)
    {
        printf("\nEXIT CODE\n");
        if (sendAll(sockfd, buff, 200, 0) < 0)
        {
            die("can't send message");
        }
        sleep(1);
        close(sockfd);
        exit(0);
    }
    
    if (flag == 1)
    {
        printf("\nIncorrect entrance\n\nAvailable commands:\n help\n update\n history\n send\n exit\n");
    }
}

int main(int argc, const char * argv[])
{
    int sockfd;
    struct sockaddr_in serv_addr;
    
    if(argc != 3)
    {
        printf("\n Usage: %s <ip of server> <nickname> \n",argv[0]);
        return 1;
    }
    
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        die("can't create socket");
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);
    
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0)
    {
        die("inet_pton error occured");
    }
    
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        die("Connect Failed");
    }
    
    printf("sending msg '%s' to server...\n", argv[2]);
    if (sendAll(sockfd, argv[2], 1 + strlen(argv[2]), 0) < 0)
    {
        die("can't send username");
    }
    
    //создание треда для обновления локальной истории
    pthread_t threadID;
    int result;
    result = pthread_create(&threadID, NULL, updateHistory, (void*)&sockfd);
    if (result) {
        printf("Can't create thread, returned value = %d\n", result);
        exit(-1);
    }
    
    printf("Connected to server and sent ur username succesfully\n");
    printf("Welcome to Chat!\n Use 'help' for list of commands\n");
    while (1)
    {
        connectToServer(sockfd);
    }
    return 0;
}
