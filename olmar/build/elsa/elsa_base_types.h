//  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *
//  See file license.txt for terms of use                              *
//**********************************************************************

// ocaml reflection for base types in the elsa ast

#ifndef ELSA_BASE_TYPES_H
#define ELSA_BASE_TYPES_H


#include "ocaml_reflection_base.h"
#include "srcloc.h"
#include "strtable.h" 		// StringRef
#include "cc_type.h"		// EnumType::Value


#define FOREACH_SOBJSET_NC(T, set, iter) \
  for(SObjSetIter<T> iter(set); !iter.isDone(); iter.adv())

#define FOREACH_STRINGOBJDICT_NC(T, dict, iter) \
  for(StringObjDict<T>::Iter iter(dict); !iter.isDone(); iter.next())

#define FOREACH_STRINGREFMAP_NC(T, map, iter) \
  for(StringRefMap<T>::SortedKeyIter iter(map); !iter.isDone(); iter.adv())


typedef int Array_size;
typedef bool BoolValue;

value ocaml_reflect_SourceLoc(SourceLoc const *);
value ocaml_reflect_Array_size(Array_size * size);



// hand written ocaml serialization function
inline
value ocaml_reflect_bool(bool const * b){
  return(Val_bool(b));
}


// hand written ocaml serialization function
inline
value ocaml_reflect_BoolValue(bool const b){
  return(Val_bool(b));
}


// hand written ocaml serialization function
inline
value ocaml_reflect_StringRef(StringRef const s){
  // StringRef is const char *
  return(ocaml_reflect_cstring(s));
}


// hand written ocaml serialization function
inline
value ocaml_reflect_int32(int32 const * i){
  // don't allocate myself
  value ret = caml_copy_int32(*i);
  xassert(IS_OCAML_INT32(ret));
  return ret;
}


// hand written ocaml serialization function
inline
value ocaml_reflect_unsigned_long(unsigned long const * ul) {
  value r = caml_copy_int32(*ul);
  xassert(IS_OCAML_INT32(r));
  return r;
}


// hand written ocaml serialization function
inline
value ocaml_reflect_unsigned_int(unsigned int const * i) {
  unsigned long ul = *i;
  return ocaml_reflect_unsigned_long(&ul);
}


// hand written ocaml serialization function
inline
value ocaml_reflect_nativeint(int const * i) {
  value r = caml_copy_nativeint(*i);
  xassert(IS_OCAML_INT32(r));
  return r;
}


// hand written ocaml serialization function
inline
value ocaml_reflect_double(const double * f) {
  value r = caml_copy_double(*f);
  xassert(IS_OCAML_FLOAT(r));
  return r;
}


// STemplateArgument union hack

class STemplateArgument;
typedef STemplateArgument STA_NONE;
typedef STemplateArgument STA_TYPE;
typedef STemplateArgument STA_INT;
typedef STemplateArgument STA_ENUMERATOR;
typedef STemplateArgument STA_REFERENCE;
typedef STemplateArgument STA_POINTER;
typedef STemplateArgument STA_MEMBER;
typedef STemplateArgument STA_DEPEXPR;
typedef STemplateArgument STA_TEMPLATE;
typedef STemplateArgument STA_ATOMIC;

// EnumType::Value hack
typedef EnumType::Value EnumValue;

// FunctionType::ExnSpec hack
typedef FunctionType::ExnSpec FunctionExnSpec;

#endif // ELSA_BASE_TYPES_H
