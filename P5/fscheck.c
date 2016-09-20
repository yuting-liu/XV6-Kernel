#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>

#define T_DIR       1   // Directory
#define T_FILE      2   // File
#define T_DEV       3   // Special device

// xv6 fs img
// similar to vsfs
// unused | superblock | inode table | bitmap (data) | data blocks
// some gaps in here

// COMMENT -> may be true
// Block 0 is unused.
// Block 1 is super block. verified -> wsect(1, sb)
// Inodes start at block 2.

#define ROOTINO 1  // root i-number
#define BSIZE 512  // block size

// File system super block
struct superblock {
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
};

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Bits per block
#define BPB           (BSIZE*8)

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEV only)
  short minor;          // Minor device number (T_DEV only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+1];   // Data block addresses
};

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
  ushort inum;
  char name[DIRSIZ];
};

// The start point of each segment in the file system
void *img_ptr;
struct superblock *sb;
struct dinode *dip;
char *bitmap;
void *db;

// Check if the address is valid
void addr_check(char *new_bitmap, uint addr) {
  if (addr == 0) {
    return;
  }
  
  // Address out of bound
  if (addr < (sb->ninodes/IPB + sb->nblocks/BPB + 4) 
      || addr >= (sb->ninodes/IPB + sb->nblocks/BPB + 4 + sb->nblocks)) {
    fprintf(stderr, "ERROR: bad address in inode.\n");
    exit(1);
  }
  
  // In use but marked free
  char byte = *(bitmap + addr / 8);
  if (!((byte >> (addr % 8)) & 0x01)) {
    fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
    exit(1);
  }
}

// Get the block number
uint get_addr(uint off, struct dinode *current_dip, int indirect_flag) {
  if (off / BSIZE <= NDIRECT && !indirect_flag) {
    return current_dip->addrs[off / BSIZE];
  } else {
    return *((uint*) (img_ptr + current_dip->addrs[NDIRECT] * BSIZE) + off / BSIZE - NDIRECT);
  }
}

// Compare two bitmaps. Return 1 when two bitmaps are different, 0 when they are the same
int bitmap_cmp(char *bitmap1, char *bitmap2, int size) {
  int i;
  for (i = 0; i < size; ++i) {
    if (*(bitmap1++) != *(bitmap2++)) {
      return 1;
    }
  }

  return 0;
}

void dfs(int *inode_ref, char* new_bitmap, int inum, int parent_inum) {
  struct dinode *current_dip = dip + inum;
    
  if (current_dip->type == 0) {
    fprintf(stderr, "ERROR: inode referred to in directory but marked free.\n");
    exit(1);
  }
  
  // Empty dir without . and ..
  if (current_dip->type == T_DIR && current_dip->size == 0) {
    fprintf(stderr, "ERROR: directory not properly formatted.\n");
  }
  
  // Update current node's reference count
  inode_ref[inum]++;
  if (inode_ref[inum] > 1 && current_dip->type == T_DIR) {
    fprintf(stderr, "ERROR: directory appears more than once in file system.\n");
    exit(1);
  }
  
  int off;
  // Mark if it is indirect block
  // Because the offset at NDIRECT will need two address check:
  // One for addrs[NDIRECT], and one for the first addr in indirect block
  int indirect_flag = 0;
  
  for (off = 0; off < current_dip->size; off += BSIZE) {
    uint addr = get_addr(off, current_dip, indirect_flag);
    addr_check(new_bitmap, addr);

    if (off / BSIZE == NDIRECT && !indirect_flag) {
      off -= BSIZE;
      indirect_flag = 1;
    }
    
    // Check duplicate and mark on new bitmap when the inode is first met
    if (inode_ref[inum] == 1) {
      char byte = *(new_bitmap + addr / 8);
      if ((byte >> (addr % 8)) & 0x01) {
        fprintf(stderr, "ERROR: address used more than once.\n");
        exit(1);
      } else {
        byte = byte | (0x01 << (addr % 8));
        *(new_bitmap + addr / 8) = byte;
      }
    }
    
    if (current_dip->type == T_DIR) {
      struct dirent *de = (struct dirent *) (img_ptr + addr * BSIZE);

      // Check . and .. in DIR
      if (off == 0) {
        if (strcmp(de->name, ".")) {
          fprintf(stderr, "ERROR: directory not properly formatted.\n");          
          exit(1);
        }
        if (strcmp((de + 1)->name, "..")) {
          fprintf(stderr, "ERROR: directory not properly formatted.\n");
          exit(1);
        }
        
        // check parent
        if ((de + 1)->inum != parent_inum) {
          if (inum == ROOTINO) {
            fprintf(stderr, "ERROR: root directory does not exist.\n");
          } else {
            fprintf(stderr, "ERROR: parent directory mismatch.\n");
          }
          
          exit(1);
        }
        
        de += 2;
      }

      for (; de < (struct dirent *)(ulong)(img_ptr + (addr + 1) * BSIZE); de++) {
        if (de->inum == 0) {
          continue;
        }

        // DFS through all the files and sub-directories
        dfs(inode_ref, new_bitmap, de->inum, inum);
      }
    }
  }
}

