// objpool.h
// custom allocator: array of objects meant to be
// re-used frequently, with high locality

#ifndef OBJPOOL_H
#define OBJPOOL_H

#include "array.h"     // GrowArray

// the class T should have:
//   // a link in the free list, as an index into the pool
//   // array; -1 means null
//   int nextInFreeList;
//
//   // object is done being used
//   void deinit();
//
//   // used when we need to make the pool larger
//   T::T();           // default ctor for making arrays
//   operator=(T&);    // assignment for copying to new storage
//                     // NOTE: must copy 'nextInFreeList'!
//   T::~T();          // dtor for when old array is cleared

template <class T>
class ObjectPool {
private:     // types
  enum { NULL_LINK = -1 };

private:     // data
  // growable array of T objects
  GrowArray<T> pool;

  // (index of) head of the free list; -1 when list is empty
  int head;          
  
private:     // funcs
  void threadLinksFrom(int start);

public:      // funcs
  ObjectPool(int initSize);
  ~ObjectPool();

  // yields a reference to an object ready to be used; typically,
  // T should have some kind of init method to play the role a
  // constructor ordinarily does; this might cause the pool to
  // expand
  T &alloc();

  // return an object to the pool of objects; dealloc internally
  // calls obj.deinit()
  void deinit(T &obj);
};


template <class T>
ObjectPool<T>::ObjectPool(int initSize)
  : pool(initSize)
{
  // build a list out of the elements we have now; each node
  // will point to the next one
  threadLinksFrom(0);
}

template <class T>
void ObjectPool::threadLinksFrom(int start)
{
  for (int i = start; i < pool.size()-1; i++) {
    pool[i].nextInFreeList = i+1;
  }
  pool[pool.size()-1].nextInFreeList = NULL_LINK;

  // 'start' will be beginning of free list
  head = start;
}


template <class T>
ObjectPool<T>::~ObjectPool()
{
  // pool deallocation takes are of everything
}


template <class T>
T &ObjectPool<T>::alloc()
{
  if (!head) {
    // need to expand the pool
    int oldSize = pool.size();
    pool.setSize(pool.size() * 2);

    // thread new nodes into a free list
    threadLinksFrom(oldSize);
  }

  T &ret = pool[head];               // prepare to return this one
  head = ret.nextInFreeList;         // move to next free node

  ret.nextInFreeList = NULL_LINK;    // paranoia

  return ret;
}


template <class T>
void ObjectPool<T>::dealloc(T &obj)
{
  // call obj's pseudo-dtor (the decision to have dealloc do this is
  // motivated by not wanting to have to remember to call deinit
  // before dealloc)
  obj.deinit();

  // I don't check that nextInFreeList == NULL_LINK, despite having set
  // it that way in alloc(), because I want to allow for users to make
  // nextInFreeList share storage (e.g. with a union) with some other
  // field that gets used while the node is allocated

  // prepend the object to the free list
  obj.nextInFreeList = head;
  head = (&obj - pool.getArray());      // ptr arith to derive index
  xassert(0 <= head && head < pool.size());
}


#endif // OBJPOOL_H
