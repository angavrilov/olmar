// cparse.h
// data structures used during parsing of C code

#ifndef CPARSE_H
#define CPARSE_H

#include "strhash.h"       // StringHash
#include "strtable.h"      // StringTable
#include "objlist.h"       // ObjList

// parsing action state
class ParseEnv {
public:
  StringRef intType;              // "int"
  ObjList<StringHash> types;      // stack of hashes which identify names of types

public:
  ParseEnv(StringTable &table);
  ~ParseEnv();

  void enterScope();
  void leaveScope();
  void addType(StringRef type);
  bool isType(StringRef name);
};

#endif // CPARSE_H
