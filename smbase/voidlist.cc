// voidlist.cc
// code for voidlist.h

#include "voidlist.h"   // this module
#include "breaker.h"    // breaker

#include <stdlib.h>     // rand()
#include <stdio.h>      // printf()


// # of items in list
int VoidList::count() const
{
  int ct=0;
  for(VoidNode *p = top; p; p = p->next) {
    ct++;
  }
  return ct;
}


// get particular item, 0 is first (item must exist)
void *VoidList::nth(int which) const
{
  VoidNode *p;
  for(p = top; which; which--) {
    xassert(p);
    p = p->next;
  }
  xassert(p);
  return p->data;
}


// return false if list fails integrity check
bool VoidList::invariant() const
{
  if (!top) {
    return true;
  }

  // The technique here is the fast/slow list traversal to find loops (which
  // are the only way a singly-linked list can be bad). Basically, if there
  // is a loop then the fast ptr will catch up to and equal the slow one; if
  // not, the fast will simply find the terminating null. It is the only way
  // I know of to find loops in O(1) space and O(n) time.

  VoidNode *slow=top, *fast=top->next;
  while (fast && fast != slow) {
    slow = slow->next;
    fast = fast->next;
    if (fast) {
      fast = fast->next;      // usually, fast jumps 2 spots per slow's 1
    }
  }

  if (fast == slow) {
    return false;        // loop
  }
  else {
    return true;         // no loop
  }
}


// insert at front
void VoidList::prepend(void *newitem)
{
  top = new VoidNode(newitem, top);
}


// insert at rear
void VoidList::append(void *newitem)
{
  if (!top) {
    prepend(newitem);
  }
  else {	   
    VoidNode *p;
    for (p = top; p->next; p = p->next)
      {}
    p->next = new VoidNode(newitem);
  }
}


// insert at particular point, index of new node becomes 'index'
void VoidList::insertAt(void *newitem, int index)
{
  if (index == 0 || isEmpty()) {
    // special case prepending or an empty list
    xassert(index == 0);     // if it's empty, index should be 0
    prepend(newitem);
  }

  else {
    // Looking at the loop below, the key things to note are:
    //  1. If index started as 1, the loop isn't executed, and the new
    //     node goes directly after the top node.
    //  2. p is never allowed to become NULL, so we can't walk off the
    //     end of the list.

    index--;
    VoidNode *p;
    for (p = top; p->next && index; p = p->next) {
      index--;
    }
    xassert(index == 0);
      // if index isn't 0, then index was greater than count()

    // put a node after p
    VoidNode *n = new VoidNode(newitem);
    n->next = p->next;
    p->next = n;
  }
}


void *VoidList::removeAt(int index)
{
  if (index == 0) {
    xassert(top != NULL);   // element must exist to remove
    VoidNode *temp = top;
    void *retval = temp->data;
    top = top->next;
    delete temp;
    return retval;
  }

  // will look for the node just before the one to delete
  index--;

  VoidNode *p;
  for (p = top; p->next && index>0;
       p = p->next, index--)
    {}

  if (p->next) {
    // index==0, so p->next is node to remove
    VoidNode *temp = p->next;
    void *retval = temp->data;
    p->next = p->next->next;
    delete temp;
    return retval;
  }
  else {
    // p->next==NULL, so index was too large
    xfailure("Tried to remove an element not on the list");
    return NULL;    // silence warning
  }
}


void VoidList::removeAll()
{
  while (top != NULL) {
    VoidNode *temp = top;
    top = top->next;
    delete temp;
  }
}



//   The difference function should return <0 if left should come before
// right, 0 if they are equivalent, and >0 if right should come before
// left. For example, if we are sorting numbers into ascending order,
// then 'diff' would simply be subtraction.
//   The 'extra' parameter passed to sort is passed to diff each time it
// is called.
//   O(n^2) time, O(1) space
void VoidList::insertionSort(VoidDiff diff, void *extra)
{
  VoidNode *primary = top;                   // primary sorting pointer
  while (primary && primary->next) {
    if (diff(primary->data, primary->next->data, extra) > 0) {   // must move next node?
      VoidNode *tomove = primary->next;
      primary->next = primary->next->next;    // patch around moving node

      if (diff(tomove->data, top->data, extra) < 0) {           // new node goes at top?
        tomove->next = top;
        top = tomove;
      }

      else {                                  // new node *not* at top
        VoidNode *searcher = top;
        while (diff(tomove->data, searcher->next->data, extra) > 0) {
          searcher = searcher->next;
        }

        tomove->next = searcher->next;        // insert new node into list
        searcher->next = tomove;
      }
    }

    else {
      primary = primary->next;              // go on if no changes
    }
  }
}


