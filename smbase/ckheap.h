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

// prints allocation statistics to stdout
void malloc_stats();


} // extern "C"

#endif // CKHEAP_H
