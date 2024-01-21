/*
    CS140 2324A PROJECT 2 - EXT2 FILESYSTEM READER
    Ivan Cassidy Cadiang (2021-12575)
    Diego Montenejo (2020-05984)
    Rohan Solas (2021-07292)
*/

// File contains function for path enumeration

#include "defs.h"

// Instead of passing the output path recursively through function calls, we mutate a single global path
char OUTPUT_PATH[MAX_PATH_LENGTH] = "/";

int enumDirEntries(__u32 blockPointer, int fd, superblock sb, int blockSize) {
    // "Directory entries are also not allowed to span multiple blocks" https://wiki.osdev.org/Ext2#Directory_Entry 
    int bytesParsed = 0;

    while (bytesParsed < blockSize) {
        dir_entry dirEntry = readDirEntry(fd, blockPointer, blockSize, bytesParsed);
        const char* dirName = (char*) dirEntry.name;
        
        // Skip current & parent directories, and skip null inodes
        if (strcmp(dirName, ".") == 0 || strcmp(dirName, "..") == 0 || dirEntry.inode_num == 0) {
            bytesParsed += dirEntry.size;
            continue; 
        }

        inode nextInode = readInode(dirEntry.inode_num, fd, sb, blockSize);
        __u16 objType = extractObjectType(nextInode);

        int oldLen = strlen(OUTPUT_PATH);
        strncat(OUTPUT_PATH, dirName, dirEntry.name_size);

        switch (objType) {
            case DIRECTORY:
                strncat(OUTPUT_PATH, "/", 2);
                enumDirectory(nextInode, fd, sb, blockSize);
                break;
            case FILE_:
                printf("%s\n", OUTPUT_PATH);
                break;
            default:
                break;
        }

        OUTPUT_PATH[oldLen] = '\0'; // After directory/file has been completely traversed, we need to be able to "move out" of the path
        bytesParsed += dirEntry.size;
    }

    return 0;
}

void enumDirectory(inode currInode, int fd, superblock sb, int blockSize) {
    printf("%s\n", OUTPUT_PATH);
    readPointers(currInode, fd, sb, blockSize, enumDirEntries);
}   