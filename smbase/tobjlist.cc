// tobjlist.cc
// test code for objlist.h

#include "objlist.h"    // this module
#include "breaker.h"    // breaker
#include "test.h"       // USUAL_MAIN

#include <stdlib.h>     // rand()
#include <stdio.h>      // printf()


// ------------ object class --------------------
// class of objects to hold in the list
class Integer {
public:
  static int ctorcount;     // # of calls to ctor
  static int dtorcount;     // # of calls to dtor
  int i;                    // data this class holds

public:
  Integer(int ii);
  ~Integer();
};

int Integer::ctorcount = 0;
int Integer::dtorcount = 0;

Integer::Integer(int ii)
  : i(ii)
{
  ctorcount++;
}

Integer::~Integer()
{
  dtorcount++;
}


// ----------- testing ObjList -----------
int intDiff(Integer *left, Integer *right, void*)
{
  return left->i - right->i;
}


// assumes we're using ptrvaldiff as the comparison fn
void verifySorted(ObjList<Integer> const &list)
{
  int prev = 0;
  ObjListIter<Integer> iter(list);
  for (; !iter.isDone(); iter.adv()) {
    int current = iter.data()->i;
    xassert(prev <= current);
    prev = current;
  }
}


void print(ObjList<Integer> const &list)
{   
  printf("{ ");
  ObjListIter<Integer> iter(list);
  for (; !iter.isDone(); iter.adv()) {
    printf("%d ", iter.data()->i);
  }
  printf("} ");
}

#define PRINT(lst) print(lst); printf("\n") /* user ; */


void testSorting()
{
  enum { ITERS=100, ITEMS=20 };

  loopi(ITERS) {
    // construct a list
    ObjList<Integer> list1;
    ObjList<Integer> list2;
    int items = rand()%ITEMS;
    loopj(items) {
      int it = rand()%ITEMS;
      list1.prepend(new Integer(it));
      list2.prepend(new Integer(it));     // two lists with identical contents
    }
    //PRINT(list1);

    // sort them
    list1.insertionSort(intDiff);
    list2.mergeSort(intDiff);
    //PRINT(list1);

    // verify structure
    xassert(list1.invariant() && list2.invariant());

    // verify length
    xassert(list1.count() == items && list2.count() == items);

    // verify sortedness
    verifySorted(list1);
    verifySorted(list2);

    // verify equality
    ObjListIter<Integer> iter1(list1);
    ObjListIter<Integer> iter2(list2);
    for (; !iter1.isDone(); iter1.adv(), iter2.adv()) {
      xassert(iter1.data()->i == iter2.data()->i);
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
    Integer *a = new Integer(1);
    Integer *b = new Integer(2);
    Integer *c = new Integer(3);
    Integer *d = new Integer(4);

    ObjList<Integer> list;

    // test simple modifiers and info
    list.append(c);     PRINT(list);   // c
    list.prepend(b);   	PRINT(list);   // b c
    list.append(d);	PRINT(list);   // b c d
    list.prepend(a);	PRINT(list);   // a b c d
    list.deleteAt(2);	PRINT(list);   // a b d

    xassert( list.count() == 3 &&
             !list.isEmpty() &&
             list.nth(0) == a &&
             list.nth(1) == b &&
             list.nth(2) == d &&
             list.invariant()     );
             
    // 'list' will delete the integers
  }

  printf("Integer ctorcount=%d dtorcount=%d\n",
         Integer::ctorcount, Integer::dtorcount);

  // this hits most of the remaining code
  // (a decent code coverage tool for C++ would be nice!)
  testSorting();

  printf("Integer ctorcount=%d dtorcount=%d\n",
         Integer::ctorcount, Integer::dtorcount);

  printf("ObjList<Integer> ok\n");
}

USUAL_MAIN

