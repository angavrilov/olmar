// cc_lang.cc            see license.txt for copyright and terms of use
// code for cc_lang.h

#include "cc_lang.h"     // this module

void CCLang::KandR_C()
{
  ANSI_C();

  emptyParamsMeansPureVarargFunc = true;
  allowCallToUndeclFunc = true;
  allow_KR_ParamOmit = true;
}

void CCLang::ANSI_C()
{
  tagsAreTypes = false;
  recognizeCppKeywords = false;
  implicitFuncVariable = true;
  gccFuncBehavior = GFB_string;         // gcc <= 3.3 compatibility by default
  noInnerClasses = true;
  uninitializedGlobalDataIsCommon = true;
  emptyParamsMeansPureVarargFunc = false;
  complainUponBadDeref = true;
  strictArraySizeRequirements = false;
  allowOverloading = false;
  compoundSelfName = false;
  allowCallToUndeclFunc = false;
  allow_KR_ParamOmit = false;
  isCplusplus = false;
  isC99 = true;
}

void CCLang::ANSI_Cplusplus()
{
  tagsAreTypes = true;
  recognizeCppKeywords = true;
  implicitFuncVariable = false;
  gccFuncBehavior = GFB_variable;       // g++ compatibility by default
  noInnerClasses = false;
  uninitializedGlobalDataIsCommon = false;
  emptyParamsMeansPureVarargFunc = false;

  // these aren't exactly ANSI C++; they might be "pragmatic C++"
  // for the current state of the parser
  complainUponBadDeref = false;
  strictArraySizeRequirements = false;

  allowOverloading = true;
  compoundSelfName = true;

  allowCallToUndeclFunc = false;
  allow_KR_ParamOmit = false;

  isCplusplus = true;
  isC99 = false;
}