// O(n log n) time, O(log n) space
void VoidList::mergeSort(VoidDiff diff, void *extra)
{
  if (top == NULL || top->next == NULL) {
    return;   // base case: 0 or 1 elements, already sorted
  }

  // half-lists
  VoidList leftHalf;
  VoidList rightHalf;

  // divide the list
  {
    // to find the halfway point, we use the slow/fast
    // technique; to get the right node for short lists
    // (like 2-4 nodes), we start fast off one ahead

    VoidNode *slow = top;
    VoidNode *fast = top->next;

    while (fast && fast->next) {
      slow = slow->next;
      fast = fast->next;
      fast = fast->next;
    }

    // at this point, 'slow' points to the node
    // we want to be the last of the 'left' half;
    // the left half will either be the same length
    // as the right half, or one node longer

    // division itself
    rightHalf.top = slow->next;	 // top half to right
    leftHalf.top = this->top;    // bottom half to left
    slow->next = NULL; 	       	 // cut the link between the halves
  }

  // recursively sort the halves
  leftHalf.mergeSort(diff, extra);
  rightHalf.mergeSort(diff, extra);

  // merge the halves into a single, sorted list
  VoidNode *merged = NULL;     	 // tail of merged list
  while (leftHalf.top != NULL &&
         rightHalf.top != NULL) {
    // see which first element to select, and remove it
    VoidNode *selected;
    if (diff(leftHalf.top->data, rightHalf.top->data, extra) < 0) {
      selected = leftHalf.top;
      leftHalf.top = leftHalf.top->next;
    }
    else {
      selected = rightHalf.top;
      rightHalf.top = rightHalf.top->next;
    }

    // append it to the merged list
    if (merged == NULL) {
      // first time; special case
      merged = this->top = selected;
    }
    else {
      // 2nd and later; normal case
      merged = merged->next = selected;
    }
  }

  // one of the halves is exhausted; concat the
  // remaining elements of the other one
  if (leftHalf.top != NULL) {
    merged->next = leftHalf.top;
    leftHalf.top = NULL;
  }
  else {
    merged->next = rightHalf.top;
    rightHalf.top = NULL;
  }

  // verify both halves are now exhausted
  xassert(leftHalf.top == NULL &&
          rightHalf.top == NULL);

  // list is sorted
}


// attach tail's nodes to this, empty tail
void VoidList::concat(VoidList &tail)
{
  if (!top) {
    top = tail.top;
  }
  else {
    VoidNode *n = top;
    for(; n->next; n = n->next)
      {}
    n->next = tail.top;
  }

  tail.top = NULL;
}


VoidList& VoidList::operator= (VoidList const &src)
{
  if (this != &src) {
    removeAll();
    
    VoidListIter srcIter(src);
    VoidListMutator destIter(*this);
    
    for (; !srcIter.isDone(); srcIter.adv()) {
      destIter.append(srcIter.data());
    }
  }
  return *this;
}


void VoidList::debugPrint() const
{
  printf("{ ");
  for (VoidListIter iter(*this); !iter.isDone(); iter.adv()) {
    printf("%p ", iter.data());
  }
  printf("}");
}


// --------------- VoidListMutator ------------------
void VoidListMutator::insertBefore(void *item)
{
  if (prev == NULL) {
    // insert at start of list
    list.prepend(item);
    reset();
  }
  else {
    current = prev->next = new VoidNode(item, current);
  }
}


void VoidListMutator::insertAfter(void *item)
{
  xassert(!isDone());
  current->next = new VoidNode(item, current->next);
}


void VoidListMutator::append(void *item)
{
  xassert(isDone());
  insertBefore(item);
  adv();
}


void *VoidListMutator::remove()
{
  xassert(!isDone());
  void *retval = data();
  if (prev == NULL) {
    // removing first node
    list.top = current->next;
    delete current;
    current = list.top;
  }
  else {
    current = current->next;
    delete prev->next;   // old 'current'
    prev->next = current;
  }
  return retval;
}


// -------------- testing --------------
#ifdef TEST_VOIDLIST
#include "test.h"     // USUAL_MAIN

int ptrvaldiff(void *left, void *right, void*)
{
  return (int)left - (int)right;
}


// assumes we're using ptrvaldiff as the comparison fn
void verifySorted(VoidList const &list)
{
  int prev = 0;
  VoidListIter iter(list);
  for (; !iter.isDone(); iter.adv()) {
    int current = (int)iter.data();
    xassert(prev <= current);    // numeric address test
    prev = current;
  }
}


#define PRINT(lst) lst.debugPrint(); printf("\n") /* user ; */

void testSorting()
{
  enum { ITERS=100, ITEMS=20 };

  loopi(ITERS) {
    // construct a list
    VoidList list1;
    int items = rand()%ITEMS;
    loopj(items) {
      list1.prepend((void*)( (rand()%ITEMS) * 4 ));
    }
    //PRINT(list1);

    // duplicate it for use with other algorithm
    VoidList list2;
    list2 = list1;

    // sort them
    list1.insertionSort(ptrvaldiff);
    list2.mergeSort(ptrvaldiff);
    //PRINT(list1);

    // verify structure
    xassert(list1.invariant() && list2.invariant());

    // verify length
    xassert(list1.count() == items && list2.count() == items);

    // verify sortedness
    verifySorted(list1);
    verifySorted(list2);

    // verify equality
    VoidListIter iter1(list1);
    VoidListIter iter2(list2);
    for (; !iter1.isDone(); iter1.adv(), iter2.adv()) {
      xassert(iter1.data() == iter2.data());
    }
    xassert(iter2.isDone());    // another length check

    // it's still conceivable the lists are not correct,
    // but highly unlikely, in my judgment
  }
}


void entry()
{
  // first set of tests
  {
    // some sample items
    void *a=(void*)4, *b=(void*)8, *c=(void*)12, *d=(void*)16;

    VoidList list;

    // test simple modifiers and info
    list.append(c);     PRINT(list);   // c
    list.prepend(b);   	PRINT(list);   // b c
    list.append(d);	PRINT(list);   // b c d
    list.prepend(a);	PRINT(list);   // a b c d
    list.removeAt(2);	PRINT(list);   // a b d

    xassert( list.count() == 3 &&
             !list.isEmpty() &&
             list.nth(0) == a &&
             list.nth(1) == b &&
             list.nth(2) == d &&
             list.invariant()     );
  }

  // this hits most of the remaining code
  // (a decent code coverage tool for C++ would be nice!)
  testSorting();
  
  printf("voidlist ok\n");
}

USUAL_MAIN

#endif // NDEBUG

