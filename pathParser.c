#include "defs.h"

void cleanPath(char *path) {
    int len = strlen(path);
    bool flag = path[len - 1] == '/';
    
    char temp[MAX_PATH_LENGTH];
    int tempIdx = 0;

    char *stack[MAX_PATH_LENGTH];
    int stackIdx = 0;

    // Tokenize the input path (Note: similar with split() in Python)
    char *token = strtok(path, "/");
    while (token != NULL) {
        // Store valid names, .., and . to the stack
        if (strlen(token) > 0) stack[stackIdx++] = token;
        token = strtok(NULL, "/");
    }

    // Build path
    for (int i = 0; i < stackIdx; i++) {
        temp[tempIdx++] = '/';
        strcpy(&temp[tempIdx], stack[i]);
        tempIdx += strlen(stack[i]);
    }
    
    if (flag) {
        temp[tempIdx] = '/';
        temp[tempIdx + 1] = '\0';
    } else {
        temp[tempIdx] = '\0';
    }

    strcpy(path, temp);
}