#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libgen.h>
#include "inode.h"
#include "blocks.h"
#include "bitmap.h"

const int BLOCK_COUNT_i = 256; // we split the "disk" into 256 blocks
const int BLOCK_SIZE_i = 4096; // = 4K
const int NUFS_SIZE_i = BLOCK_SIZE_i * BLOCK_COUNT_i; // = 1MB
const int BLOCK_BITMAP_SIZE_i = BLOCK_COUNT_i / 8;

void print_inode(inode_t *node) {
  printf("--inode print--\n Reference count: %d\n Mode: %d\n Size: %d\n Block pointer: %d\n", node->refs, node->mode, node->size, node->block);
}

// Returns the address of the inode at the given int
inode_t *get_inode(int inum) {
  inode_t* inode_p = get_inode_bitmap() + BLOCK_BITMAP_SIZE_i; // inode bitmap is stored immediately after the block bitmap
  return &inode_p[inum];
}

// Allocates space for an inode, returns inode index on success, -1 on failure
int alloc_inode() {
  int nodeIndex = -1;
  void* nodeBitMap = get_inode_bitmap(); 
  for(int i = 0; i < BLOCK_COUNT_i; i++) {
    if(bitmap_get(nodeBitMap, i) == 0) {
      nodeIndex = i;
      printf("inode alloced at: %d\n", nodeIndex);
      break;
    }
  }
  bitmap_put(nodeBitMap, nodeIndex, 1);

  inode_t* newNode = get_inode(nodeIndex);
  newNode->mode = 0;
  newNode->size = 0;
  newNode->refs = 1;
  newNode->block = alloc_block();
  newNode->create_time = time(NULL);

  return nodeIndex;
}

// Frees the given inode and resets the pointers
void free_inode(int inum) {
  inode_t* target_inode = get_inode(inum);
  shrink_inode(target_inode, 0);
  free_block(target_inode->block);
  bitmap_put(get_inode_bitmap(), inum, 0);
  printf("inode #%d freed\n", inum);
}

// Increases inode size
int grow_inode(inode_t *node, int size) {
  if(size <= 4096) {
    node->size = size;
  } else {
    printf("attemping to grow inode past page boundary");
  }
  
}

// Shrinks inode size
int shrink_inode(inode_t *node, int size) {
  if(size <= 4096) {
    node->size = size;
  } else {
    printf("attemping to shrink inode past page boundary");
  }
}

// Gets the inode's block
int inode_get_pnum(inode_t *node, int fpn) {
  return node->block;
}