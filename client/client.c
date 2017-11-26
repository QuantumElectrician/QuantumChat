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

void die(char*);

void die(char* msg)
{
    printf("error: %s, %s\n", msg, strerror(errno));
    exit(1);
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
    printf("sending username have done\n");
    while (1)
    {
        char buff[100];
        printf("Enter ur message:");
        //scanf("%s", buff);
        strcpy(buff, inputString());
        if (send(sockfd, buff, 1 + strlen(buff), 0) < 0)
        {
            die("can't send username");
        }
        if (strcmp(buff, "exit") == 0)
        {
            printf("\nEXIT CODE\n");
            usleep(1000);
            //close(sockfd);
            return 0;
        }
    }
    return 0;
}
