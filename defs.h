#include <linux/types.h>

extern struct superblock {
    __u32 block_size;
    __u32 blocks_per_group;
    __u32 inodes_per_group;
    __u32 first_blknum_bgdt;
};