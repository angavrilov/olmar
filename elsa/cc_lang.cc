// cc_lang.cc            see license.txt for copyright and terms of use
// code for cc_lang.h

#include "cc_lang.h"     // this module

void CCLang::KandR_C()
{
  ANSI_C();

  emptyParamsMeansPureVarargFunc = true;
  allowCallToUndeclFunc = true;
  allow_KR_ParamOmit = true;
  allowImplicitIntRetType = true;
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

  // FIX: 1) If this is right, the 'allowImplicitIntRetType = true' in
  // K&R above is redundant; 2) I don't know if this is right, but
  // looking at the ANSI C grammar I can find just now, it looks as
  // though it is: http://www.lysator.liu.se/c/ANSI-C-grammar-y.html;
  // 3) if you change this to false then you will get an assertion
  // failure in the parser when it thinks its C++ and goes to make a
  // constructor and then realizes it isn't C++.
  allowImplicitIntRetType = true;

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
  allowImplicitIntRetType = false;

  isCplusplus = true;
  isC99 = false;
}
