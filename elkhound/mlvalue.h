// mlvalue.h
// support routines for emitting ML data structures

#ifndef MLVALUE_H
#define MLVALUE_H

#include "str.h"       // string
#include "objlist.h"   // ObjList
#include "sobjlist.h"  // SObjList

// type for tags that get constructed and passed around
typedef char const *MLTag;

// macro for defining C symbols to refer to ML tag values and names
// current form:
//   MAKE_ML_TAG(pre, 5, Hello)
// will declare
//   static char const *pre_Hello = "Hello(5)";
#define DECLARE_ML_TAG(name, text) \
  static MLTag name __attribute__((unused)) = text;
#define MAKE_ML_TAG(prefix, n, t) \
  DECLARE_ML_TAG(prefix##_##t, #t "(" #n ")")


// type of ML values we pass around; at the moment, we
// pass around ascii representations
typedef string MLValue;

// literals
MLValue mlString(char const *s);
MLValue mlInt(int i);
MLValue mlBool(bool b);

// lists
MLValue mlNil();
MLValue mlCons(MLValue hd, MLValue tl);
bool mlIsNil(MLValue v);

// tagged tuples
MLValue mlTuple0(MLTag tag);
MLValue mlTuple1(MLTag tag, MLValue v);
MLValue mlTuple2(MLTag tag, MLValue v1, MLValue v2);
MLValue mlTuple3(MLTag tag, MLValue v1, MLValue v2, MLValue v3);
MLValue mlTuple4(MLTag tag, MLValue v1, MLValue v2, MLValue v3, MLValue v4);

// records (untagged, but the ascii has field names)
MLValue mlRecord3(char const *name1, MLValue field1,
                  char const *name2, MLValue field2,
                  char const *name3, MLValue field3);

MLValue mlRecord4(char const *name1, MLValue field1,
                  char const *name2, MLValue field2,
                  char const *name3, MLValue field3,
                  char const *name4, MLValue field4);

MLValue mlRecord7(char const *name1, MLValue field1,
                  char const *name2, MLValue field2,
                  char const *name3, MLValue field3,
                  char const *name4, MLValue field4,
                  char const *name5, MLValue field5,
                  char const *name6, MLValue field6,
                  char const *name7, MLValue field7);

// references
MLValue mlRef(MLValue v);


// 'option'
MLValue mlSome(MLValue v);
MLValue mlNone();


// construct an ML list from one of my lists
template <class T>
MLValue mlObjList(ObjList<T> const &list)
{
  MLValue ret = mlNil();

  for (int i=list.count()-1; i>=0; i--) {
    ret = mlCons(list.nthC(i)->toMLValue(), ret);
  }

  return ret;
}


// need one for serf lists too
template <class T>
MLValue mlSObjList(SObjList<T> const &list)
{
  MLValue ret = mlNil();

  for (int i=list.count()-1; i>=0; i--) {
    ret = mlCons(list.nthC(i)->toMLValue(), ret);
  }

  return ret;
}


#endif // MLVALUE_H
