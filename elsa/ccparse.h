// ccparse.h            see license.txt for copyright and terms of use
// data structures used during parsing of C++ code

#ifndef CCPARSE_H
#define CCPARSE_H

#include "strhash.h"       // StringHash
#include "strtable.h"      // StringTable
#include "objlist.h"       // ObjList
#include "array.h"         // ArrayStack
#include "cc_flags.h"      // UberModifiers, SimpleTypeId
#include "cc_ast.h"        // C++ AST classes, needed for the action function signatures
#include "srcloc.h"        // SourceLoc

class CCLang;              // cc_lang.h

// parsing action state
class ParseEnv {
private:
  ArrayStack<StringRef> classNameStack;   // stack of class names

public:
  StringTable &str;                       // string table
  int errors;                             // parse errors
  StringRef strRefAttr;                   // "attr"
  CCLang &lang;                           // language options

public:
  ParseEnv(StringTable &table, CCLang &aLang)
    : classNameStack(),
      str(table),
      errors(0),
      strRefAttr(table.add("attr")),
      lang(aLang)
   {}
  ~ParseEnv() {}

  // manipulate the name of the class whose declaration we're inside;
  // this is *not* the general class context, merely the innermost
  // syntactically-occurring "class { ... }" declaration syntax, and
  // it is used only to recognize declarations of constructors
  void pushClassName(StringRef n)  { classNameStack.push(n); }
  void popClassName()              { classNameStack.pop(); }
  StringRef curClassName() const   { return classNameStack.top(); }

  // manipulate UberModifiers
  SimpleTypeId uberSimpleType(SourceLoc loc, UberModifiers m);
  UberModifiers uberCombine(SourceLoc loc, UberModifiers m1, UberModifiers m2);
  
  // generate a LocString suitable for use during parsing
  LocString * /*owner*/ ls(SourceLoc loc, char const *name);
  
  // report an error
  void error(SourceLoc loc, char const *msg);
};


// count ambiguous nodes; actually, this may double-count some nodes
// because of sharing in the ambiguous forest--but it will
// double-count exactly to the degree that debugPrint will
// double-print, so they will agree in that regard
int numAmbiguousNodes(TranslationUnit *unit);


#endif // CCPARSE_H
