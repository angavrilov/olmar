// cc_lang.cc            see license.txt for copyright and terms of use
// code for cc_lang.h

#include "cc_lang.h"     // this module

void CCLang::ANSI_C()
{
  tagsAreTypes = false;
  recognizeCppKeywords = false;
  implicitFuncVariable = true;
  noInnerClasses = true;
  complainUponBadDeref = true;
}

void CCLang::ANSI_Cplusplus()
{
  tagsAreTypes = true;
  recognizeCppKeywords = true;
  implicitFuncVariable = false;
  noInnerClasses = false;
  complainUponBadDeref = false;
}

