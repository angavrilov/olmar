// cc_lang.cc            see license.txt for copyright and terms of use
// code for cc_lang.h

#include "cc_lang.h"     // this module

#include <string.h>      // memset


void CCLang::ANSI_C()
{
  // just in case I forget to initialize something....
  memset(this, 0, sizeof(*this));

  tagsAreTypes = false;
  recognizeCppKeywords = false;
  implicitFuncVariable = false;
  gccFuncBehavior = GFB_none;
  noInnerClasses = true;
  uninitializedGlobalDataIsCommon = true;
  emptyParamsMeansPureVarargFunc = false;
  complainUponBadDeref = true;
  strictArraySizeRequirements = false;
  allowOverloading = false;
  compoundSelfName = false;
  allowCallToUndeclFunc = false;
  allow_KR_ParamOmit = false;
  allowImplicitInt = true;
  allowDynamicallySizedArrays = false;
  allowIncompleteEnums = false;
  declareGNUBuiltins = false;

  isCplusplus = false;
  isC99 = false;
}

void CCLang::KandR_C()
{
  ANSI_C();

  emptyParamsMeansPureVarargFunc = true;
  allowCallToUndeclFunc = true;
  allow_KR_ParamOmit = true;
  allowImplicitInt = true;
  
  // our K&R is really GNU K&R ...
  declareGNUBuiltins = true;
}

void CCLang::ANSI_C99()
{
  ANSI_C();

  implicitFuncVariable = true;
  allowImplicitInt = false;
  isC99 = true;
}

void CCLang::GNU_C()
{
  ANSI_C99();

  allowImplicitInt = true;
  gccFuncBehavior = GFB_string;
  allowDynamicallySizedArrays = true;
  declareGNUBuiltins = true;

  // I'm just guessing this is GNU only.... yep:
  // http://gcc.gnu.org/onlinedocs/gcc-3.1/gcc/Incomplete-Enums.html
  allowIncompleteEnums = true;
}

void CCLang::GNU_KandR_C()
{
  KandR_C();

  implicitFuncVariable = true;
  gccFuncBehavior = GFB_string;
  allowDynamicallySizedArrays = true;
  
  // this seems wrong, but Oink's tests want it this way...
  isC99 = true;
}


void CCLang::ANSI_Cplusplus()
{
  tagsAreTypes = true;
  recognizeCppKeywords = true;
  implicitFuncVariable = false;
  gccFuncBehavior = GFB_none;
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
  allowImplicitInt = false;
  allowDynamicallySizedArrays = false;
  allowIncompleteEnums = false;
  declareGNUBuiltins = false;

  isCplusplus = true;
  isC99 = false;
}

void CCLang::GNU_Cplusplus()
{
  ANSI_Cplusplus();

  gccFuncBehavior = GFB_variable;

  // is this really right?  Oink tests it like it is ...
  allowDynamicallySizedArrays = true;

  declareGNUBuiltins = true;
}


// EOF
