/*
    CS140 2324A PROJECT 2 - EXT2 FILESYSTEM READER
    Ivan Cassidy Cadiang (2021-12575)
    Diego Montenejo (2020-05984)
    Rohan Solas (2021-07292)
*/

#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include "defs.h"

// Debug Lines. Set these to 1 depending on what you want to display
#define SB_DBG      0   // Superblock
#define BGDT_DBG    0   // Block Group Descriptor Table


/*
    EXT2 SUPERBLOCK PARSER
    LITTLE ENDIAN SYSTEM

    Boot Block (1024 bytes) -> Block Group 1 -> ... -> Block Group N

    BG1 => Superblock (1024 bytes) -> BGDT -> Data Block Bitmap -> Inode Bitmap
            -> Inode Table -> Data Blocks

    Note: Use sudo when having permission issues
*/

int main(int argc, char* argv[]) {
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("[Error] Opening device failed");
        exit(EXIT_FAILURE);
    }

    superblock sb = readSuperblock(fd);

    if (sb.ext2_sig != EXT2_MAGIC_NUMBER) {
        perror("[Error] Unrecognized filesystem");
        close(fd);
        exit(EXIT_FAILURE);
    }

    __u32 block_size = 1024 << sb.lg_block_size;
    __u32 total_block_groups = ceil(sb.total_blocks / sb.total_blocks_in_blockgroup + 1);

    if (SB_DBG) {
        printf("    Superblock Number: %d\n", sb.superblock_block_num);
        printf("    Partition size: %d\n", sb.total_blocks * block_size);
        printf("    Total # of blocks: %d\n", sb.total_blocks);
        printf("    Total # of block groups: %d\n", total_block_groups);
        printf("    Total # of inodes: %d\n", sb.total_inodes);

        printf("    Block size: %d\n", block_size);
        printf("    # of blocks per block group: %d\n", sb.total_blocks_in_blockgroup); // FSR number is higher than total number of blocks
        printf("    inode size: %d\n", sb.inode_size);
        printf("    # of inodes per block group: %d\n", sb.total_inodes_in_blockgroup);
        printf("    # of inode blocks per block group: %d\n", (sb.total_inodes_in_blockgroup / (block_size / sb.inode_size)));
    }


    // ===== Seek to the Block Group Descriptor Table position (skip 4096 bytes = 1 block)
    //! Commented BGDT Iteration because it doesn't seem to be needed?
    blk_groupdesc* bgdt = (blk_groupdesc*) malloc(total_block_groups * sizeof(blk_groupdesc));
    for (int bgdOffset = 0; bgdOffset < total_block_groups; bgdOffset++) {
        readBGD(fd, bgdt, bgdOffset, block_size);

        if (!BGDT_DBG) continue;
        printf("\n-----BGD ENTRY %d INFO-----\n", bgdOffset);
        printf("    Block bitmap block address: %d\n", bgdt[bgdOffset].block_bitmap);
        printf("    inode bitmap block address: %d\n", bgdt[bgdOffset].inode_bitmap);
        printf("    inode table starting block address: %d\n", bgdt[bgdOffset].inode_table);
        printf("    Unallocated blocks: %d\n", bgdt[bgdOffset].total_unallocated_blocks);
        printf("    Total directories: %d\n", bgdt[bgdOffset].total_dirs);
    }

    inode rootinode;

    char path[MAX_PATH_LENGTH];

    switch (argc) {
        case 2:
            // **OP 1: PATH ENUMERATION       (No additional arguments)
            //! BGDT TRAVERSAL USED TO BE HERE.
            // IF anything fails, try putting the BGDT traversal loop back here and then
            //     put traverseAllPaths() inside the loop

            // ===== Find an inode
            rootinode = readInode(2, fd, sb, block_size); // read root inode
            
            strcpy(path, "/");
            enumAllPaths(rootinode, fd, sb, block_size, path);  
            // --------------------------------------------------
            break;
        case 3:
            if (!isAbsolutePath(argv[2])) {
                fprintf(stderr, "INVALID PAT\n");
                close(fd);
                free(bgdt);
                return -1;
            }

            // **OP 2: FILESYSTEM EXTRACTION  (Additional argument given)
            recreatePath(argv[2]); // clean path for easier search
            strcpy(path, argv[2]);

            rootinode = readInode(2, fd, sb, block_size);
            int targetType;
            
            if ((targetType = searchForTarget(&rootinode, fd, sb, block_size, path)) == -1) {
                fprintf(stderr, "INVALID PATH\n");
                close(fd);
                free(bgdt);
                return -1;
            } else if (targetType == 1) {
                //* Target was found successfully and is a File
                //! IMPORTANT: searchForTarget modifies the currInode argument passed to it


            } else if (targetType == 2) {
                //* Target was found successfully and is a Directory
                //! IMPORTANT: searchForTarget modifies the currInode argument passed to it

            }
            
            // --------------------------------------------------
            break;
        default:
            printf("Too many arguments supplied.");
            break;
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
    close(fd);
    free(bgdt);
    return 0;
}