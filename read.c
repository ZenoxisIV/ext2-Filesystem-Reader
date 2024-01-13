/*
    CS140 2324A PROJECT 2 - EXT2 FILESYSTEM READER
    Ivan Cassidy Cadiang (2021-12575)
    Diego Montenejo (2020-05984)
    Rohan Solas (2021-07292)
*/

// File contains functions for reading special blocks of ext2

#include <fcntl.h>
#include <unistd.h>
#include "defs.h"

superblock readSuperblock(int fd) {
    superblock sb;
    // ===== Seek to the superblock position (skip 1024 bytes)
    //printf("-----SUPERBLOCK INFO-----\n");
    if (lseek(fd, SUPERBLOCK_OFFSET, SEEK_SET) == -1) {
        perror("[Error] Seeking to superblock failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    read(fd, &sb, sizeof(superblock));
    return sb;
}

void readBGD(int fd, blk_groupdesc* bgdt, int bgdOffset, int blockSize) {
    if (lseek(fd, blockSize + (bgdOffset * 32), SEEK_SET) == -1) {
        perror("[Error] Seeking to BGDT failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    read(fd, &bgdt[bgdOffset], sizeof(blk_groupdesc));
}

inode readInode(int inodeNum, int fd, superblock sb, int blockSize) {
    // ===== Find an inode
    inode currInode;
    //printf("-----INODE %d LOCATION-----\n", inodeNum);

    //int blockGroup = (inodeNum - 1) / sb.total_inodes_in_blockgroup;
    int inodeIndex = (inodeNum - 1) % sb.total_inodes_in_blockgroup;
    //int containingBlock = (inodeIndex * sb.inode_size) / blockSize;

    //printf("    Block group: %d\n", blockGroup);
    //printf("    Index: %d\n", inodeIndex);
    //printf("    Containing block: %d\n", containingBlock);

    // ===== Seek to the inode table position (skip 4096 bytes * 4 = 4 blocks) 
    //printf("-----INODE %d INFO-----\n", inodeNum);

    if (lseek(fd, blockSize * 4 + (inodeIndex * sb.inode_size), SEEK_SET) == -1) {
        perror("[Error] Seeking to inode table failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    read(fd, &currInode, sizeof(currInode));

    /*
    printf("    Type and permissions: %x\n", inode.type_and_perm);
    printf("    File size (lo): %d\n", inode.lo_size);
    for (int i = 1; i <= 12; i++){
        printf("    Direct Pointer %d: %d\n", i, inode.dp[i-1]);
    }
    */

    return currInode;
}

dir_entry readDirEntry(int fd, __u32 dp, int blockSize, int bytesParsed) {
    dir_entry entry;

    if (lseek(fd, blockSize * dp + bytesParsed, SEEK_SET) == -1) {
        perror("[Error] Seeking to data block failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // ===== Read data block(s) pointed to by the direct pointer(s)
    // NOTE: Code below only for one direct pointer (from root inode which only has one pointer)
    //printf("-----DATA BLOCK %d-----\n", inode.dp[0]);
    read(fd, &entry, sizeof(dir_entry));

    return entry;
}

__u16 extractObjectType(inode currInode) {
    return currInode.type_and_perm & 0xF000; // Extract type
}

void parseBlock(__u32 blockPointer, int fd, superblock sb, int blockSize, char path[], void (*traverseFunc)(inode, int, superblock, int, char*)){
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

        if (strcmp(dirName, ".") == 0 || strcmp(dirName, "..") == 0) {
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
                traverseFunc(nextInode, fd, sb, blockSize, newPath);
                break;
            case FILE:
                printf("%s\n", newPath);
                break;
            default:
                // printf("Warning: Unknown object found\n");
                break;
        }

        bytesParsed += directory_entry.size;
    }
}

__u32 readIndirectBlock(int fd, __u32 blockPointer, int blockSize, int blockOffset) {
    __u32 retPointer;

    if (lseek(fd, blockSize * blockPointer + blockOffset, SEEK_SET) == -1) {
        perror("[Error] Seeking to indirect block failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    read(fd, &retPointer, sizeof(__u32));

    return retPointer;
}