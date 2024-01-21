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
    if (lseek(fd, SUPERBLOCK_OFFSET, SEEK_SET) == -1) {
        perror("[Error] Seeking to superblock failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    read(fd, &sb, sizeof(superblock));
    return sb;
}

blk_groupdesc readBGD(int fd, int blockSize, int blockGroup) {
    blk_groupdesc bgd;

    int bgdtOffset = blockGroup * sizeof(blk_groupdesc);
    const int bgdtLoc = blockSize == 1024 ? 2 : 1;

    if (lseek(fd, blockSize * bgdtLoc + bgdtOffset, SEEK_SET) == -1) {
        perror("[Error] Seeking to BGDT failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    read(fd, &bgd, sizeof(blk_groupdesc));

    return bgd;
}

inode readInode(int inodeNum, int fd, superblock sb, int blockSize) {
    // ===== Find an inode
    inode currInode;
    
    int blockGroup = (inodeNum - 1) / sb.total_inodes_in_blockgroup;
    int inodeIndex = (inodeNum - 1) % sb.total_inodes_in_blockgroup;

    blk_groupdesc bgd = readBGD(fd, blockSize, blockGroup);
    int inodeTableStartBlock = bgd.inode_table;

    if (lseek(fd, blockSize * inodeTableStartBlock + (inodeIndex * sb.inode_size), SEEK_SET) == -1) {
        perror("[Error] Seeking to inode table failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    read(fd, &currInode, sizeof(currInode));

    return currInode;
}

dir_entry readDirEntry(int fd, __u32 dp, int blockSize, int bytesParsed) {
    dir_entry entry;

    if (lseek(fd, blockSize * dp + bytesParsed, SEEK_SET) == -1) {
        perror("[Error] Seeking to data block failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

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

int readPointers(inode currInode, int fd, superblock sb, int blockSize, int(blockAction)(__u32, int, superblock, int)) {
    int ret; // If the blockAction returns a value != 0, then readPointers returns that ret value

    // === Direct
    for (int i = 0; i < NDIRECT; i++){
        if (currInode.dp[i] == 0) continue; // Don't even bother with null pointers
        
        if ((ret = blockAction(currInode.dp[i], fd, sb, blockSize)) != 0) {
            return ret;
        }
    }

    __u32 nindirect = blockSize / sizeof(__u32);

    // === Single Indirect
    if (currInode.sip != 0) { 
        for (int j = 0; j < nindirect; j++){
            __u32 directPointer = readIndirectBlock(fd, currInode.sip, blockSize, j*4); // DP points to block with more directory entries

            if (directPointer == 0) continue; // Don't even bother with null pointers

            if ((ret = blockAction(directPointer, fd, sb, blockSize)) != 0) {
                return ret;
            }
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

                if ((ret = blockAction(directPointer, fd, sb, blockSize)) != 0) {
                    return ret;
                }
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

                    if ((ret = blockAction(directPointer, fd, sb, blockSize)) != 0) {
                        return ret;
                    }
                }
            }
        }
    }

    return 0;
}