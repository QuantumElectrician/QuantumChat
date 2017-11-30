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

typedef struct{
    char total[200];
}message_t;

message_t localHistory[HISTORY_N];
int historyPoint = 0;

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
    int i = 0;
    while (1)
    {
        if ( (i = (int)recv(*connectSocket, buff, 200, MSG_PEEK) ) > 0)
        {
            if (strcmp(brkFind(buff, 1), "update") == 0)
            {
                recv(*connectSocket, buff, 200, 0);
                //printf("NEW UPDATE ADDED TO HISTORY\n");
                strcpy(localHistory[historyPoint].total, fromWordToEnd(buff, 1));
                historyPoint++;
                usleep(1000);
            }
        }
    }
}


void connectToServer(int sockfd) {
    char buff[100];
    int flag = 1;
    printf("Enter ur command:");
    strcpy(buff, inputString());
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
        char buffToSend[100];
        sprintf(buffToSend, "message %s", buff);
        if (send(sockfd, buffToSend, 1 + strlen(buffToSend), 0) < 0)
        {
            die("can't send message");
        }
    }

    if (strcmp(buff, "exit") == 0)
    {
        printf("\nEXIT CODE\n");
        if (send(sockfd, buff, 1 + strlen(buff), 0) < 0)
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
    if (send(sockfd, argv[2], 1 + strlen(argv[2]), 0) < 0)
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
