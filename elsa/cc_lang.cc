// cc_lang.cc            see license.txt for copyright and terms of use
// code for cc_lang.h

#include "cc_lang.h"     // this module

void CCLang::KandR_C()
{
  tagsAreTypes = false;
  recognizeCppKeywords = false;
  implicitFuncVariable = true;
  noInnerClasses = true;
  uninitializedGlobalDataIsCommon = true;
  emptyParamsMeansPureVarargFunc = true;
  complainUponBadDeref = true;
  strictArraySizeRequirements = false;
  allowOverloading = false;
  hasImplicitStuff = false;
  compoundSelfName = false;
}

void CCLang::ANSI_C()
{
  tagsAreTypes = false;
  recognizeCppKeywords = false;
  implicitFuncVariable = true;
  noInnerClasses = true;
  uninitializedGlobalDataIsCommon = true;
  emptyParamsMeansPureVarargFunc = false;
  complainUponBadDeref = true;
  strictArraySizeRequirements = false;
  allowOverloading = false;
  hasImplicitStuff = false;
  compoundSelfName = false;
}

void CCLang::ANSI_Cplusplus()
{
  tagsAreTypes = true;
  recognizeCppKeywords = true;
  implicitFuncVariable = false;
  noInnerClasses = false;
  uninitializedGlobalDataIsCommon = false;
  emptyParamsMeansPureVarargFunc = false;

  // these aren't exactly ANSI C++; they might be "pragmatic C++"
  // for the current state of the parser
  complainUponBadDeref = false;
  strictArraySizeRequirements = false;

  allowOverloading = true;
  hasImplicitStuff = true;
  compoundSelfName = true;
}
