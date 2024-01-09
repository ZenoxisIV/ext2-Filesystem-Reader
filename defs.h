#include <linux/types.h>

#define SUPERBLOCK_OFFSET 1024
#define FD_DEV "/dev/loop20" // replace with the appropriate device
#define EXT2_MAGIC_NUMBER 0xEF53

// Reference: https://wiki.osdev.org/Ext2 Base Superblock Fields
typedef struct superblock {
	__u32 total_inodes;
	__u32 total_blocks;
	__u32 blocks_reserved_for_superuser;
	__u32 total_unallocated_blocks;
	__u32 total_unallocated_inodes;
	__u32 superblock_block_num;
	__u32 lg_block_size; // (not actual block size) block size is computed as 1024 shifted to the left by block_size bits
	__u32 lg_fragment_size;
	__u32 total_blocks_in_blockgroup;
	__u32 total_fragments_in_blockgroup;
	__u32 total_inodes_in_blockgroup;
	__u32 last_mount_time;
	__u32 last_write_time;
	__u16 mounts_since_last_check;
	__u16 allowable_mounts_since_last_check;
	__u16 ext2_sig; // 0xEF53 (for verification purposes)
	__u16 filesystem_state;
	__u16 operation_on_error;
	__u16 minor_version;
	__u32 last_check;
	__u32 interval_time_in_forced_checks;
	__u32 os_id;
	__u32 major_version;
	__u16 user_id;
	__u16 group_id;
	__u8 unused[940];
} superblock;

// Block Group Descriptor Table
typedef struct blk_groupdesc_tbl {
	__u32 block_bitmap;
	__u32 inode_bitmap;
	__u32 inode_table;
	__u16 total_unallocated_blocks;
	__u16 total_unallocated_inodes;
	__u16 total_dirs;
	__u8 unused[14];
} blk_groupdesc_tbl;