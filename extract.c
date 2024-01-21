/*
    CS140 2324A PROJECT 2 - EXT2 FILESYSTEM READER
    Ivan Cassidy Cadiang (2021-12575)
    Diego Montenejo (2020-05984)
    Rohan Solas (2021-07292)
*/
#include <unistd.h>
#include <sys/stat.h>
#include "defs.h"

char DEST_PATH[MAX_PATH_LENGTH] = "";   // Path that files will be extracted to
dir_entry TARGET_DIR;                   // Directory Entry of whatever file is being extracted
inode TARGET_INODE;                     // Target Inode of the root dir or main file to be extracted
char TARGET_NAME[MAX_PATH_LENGTH];      // Tokenized Target File/Dir name to be searched for

int READ_BUFFER[100000];                // Read Buffer for extraction
FILE* FILE_PT;                          // File pointer for file writing

// DIRECTORY EXTRACTION
// =======================================================================================
int extDirEntries(__u32 blockPointer, int fd, superblock sb, int blockSize) {
    // "Directory entries are also not allowed to span multiple blocks" https://wiki.osdev.org/Ext2#Directory_Entry 

    dir_entry dirEntry;

    int bytesParsed = 0;
    while (bytesParsed < blockSize) {
        dirEntry = readDirEntry(fd, blockPointer, blockSize, bytesParsed);
        const char* dirName = (char*) dirEntry.name;
        
        if (strcmp(dirName, ".") == 0 || strcmp(dirName, "..") == 0 || dirEntry.inode_num == 0) {
            bytesParsed += dirEntry.size;
            continue; // Skip current and parent directory entries
        }

        inode nextInode = readInode(dirEntry.inode_num, fd, sb, blockSize);
        __u16 objType = extractObjectType(nextInode);

        dir_entry oldEntry = TARGET_DIR;
        TARGET_DIR = dirEntry;
        int oldLen = strlen(DEST_PATH);

        switch (objType) {
            case DIRECTORY:
                extractDir(nextInode, fd, sb, blockSize, 0);
                break;
            case FILE_:
                extractFile(nextInode, fd, sb, blockSize, 0);
                break;
            default:
                break;
        }

        DEST_PATH[oldLen] = '\0';
        TARGET_DIR = oldEntry;
        bytesParsed += dirEntry.size;
    }

    return 0;
}

// Extracts all the subfiles/directories in a given directory
void extractDir(inode currInode, int fd, superblock sb, int blockSize, int isRoot) {
    if (isRoot) {
        currInode = TARGET_INODE;
        strcpy(DEST_PATH, "output");
    } else
        strcat(DEST_PATH, (char*)TARGET_DIR.name);

    mkdir(DEST_PATH, 0777);
    strcat(DEST_PATH, "/");

    readPointers(currInode, fd, sb, blockSize, extDirEntries);
}

// FILE EXTRACTION
// =======================================================================================

int copyBlockToFile(__u32 dp, int fd, superblock sb, int blockSize) {
    lseek(fd, blockSize * dp, SEEK_SET);
    read(fd, READ_BUFFER, blockSize);

    fwrite((char*)READ_BUFFER, sizeof(char), strlen((char*)READ_BUFFER), FILE_PT);

    return 0;
}

// File contains function for File Extraction
void extractFile(inode currInode, int fd, superblock sb, int blockSize, int isRoot) {
    if (isRoot) currInode = TARGET_INODE;

    int oldLen = strlen(DEST_PATH);
    strcat(DEST_PATH, (char*)TARGET_DIR.name);

    FILE_PT = fopen(DEST_PATH, "w");

    readPointers(currInode, fd, sb, blockSize, copyBlockToFile);

    DEST_PATH[oldLen] = '\0';

    fclose(FILE_PT);
}

// TARGET FILE/DIR SEARCHING
// =======================================================================================

// Checks the block pointer if the target directory or file is present within it
int checkEntries(__u32 blockPointer, int fd, superblock sb, int blockSize){
    // "Directory entries are also not allowed to span multiple blocks" https://wiki.osdev.org/Ext2#Directory_Entry 
    dir_entry directory_entry;

    int bytesParsed = 0;
    while (bytesParsed < blockSize) {
        directory_entry = readDirEntry(fd, blockPointer, blockSize, bytesParsed);

        const char* dirName = ( char*) directory_entry.name;

        // We found our target
        if (strcmp(dirName, TARGET_NAME) == 0) {
            // If the target is found, update the current inode

            TARGET_INODE = readInode(directory_entry.inode_num, fd, sb, blockSize);
            TARGET_DIR = directory_entry;
            return 1;
        }

        bytesParsed += directory_entry.size;
    }

    return 0;
}

int searchForTarget(inode currInode, int fd, superblock sb, int blockSize, char path[]) {
    TARGET_INODE = currInode;
    int targetIsDir = 0;
    
    if (path[strlen(path) - 1] == '/') 
        targetIsDir = 1; // target is directory

    //* 1. Tokenize Path to loop through each directory/file
    // Current token is the target directory/file to be looking for
    // ---------------------------------------------------------
    char* token = strtok(path, "/");

    while (token != NULL) {
        //* 2. Search through all Direct & Indirect Pointers for the current target
        strncpy(TARGET_NAME, token , strlen(token));
        TARGET_NAME[strlen(token)] = '\0';

        if (readPointers(TARGET_INODE, fd, sb, blockSize, checkEntries) == 1) {
            // Target was found
            goto targetFound;
        }

        // Went through all DP and IPs, could not find target. Path must not exist. Return Error
        return -1;

targetFound: 
        token = strtok(NULL, "/");
    }

    __u16 objType = extractObjectType(TARGET_INODE);

    switch (objType) {
        case FILE_:
            if (targetIsDir) return -1;
            else return 1;
            break;
        case DIRECTORY:
            return 2;
            break;
        default:
            return -1;
    }

    return 0;
}