// asthelp.h
// included by generated ast code

#ifndef ASTHELP_H
#define ASTHELP_H

#include "astlist.h"     // ASTList
#include "str.h"         // string
#include "locstr.h"      // LocString

#include <iostream.h>    // ostream

// ----------------- downcasts --------------------
// the 'if' variants return NULL if the type isn't what's expected;
// the 'as' variants throw an exception in that case
#define DECL_AST_DOWNCASTS(type, tag)            \
  type const *if##type##C() const;               \
  type *if##type()                               \
    { return const_cast<type*>(if##type##C()); } \
  type const *as##type##C() const;               \
  type *as##type()                               \
    { return const_cast<type*>(as##type##C()); } \
  bool is##type() const                          \
    { return kind() == tag; }


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


// ------------------- const typecase --------------------
#define ASTSWITCHC(supertype, nodeptr)           \
{                                                \
  supertype const *switch_nodeptr = (nodeptr);   \
  switch (switch_nodeptr->kind())

#define ASTCASEC(type, var)                           \
  case type::TYPE_TAG: {                              \
    type const *var = switch_nodeptr->as##type##C();

#define ASTNEXTC(type, var)                           \
    break;                                            \
  } /* end previous case */                           \
  case type::TYPE_TAG: {                              \
    type const *var = switch_nodeptr->as##type##C();

// end a case, and add an empty 'default' construct
#define ASTENDCASECD                                  \
    break;                                            \
  } /* end final case */                              \
  default: ;    /* silence warning */                 \
} /* end scope started before switch */

#define ASTDEFAULTC                                   \
    break;                                            \
  } /* end final case */                              \
  default: {                                          \

// end a case where an explicit default was present, or
// there is no need to add one (e.g. because it was exhaustive)
#define ASTENDCASEC                                   \
    break;                                            \
  } /* end final case */                              \
} /* end scope started before switch */


// ------------------- non-const typecase --------------------
#define ASTSWITCH(supertype, nodeptr)            \
{                                                \
  supertype *switch_nodeptr = (nodeptr);         \
  switch (switch_nodeptr->kind())

#define ASTCASE(type, var)                            \
  case type::TYPE_TAG: {                              \
    type *var = switch_nodeptr->as##type();

#define ASTNEXT(type, var)                            \
    break;                                            \
  } /* end previous case */                           \
  case type::TYPE_TAG: {                              \
    type *var = switch_nodeptr->as##type();
                              
// end-of-switch behavior is same as in const case
#define ASTENDCASED ASTENDCASECD
#define ASTDEFAULT ASTDEFAULTC
#define ASTENDCASE ASTENDCASEC


// ------------------- debug print helpers -----------------
ostream &ind(ostream &os, int indent);

#define PRINT_HEADER(clsname)         \
  ind(os, indent) << #clsname ":\n";  \
  indent += 2   /* user ; */


#define PRINT_STRING(var) \
  debugPrintStr(var, #var, os, indent)    /* user ; */

void debugPrintStr(string const &s, char const *name,
                   ostream &os, int indent);


#define PRINT_LIST(T, list) \
  debugPrintList(list, #list, os, indent)     /* user ; */

template <class T>
void debugPrintList(ASTList<T> const &list, char const *name,
                    ostream &os, int indent)
{
  ind(os, indent) << name << ":\n";
  {
    FOREACH_ASTLIST(T, list, iter) {
      iter.data()->debugPrint(os, indent+2);
    }
  }
}

// provide explicit specialization for strings
void debugPrintList(ASTList<string> const &list, char const *name,
                    ostream &os, int indent);
void debugPrintList(ASTList<LocString> const &list, char const *name,
                    ostream &os, int indent);


#define PRINT_SUBTREE(tree)                     \
  if (tree) {                                   \
    (tree)->debugPrint(os, indent);             \
  }                                             \
  else {                                        \
    ind(os, indent) << #tree << " is null\n";   \
  } /* user ; (optional) */


#define PRINT_GENERIC(var) \
  ind(os, indent) << #var << " = " << ::toString(var) << "\n"   /* user ; */


#define PRINT_BOOL(var) \
  ind(os, indent) << #var << " = " << (var? "true" : "false") << "\n"   /* user ; */



#endif // ASTHELP_H
