// embedded.cc
// code for embedded.h

#include "embedded.h"     // EmbeddedLang

EmbeddedLang::EmbeddedLang(ReportError &e)
  : err(e),
    text(),
    exprOnly(false),
    isDeclaration(false)
{}

EmbeddedLang::~EmbeddedLang()
{}
