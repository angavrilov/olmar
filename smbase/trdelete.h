// trdelete.h
// objects which trash their contents upon deletion
// I would love to have implemented this as a base class and simply derive
//   things from it, but a poor implementation choice by Borland makes this
//   too costly in terms of performance

#ifndef __TRDELETE_H
#define __TRDELETE_H

#include <stddef.h>      // size_t

void trashingDelete(void *blk, size_t size);
void trashingDeleteArr(void *blk, size_t size);

// to use, include the TRASHINGDELETE macro in the public section of a class

#define TRASHINGDELETE                                                              \
  void operator delete(void *blk, size_t size) { trashingDelete(blk, size); }       \
  void operator delete[](void *blk, size_t size) { trashingDeleteArr(blk, size); }

#endif // __TRDELETE_H
