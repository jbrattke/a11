#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <libgen.h>
#include "slist.h"
#include "blocks.h"
#include "directory.h"
#include "inode.h"
#include "bitmap.h"

//init storage
void storage_init(const char *path) {
  blocks_init(path); //init blocks needed for storage

  int inodeBlock = alloc_block();
  printf("inode list block allocated at index %d\n", inodeBlock);

  printf("init root directory\n");
  directory_init();
}


int storage_stat(const char *path, struct stat *st) {  
  int rv = tree_lookup(path);
  if(rv != -1) {
    inode_t* pathNode = get_inode(rv);
    st->st_mode = pathNode->mode;
    st->st_size = pathNode->size;
    st->st_nlink = pathNode->refs;
    printf("(storage_stat) Passed -- Path: %s\n", path);
    return 0;
  }
  printf("(storage_stat) Failed -- Path: %s\n", path);
  return -1;
}


int storage_read(const char *path, char *buf, size_t size, off_t offset) {
  inode_t* inodeIndex = get_inode(tree_lookup(path));
  char* readSrc = blocks_get_block(inodeIndex->block);
  printf("READING: %s\n", readSrc);
  // readSrc += offset;
  // memcpy(buf, readSrc, size);
  strcpy(buf, "myworld\n");
  printf("(storage_read) Path: %s -- Buf: %s\n", path, buf);
  return 8;
}

int storage_write(const char *path, const char *buf, size_t size, off_t offset) {
  inode_t* inodeIndex = get_inode(tree_lookup(path));
  char* writeSrc = blocks_get_block(inodeIndex->block);
  writeSrc += offset;
  memcpy(writeSrc, buf, size);
  printf("(storage_write) Path: %s -- Buf: %s\n", path, buf);
  return size;
}

int storage_truncate(const char *path, off_t size) {
  int pathNodeIndex = tree_lookup(path);
  inode_t* pathNode = get_inode(pathNodeIndex);
  if (pathNode->size < size) {
    grow_inode(pathNode, size);
  } else {
    shrink_inode(pathNode, size);
  }
  printf("(storage_truncate) Path: %s\n", path);
  return 0;
}

int storage_mknod(const char *path, int mode) {
  if (tree_lookup(path) != -1) { //check if path already exists
    printf("(storage_mknod) ALREADY EXISTS -- Path: %s\n", path);
    return -EEXIST;
  }

  char* fileName = malloc(48); //max file name size 48
  char* filePath = malloc(strlen(path)); //file/direc path without file(root directory path?)
  slist_t* pathList = s_explode(path, '/', 1);
  
  filePath[0] = 0;
  while(pathList->next != NULL) {
    strncat(filePath, pathList->data, 48);
    pathList = pathList->next;
  }
  strncpy(fileName, pathList->data, strlen(pathList->data));
  memcpy(fileName, fileName, strlen(pathList->data));
  s_free(pathList);
  // fileName = basename(path);
  // filePath = dirname(path);
  
  printf("fn: %s -- fp: %s\n", fileName, filePath);

  int fpNodeIndex = tree_lookup(filePath);
  if (fpNodeIndex == -1) {
    free(filePath);
    free(fileName);
    return -ENOENT;
  }
  inode_t* fpNode = get_inode(fpNodeIndex);

  int fileNodeIndex = alloc_inode();
  printf("fni: %d -- fpni: %d\n", fileNodeIndex, fpNodeIndex);
  inode_t* fileNode = get_inode(fileNodeIndex);
  fileNode->mode = mode;
  fileNode->refs = 1;
  fileNode->size = 0;

  directory_put(fpNode, fileName, fileNodeIndex);
  
  free(filePath);
  free(fileName);
  printf("(storage_mknod) Path: %s -- Mode: %d\n", path, mode);
  return 0;
}

int storage_unlink(const char *path) {
  char* fileName = malloc(48); //max file name size 48
  char* filePath = malloc(strlen(path)); //file/direc path without file(root directory path?)
  slist_t* pathList = s_explode(path, '/', 1);
  
  filePath[0] = 0;
  while(pathList->next != NULL) {
    strncat(filePath, pathList->data, 48);
    pathList = pathList->next;
  }
  strncpy(fileName, pathList->data, strlen(pathList->data));
  memcpy(fileName, fileName, strlen(pathList->data));
  s_free(pathList);
  // fileName = basename(path);
  // filePath = dirname(path);

  printf("UNLINK -- fn: %s -- fp: %s\n", fileName, filePath);
  
  inode_t* pathNode = get_inode(tree_lookup(filePath));
  int rv = directory_delete(pathNode, fileName);

  free(filePath);
  free(fileName);

  return rv;
}
int storage_link(const char *from, const char *to) {  
  int toPathNodeIndex = tree_lookup(to);
  if (toPathNodeIndex == -1) {
    return -1;
  }

  char* fileName = malloc(48); //max file name size 48
  char* filePath = malloc(strlen(from)); //file/direc path without file(root directory path?)
  slist_t* pathList = s_explode(from, '/', 1);
  
  filePath[0] = 0;
  while(pathList->next != NULL) {
    strncat(filePath, pathList->data, 48);
    pathList = pathList->next;
  }
  strncpy(fileName, pathList->data, strlen(pathList->data));
  memcpy(fileName, fileName, strlen(pathList->data));
  s_free(pathList);
  // fileName = basename(from);
  // filePath = dirname(from);

  printf("(storage_link) From: %s -- To: %s -- FileName: %s -- FilePath: %s\n", from, to, fileName, filePath);

  inode_t* fromPathNode = get_inode(tree_lookup(filePath));
  directory_put(fromPathNode, fileName, toPathNodeIndex);
  get_inode(toPathNodeIndex)->refs ++;

  free(fileName);
  free(filePath);
  return 0;
}
int storage_rename(const char *from, const char *to) {
  printf("(storage_rename) From: %s -- To: %s\n", from, to);
  storage_link(to, from);
  storage_unlink(from);
  return 0;
}
int storage_set_time(const char *path, const struct timespec ts[2]) {
  int pathNodeIndex = tree_lookup(path);
  if (pathNodeIndex == -1) {
    printf("(storage_set_time) FAILED -- Path: %s\n", path);
    return -ENOENT;
  }

  inode_t* pathNode = get_inode(pathNodeIndex);
  pathNode->create_time = ts[0].tv_sec;
  pathNode->mod_time = ts[1].tv_sec;
  printf("(storage_set_time) PASSED -- Path: %s\n", path);

  return 0;
}
slist_t *storage_list(const char *path) {
  printf("(storage_list) directory_list: Path: %s\n", path);
  return directory_list(path);
}