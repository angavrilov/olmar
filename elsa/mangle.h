// mangle.h
// name mangling

// For now, this is just a verson of Type::toString that does
// not print parameter names.  It's a hack.

// Eventually, it should:
//   - mangle names compactly
//   - demangle names back into types
//   - support more than one algorithm (gcc's, for example) (?)


#ifndef MANGLE_H
#define MANGLE_H

#include "str.h"      // string

// forwards for cc_type.h and variable.h
class Type;
class AtomicType;
class Variable;
class TemplateInfo;


// main entry point
string mangle(Type const *t);


// helpers
string mangleAtomic(AtomicType const *t);
string leftMangle(Type const *t, bool innerParen = true);
string rightMangle(Type const *t, bool innerParen = true);
string mangleVariable(Variable const *v);
string mangleTemplateParams(TemplateInfo const *tp);


#endif // MANGLE_H
