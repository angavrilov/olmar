// array.h
// some array classes

#ifndef ARRAY_H
#define ARRAY_H

#include "xassert.h"      // xassert


// ------------------ GrowArray --------------------
template <class T>
class GrowArray {
private:     // data
  T *arr;                 // underlying array
  int len;                // # allocated entries in 'arr'

private:     // funcs
  void bc(int i);         // bounds-check an index

public:      // funcs
  GrowArray(int initLen);
  ~GrowArray();

  int length() const { return len; }

  // element access
  T const& operator[] (int i) const   { bc(i); return arr[i]; }
  T      & operator[] (int i)         { bc(i); return arr[i]; }

  // set length, reallocating if old length is different; if the
  // array gets bigger, existing elements are preserved; if the
  // array gets smaller, elements are truncated
  void setLength(int newLen);

  // make sure there are at least 'minLen' elements in the array;
  void ensureAtLeast(int minLen)
    { if (minLen > len) { setLength(minLen); } }

  // grab a read-only pointer to the raw array
  T const *getArray() const { return arr; }
  
  // make sure the given index is valid; if this requires growing,
  // do so by doubling the size of the array (repeatedly, if
  // necessary)
  void ensureIndexDoubler(int index);
  
  // set an element, using the doubler if necessary
  void setIndexDoubler(int index, T const &value)
    { ensureIndexDoubler(index); arr[index] = value; }
};


template <class T>
GrowArray<T>::GrowArray(int initLen)
{
  len = initLen;
  if (len > 0) {
    arr = new T[len];
  }
  else {
    arr = NULL;
  }
}


template <class T>
GrowArray<T>::~GrowArray()
{
  if (arr) {
    delete[] arr;
  }
}


template <class T>
void GrowArray<T>::bc(int i)
{
  xassert((unsigned)i < (unsigned)len);
}


template <class T>
void GrowArray<T>::setLength(int newLen)
{
  if (newLen != len) {
    // keep track of old
    int oldLen = len;
    T *oldArr = arr;

    // make new
    len = newLen;
    if (len > 0) {
      arr = new T[len];
    }
    else {
      arr = NULL;
    }

    // copy elements in common
    for (int i=0; i<len && i<oldLen; i++) {
      arr[i] = oldArr[i];
    }

    // get rid of old
    if (oldArr) {
      delete[] oldArr;
    }
  }
}


template <class T>
void GrowArray<T>::ensureIndexDoubler(int index)
{                  
  if (len-1 >= index) {
    return;
  }

  int newLen = len;
  while (newLen-1 < index) {
    int prevLen = newLen;
    if (newLen == 0) {
      newLen = 1;
    }
    newLen = newLen*2;
    xassert(newLen > prevLen);   // otherwise overflow -> infinite loop
  }

  setLength(newLen);
}


// ---------------------- ArrayStack ---------------------
template <class T>
class ArrayStack {
private:
  GrowArray<T> arr;      // array implementing stack
  int size;              // # of elts in the stack

public:
  ArrayStack(int initArraySize = 10)
    : arr(initArraySize),
      size(0)
    {}
  ~ArrayStack();

  void push(T &val)
    { arr.setIndexDoubler(size++, val); }
  T pop()
    { return arr[--size]; }

  int getSize() const
    { return size; }
  bool isEmpty() const
    { return size==0; }
  bool isNotEmpty() const
    { return !isEmpty(); }
};

template <class T>
ArrayStack<T>::~ArrayStack()
{}


#endif // ARRAY_H
