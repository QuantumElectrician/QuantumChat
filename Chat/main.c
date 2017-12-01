//
//  main.c
//  Chat
//
//  Created by Владислав Агафонов on 26.11.2017.
//  Copyright © 2017 Владислав Агафонов. All rights reserved.
//
//memset(buf, 0, sizeof(buf)); очистка буфера
#include <stdio.h>
#include "parcer.h"

int main(int argc, const char * argv[]) {
    char a[100];
    char b[100];
    strcpy(a, "message sdmnvksfjnvkjfnvjkndsfkjn");
    while (a[0] == ' ')
    {
        int i = 0;
        while (a[i] != '\0')
        {
            a[i] = a[i+1];
            i++;
        }
        
    }
    strcpy(b, fromWordToEnd(a, 1));
    
    printf("%s\n%s\n", a, b);
    return 0;
}
