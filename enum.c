/*
    CS140 2324A PROJECT 2 - EXT2 FILESYSTEM READER
    Ivan Cassidy Cadiang (2021-12575)
    Diego Montenejo (2020-05984)
    Rohan Solas (2021-07292)
*/

// File contains function for path enumeration

#include "defs.h"

void readDirEntries_enum(__u32 blockPointer, int fd, superblock sb, int blockSize, char path[]){
    // "Directory entries are also not allowed to span multiple blocks" https://wiki.osdev.org/Ext2#Directory_Entry 

    dir_entry directory_entry;

    int bytesParsed = 0;
    while (bytesParsed < blockSize) {
        directory_entry = readDirEntry(fd, blockPointer, blockSize, bytesParsed);

        const char* dirName = (char*) directory_entry.name;

        //printf("    inode number: %d\n", directory_entry.inode_num);
        //printf("    Directory entry size: %d\n", directory_entry.size);
        //printf("    Name size: %d\n", directory_entry.name_size);
        //printf("    Directory entry name: %s\n", directory_entry.name);
        
        if (strcmp(dirName, ".") == 0 || strcmp(dirName, "..") == 0 || directory_entry.inode_num == 0) {
            bytesParsed += directory_entry.size;
            continue; // Skip current and parent directory entries
        }

        inode nextInode = readInode(directory_entry.inode_num, fd, sb, blockSize);
        __u16 objType = extractObjectType(nextInode);

        char newPath[MAX_PATH_LENGTH];
        strncpy(newPath, path, MAX_PATH_LENGTH);
        strncat(newPath, dirName, directory_entry.name_size);

        switch (objType) {
            case DIRECTORY:
                strncat(newPath, "/", 2);
                enumAllPaths(nextInode, fd, sb, blockSize, newPath);
                break;
            case FILE_:
                printf("%s\n", newPath);
                break;
            default:
                // printf("Warning: Unknown object found\n");
                break;
        }
        
        bytesParsed += directory_entry.size;
    }
}

void enumAllPaths(inode currInode, int fd, superblock sb, int blockSize, char path[]) {
    printf("%s\n", path);

    // === Direct
    for (int i = 0; i < NDIRECT; i++){
        if (currInode.dp[i] == 0) continue; // Don't even bother with null pointers

        readDirEntries_enum(currInode.dp[i], fd, sb, blockSize, path);
    }

    __u32 nindirect = blockSize / sizeof(__u32);

    // === Single Indirect
    if (currInode.sip != 0) { 
        for (int j = 0; j < nindirect; j++){
            __u32 directPointer = readIndirectBlock(fd, currInode.sip, blockSize, j*4); // DP points to block with more directory entries

            if (directPointer == 0) continue; // Don't even bother with null pointers

            readDirEntries_enum(directPointer, fd, sb, blockSize, path);
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

                readDirEntries_enum(directPointer, fd, sb, blockSize, path);
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

                    readDirEntries_enum(directPointer, fd, sb, blockSize, path);
                }
            }
        }
    }
}