// vdtllist.cc
// code for vdtllist.h

#include "vdtllist.h"      // this module

void VoidTailList::steal(VoidTailList *src)
{
  if (src) {
    top = src->top;
    tail = src->tail;
    src->top = NULL;    // paranoia
    delete src;
  }
  else {
    top = NULL;
    tail = NULL;
  }
}

void VoidTailList::prepend(void *newitem)
{
  VoidList::prepend(newitem);
  if (!tail) {
    tail = top;
  }
}

void VoidTailList::append(void *newitem)
{
  if (isEmpty()) {
    prepend(newitem);
  }
  else {
    // payoff: constant-time append
    tail->next = new VoidNode(newitem, NULL);
    tail = tail->next;
  }
}

void VoidTailList::insertAt(void *newitem, int index)
{
  VoidList::insertAt(newitem, index);
  adjustTail();
}

void VoidTailList::adjustTail()
{
  if (!tail) {
    tail = top;
  }
  else if (tail->next) {
    tail = tail->next;
  }
  xassert(tail->next == NULL);
}

void *VoidTailList::removeFirst()
{
  xassert(top);
  if (top == tail) {
    tail = NULL;
  }
  void *retval = top->data;
  VoidNode *tmp = top;
  top = top->next;
  delete tmp;
  return retval;
}

void *VoidTailList::removeLast()
{
  xassert(top);
  if (top == tail) {
    return removeFirst();
  }
  
  VoidNode *before = top;
  while (before->next != tail) {
    before = before->next;
  }
  void *retval = tail->data;
  delete tail;
  tail = before;
  tail->next = NULL;
  return retval;
}

void *VoidTailList::removeAt(int index)
{
  xassert(top);
  if (index == 0) {
    return removeFirst();
  }

  VoidNode *before = top;    // will point to node before one to be removed
  index--;                    
  while (index > 0) {
    before = before->next;
    index--;
  }             
  xassert(index == 0);
  
  // fix 'tail' if necessary
  if (tail == before->next) {
    tail = before;
  }                         

  // patch around before->next
  VoidNode *toDelete = before->next;
  void *retval = toDelete->data;
  before->next = toDelete->next;
  delete toDelete;

  return retval;
}

void VoidTailList::removeAll()
{
  VoidList::removeAll();
  tail = NULL;
}

bool VoidTailList::prependUnique(void *newitem)
{
  bool retval = VoidList::prependUnique(newitem);
  adjustTail();
  return retval;
}

bool VoidTailList::appendUnique(void *newitem)
{
  bool retval = VoidList::appendUnique(newitem);
  adjustTail();
  return retval;
}

void VoidTailList::selfCheck() const
{
  VoidList::selfCheck();

  if (isNotEmpty()) {
    // verify 'tail' is in list
    xassert(contains(tail));

    // verify 'tail' is last element
    xassert(tail->next == NULL);
  }
  else {
    xassert(tail == NULL);
  }
}
