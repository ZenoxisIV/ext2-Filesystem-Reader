#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "defs.h"

int main() {
    /*
    EXT2 SUPERBLOCK PARSER
    LITTLE ENDIAN SYSTEM

    Boot Block (1024 bytes) -> Block Group 1 -> ... -> Block Group N

    BG1 => Superblock (1024 bytes) -> Group Descriptors -> Data Block Bitmap -> Inode Bitmap
            -> Inode Table -> Data Blocks

    Note: Use sudo when having permission issues
    */

    superblock sb;

    int fd = open(FD_DEV, O_RDONLY);
    if (fd == -1) {
        perror("Error: Opening device failed");
        exit(EXIT_FAILURE);
    }

    // Seek to the superblock position (skip 1024 bytes)
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

    printf("Superblock Number: %d\n", sb.superblock_block_num);

    __u32 block_size = 1024 << sb.lg_block_size;
    printf("Block Size: %d\n", block_size); // Block size
    printf("Number of blocks per block group: %d\n", sb.total_blocks_in_blockgroup);
    printf("Number of inodes per block group: %d\n", sb.total_inodes_in_blockgroup);

    // Seek to the Block Group Descriptor Table position (skip 1024 bytes) 
    blk_groupdesc_tbl bgdt;

    if (lseek(fd, block_size, SEEK_SET) == -1) {
        perror("Error: Seeking to BGDT failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    read(fd, &bgdt, sizeof(blk_groupdesc_tbl));

    printf("Block bitmap: %d\n", bgdt.block_bitmap);
    printf("inode bitmap %d\n", bgdt.inode_bitmap);
    printf("inode table: %d\n", bgdt.inode_table);
    printf("unalloc blocks: %d\n", bgdt.total_unallocated_blocks);

    close(fd);

    return 0;
}