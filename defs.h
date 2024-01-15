/*
    CS140 2324A PROJECT 2 - EXT2 FILESYSTEM READER
    Ivan Cassidy Cadiang (2021-12575)
    Diego Montenejo (2020-05984)
    Rohan Solas (2021-07292)
*/

// File contains important definitions, structs, and prototypes

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <linux/types.h>

#define SUPERBLOCK_OFFSET 1024
#define EXT2_MAGIC_NUMBER 0xEF53

#define DIRECTORY 0x4000
#define FILE_ 0x8000
#define NDIRECT 12
#define MAX_PATH_LENGTH 4096

// === Structs ===
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

// === Function Signatures ===

// read.c
superblock readSuperblock(int);
void readBGD(int, blk_groupdesc*, int, int);
inode readInode(int, int, superblock, int);
dir_entry readDirEntry(int, __u32, int, int);
__u16 extractObjectType(inode);
// void parseBlock(__u32, int, superblock, int, char*, void (*traverseFunc)(inode, int, superblock, int, char*));
__u32 readIndirectBlock(int, __u32, int, int);

// enum.c
void enumAllPaths(inode, int, superblock, int, char*);

// extract.c
void extractSinglePath(inode, dir_entry, int, superblock, int, char*);
void extractAllPaths(inode, dir_entry, int, superblock, int, char*, int);
int searchForTarget(inode*, dir_entry*, int, superblock, int, char*); //! IMPORTANT: function modifies the currInode argument passed to it


// pathParser.c
void recreatePath(char*);

// error.c
bool isAbsolutePath(char*);