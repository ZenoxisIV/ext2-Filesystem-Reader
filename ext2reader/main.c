/*
    CS140 2324A PROJECT 2 - EXT2 FILESYSTEM READER
    Ivan Cassidy Cadiang (2021-12575)
    Diego Montenejo (2020-05984)
    Rohan Solas (2021-07292)
*/
#include <stdio.h>
#include "defs.h"

int main(int argc, char* argv[]) {
    switch(argc) {
        case 1:
            // **OP 1: PATH ENUMERATION       (No additional arguments) 
            // --------------------------------------------------
            EnumeratePaths();
            break;
        case 2:
            // **OP 2: FILESYSTEM EXTRACTION  (Additional argument given)
            // --------------------------------------------------
            ExtractFile();
            break;
        
        default:
            printf("Too many arguments supplied.\nCommand only supports 0 or 1 argument/s.\n");
            break;
    }

    return 0;   
}