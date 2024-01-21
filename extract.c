/*
    CS140 2324A PROJECT 2 - EXT2 FILESYSTEM READER
    Ivan Cassidy Cadiang (2021-12575)
    Diego Montenejo (2020-05984)
    Rohan Solas (2021-07292)
*/
#include <unistd.h>
#include <sys/stat.h>
#include "defs.h"

void parseBlock2_ext(__u32 blockPointer, int fd, superblock sb, int blockSize, char* buildPath){
    // "Directory entries are also not allowed to span multiple blocks" https://wiki.osdev.org/Ext2#Directory_Entry 

    dir_entry directory_entry;

    int bytesParsed = 0;
    while (bytesParsed < blockSize) {

        directory_entry = readDirEntry(fd, blockPointer, blockSize, bytesParsed);

        const char* dirName = (char*) directory_entry.name;
        
        if (strcmp(dirName, ".") == 0 || strcmp(dirName, "..") == 0 || directory_entry.inode_num == 0) {
            bytesParsed += directory_entry.size;
            continue; // Skip current and parent directory entries
        }

        inode nextInode = readInode(directory_entry.inode_num, fd, sb, blockSize);
        __u16 objType = extractObjectType(nextInode);

        switch (objType) {
            case DIRECTORY:
                // strncat(newPath, "/", 2);
                extractAllPaths(nextInode, directory_entry, fd, sb, blockSize, buildPath, 0);
                break;
            case FILE_:
                extractSinglePath(nextInode, directory_entry, fd, sb, blockSize, buildPath);
                break;
            default:
                break;
        }

        bytesParsed += directory_entry.size;
    }
}

void copyBlockToFile(int fd, __u32 dp, FILE** fp, int* buffer, int blockSize) {
    lseek(fd, blockSize * dp, SEEK_SET);
    read(fd, buffer, blockSize);

    fwrite((char*)buffer, sizeof(char), strlen((char*)buffer), *fp);
}

// File contains function for File Extraction
void extractSinglePath(inode currInode, dir_entry targetDir, int fd, superblock sb, int blockSize, char* destPath) {
    int readBuffer[blockSize];

    char dest[MAX_PATH_LENGTH] = "";
    strcpy(dest, destPath);
    strcat(dest, (char*)targetDir.name);

    FILE* fp = fopen(dest, "w");
    
    // === Direct
    for (int i = 0; i < NDIRECT; i++){
        if (currInode.dp[i] == 0) continue; // Don't even bother with null pointers

        copyBlockToFile(fd, currInode.dp[i], &fp, readBuffer, blockSize);
    }

    __u32 nindirect = blockSize / sizeof(__u32);

    // === Single Indirect
    if (currInode.sip != 0) { 
        for (int j = 0; j < nindirect; j++){
            __u32 directPointer = readIndirectBlock(fd, currInode.sip, blockSize, j*4); // DP points to block with more directory entries

            if (directPointer == 0) continue; // Don't even bother with null pointers

            copyBlockToFile(fd, directPointer, &fp, readBuffer, blockSize);
        }
    }

    // === Double Indirect
    if (currInode.dip != 0) {
        for (int j = 0; j < nindirect; j++){
            __u32 singleIP = readIndirectBlock(fd, currInode.dip, blockSize, j*4); // IP points to block with more DPs

            if (singleIP == 0) continue; // Don't even bother with null pointers

            for (int k = 0; k < nindirect; k++){
                __u32 directPointer = readIndirectBlock(fd, singleIP, blockSize, k*4); // DP points to block with more dir entries

                if (directPointer == 0) continue; // Don't even bother with null pointers

                copyBlockToFile(fd, directPointer, &fp, readBuffer, blockSize);
            }
        }
    }

    // === Triple Indirect
    if (currInode.tip != 0) {
        for (int j = 0; j < nindirect; j++){
            __u32 doubleIP = readIndirectBlock(fd, currInode.tip, blockSize, j*4); // IP points to block with more IPs

            if (doubleIP == 0) continue; // Don't even bother with null pointers

            for (int k = 0; k < nindirect; k++){
                __u32 singleIP = readIndirectBlock(fd, doubleIP, blockSize, k*4); // IP points to block with even more IPs

                if (singleIP == 0) continue; // Don't even bother with null pointers

                for (int l = 0; l < nindirect; l++){
                    __u32 directPointer = readIndirectBlock(fd, singleIP, blockSize, l*4); // IP points to block with even more dir entries

                    if (directPointer == 0) continue; // Don't even bother with null pointers

                    copyBlockToFile(fd, directPointer, &fp, readBuffer, blockSize);
                }
            }
        }
    }

    fclose(fp);
}

