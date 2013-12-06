// ext2 definitions from the real driver in the Linux kernel.
#include "ext2fs.h"

// This header allows your project to link against the reference library. If you
// complete the entire project, you should be able to remove this directive and
// still compile your code.
#include "reference_implementation.h"

// Definitions for ext2cat to compile against.
#include "ext2_access.h"



///////////////////////////////////////////////////////////
//  Accessors for the basic components of ext2.
///////////////////////////////////////////////////////////

// Return a pointer to the primary superblock of a filesystem.
struct ext2_super_block * get_super_block(void * fs) {
    return (struct ext2_super_block*)((size_t)fs + SUPERBLOCK_OFFSET);
}


// Return the block size for a filesystem.
__u32 get_block_size(void * fs) {
  return EXT2_BLOCK_SIZE(get_super_block(fs));
}


// Return a pointer to a block given its number.
// get_block(fs, 0) == fs;
void * get_block(void * fs, __u32 block_num) {
  return (void*)((size_t)fs + block_num * get_block_size(fs));
}


// Return a pointer to the first block group descriptor in a filesystem. Real
// ext2 filesystems will have several of these, but, for simplicity, we will
// assume there is only one.
struct ext2_group_desc * get_block_group(void * fs, __u32 block_group_num) {
  // spec linked to in the assignment sheet says block group descriptor
  // is always in block 2
  return (struct ext2_group_desc*)get_block(fs, 2);
}


// Return a pointer to an inode given its number. In a real filesystem, this
// would require finding the correct block group, but you may assume it's in the
// first one.
struct ext2_inode * get_inode(void * fs, __u32 inode_num) {
  return (struct ext2_inode*)((size_t)get_block(fs, get_block_group(fs, 1)->bg_inode_table)
                               + sizeof(struct ext2_inode) * (inode_num - 1));
}



///////////////////////////////////////////////////////////
//  High-level code for accessing filesystem components by path.
///////////////////////////////////////////////////////////

// Chunk a filename into pieces.
// split_path("/a/b/c") will return {"a", "b", "c"}.
//
// This one's a freebie.
char ** split_path(char * path) {
    int num_slashes = 0;
    for (char * slash = path; slash != NULL; slash = strchr(slash + 1, '/')) {
        num_slashes++;
    }

    // Copy out each piece by advancing two pointers (piece_start and slash).
    char ** parts = (char **) calloc(num_slashes, sizeof(char *));
    char * piece_start = path + 1;
    int i = 0;
    for (char * slash = strchr(path + 1, '/');
         slash != NULL;
         slash = strchr(slash + 1, '/')) {
        int part_len = slash - piece_start;
        parts[i] = (char *) calloc(part_len + 1, sizeof(char));
        strncpy(parts[i], piece_start, part_len);
        piece_start = slash + 1;
        i++;
    }
    // Get the last piece.
    parts[i] = (char *) calloc(strlen(piece_start) + 1, sizeof(char));
    strncpy(parts[i], piece_start, strlen(piece_start));
    return parts;
}


// Convenience function to get the inode of the root directory.
struct ext2_inode * get_root_dir(void * fs) {
    return get_inode(fs, EXT2_ROOT_INO);
}


// Given the inode for a directory and a filename, return the inode number of
// that file inside that directory, or 0 if it doesn't exist there.
// 
// name should be a single component: "foo.txt", not "/files/foo.txt".
__u32 get_inode_from_dir(void * fs, struct ext2_inode * dir, 
        char * name) {
  __u32 block = dir->i_block[0];
  struct ext2_dir_entry_2* directory = (struct ext2_dir_entry_2*)get_block(fs, block);
  __u32 directory_size = dir->i_size;
  __u32 seen_size = 0;
  int match;
  do
  {
    match = 1;
    __u16 name_len = directory->name_len;
    if(strlen(name) == name_len)
    {
      __u16 i;
      for(i = 0; i < name_len; ++i)
        if(name[i] != directory->name[i])
        {
          match = 0;
          break;
        }
    }
    else
      match = 0;
    
    if(!match)
    {
      __u16 rec_len = directory->rec_len;
      seen_size += rec_len;
      if(seen_size >= directory_size)
        return 0;
      directory = (struct ext2_dir_entry_2*)((size_t)directory + rec_len);
    }
  }while(!match);
  return directory->inode;
}


// Find the inode number for a file by its full path.
// This is the functionality that ext2cat ultimately needs.
__u32 get_inode_by_path(void * fs, char * path) {
    
  int num_slashes = 0;
  __u32 ret = 0;
  for (char * slash = path; slash != NULL; slash = strchr(slash + 1, '/')) {
      num_slashes++;
  }
  char** path_parts = split_path(path);
  struct ext2_inode* dir = get_root_dir(fs);
  int i;
  for(i = 0; i < num_slashes; ++i)
  {
    ret = get_inode_from_dir(fs, dir, path_parts[i]);
    if(ret == 0)
      break;
    dir = get_inode(fs, ret);
  }
  for(i = 0; i < num_slashes; ++i)
    free(path_parts[i]);
  free(path_parts);

  return ret;

}

