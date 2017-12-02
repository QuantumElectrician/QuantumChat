//
//  main.c
//  Chat
//
//  Created by Владислав Агафонов on 26.11.2017.
//  Copyright © 2017 Владислав Агафонов. All rights reserved.
//
#include <stdio.h>
#include "parcer.h"

int hash(char* string)
{
    int hash = 0;
    for (int i = 0; i < strlen(string); i++)
    {
        hash = string[i] + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

int main(int argc, const char * argv[]) {
    char a[100];
    char b[100];
    char c[100];
    strcpy(a, "message first second");
    strcpy(b, fromWordToEnd(a, 1));
    strcpy(c, fromWordToEnd(a, 2));
    printf("%s hash = %d\n", a, hash(a));
    printf("%s hash = %d\n", b, hash(b));
    printf("%s hash = %d\n", c, hash(c));
    return 0;
}
