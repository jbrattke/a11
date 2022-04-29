// Directory manipulation functions.
//
// Feel free to use as inspiration.

// based on cs3650 starter code

#ifndef DIRECTORY_H
#define DIRECTORY_H

#define DIR_NAME_LENGTH 48

#include <fuse.h>
#include "blocks.h"
#include "inode.h"
#include "slist.h"

typedef struct dirent_t {
  char name[DIR_NAME_LENGTH];
  int inum;
  int active;
  char _reserved[12];
} dirent_t;

void directory_init();
int directory_lookup(inode_t *dd, const char *name);
int tree_lookup(const char *path);
int directory_put(inode_t *dd, const char *name, int inum);
int directory_delete(inode_t *dd, const char *name);
slist_t *directory_list(const char *path);
void print_directory(inode_t *dd);
int read_directory(const char *path, void *buf, fuse_fill_dir_t filler);

#endif
