// ccparse.h            see license.txt for copyright and terms of use
// data structures used during parsing of C++ code

#ifndef CPARSE_H
#define CPARSE_H

#include "strhash.h"       // StringHash
#include "strtable.h"      // StringTable
#include "objlist.h"       // ObjList
#include "array.h"         // ArrayStack

class PQName;              // cc.ast.gen.h

// parsing action state
class ParseEnv {
private:
  ArrayStack<StringRef> classNameStack;   // stack of class names

public:
  StringTable &str;                       // string table

public:
  ParseEnv(StringTable &table)
    : classNameStack(), str(table) {}
  ~ParseEnv() {}

  // manipulate the name of the class whose declaration we're inside;
  // this is *not* the general class context, merely the innermost
  // syntactically-occurring "class { ... }" declaration syntax, and
  // it is used only to recognize declarations of constructors
  void pushClassName(StringRef n)  { classNameStack.push(n); }
  void popClassName()              { classNameStack.pop(); }
  StringRef curClassName() const   { return classNameStack.top(); }
};

#endif // CPARSE_H
