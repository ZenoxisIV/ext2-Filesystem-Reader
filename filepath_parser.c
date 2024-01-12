#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_PATH_LENGTH 4096

void recreatePath(char*);

int main() {
    char path1[] = "/dir1/cs153.txt";
    char path2[] = "/dir1/./././cs153.txt";
    char path3[] = "/./dir1/cs153.txt";
    char path4[] = "/../dir1/cs153.txt";
    char path5[] = "/../../dir1/../dir1/./////cs153.txt";
    char path6[] = "/dir2/directory name with spaces/../../dir1/cs153.txt";
    
    recreatePath(path1);
    recreatePath(path2);
    recreatePath(path3);
    recreatePath(path4);
    recreatePath(path5);
    recreatePath(path6);

    printf("%s\n", path1);
    printf("%s\n", path2);
    printf("%s\n", path3);
    printf("%s\n", path4);
    printf("%s\n", path5);
    printf("%s\n", path6);
    
    return 0;
}

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