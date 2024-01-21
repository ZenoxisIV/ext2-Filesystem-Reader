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
int READ_BUFFER[100000];                // Read Buffer for extraction
FILE* FILE_PT;                          // File pointer for file writing

void parseBlock2_ext(__u32 blockPointer, int fd, superblock sb, int blockSize) {
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
                extractAllPaths(nextInode, fd, sb, blockSize, 0);
                break;
            case FILE_:
                extractSinglePath(nextInode, fd, sb, blockSize);
                break;
            default:
                break;
        }

        DEST_PATH[oldLen] = '\0';
        TARGET_DIR = oldEntry;
        bytesParsed += dirEntry.size;
    }
}

void copyBlockToFile(__u32 dp, int fd, superblock sb, int blockSize) {
    lseek(fd, blockSize * dp, SEEK_SET);
    read(fd, READ_BUFFER, blockSize);

    fwrite((char*)READ_BUFFER, sizeof(char), strlen((char*)READ_BUFFER), FILE_PT);
}

// File contains function for File Extraction
void extractSinglePath(inode currInode, int fd, superblock sb, int blockSize) {
    int oldLen = strlen(DEST_PATH);
    strcat(DEST_PATH, (char*)TARGET_DIR.name);

    FILE_PT = fopen(DEST_PATH, "w");

    readPointers(currInode, fd, sb, blockSize, copyBlockToFile);

    DEST_PATH[oldLen] = '\0';

    fclose(FILE_PT);
}

// Extracts all the subfiles/directories in a given directory
void extractAllPaths(inode currInode, int fd, superblock sb, int blockSize, int isRoot) {
    if (isRoot)
        strcpy(DEST_PATH, "output");
    else
        strcat(DEST_PATH, (char*)TARGET_DIR.name);

    mkdir(DEST_PATH, 0777);
    strcat(DEST_PATH, "/");

    readPointers(currInode, fd, sb, blockSize, parseBlock2_ext);
}


// Checks the block pointer if the target directory or file is present within it
//!     IMPORTANT: function modifies the currInode argument passed to it 
int checkBlock(inode* currInode, __u32 blockPointer, int fd, superblock sb, int blockSize, char* target){
    // "Directory entries are also not allowed to span multiple blocks" https://wiki.osdev.org/Ext2#Directory_Entry 
    dir_entry directory_entry;

    int bytesParsed = 0;
    while (bytesParsed < blockSize) {
        directory_entry = readDirEntry(fd, blockPointer, blockSize, bytesParsed);

        const char* dirName = ( char*) directory_entry.name;

        // We found our target
        if (strcmp(dirName, target) == 0) {
            // If the target is found, update the current inode

            *currInode = readInode(directory_entry.inode_num, fd, sb, blockSize);
            TARGET_DIR = directory_entry;
            return 1;
        }

        bytesParsed += directory_entry.size;
    }

    return 0;
}

//!     IMPORTANT: function modifies the currInode argument passed to it
int searchForTarget(inode* currInode, int fd, superblock sb, int blockSize, char path[]) {
    int targetIsDir = 0;
    
    if (path[strlen(path) - 1] == '/') 
        targetIsDir = 1; // target is directory

    //* 1. Tokenize Path to loop through each directory/file
    // Current token is the target directory/file to be looking for
    // ---------------------------------------------------------
    char* token = strtok(path, "/");

    while (token != NULL) {
        //* 2. Search through all Direct & Indirect Pointers for the current target
        // ---------------------------------------------------------
        // === Direct
        for (int i = 0; i < NDIRECT; i++){
            if (currInode->dp[i] == 0) continue; // Don't even bother with null pointers

            if (checkBlock(currInode, currInode->dp[i], fd, sb, blockSize, token)) {
                goto targetFound;
            }
        }

        __u32 nindirect = blockSize / sizeof(__u32);

        // === Single Indirect
        if (currInode->sip != 0) {
            for (int j = 0; j < nindirect; j++){
                __u32 directPointer = readIndirectBlock(fd, currInode->sip, blockSize, j*4); // DP points to block with more directory entries

                if (directPointer == 0) continue; // Don't even bother with null pointers

                if (checkBlock(currInode, directPointer, fd, sb, blockSize, token)) {
                    goto targetFound;
                }
            }
        }

        // === Double Indirect
        if (currInode->dip != 0) {
            for (int j = 0; j < nindirect; j++){
                __u32 singleIP = readIndirectBlock(fd, currInode->dip, blockSize, j*4); // IP points to block with more DPs

                if (singleIP == 0) continue; // Don't even bother with null pointers

                for (int k = 0; k < nindirect; k++){
                    __u32 directPointer = readIndirectBlock(fd, singleIP, blockSize, k*4); // DP points to block with more dir entries

                    if (directPointer == 0) continue; // Don't even bother with null pointers

                    if (checkBlock(currInode, directPointer, fd, sb, blockSize, token)) {
                        goto targetFound;
                    }
                }
            }
        }

        // === Triple Indirect
        if (currInode->tip != 0) {
            for (int j = 0; j < nindirect; j++){
                __u32 doubleIP = readIndirectBlock(fd, currInode->tip, blockSize, j*4); // IP points to block with more IPs

                if (doubleIP == 0) continue; // Don't even bother with null pointers

                for (int k = 0; k < nindirect; k++){
                    __u32 singleIP = readIndirectBlock(fd, doubleIP, blockSize, k*4); // IP points to block with even more IPs

                    if (singleIP == 0) continue; // Don't even bother with null pointers

                    for (int l = 0; l < nindirect; l++){
                        __u32 directPointer = readIndirectBlock(fd, singleIP, blockSize, l*4); // IP points to block with even more dir entries

                        if (directPointer == 0) continue; // Don't even bother with null pointers

                        if (checkBlock(currInode, directPointer, fd, sb, blockSize, token)) {
                            goto targetFound;
                        }
                    }
                }
            }
        }
        // Went through all DP and IPs, could not find target. Path must not exist. Return Error
        return -1;

targetFound: 
        token = strtok(NULL, "/");
    }

    __u16 objType = extractObjectType(*currInode);

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