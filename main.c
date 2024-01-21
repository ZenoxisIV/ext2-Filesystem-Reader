/*
    CS140 2324A PROJECT 2 - EXT2 FILESYSTEM READER
    Ivan Cassidy Cadiang (2021-12575)
    Diego Montenejo (2020-05984)
    Rohan Solas (2021-07292)
*/

#include <fcntl.h>
#include <unistd.h>
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
    //__u32 total_block_groups = (__u32) ceiling(((double) sb.total_blocks) / sb.total_blocks_in_blockgroup);

    inode rootInode;

    char path[MAX_PATH_LENGTH];
    
    if (argc == 2) {
        // **OP 1: PATH ENUMERATION (No additional arguments)
        rootInode = readInode(2, fd, sb, block_size); // read root inode
        
        strcpy(path, "/");
        enumDirectory(rootInode, fd, sb, block_size);  
        // --------------------------------------------------
    } else if (argc >= 3) {
        if (!isAbsolutePath(argv[2])) {
            fprintf(stderr, "INVALID PATH\n");
            close(fd);
            return -1;
        }

        strcpy(path, argv[2]);

        for (int i = 3; i < argc; i++) {
            strcat(path, " ");
            strcat(path, argv[i]);
        }

        // **OP 2: FILESYSTEM EXTRACTION (Additional argument given)
        cleanPath(path); // remove redundant forward slashes in path

        rootInode = readInode(2, fd, sb, block_size);
        int targetType;
        
        if ((targetType = searchForTarget(rootInode, fd, sb, block_size, path)) == -1) {
            fprintf(stderr, "INVALID PATH\n");
            close(fd);
            return -1;

        } else if (targetType == 1) {
            //* Target was found successfully and is a File
            extractFile(rootInode, fd, sb, block_size, 1);

        } else if (targetType == 2) {
            //* Target was found successfully and is a Directory
            extractDir(rootInode, fd, sb, block_size, 1);
        }
        // --------------------------------------------------
    }

    close(fd);
    return 0;
}