// Extracts all the subfiles/directories in a given directory
void extractAllPaths(inode currInode, dir_entry targetDir, int fd, superblock sb, int blockSize, char* destPath, int isRoot) {
    char buildPath[MAX_PATH_LENGTH] = "output";

    if (!isRoot) {
        strcpy(buildPath, destPath);
        strcat(buildPath, (char*)targetDir.name);
    }

    mkdir(buildPath, 0777);
    strcat(buildPath, "/");

    // === Direct
    for (int i = 0; i < NDIRECT; i++) {
        if (currInode.dp[i] == 0) continue; // Don't even bother with null pointers

        parseBlock2_ext(currInode.dp[i], fd, sb, blockSize, buildPath);   
    }

    __u32 nindirect = blockSize / sizeof(__u32);

    // === Single Indirect
    if (currInode.sip != 0) { 
        for (int j = 0; j < nindirect; j++){
            __u32 directPointer = readIndirectBlock(fd, currInode.sip, blockSize, j*4); // DP points to block with more directory entries

            if (directPointer == 0) continue; // Don't even bother with null pointers

            parseBlock2_ext(directPointer, fd, sb, blockSize, buildPath);
        }
    }

    // === Double Indirect
    if (currInode.dip != 0) {
        for (int j = 0; j < nindirect; j++){
            __u32 singleIP = readIndirectBlock(fd, currInode.dip, blockSize, j*4); // IP points to block with more DPs

            if (singleIP == 0) continue; // Don't even bother with null pointers

            for (int k = 0; k < nindirect; k++){
                __u32 directPointer = readIndirectBlock(fd, singleIP, blockSize, k*4); // DP points to block with more dir entries

                if (directPointer == 0) continue; // Don't even bother with null pointers

                parseBlock2_ext(directPointer, fd, sb, blockSize, buildPath);
            }
        }
    }

    // === Triple Indirect
    if (currInode.tip != 0) {
        for (int j = 0; j < nindirect; j++){
            __u32 doubleIP = readIndirectBlock(fd, currInode.tip, blockSize, j*4); // IP points to block with more IPs

            if (doubleIP == 0) continue; // Don't even bother with null pointers

            for (int k = 0; k < nindirect; k++){
                __u32 singleIP = readIndirectBlock(fd, doubleIP, blockSize, k*4); // IP points to block with even more IPs

                if (singleIP == 0) continue; // Don't even bother with null pointers

                for (int l = 0; l < nindirect; l++){
                    __u32 directPointer = readIndirectBlock(fd, singleIP, blockSize, l*4); // IP points to block with even more dir entries

                    if (directPointer == 0) continue; // Don't even bother with null pointers

                    parseBlock2_ext(directPointer, fd, sb, blockSize, buildPath);
                }
            }
        }
    }
}


// Checks the block pointer if the target directory or file is present within it
//!     IMPORTANT: function modifies the currInode & targetDir argument passed to it 
int checkBlock(inode* currInode, dir_entry* targetDir, __u32 blockPointer, int fd, superblock sb, int blockSize, char* target){
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
            *targetDir = directory_entry;
            return 1;
        }

        bytesParsed += directory_entry.size;
    }

    return 0;
}

//!     IMPORTANT: function modifies the currInode & targetDir argument passed to it
int searchForTarget(inode* currInode, dir_entry* targetDir, int fd, superblock sb, int blockSize, char path[]) {
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

            if (checkBlock(currInode, targetDir, currInode->dp[i], fd, sb, blockSize, token)) {
                goto targetFound;
            }
        }

        __u32 nindirect = blockSize / sizeof(__u32);

        // === Single Indirect
        if (currInode->sip != 0) {
            for (int j = 0; j < nindirect; j++){
                __u32 directPointer = readIndirectBlock(fd, currInode->sip, blockSize, j*4); // DP points to block with more directory entries

                if (directPointer == 0) continue; // Don't even bother with null pointers

                if (checkBlock(currInode,  targetDir, directPointer, fd, sb, blockSize, token)) {
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

                    if (checkBlock(currInode, targetDir, directPointer, fd, sb, blockSize, token)) {
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

                        if (checkBlock(currInode,  targetDir, directPointer, fd, sb, blockSize, token)) {
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