int main(int argc, char** argv) 
{
  int fd = open(argv[1], O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "image not found.\n");
    exit(1);
  }
  
  int rc;
  struct stat sbuf;
  rc = fstat(fd, &sbuf);
  assert(rc == 0);
  
  // use mmap()
  img_ptr = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, 
                       fd, 0);
  assert(img_ptr != MAP_FAILED);
  
  sb = (struct superblock *) (img_ptr + BSIZE);
  
  // inodes
  int i;
  dip = (struct dinode *) (img_ptr + 2 * BSIZE);
  bitmap = (char *) (img_ptr + (sb->ninodes / IPB + 3) * BSIZE);
  db = (void *) (img_ptr + (sb->ninodes/IPB + sb->nblocks/BPB + 4) * BSIZE);
  
  int bitmap_size = (sb->nblocks + sb->ninodes/IPB + sb->nblocks/BPB + 4) / 8;
  int data_offset = sb->ninodes/IPB + sb->nblocks/BPB + 4;
  int inode_ref[sb->ninodes + 1];
  memset(inode_ref, 0, (sb->ninodes + 1) * sizeof(int));
  char new_bitmap[bitmap_size];
  // Initialize new bitmap
  memset(new_bitmap, 0, bitmap_size);
  memset(new_bitmap, 0xFF, data_offset / 8);
  char last = 0x00;
  for (i = 0; i < data_offset % 8; ++i) {
    last = (last << 1) | 0x01;
  }
  new_bitmap[data_offset / 8] = last;
  
  // Check root directory
  if (!(dip + ROOTINO) || (dip + ROOTINO)->type != T_DIR) {
    fprintf(stderr, "ERROR: root directory does not exist.\n");
    exit(1);
  }
  
  // dfs
  dfs(inode_ref, new_bitmap, ROOTINO, ROOTINO);
  
  struct dinode *current_dip = dip;
  
  for (i = 1; i < sb->ninodes; i++) {
    current_dip++;
    // Unallocated
    if (current_dip->type == 0) {
      continue;
    }
    
    // Invalid types
    if (current_dip->type != T_FILE 
        && current_dip->type != T_DIR 
        && current_dip->type != T_DEV) {
      fprintf(stderr, "ERROR: bad inode.\n");
      exit(1);
    }
    
    // Inode in use but not in the directory
    if (inode_ref[i] == 0) {
      fprintf(stderr, "ERROR: inode marked use but not found in a directory.\n");
      exit(1);
    }
    
    // Bad reference count
    if (inode_ref[i] != current_dip->nlink) {
      fprintf(stderr, "ERROR: bad reference count for file.\n");
      exit(1);
    }
    
    // Extra links on directories
    if (current_dip->type == T_DIR && current_dip->nlink > 1) {
      fprintf(stderr, "ERROR: directory appears more than once in file system.\n");
      exit(1);
    }
  }

  // Not in use but marked in use
  if (bitmap_cmp(bitmap, new_bitmap, bitmap_size)) {
    fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.\n");
    exit(1);
  }
  
  return 0;
}
