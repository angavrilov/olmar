// mangle.h
// name mangling

// For now, this is just a verson of CType::toString that does
// not print parameter names.  It's a hack.

// Eventually, it should:
//   - mangle names compactly
//   - demangle names back into types
//   - support more than one algorithm (gcc's, for example) (?)


#ifndef MANGLE_H
#define MANGLE_H

#include "str.h"             // string, stringBuilder
#include "objlist.h"         // ObjList

class CType;                  // cc_type.h
class AtomicType;            // cc_type.h
class Variable;              // variable.h
class TemplateInfo;          // template.h
class STemplateArgument;     // template.h


// main entry point
string mangle(CType const *t);


// helpers
string mangleAtomic(AtomicType const *t);
string leftMangle(CType const *t, bool innerParen = true);
string rightMangle(CType const *t, bool innerParen = true);
string mangleVariable(Variable const *v);
string mangleTemplateParams(TemplateInfo const *tp);
void mangleSTemplateArgs(stringBuilder &sb, ObjList<STemplateArgument> const &args);


#endif // MANGLE_H
 
