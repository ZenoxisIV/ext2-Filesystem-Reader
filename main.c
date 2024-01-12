#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include "defs.h"

/*
    EXT2 SUPERBLOCK PARSER
    LITTLE ENDIAN SYSTEM

    Boot Block (1024 bytes) -> Block Group 1 -> ... -> Block Group N

    BG1 => Superblock (1024 bytes) -> BGDT -> Data Block Bitmap -> Inode Bitmap
            -> Inode Table -> Data Blocks

    Note: Use sudo when having permission issues
*/

int main(int argc, char* argv[]) {
    if (argv[2] == NULL) {
        recreatePath(argv[1]);

        superblock sb;

        int fd = open(argv[1], O_RDONLY);
        if (fd == -1) {
            perror("[Error] Opening device failed");
            exit(EXIT_FAILURE);
        }

        sb = readSuperblock(fd);

        if (sb.ext2_sig != EXT2_MAGIC_NUMBER) {
            perror("[Error] Unrecognized filesystem");
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
    }

    /*
    char path1[] = "/dir1/cs153.txt";
    char path2[] = "/dir1/./././cs153.txt";
    char path3[] = "/./dir1/cs153.txt";
    char path4[] = "/../dir1/cs153.txt";
    char path5[] = "/../../dir1/../dir1/./////cs153.txt";
    char path6[] = "/dir2/directory name with spaces/../../dir1/cs153.txt";
    
    recreatePath(path1);
    recreatePath(path2);
    recreatePath(path3);
    recreatePath(path4);
    recreatePath(path5);
    recreatePath(path6);

    printf("%s\n", path1);
    printf("%s\n", path2);
    printf("%s\n", path3);
    printf("%s\n", path4);
    printf("%s\n", path5);
    printf("%s\n", path6);
    */

    return 0;
}