#ifndef DSWUTIL_H
#define DSWUTIL_H

#include "cc.ast.gen.h"

PQName *appendToDeclaratorName(Declarator *d, StringRef suffix);
PQName *appendToDeclaratorName(IDeclarator *d, StringRef suffix);

#endif // DSWUTIL_H
