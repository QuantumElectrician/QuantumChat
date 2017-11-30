//
//  main.c
//  Chat
//
//  Created by Владислав Агафонов on 26.11.2017.
//  Copyright © 2017 Владислав Агафонов. All rights reserved.
//

#include <stdio.h>
#include "parcer.h"

int main(int argc, const char * argv[]) {
    char a[100];
    strcpy(a, "first second third");
    printf("%s %s %s\n", brkFind(a, 1), brkFind(a, 2), brkFind(a, 3));
    printf("%s\n", fromWordToEnd(a, 2));
    return 0;
}
