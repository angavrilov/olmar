// voidlist.h
// list of void*

#ifndef __VOIDLIST_H
#define __VOIDLIST_H

#include "xassert.h"     // xassert
#include "typ.h"         // bool

// -------------------------- non-typesafe core -----------------------------
// non-typesafe list node
class VoidNode {
public:
  VoidNode *next;           // (owner) next item in list, or NULL if last item
  void *data;               // whatever it is the list is holding

  VoidNode(void *aData=NULL, VoidNode *aNext=NULL) { data=aData; next=aNext; }
};


// The difference function should return <0 if left should come before
// right, 0 if they are equivalent, and >0 if right should come before
// left.  For example, if we are sorting numbers into ascending order,
// then 'diff' would simply be subtraction.
typedef int (*VoidDiff)(void *left, void *right, void *extra);


// list of void*; at this level, the void* are completely opaque;
// the list won't attempt to delete(), compare them, or anything else
class VoidList {
private:
  friend class VoidListIter;
  friend class VoidListMutator;

protected:
  VoidNode *top;                     // (owner) first node, or NULL if list is empty

private:
  VoidList(VoidList const &obj)      {}  // not allowed

public:
  VoidList()                         { top=NULL; }
  ~VoidList()                        { removeAll(); }

  // selectors
  int count() const;                 // # of items in list
  void *nth(int which) const;        // get particular item, 0 is first (item must exist)
  bool isEmpty() const               { return top == NULL; }

  // insertion
  void prepend(void *newitem);       // insert at front
  void append(void *newitem);        // insert at rear
  void insertAt(void *newitem, int index);
    // new item is inserted such that its index becomdes 'index'

  // removal
  void *removeAt(int index);         // remove from list (must exist), and return removed item
  void removeAll();

  // complex modifiers
  void insertionSort(VoidDiff diff, void *extra=NULL);
  void mergeSort(VoidDiff diff, void *extra=NULL);

  // multiple lists
  void concat(VoidList &tail);       // tail is emptied, nodes appended to this
  VoidList& operator= (VoidList const &src);

  // debugging
  bool invariant() const;            // test this list; return false if malformed
  void debugPrint() const;           // print list contents to stdout
};


// for traversing the list without modifying it
// NOTE: no list-modification fns should be called on 'list' while this
//       iterator exists
class VoidListIter {
protected:
  VoidNode *p;                        // (serf) current item

public:
  VoidListIter(VoidList const &list)  { reset(list); }
  ~VoidListIter()                     {}

  void reset(VoidList const &list)    { p = list.top; }

  // iterator actions
  bool isDone() const                 { return p == NULL; }
  void adv()                          { p = p->next; }
  void *data() const                  { return p->data; }
};


// for traversing the list and modifying it
// NOTE: no list-modification fns should be called on 'list' while this
//       iterator exists, and only one such iterator should exist for
//       any given list
class VoidListMutator {
protected:
  VoidList &list; 	  // underlying list
  VoidNode *prev;         // (serf) previous node; NULL if at list's head
  VoidNode *current;      // (serf) node we're considered to be pointing at

public:
  VoidListMutator(VoidList &lst)   : list(lst) { reset(); }
  ~VoidListMutator()               {}

  void reset()                     { prev=NULL; current=list.top; }

  // iterator actions
  bool isDone() const              { return current == NULL; }
  void adv()                       { prev=current; current = current->next; }
  void *data()                     { return current->data; }

  // insertion
  void insertBefore(void *item);
    // 'item' becomes the new 'current', and the current 'current' is
    // pushed forward (so the next adv() will make it current again)

  void insertAfter(void *item);
    // 'item' becomes what we reach with the next adv();
    // isDone() must be false

  void append(void *item);
    // only valid while isDone() is true, it inserts 'item' at the end of
    // the list, and advances such that isDone() remains true; equivalent
    // to { xassert(isDone()); insertBefore(item); adv(); }

  // removal
  void *remove();
    // 'current' is removed from the list and returned, and whatever was
    // next becomes the new 'current'

  // debugging
  bool invariant() const
    { return (prev->next == current && current != list.top) ||
             (prev==NULL && current==list.top); }
};

#endif // __VOIDLIST_H
