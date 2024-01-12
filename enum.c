#include "defs.h"

void traverseAllPaths(inode currInode, int fd, superblock sb, int blockSize, char path[]) {
    printf("%s\n", path);

    // === Direct
    for (int i = 0; i < NDIRECT; i++){
        if (currInode.dp[i] == 0) continue; // Don't even bother with null pointers

        parseBlock(currInode.dp[i], fd, sb, blockSize, path, traverseAllPaths);
    }

    __u32 nindirect = blockSize / sizeof(__u32);

    // === Single Indirect
    if (currInode.sip != 0) {
        for (int j = 0; j < nindirect; j++){
            __u32 directPointer = readIndirectBlock(fd, currInode.sip, blockSize, j*4); // DP points to block with more directory entries

            if (directPointer == 0) continue; // Don't even bother with null pointers

            parseBlock(directPointer, fd, sb, blockSize, path, traverseAllPaths);
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

                parseBlock(directPointer, fd, sb, blockSize, path, traverseAllPaths);
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

                    parseBlock(directPointer, fd, sb, blockSize, path, traverseAllPaths);
                }
            }
        }
    }
}