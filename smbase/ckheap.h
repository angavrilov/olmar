// ckheap.h
// interface to check heap integrity, etc.

#ifndef CKHEAP_H
#define CKHEAP_H

extern "C" {


// check heap integrity, and fail an assertion if it's bad
void checkHeap();

// check that a given pointer is a valid allocated object;
// fail assertion if not
void checkHeapNode(void *node);

// prints allocation statistics to stderr
void malloc_stats();


// actions the heap walk iterator might request
enum HeapWalkOpts {
  HW_GO = 0,           // keep going
  HW_STOP = 1,         // stop iteraing
  HW_FREE = 2,         // free the block I just examined
};

// function for walking the heap
//   block:   pointer to the malloc'd block of memory
//   size:    # of bytes in the block; possibly larger than
//            what was requested
//   returns: bitwise OR of HeapWalkOpts options
typedef HeakWalkOpts (*HeakWalkFn)(void *block, int size);

// heap walk entry
void walkMallocHeap(HealkWalkFn func);


} // extern "C"

#endif // CKHEAP_H
