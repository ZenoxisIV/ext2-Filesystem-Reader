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

inode readInode(int inodeNum, int fd, superblock sb, int blockSize) {
    // ===== Find an inode
    inode currInode;
    blk_groupdesc bgd;
    // printf("-----INODE %d LOCATION-----\n", inodeNum);

    int blockGroup = (inodeNum - 1) / sb.total_inodes_in_blockgroup;
    int inodeIndex = (inodeNum - 1) % sb.total_inodes_in_blockgroup;
    // int containingBlock = (inodeIndex * sb.inode_size) / blockSize;

   if (blockSize == 1024) {
    if (lseek(fd, blockSize * 2 + (blockGroup * sizeof(blk_groupdesc)), SEEK_SET) == -1) {
        perror("[Error] Seeking to BGDT failed");
        close(fd);
        exit(EXIT_FAILURE);
    }
   } else {
    if (lseek(fd, blockSize * 1 + (blockGroup * sizeof(blk_groupdesc)), SEEK_SET) == -1) {
        perror("[Error] Seeking to BGDT failed");
        close(fd);
        exit(EXIT_FAILURE);
    }
   }

    read(fd, &bgd, sizeof(blk_groupdesc));

    int inodeTableStartBlock = bgd.inode_table;

    // printf("    Block group: %d\n", blockGroup);
    // printf("    Index: %d\n", inodeIndex);
    // printf("    Containing block: %d\n", containingBlock);

    // ===== Seek to the inode table position (skip 4096 bytes * 4 = 4 blocks) 
    // printf("-----INODE %d INFO-----\n", inodeNum);

    if (lseek(fd, blockSize * inodeTableStartBlock + (inodeIndex * sb.inode_size), SEEK_SET) == -1) {
        perror("[Error] Seeking to inode table failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    read(fd, &currInode, sizeof(currInode));


    // printf("    Type and permissions: %x\n", currInode.type_and_perm);
    // printf("    File size (lo): %d\n", currInode.lo_size);
    // for (int i = 1; i <= 12; i++){
    //     printf("    Direct Pointer %d: %d\n", i, currInode.dp[i-1]);
    // }

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

void readPointers(inode currInode, int fd, superblock sb, int blockSize, void(blockAction)(__u32, int, superblock, int)) {
    // === Direct
    for (int i = 0; i < NDIRECT; i++){
        if (currInode.dp[i] == 0) continue; // Don't even bother with null pointers
        
        blockAction(currInode.dp[i], fd, sb, blockSize);
        // readDirEntries_enum(currInode.dp[i], fd, sb, blockSize, path);
    }

    __u32 nindirect = blockSize / sizeof(__u32);

    // === Single Indirect
    if (currInode.sip != 0) { 
        for (int j = 0; j < nindirect; j++){
            __u32 directPointer = readIndirectBlock(fd, currInode.sip, blockSize, j*4); // DP points to block with more directory entries

            if (directPointer == 0) continue; // Don't even bother with null pointers

            blockAction(directPointer, fd, sb, blockSize);
            // readDirEntries_enum(directPointer, fd, sb, blockSize, path);
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

                blockAction(directPointer, fd, sb, blockSize);
                // readDirEntries_enum(directPointer, fd, sb, blockSize, path);
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

                    blockAction(directPointer, fd, sb, blockSize);
                    // readDirEntries_enum(directPointer, fd, sb, blockSize, path);
                }
            }
        }
    }
}