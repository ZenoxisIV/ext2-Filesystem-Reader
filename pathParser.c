#include "defs.h"

void recreatePath(char *path) {
    int len = strlen(path);
    bool flag = path[len - 1] == '/';
    
    char temp[MAX_PATH_LENGTH];
    int tempIdx = 0;

    char *stack[MAX_PATH_LENGTH];
    int stackIdx = 0;

    // Tokenize the input path (Note: similar with split() in Python)
    char *token = strtok(path, "/");
    while (token != NULL) {
        if (strcmp(token, "..") == 0) {
            // Move to the parent directory, if possible
            if (stackIdx > 0) {
                stackIdx--;
            }
        } else if (strcmp(token, ".") != 0 && strlen(token) > 0) {
            // Skip current directory, and add valid directory names to the stack
            stack[stackIdx++] = token;
        }

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