// macros.h
// grab-bag of useful macros, stashed here to avoid mucking up
//   other modules with more focus
// (no configuration stuff here!)

#ifndef __MACROS_H
#define __MACROS_H

#include "typ.h"        // bool

// complement of ==
#define NOTEQUAL_OPERATOR(T)             \
  bool operator != (T const &obj) const  \
    { return !operator==(obj); }

// toss this into a class that already has == and < defined, to
// round out the set of relational operators
#define RELATIONAL_OPERATORS(T)                    \
  NOTEQUAL_OPERATOR(T)                             \
  bool operator <= (T const &obj) const            \
    { return !obj.operator<(*this); }              \
  bool operator > (T const &obj) const             \
    { return obj.operator<(*this); }               \
  bool operator >= (T const &obj) const            \
    { return !operator<(obj); }


// member copy in constructor initializer list
#define DMEMB(var) var(obj.var)

// member copy in operator =
#define CMEMB(var) var = obj.var

// member comparison in operator ==
#define EMEMB(var) var == obj.var


// standard insert operator
// (note that you can put 'virtual' in front of the macro call if desired)
#define INSERT_OSTREAM(T)                                \
  void insertOstream(ostream &os) const;                 \
  friend ostream& operator<< (ostream &os, T const &obj) \
    { obj.insertOstream(os); return os; }


// usual declarations for a data object (as opposed to control object)
#define DATA_OBJ_DECL(T)                \
  T();                                  \
  T(T const &obj);                      \
  ~T();                                 \
  T& operator= (T const &obj);          \
  bool operator== (T const &obj) const; \
  NOTEQUAL_OPERATOR(T)                  \
  INSERTOSTREAM(T)


// copy this to the .cc file for implementation of DATA_OBJ_DECL
#if 0
T::T()
{}

T::T(T const &obj)
  : DMEMB(),
    DMEMB(),
    DMEMB()
{}

T::~T()
{}

T& T::operator= (T const &obj)
{
  if (this != &obj) {
    CMEMB();
  }
  return *this;
}

bool T::operator== (T const &obj) const
{
  return
    EMEMB() &&
    EMEMB();
}

void T::insertOstream(ostream &os) const
{}
#endif // 0


#endif // __MACROS_H
