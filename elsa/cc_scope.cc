// cc_scope.cc            see license.txt for copyright and terms of use
// code for cc_scope.h

#include "cc_scope.h"     // this module

Scope::Scope(int cc, SourceLocation const &initLoc)
  : variables(),
    compounds(),
    enums(),
    changeCount(cc),
    curCompound(NULL),
    curFunction(NULL),
    curLoc(initLoc)
{}

Scope::~Scope()
{}
