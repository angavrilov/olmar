// cc_lang.cc            see license.txt for copyright and terms of use
// code for cc_lang.h

#include "cc_lang.h"     // this module

#include <string.h>      // memset


void CCLang::ANSI_C89()
{
  // just in case I forget to initialize something....
  memset(this, 0, sizeof(*this));

  tagsAreTypes = false;
  recognizeCppKeywords = false;
  implicitFuncVariable = false;
  gccFuncBehavior = GFB_none;
  noInnerClasses = true;
  uninitializedGlobalDataIsCommon = true;
  emptyParamsMeansNoInfo = true;
  complainUponBadDeref = true;
  strictArraySizeRequirements = false;
  assumeNoSizeArrayHasSizeOne = false;
  allowOverloading = false;
  compoundSelfName = false;
  allowImplicitFunctionDecls = true;        // C89 does not require prototypes
  allowImplicitInt = true;
  allowDynamicallySizedArrays = false;
  allowIncompleteEnums = false;
  allowMemberWithClassName = true;
  nonstandardAssignmentOperator = false;
  allowExternCThrowMismatch = true;
  allowImplicitIntForMain = false;
  declareGNUBuiltins = false;
  allowGnuExternInlineFuncReplacement = false;

  isCplusplus = false;
  predefined_Bool = false;
}

void CCLang::KandR_C()
{
  ANSI_C89();

  allowImplicitInt = true;
}

void CCLang::ANSI_C99()
{
  ANSI_C89();

  // new features
  implicitFuncVariable = true;
  predefined_Bool = true;

  // removed C89 features
  allowImplicitInt = false;
  allowImplicitFunctionDecls = false;
}

void CCLang::GNU_C()
{
  ANSI_C89();

  // C99 features
  implicitFuncVariable = true;
  predefined_Bool = true;

  gccFuncBehavior = GFB_string;
  allowDynamicallySizedArrays = true;
  assumeNoSizeArrayHasSizeOne = true;
  declareGNUBuiltins = true;
  allowGnuExternInlineFuncReplacement = true;

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
  assumeNoSizeArrayHasSizeOne = true;
  allowIncompleteEnums = true;  // gnu according to Scott, above
  declareGNUBuiltins = true;
  allowGnuExternInlineFuncReplacement = true;

  // this seems wrong, but Oink's tests want it this way...
  predefined_Bool = true;
}

void CCLang::GNU2_KandR_C()
{
  GNU_KandR_C();

  // dsw: seems to not be true for gcc 2.96 at least
  predefined_Bool = false;
}


void CCLang::ANSI_Cplusplus()
{
  // just in case...
  memset(this, 0, sizeof(*this));

  tagsAreTypes = true;
  recognizeCppKeywords = true;
  implicitFuncVariable = false;
  gccFuncBehavior = GFB_none;
  noInnerClasses = false;
  uninitializedGlobalDataIsCommon = false;
  emptyParamsMeansNoInfo = false;

  // these aren't exactly ANSI C++; they might be "pragmatic C++"
  // for the current state of the parser
  complainUponBadDeref = false;
  strictArraySizeRequirements = false;
  assumeNoSizeArrayHasSizeOne = false;

  allowOverloading = true;
  compoundSelfName = true;

  allowImplicitFunctionDecls = false;
  allowImplicitInt = false;
  allowDynamicallySizedArrays = false;
  allowIncompleteEnums = false;
  allowMemberWithClassName = false;

  // indeed this is nonstandard but everyone seems to do it this way ...
  nonstandardAssignmentOperator = true;
  allowExternCThrowMismatch = false;
  allowImplicitIntForMain = false;

  predefined_Bool = false;
  declareGNUBuiltins = false;
  allowGnuExternInlineFuncReplacement = false;

  isCplusplus = true;
}

void CCLang::GNU_Cplusplus()
{
  ANSI_Cplusplus();

  gccFuncBehavior = GFB_variable;

  // is this really right?  Oink tests it like it is ...
  allowDynamicallySizedArrays = true;

  allowMemberWithClassName = true;
  allowExternCThrowMismatch = true;
  allowImplicitIntForMain = true;
  declareGNUBuiltins = true;
  allowGnuExternInlineFuncReplacement = true;
}


// EOF
