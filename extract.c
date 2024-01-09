#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "defs.h"

int main() {
    /*
    EXT2 SUPERBLOCK PARSER
    LITTLE ENDIAN SYSTEM

    Boot Block (1024 bytes) -> Block Group 1 -> ... -> Block Group N

    BG1 => Superblock (1024 bytes) -> BGDT -> Data Block Bitmap -> Inode Bitmap
            -> Inode Table -> Data Blocks

    Note: Use sudo when having permission issues
    */

    superblock sb;

    int fd = open(FD_DEV, O_RDONLY);
    if (fd == -1) {
        perror("Error: Opening device failed");
        exit(EXIT_FAILURE);
    }

    // ===== Seek to the superblock position (skip 1024 bytes)
    printf("-----SUPERBLOCK INFO-----\n");
    if (lseek(fd, SUPERBLOCK_OFFSET, SEEK_SET) == -1) {
        perror("Error: Seeking to superblock failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    read(fd, &sb, sizeof(superblock));

    /*
    You are expected to parse the following pieces of information from the superblock:
        • Block size
        • Number of blocks per block group
        • Number of inodes per block group
        • Block number containing the starting address (of a copy) of the BGDT
    */

    if (sb.ext2_sig != EXT2_MAGIC_NUMBER) {
        perror("Error: Unrecognized filesystem");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("    Superblock Number: %d\n", sb.superblock_block_num);

    __u32 block_size = 1024 << sb.lg_block_size;

    __u32 partition_size = sb.total_blocks * block_size;

    __u32 total_block_groups = ceil(sb.total_blocks / sb.total_blocks_in_blockgroup + 1);


    printf("    Partition size: %d\n", partition_size);
    printf("    Total # of blocks: %d\n", sb.total_blocks);
    printf("    Total # of block groups: %d\n", total_block_groups);
    printf("    Total # of inodes: %d\n", sb.total_inodes);

    printf("    Block size: %d\n", block_size);
    printf("    # of blocks per block group: %d\n", sb.total_blocks_in_blockgroup); // FSR number is higher than total number of blocks
    printf("    inode size: %d\n", sb.inode_size);
    printf("    # of inodes per block group: %d\n", sb.total_inodes_in_blockgroup);
    printf("    # of inode blocks per block group: %d\n", (sb.total_inodes_in_blockgroup / (block_size / sb.inode_size)));

    // ===== Seek to the Block Group Descriptor Table position (skip 4096 bytes = 1 block)
    int bgdOffset = 0;
    printf("-----BGD ENTRY %d INFO-----\n", bgdOffset);

    blk_groupdesc bgdt[total_block_groups];

    if (lseek(fd, block_size + (bgdOffset * 32), SEEK_SET) == -1) {
        perror("Error: Seeking to BGDT failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    read(fd, &bgdt[bgdOffset], sizeof(blk_groupdesc));

    printf("    Block bitmap block address: %d\n", bgdt[bgdOffset].block_bitmap);
    printf("    inode bitmap block address: %d\n", bgdt[bgdOffset].inode_bitmap);
    printf("    inode table starting block address: %d\n", bgdt[bgdOffset].inode_table);
    printf("    Unallocated blocks: %d\n", bgdt[bgdOffset].total_unallocated_blocks);
    printf("    Total directories: %d\n", bgdt[bgdOffset].total_dirs);

    // ===== Find an inode
    int inodeNum = 2; // root
    inode inode;
    printf("-----INODE %d LOCATION-----\n", inodeNum);

    int blockGroup = (inodeNum - 1) / sb.total_inodes_in_blockgroup;
    int inodeIndex = (inodeNum - 1) % sb.total_inodes_in_blockgroup;
    int containingBlock = (inodeIndex * sb.inode_size) / block_size;

    printf("    Block group: %d\n", blockGroup);
    printf("    Index: %d\n", inodeIndex);
    printf("    Containing block: %d\n", containingBlock);

    // ===== Seek to the inode table position (skip 4096 bytes * 4 = 4 blocks) 
    printf("-----INODE %d INFO-----\n", inodeNum);

    if (lseek(fd, block_size * 4 + (inodeIndex * sb.inode_size), SEEK_SET) == -1) {
        perror("Error: Seeking to inode table failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    read(fd, &inode, sizeof(inode));

    printf("    Type and permissions: %x\n", inode.type_and_perm);
    printf("    File size (lo): %d\n", inode.lo_size);
    for (int i = 1; i <= 12; i++){
        printf("    Direct Pointer %d: %d\n", i, inode.dp[i-1]);
    }

    // ===== Read data block(s) pointed to by the direct pointer(s)
    // NOTE: Code below only for one direct pointer (from root inode which only has one pointer)
    printf("-----DATA BLOCK %d-----\n", inode.dp[0]);
    dir_entry directory_entry;

    int bytesParsed = 0;
    while (bytesParsed < block_size) {
        if (lseek(fd, block_size * inode.dp[0] + bytesParsed, SEEK_SET) == -1) {
            perror("Error: Seeking to data block failed");
            close(fd);
            exit(EXIT_FAILURE);
        }
        read(fd, &directory_entry, sizeof(dir_entry));
        printf("    inode number: %d\n", directory_entry.inode_num);
        printf("    Directory entry size: %d\n", directory_entry.size);
        printf("    Name size: %d\n", directory_entry.name_size);
        printf("    Directory entry name: %s\n", directory_entry.name);

        bytesParsed += directory_entry.size;
    }

    close(fd);

    return 0;
}