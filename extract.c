#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "defs.h"

#define DIRECTORY 0x4000
#define FILE 0x8000
#define MAX_PATH_LENGTH 4096
#define NDIRECT 12


superblock readSuperblock(int);
void readBGD(int, blk_groupdesc*, int, int);
inode readInode(int, int, superblock, int);
dir_entry readDirEntry(int, __u32, int, int);
__u16 extractObjectType(inode);
void parseBlock(__u32, int, superblock, int, char*, void (*traverseFunc)(inode, int, superblock, int, char*));
__u32 readIndirectBlock(int, __u32, int, int);
void traverseAllPaths(inode, int, superblock, int, char*);

/*
    EXT2 SUPERBLOCK PARSER
    LITTLE ENDIAN SYSTEM

    Boot Block (1024 bytes) -> Block Group 1 -> ... -> Block Group N

    BG1 => Superblock (1024 bytes) -> BGDT -> Data Block Bitmap -> Inode Bitmap
            -> Inode Table -> Data Blocks

    Note: Use sudo when having permission issues
*/

int main() {
    superblock sb;

    int fd = open(FD_DEV, O_RDONLY);
    if (fd == -1) {
        perror("[Error] Opening device failed\n");
        exit(EXIT_FAILURE);
    }

    sb = readSuperblock(fd);

    if (sb.ext2_sig != EXT2_MAGIC_NUMBER) {
        perror("[Error] Unrecognized filesystem\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    //printf("    Superblock Number: %d\n", sb.superblock_block_num);

    __u32 block_size = 1024 << sb.lg_block_size;
    __u32 partition_size = sb.total_blocks * block_size;
    __u32 total_block_groups = ceil(sb.total_blocks / sb.total_blocks_in_blockgroup + 1);

    /*
    printf("    Partition size: %d\n", partition_size);
    printf("    Total # of blocks: %d\n", sb.total_blocks);
    printf("    Total # of block groups: %d\n", total_block_groups);
    printf("    Total # of inodes: %d\n", sb.total_inodes);

    printf("    Block size: %d\n", block_size);
    printf("    # of blocks per block group: %d\n", sb.total_blocks_in_blockgroup); // FSR number is higher than total number of blocks
    printf("    inode size: %d\n", sb.inode_size);
    printf("    # of inodes per block group: %d\n", sb.total_inodes_in_blockgroup);
    printf("    # of inode blocks per block group: %d\n", (sb.total_inodes_in_blockgroup / (block_size / sb.inode_size)));
    */

    // ===== Seek to the Block Group Descriptor Table position (skip 4096 bytes = 1 block)
    int bgdOffset = 0;
    //printf("-----BGD ENTRY %d INFO-----\n", bgdOffset);
    blk_groupdesc* bgdt = (blk_groupdesc*) malloc(total_block_groups * sizeof(blk_groupdesc));
    readBGD(fd, bgdt, bgdOffset, block_size);

    /*
    printf("    Block bitmap block address: %d\n", bgdt[bgdOffset].block_bitmap);
    printf("    inode bitmap block address: %d\n", bgdt[bgdOffset].inode_bitmap);
    printf("    inode table starting block address: %d\n", bgdt[bgdOffset].inode_table);
    printf("    Unallocated blocks: %d\n", bgdt[bgdOffset].total_unallocated_blocks);
    printf("    Total directories: %d\n", bgdt[bgdOffset].total_dirs);
    */

    // ===== Find an inode
    inode rootinode;
    rootinode = readInode(2, fd, sb, block_size); // read root inode
    
    //! WARNING: Beyond this point are experimental attempts.
    // For now, traversal is limited to the first direct block
    char path[MAX_PATH_LENGTH] = "/";
    traverseAllPaths(rootinode, fd, sb, block_size, path);

    free(bgdt);

    close(fd);

    return 0;
}

superblock readSuperblock(int fd) {
    superblock sb;
    // ===== Seek to the superblock position (skip 1024 bytes)
    //printf("-----SUPERBLOCK INFO-----\n");
    if (lseek(fd, SUPERBLOCK_OFFSET, SEEK_SET) == -1) {
        perror("[Error] Seeking to superblock failed\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    read(fd, &sb, sizeof(superblock));
    return sb;
}

void readBGD(int fd, blk_groupdesc* bgdt, int bgdOffset, int blockSize) {
    if (lseek(fd, blockSize + (bgdOffset * 32), SEEK_SET) == -1) {
        perror("[Error] Seeking to BGDT failed\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    read(fd, &bgdt[bgdOffset], sizeof(blk_groupdesc));
}


inode readInode(int inodeNum, int fd, superblock sb, int blockSize) {
    // ===== Find an inode
    inode currInode;
    //printf("-----INODE %d LOCATION-----\n", inodeNum);

    int blockGroup = (inodeNum - 1) / sb.total_inodes_in_blockgroup;
    int inodeIndex = (inodeNum - 1) % sb.total_inodes_in_blockgroup;
    int containingBlock = (inodeIndex * sb.inode_size) / blockSize;

    //printf("    Block group: %d\n", blockGroup);
    //printf("    Index: %d\n", inodeIndex);
    //printf("    Containing block: %d\n", containingBlock);

    // ===== Seek to the inode table position (skip 4096 bytes * 4 = 4 blocks) 
    //printf("-----INODE %d INFO-----\n", inodeNum);

    if (lseek(fd, blockSize * 4 + (inodeIndex * sb.inode_size), SEEK_SET) == -1) {
        perror("[Error] Seeking to inode table failed\n");
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
        perror("[Error] Seeking to data block failed\n");
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

        //printf("    inode number: %d\n", directory_entry.inode_num);
        //printf("    Directory entry size: %d\n", directory_entry.size);
        //printf("    Name size: %d\n", directory_entry.name_size);
        //printf("    Directory entry name: %s\n", directory_entry.name);

        if (strcmp(directory_entry.name, ".") == 0 || strcmp(directory_entry.name, "..") == 0) {
            bytesParsed += directory_entry.size;
            continue; // Skip current and parent directory entries
        }

        inode nextInode = readInode(directory_entry.inode_num, fd, sb, blockSize);
        __u16 objType = extractObjectType(nextInode);

        char newPath[MAX_PATH_LENGTH];
        strncpy(newPath, path, MAX_PATH_LENGTH);
        strncat(newPath, directory_entry.name, directory_entry.name_size);

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
        perror("[Error] Seeking to indirect block failed\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    read(fd, &retPointer, sizeof(__u32));

    return retPointer;
}

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

