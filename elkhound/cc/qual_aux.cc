// qual_aux.cc
// some auxillary functions for cc_qual.ast

#include "cc.ast.gen.h"       // C++ AST


char const *TS_name::getTypeName() const
{
  return name->getName();
}

char const *TS_elaborated::getTypeName() const
{
  return name->getName();
}

char const *TS_classSpec::getTypeName() const
{
  return name? name->getName() : "anonymous_TypeSpecifier";
}

char const *TS_enumSpec::getTypeName() const
{
  return name? name : "anonymous_TypeSpecifier";
}


// sm: I reproduced most of the same names that dsw was using
// for these names though there were a couple of inconsistencies
// that I had to remove
char const *TS_simple::getTypeName() const
{
  switch (id) {
    default: xfailure("bad code");
    case ST_CHAR:               return "anonymous_char";
    case ST_UNSIGNED_CHAR:      return "anonymous_unsigned_char";
    case ST_SIGNED_CHAR:        return "anonymous_signed_char";
    case ST_BOOL:               return "anonymous_bool";
    case ST_INT:                return "anonymous_int";
    case ST_UNSIGNED_INT:       return "anonymous_unsigned_int";
    case ST_LONG_INT:           return "anonymous_long_int";
    case ST_UNSIGNED_LONG_INT:  return "anonymous_unsigned_long_int";
    case ST_LONG_LONG:          return "anonymous_long_long";
    case ST_UNSIGNED_LONG_LONG: return "anonymous_unsigned_long_long";
    case ST_SHORT_INT:          return "anonymous_short_int";
    case ST_UNSIGNED_SHORT_INT: return "anonymous_unsigned_short_int";
    case ST_WCHAR_T:            return "anonymous_wchar_t";
    case ST_FLOAT:              return "anonymous_float";
    case ST_DOUBLE:             return "anonymous_double";
    case ST_LONG_DOUBLE:        return "anonymous_long_double";
    case ST_VOID:               return "anonymous_void";
    case ST_ELLIPSIS:           return "anonymous_ellipsis";
    case ST_CDTOR:              return "<cdtor>";
    case ST_ERROR:              return "<error>";
    case ST_DEPENDENT:          return "<dependent>";
  }
}





