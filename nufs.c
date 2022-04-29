// based on cs3650 starter code

#include <assert.h>
#include <bsd/string.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "storage.h"
#include "directory.h"
#include "inode.h"

// implementation for: man 2 access
// Checks if a file exists.
int nufs_access(const char *path, int mask) {
  int rv = tree_lookup(path);
  if (rv != -1) {
    inode_t* node = get_inode(rv);
  } else {
    return -ENOENT; 
  }
  printf("access(%s, %04o) -> %d\n", path, mask, rv);
  return 0;
}

// Gets an object's attributes (type, permissions, size, etc).
// Implementation for: man 2 stat
// This is a crucial function.
int nufs_getattr(const char *path, struct stat *st) {
  int rv = 0;

  // Return some metadata for the root directory...
  if (strcmp(path, "/") == 0 || strcmp(path, "") == 0) {
    st->st_mode = 040755; // directory
    st->st_size = 0;
    st->st_uid = getuid();
    st->st_nlink = 1;
  }
  else {
    rv = storage_stat(path, st);
    st->st_mode = 0100644;
    st->st_uid = getuid();
  }
  if (rv == -1) {
    return -ENOENT; 
  }
  printf("getattr(%s) -> (%d) {mode: %04o, size: %ld}\n", path, rv, st->st_mode,st->st_size);
  
  return rv;
}

// implementation for: man 2 readdir
// lists the contents of a directory
int nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info *fi) {

  int pathNodeIndex = tree_lookup(path);
  inode_t* pathNode = get_inode(pathNodeIndex);
  print_directory(pathNode);

  // for(int i = 0; i < pathNode->size / DIR_SIZE; i++) {
  //   printf("Name : %s -- Node : %d -- Active : %d\n", pathDirec[i].name, pathDirec[i].inum, pathDirec[i].active);
  // }

  //--------------------------------------------------------------------
  // struct stat st;
  // int rv;

  // rv = nufs_getattr("/", &st);
  // assert(rv == 0);

  // filler(buf, ".", &st, 0);

  // rv = nufs_getattr("/hello.txt", &st);
  // assert(rv == 0);
  // filler(buf, "hello.txt", &st, 0);

  // printf("readdir(%s) -> %d\n", path, rv);
  // return 0;

  //--------------------------------------------------------------------
  // struct stat st;
  // int rv;

  // rv = nufs_getattr(path, &st);
  // assert(rv == 0);

  // slist_t* pathFiles = storage_list(path);
  // filler(buf, ".", &st, 0);
  // if (dirnames == NULL) {
  //   printf("readdir(%s) -> %d\n", path, rv);
  //   return 0;
  // }
  
  // while(pathFiles != NULL) {
  //   char currpath[strlen(path) + 50];
  //   strncpy(currpath, path, strlen(path));
    
  //   if (path[strlen(path)-1] == '/') {
  //     currpath[strlen(path)] = 0;
  //   } else {
  //     currpath[strlen(path)] = '/';
  //     currpath[strlen(path) + 1] = 0;
  //   }
    
  //   strncat(currpath, currname->data, 48);
  //   nufs_getattr(currpath, &st);
  //   filler(buf, currname->data, &st, 0);
  //   currname = currname->next;
  // }

  // printf("readdir(%s) -> %d\n", path, rv);
  // s_free(dirnames);
  return 0;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
// Note, for this assignment, you can alternatively implement the create
// function.
int nufs_mknod(const char *path, mode_t mode, dev_t rdev) {
  int rv = -1;
  rv = storage_mknod(path, mode);
  printf("mknod(%s, %04o) -> %d\n", path, mode, rv);
  return rv;
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
int nufs_mkdir(const char *path, mode_t mode) {
  int rv = nufs_mknod(path, mode | 040000, 0);
  printf("mkdir(%s) -> %d\n", path, rv);
  return rv;
}

int nufs_unlink(const char *path) {
  int rv = -1;
  rv = storage_unlink(path);
  printf("unlink(%s) -> %d\n", path, rv);
  return rv;
}

int nufs_link(const char *from, const char *to) {
  int rv = -1;
  rv = storage_link(to, from);
  printf("link(%s => %s) -> %d\n", from, to, rv);
  return rv;
}

int nufs_rmdir(const char *path) {
  int rv = -1;
  // rv = directory_delete(get_inode(tree_lookup(path)), )
  printf("rmdir(%s) -> %d\n", path, rv);
  return rv;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int nufs_rename(const char *from, const char *to) {
  int rv = -1;
  printf("rename(%s => %s) -> %d\n", from, to, rv);
  return rv;
}

int nufs_chmod(const char *path, mode_t mode) {
  int rv = -1;
  printf("chmod(%s, %04o) -> %d\n", path, mode, rv);
  return rv;
}

int nufs_truncate(const char *path, off_t size) {
  int rv = -1;
  rv = storage_truncate(path, size);
  printf("truncate(%s, %ld bytes) -> %d\n", path, size, rv);
  return rv;
}

// This is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
// You can just check whether the file is accessible.
int nufs_open(const char *path, struct fuse_file_info *fi) {
  int rv = 0;
  // rv = nufs_access(path, 0);
  printf("open(%s) -> %d\n", path, rv);
  return rv;
}

// Actually read data
int nufs_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi) {
  int rv = -1;
  rv = storage_read(path, buf, size, offset);
  printf("read(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
  return rv;
}

// Actually write data
int nufs_write(const char *path, const char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi) {
  int rv = -1;
  rv = storage_write(path, buf, size, offset);
  printf("write(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
  return rv;
}

// Update the timestamps on a file or directory.
int nufs_utimens(const char *path, const struct timespec ts[2]) {
  int rv = -1;
  printf("utimens(%s, [%ld, %ld; %ld %ld]) -> %d\n", path, ts[0].tv_sec,
         ts[0].tv_nsec, ts[1].tv_sec, ts[1].tv_nsec, rv);
  return rv;
}

// Extended operations
int nufs_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi,
               unsigned int flags, void *data) {
  int rv = -1;
  printf("ioctl(%s, %d, ...) -> %d\n", path, cmd, rv);
  return rv;
}

void nufs_init_ops(struct fuse_operations *ops) {
  memset(ops, 0, sizeof(struct fuse_operations));
  ops->access = nufs_access;
  ops->getattr = nufs_getattr;
  ops->readdir = nufs_readdir;
  ops->mknod = nufs_mknod;
  // ops->create   = nufs_create; // alternative to mknod
  ops->mkdir = nufs_mkdir;
  ops->link = nufs_link;
  ops->unlink = nufs_unlink;
  ops->rmdir = nufs_rmdir;
  ops->rename = nufs_rename;
  ops->chmod = nufs_chmod;
  ops->truncate = nufs_truncate;
  ops->open = nufs_open;
  ops->read = nufs_read;
  ops->write = nufs_write;
  ops->utimens = nufs_utimens;
  ops->ioctl = nufs_ioctl;
};

struct fuse_operations nufs_ops;

int main(int argc, char *argv[]) {
  assert(argc > 2 && argc < 6);
  storage_init(argv[--argc]);
  nufs_init_ops(&nufs_ops);
  return fuse_main(argc, argv, &nufs_ops, NULL);
}
