// asthelp.h
// included by generated ast code

#ifndef ASTHELP_H
#define ASTHELP_H

#include "astlist.h"     // ASTList

#include <iostream.h>    // ostream

// the 'if' variants return NULL if the type isn't what's expected;
// the 'as' variants throw an exception in that case
#define DECL_AST_DOWNCASTS(type)                 \
  type const *if##type##C() const;               \
  type *if##type()                               \
    { return const_cast<type*>(if##type##C()); } \
  type const *as##type##C() const;               \
  type *as##type()                               \
    { return const_cast<type*>(as##type##C()); }


#define DEFN_AST_DOWNCASTS(superclass, type, tag)\
  type const *superclass::if##type##C() const    \
  {                                              \
    if (kind() == tag) {                         \
      return (type const*)this;                  \
    }                                            \
    else {                                       \
      return NULL;                               \
    }                                            \
  }                                              \
                                                 \
  type const *superclass::as##type##C() const    \
  {                                              \
    xassert(kind() == tag);                      \
    return (type const*)this;                    \
  }







#endif // ASTHELP_H
