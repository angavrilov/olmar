// ccparse.h            see license.txt for copyright and terms of use
// data structures used during parsing of C++ code

#ifndef CPARSE_H
#define CPARSE_H

#include "strhash.h"       // StringHash
#include "strtable.h"      // StringTable
#include "objlist.h"       // ObjList

class PQName;              // cc.ast.gen.h

// parsing action state
class ParseEnv {
public:
  StringTable &str;               // string table

public:
  ParseEnv(StringTable &table) : str(table) {}
  ~ParseEnv() {}

  void enterScope() {}
  void leaveScope() {}
  void addType(PQName const *type) {}
  bool isType(PQName const *name) { return false; }
};

#endif // CPARSE_H
