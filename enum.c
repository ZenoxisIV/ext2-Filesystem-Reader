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

void enumDirEntries(__u32 blockPointer, int fd, superblock sb, int blockSize) {
    // "Directory entries are also not allowed to span multiple blocks" https://wiki.osdev.org/Ext2#Directory_Entry 
    int bytesParsed = 0;

    while (bytesParsed < blockSize) {
        dir_entry directory_entry = readDirEntry(fd, blockPointer, blockSize, bytesParsed);
        const char* dirName = (char*) directory_entry.name;
        
        // Skip current & parent directories, and skip null inodes
        if (strcmp(dirName, ".") == 0 || strcmp(dirName, "..") == 0 || directory_entry.inode_num == 0) {
            bytesParsed += directory_entry.size;
            continue; 
        }

        inode nextInode = readInode(directory_entry.inode_num, fd, sb, blockSize);
        __u16 objType = extractObjectType(nextInode);

        int oldLen = strlen(OUTPUT_PATH);
        strncat(OUTPUT_PATH, dirName, directory_entry.name_size);

        switch (objType) {
            case DIRECTORY:
                strncat(OUTPUT_PATH, "/", 2);
                printf("%s\n", OUTPUT_PATH);

                enumDirectory(nextInode, fd, sb, blockSize);
                break;
            case FILE_:
                printf("%s\n", OUTPUT_PATH);
                break;
            default:
                break;
        }

        OUTPUT_PATH[oldLen] = '\0'; // After directory/file has been completely traversed, we need to be able to "move out" of the path
        bytesParsed += directory_entry.size;
    }
}

void enumDirectory(inode currInode, int fd, superblock sb, int blockSize) {
    readPointers(currInode, fd, sb, blockSize, enumDirEntries);
}   