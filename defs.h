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
	__u32 lg_block_size; // (not actual block size) block size is computed as 1024 shifted to the left by lg_block_size bits
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
	__u32 first_nonreserved_inode;
	__u16 inode_size;
	__u8 unused[934];
} superblock;

// Block Group Descriptor
typedef struct blk_groupdesc {
	__u32 block_bitmap;
	__u32 inode_bitmap;
	__u32 inode_table;
	__u16 total_unallocated_blocks;
	__u16 total_unallocated_inodes;
	__u16 total_dirs;
	__u8 unused[14];
} blk_groupdesc;

// Inodes
typedef struct inode {
	__u16 type_and_perm;
	__u16 user_id;
	__u32 lo_size;
	__u32 last_access;
	__u32 creation_time;
	__u32 last_mod;
	__u32 del_time;
	__u16 group_id;
	__u16 count_hard_links;
	__u32 count_sectors;
	__u32 flags;
	__u32 ignored;
	__u32 dp[12];
	__u32 sip;
	__u32 dip;
	__u32 tip;
	__u8 unused[28];
} inode;

// Directory Entry
typedef struct dir_entry {
	__u32 inode_num;
	__u16 size;
	__u8  name_size;
	__u8  ignored;
	__u8  name[256]; // MAY NEED TO CHANGE
} dir_entry;