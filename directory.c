#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <libgen.h>
#include <dirent.h>
#include "slist.h"
#include "blocks.h"
#include "inode.h"
#include "directory.h"
#include "bitmap.h"

const int DIR_SIZE = sizeof(dirent_t); //directory size
int rootNodeIndex = -1;

//init root directory
void directory_init() {
  rootNodeIndex = alloc_inode();
  printf("(directory_init) executing: root node at %d\n", rootNodeIndex);
  inode_t* rootDirNode = get_inode(rootNodeIndex);
  rootDirNode->mode = 040755;
}

// Fetches a specific directory from the given inode using directory ‘name’ inside the “tree” of the file system.
int directory_lookup(inode_t *dd, const char *name) {
  if (strcmp(name, "") == 0 || strcmp(name, "/") == 0) { //root dir
    printf("(directory_lookup) ROOT DIRECTORY: %d\n", rootNodeIndex);
    return 0; //bc root directory is in inode 0
    //NOTE: doing this bc we want the files to stay over mounts/unmounts(otherwise would use rootNodeIndex)
  } else {
  
    dirent_t* directories = blocks_get_block(dd->block);

    for(int i = 0; i < 32; i++) {
      dirent_t subdir = directories[i];
      if (strcmp(name, subdir.name) == 0) {
        printf("(directory_lookup) NAME FOUND: %s -- Index: %d\n", name, subdir.inum);
        return subdir.inum; //name found
      }
    }

    printf("(directory_lookup) NOT FOUND: %s\n", name);

    return -1; //when not found
  }
}

// Search for given path from root, return inode index, -1 on failure
int tree_lookup(const char *path) {
  if (strcmp(path, "") == 0 || strcmp(path, "/") == 0) {
    printf("(tree_lookup) ROOT - Path: %s -- Index: %d\n", path, rootNodeIndex);
    return 0;
  }
  int inodeIndex = 0;
  slist_t* explodedPath = s_explode(path, '/', 0);

  while(explodedPath != NULL) {
    inodeIndex = directory_lookup(get_inode(inodeIndex), explodedPath->data);
    if (inodeIndex == -1) break; //part of path couldn't be found, abort
    explodedPath = explodedPath->next;
  }
  
  s_free(explodedPath);
  printf("(tree_lookup) Path: %s -- Index: %d\n", path, inodeIndex);
  return inodeIndex;
}

// Inserts a new directory
int directory_put(inode_t *dd, const char *name, int inum) {
  dirent_t* currentDirec = blocks_get_block(dd->block);

  dirent_t newDirec;
  newDirec.inum = inum;
  newDirec.active = 1;
  strncpy(newDirec.name, name, DIR_NAME_LENGTH);

  int flag = 0; //flag for whether we need to acc more space in currentDirec for requested
  for (int i = 1; i < dd->size / DIR_SIZE; i++) {
    if(currentDirec[i].active == 0) {
      currentDirec[i] = newDirec;
      flag = 1;
      break;
    }
  }
  if (!flag) {
    currentDirec[dd->size / DIR_SIZE] = newDirec;
    dd->size += DIR_SIZE;
  }

  printf("(directory_put) Directory Name: %s -- inum: %d\n", name, inum);

  return 0;
}


int directory_delete(inode_t *dd, const char *name) {
  dirent_t* currentDirec = blocks_get_block(dd->block);
  for (int i = 0; i < dd->size / DIR_SIZE; i++) {
    printf("DIREC: %s vs %s -- %d\n", currentDirec[i].name, name, currentDirec[i].active);
    if (strcmp(currentDirec[i].name, name) == 0 && currentDirec[i].active == 1) {
      currentDirec[i].active = 0;
      printf("(directory_delete) deleting Directory Name: %s\n", name);
      return 0;
    }
  }
  printf("file not found\n");
  return -ENOENT;
}

// Gives the list of directories at the given path
slist_t *directory_list(const char *path) {
  int pathNodeIndex = tree_lookup(path);
  inode_t* pathNode = get_inode(pathNodeIndex);

  dirent_t* pathDirec = blocks_get_block(pathNode->block);

  slist_t* output = NULL;
  for(int i = 0; i < pathNode->size / DIR_SIZE; i++) {
    if (pathDirec[i].active) {
      output = s_cons(pathDirec[i].name, output);
    }
  }
  printf("(directory_list) Path: %s -- Output->data: %s\n", path, output->data);
  return output;
}

//print directory into console
void print_directory(inode_t *dd) {
  dirent_t* pathDirec = blocks_get_block(dd->block);

  slist_t* output = NULL;
  for(int i = 0; i < dd->size / DIR_SIZE; i++) {
    printf("Name : %s -- Node : %d -- Active : %d\n", pathDirec[i].name, pathDirec[i].inum, pathDirec[i].active);
  }
}

//read directory into buf
int read_directory(const char *path, void *buf, fuse_fill_dir_t filler) {  
  struct stat st;
  int pathNodeIndex = tree_lookup(path);
  inode_t* pathNode = get_inode(pathNodeIndex);
  dirent_t* pathDirec = blocks_get_block(pathNode->block);

  for(int i = 0; i < pathNode->size / DIR_SIZE; i++) {
    if (pathDirec[i].active == 1) {
      filler(buf, pathDirec[i].name, &st, 0); 
    }
  }
  printf("(read_directory) Path: %s\n", path);
  return 0;
}