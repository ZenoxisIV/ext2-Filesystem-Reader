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
    char dev[] = "/dev/loop20"; // replace with the appropriate device

    int fd = open(dev, O_RDONLY);
    if (fd == -1) {
        perror("Error: Opening block device failed");
        return -1;
    }

    // Seek to the superblock position (skip 1024 bytes)
    if (lseek(fd, 1024, SEEK_SET) == -1) {
        perror("Error: Seeking to superblock failed");
        close(fd);
        return -1;
    }
    
    read(fd, &sb, sizeof(superblock));

    /*
    You are expected to parse the following pieces of information from the superblock:
        • Block size
        • Number of blocks per block group
        • Number of inodes per block group
        • Block number containing the starting address (of a copy) of the BGDT
    */

    printf("ext2 Magic Number: %x\n", sb.ext2_sig); // ext2 Signature (must be 0xEF53)

    __u32 block_size = 1024 << sb.block_size;
    printf("Block Size: %d\n", block_size); // Block size
    printf("Number of blocks per block group: %d\n", sb.total_blocks_in_blockgroup);
    printf("Number of inodes per block group: %d\n", sb.total_inodes_in_blockgroup);

    close(fd);

    return 0;
}