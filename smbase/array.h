// array.h            see license.txt for copyright and terms of use
// some array classes

#ifndef ARRAY_H
#define ARRAY_H

#include "xassert.h"      // xassert


// -------------------- Array ----------------------
// same as C's built-in array, but automatically deallocates
template <class T>
class Array {
private:     // data
  T *arr;

private:     // not allowed
  Array(Array&);
  void operator=(Array&);

public:
  Array(int len) : arr(new T[len]) {}
  ~Array() { delete[] arr; }
  
  T const &operator[] (int i) const { return arr[i]; }
  T &operator[] (int i) { return arr[i]; }

  operator T const* () const { return arr; }
  operator T const* () { return arr; }
  operator T * () { return arr; }
  
  T const* operator+ (int i) const { return arr+i; }
  T * operator+ (int i) { return arr+i; }
};


// ------------------ GrowArray --------------------
// this class implements an array of T's; it automatically expands
// when 'ensureAtLeast' or 'ensureIndexDoubler' is used; it does not
// automatically contract
//
// class T must have:
//   T::T();           // default ctor for making arrays
//   operator=(T&);    // assignment for copying to new storage
//   T::~T();          // dtor for when old array is cleared
template <class T>
class GrowArray {
private:     // data
  T *arr;                 // underlying array
  int sz;                 // # allocated entries in 'arr'

private:     // funcs
  void bc(int i) const    // bounds-check an index
    { xassert((unsigned)i < (unsigned)sz); }
  void eidLoop(int index);

public:      // funcs
  GrowArray(int initSz);
  ~GrowArray();

  int size() const { return sz; }

  // element access
  T const& operator[] (int i) const   { bc(i); return arr[i]; }
  T      & operator[] (int i)         { bc(i); return arr[i]; }

  // set size, reallocating if old size is different; if the
  // array gets bigger, existing elements are preserved; if the
  // array gets smaller, elements are truncated
  void setSize(int newSz);

  // make sure there are at least 'minSz' elements in the array;
  void ensureAtLeast(int minSz)
    { if (minSz > sz) { setSize(minSz); } }

  // grab a read-only pointer to the raw array
  T const *getArray() const { return arr; }

  // grab a writable pointer; use with care
  T *getDangerousWritableArray() { return arr; }
  T *getArrayNC() { return arr; }     // ok, not all that dangerous..

  // make sure the given index is valid; if this requires growing,
  // do so by doubling the size of the array (repeatedly, if
  // necessary)
  void ensureIndexDoubler(int index)
    { if (sz-1 < index) { eidLoop(index); } }

  // set an element, using the doubler if necessary
  void setIndexDoubler(int index, T const &value)
    { ensureIndexDoubler(index); arr[index] = value; }
    
  // swap my data with the data in another GrowArray object
  void swapWith(GrowArray<T> &obj) {
    T *tmp1 = obj.arr; obj.arr = this->arr; this->arr = tmp1;
    int tmp2 = obj.sz; obj.sz = this->sz; this->sz = tmp2;
  }
};


template <class T>
GrowArray<T>::GrowArray(int initSz)
{
  sz = initSz;
  if (sz > 0) {
    arr = new T[sz];
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
void GrowArray<T>::setSize(int newSz)
{
  if (newSz != sz) {
    // keep track of old
    int oldSz = sz;
    T *oldArr = arr;

    // make new
    sz = newSz;
    if (sz > 0) {
      arr = new T[sz];
    }
    else {
      arr = NULL;
    }

    // copy elements in common
    for (int i=0; i<sz && i<oldSz; i++) {
      arr[i] = oldArr[i];
    }

    // get rid of old
    if (oldArr) {
      delete[] oldArr;
    }
  }
}


// this used to be ensureIndexDoubler's implementation, but
// I wanted the very first check to be inlined
template <class T>
void GrowArray<T>::eidLoop(int index)
{
  if (sz-1 >= index) {
    return;
  }

  int newSz = sz;
  while (newSz-1 < index) {
    #ifndef NDEBUG_NO_ASSERTIONS    // silence warning..
      int prevSz = newSz;
    #endif
    if (newSz == 0) {
      newSz = 1;
    }
    newSz = newSz*2;
    xassert(newSz > prevSz);        // otherwise overflow -> infinite loop
  }

  setSize(newSz);
}


// ---------------------- ArrayStack ---------------------
template <class T>
class ArrayStack : public GrowArray<T> {
private:
  int len;               // # of elts in the stack

public:
  ArrayStack(int initArraySize = 10)
    : GrowArray<T>(initArraySize),
      len(0)
    {}
  ~ArrayStack();

  void push(T const &val)
    { setIndexDoubler(len++, val); }
  T pop()
    { return operator[](--len); }
  T const &top() const
    { return operator[](len-1); }
  T &top()
    { return operator[](len-1); }

  // alternate interface, where init/deinit is done explicitly
  // on returned references
  T &pushAlt()    // returns newly accessible item
    { ensureIndexDoubler(len++); return top(); }
  T &popAlt()     // returns item popped
    { return operator[](--len); }

  int length() const
    { return len; }
  bool isEmpty() const
    { return len==0; }
  bool isNotEmpty() const
    { return !isEmpty(); }

  void popMany(int ct)
    { len -= ct; xassert(len >= 0); }
  void empty()
    { len = 0; }

  // useful when someone has used 'getDangerousWritableArray' to
  // fill the array's internal storage
  void setLength(int L) { len = L; }

  // swap
  void swapWith(ArrayStack<T> &obj) {
    GrowArray<T>::swapWith(obj);
    int tmp = obj.len; obj.len = this->len; this->len = tmp;
  }
};

template <class T>
ArrayStack<T>::~ArrayStack()
{}


// iterator over contents of an ArrayStack, to make it easier to
// switch between it and SObjList as a representation
template <class T>
class ArrayStackIterNC {
  NO_OBJECT_COPIES(ArrayStackIterNC);   // for now

private:     // data
  ArrayStack<T> /*const*/ &arr;   // array being accessed
  int index;                      // current element

public:      // funcs
  ArrayStackIterNC(ArrayStack<T> /*const*/ &a) : arr(a), index(0) {}

  // iterator actions
  bool isDone() const             { return index >= arr.length(); }
  void adv()                      { xassert(!isDone()); index++; }
  T /*const*/ *data() const       { return &(arr[index]); }
};


// I want const polymorphism!


// ------------------- ObjArrayStack -----------------
// an ArrayStack of owner pointers
template <class T>
class ObjArrayStack {
private:    // data
  ArrayStack<T*> arr;

public:     // funcs
  ObjArrayStack(int initArraySize = 10)
    : arr(initArraySize)
    {}
  ~ObjArrayStack() { deleteAll(); }

  void push(T *ptr)          { arr.push(ptr); }
  T *pop()                   { return arr.pop(); }
  T const *top() const       { return arr.top(); }

  T const * operator[](int index) const  { return arr[index]; }
  T *       operator[](int index)        { return arr[index]; }

  int length() const         { return arr.length(); }
  bool isEmpty() const       { return arr.isEmpty(); }
  bool isNotEmpty() const    { return !isEmpty(); }

  void deleteTopSeveral(int ct);
  void deleteAll()           { deleteTopSeveral(length()); }
};


template <class T>
void ObjArrayStack<T>::deleteTopSeveral(int ct)
{
  while (ct--) {
    delete pop();
  }
}


#endif // ARRAY_H
