// cc_qual.h            see license.txt for copyright and terms of use
// dummy declarations for C++/cqual interface; the AST entry points
// are declared in cc.ast

#ifndef CC_QUAL_H
#define CC_QUAL_H

#include "cc.ast.gen.h"         // C++ AST; this module

// global context for a ccqual-walk
class QualEnv : public ostream {
public:
  QualEnv(ostream &out) {}
  QualEnv(stringBuilder &sb) {}
};

#endif // CC_QUAL_H
