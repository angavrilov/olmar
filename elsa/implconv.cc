// implconv.cc                       see license.txt for copyright and terms of use
// code for implconv.h

#include "implconv.h"      // this module
#include "cc_env.h"        // Env
#include "variable.h"      // Variable
#include "overload.h"      // resolveOverload
#include "trace.h"         // tracingSys


// prototypes
StandardConversion tryCallCtor
  (Variable const *var, SpecialExpr special, Type const *src);



// ------------------- ImplicitConversion --------------------
char const * const ImplicitConversion::kindNames[NUM_KINDS] = {
  "IC_NONE",
  "IC_STANDARD",
  "IC_USER_DEFINED",
  "IC_ELLIPSIS",
  "IC_AMBIGUOUS"
};

void ImplicitConversion::addStdConv(StandardConversion newScs)
{
  if (kind != IC_NONE) {
    kind = IC_AMBIGUOUS;
    return;
  }

  kind = IC_STANDARD;
  scs = newScs;
}


void ImplicitConversion
  ::addUserConv(StandardConversion first, Variable const *userFunc,
                StandardConversion second)
{
  if (kind != IC_NONE) {
    kind = IC_AMBIGUOUS;
    return;
  }

  kind = IC_USER_DEFINED;
  scs = first;
  user = userFunc;
  scs2 = second;
}


void ImplicitConversion::addEllipsisConv()
{
  if (kind != IC_NONE) {
    kind = IC_AMBIGUOUS;
    return;
  }

  kind = IC_ELLIPSIS;
}


string ImplicitConversion::debugString() const
{
  stringBuilder sb;
  sb << kindNames[kind];

  if (kind == IC_STANDARD || kind == IC_USER_DEFINED) {
    sb << "(" << toString(scs); 

    if (kind == IC_USER_DEFINED) {
      sb << ", " << user->name << " @ " << toString(user->loc)
         << ", " << toString(scs2);
    }
    
    sb << ")";
  }
  
  return sb;
}


// --------------------- getImplicitConversion ---------------
ImplicitConversion getImplicitConversion
  (Env &env, SpecialExpr special, Type const *src, Type const *dest)
{
  ImplicitConversion ret;

  // check for a standard sequence
  {
    StandardConversion scs = 
      getStandardConversion(NULL /*errorMsg*/, special, src, dest);
    if (scs != SC_ERROR) {
      ret.addStdConv(scs);
    }
  }

  // check for a constructor to make the dest type; for this to
  // work, the dest type must be a class type or a const reference
  // to one
  if (dest->isCompoundType() ||
      (dest->asRvalC()->isCompoundType() &&
       dest->asRvalC()->isConst())) {
    CompoundType const *ct = dest->asRvalC()->asCompoundTypeC();

    // get the overload set of constructors
    Variable const *ctor = ct->getNamedFieldC(env.constructorSpecialName, env);
    if (!ctor) {
      // ideally we'd have at least one ctor for every class, but I
      // think my current implementation doesn't add all of the
      // implicit constructors.. and if there are only implicit
      // constructors, they're handled specially anyway (I think),
      // so we can just disregard the possibility of using one
    }
    else {
      if (ctor->overload) {
        // multiple ctors, resolve overloading; but don't further
        // consider user-defined conversions
        GrowArray<ArgumentInfo> argTypes(1);
        argTypes[0] = ArgumentInfo(special, src);
        TRACE("overload", "  overloaded call to constructor " << ct->name);
        ctor = resolveOverload(env, OF_NO_USER|OF_NO_ERRORS,
                               ctor->overload->set, argTypes);
        if (ctor) {
          TRACE("overload", "  selected constructor at " << toString(ctor->loc));
        }
        else {
          TRACE("overload", "  no constructor matches");
        }
      }
      
      if (ctor) {
        // only one ctor now.. can we call it?
        StandardConversion first = tryCallCtor(ctor, special, src);
        if (first != SC_ERROR) {
          // success
          ret.addUserConv(first, ctor, SC_IDENTITY);
        }
      }
    }
  }

  // TODO: take conversion functions into account

  return ret;
}


StandardConversion tryCallCtor
  (Variable const *var, SpecialExpr special, Type const *src)
{
  // certainly should be a function
  FunctionType *ft = var->type->asFunctionType();

  int numParams = ft->params.count();
  if (numParams == 0) {
    if (ft->acceptsVarargs()) {
      // I'm not sure about this.. there's no SC_ELLIPSIS..
      return SC_IDENTITY;
    }
    else {
      return SC_ERROR;
    }
  }

  if (numParams > 1) {
    if (ft->params.nthC(2)->value) {
      // the 2nd param has a default, which implies all params
      // after have defaults, so this is ok
    }
    else {
      return SC_ERROR;     // requires at least 2 arguments
    }
  }
  
  Variable const *param = ft->params.firstC();
  return getStandardConversion(NULL /*errorMsg*/, special, src, param->type);
}


// ----------------- test_getImplicitConversion ----------------
int getLine(SourceLoc loc)
{
  return sourceLocManager->getLine(loc);
}


bool matchesExpectation(ImplicitConversion const &actual,
  int expectedKind, int expectedSCS, int expectedUserLine, int expectedSCS2)
{
  if (expectedKind != actual.kind) return false;

  if (actual.kind == ImplicitConversion::IC_STANDARD) {
    return actual.scs == expectedSCS;
  }

  if (actual.kind == ImplicitConversion::IC_USER_DEFINED) {
    int actualLine = getLine(actual.user->loc);
    return actual.scs == expectedSCS &&
           actualLine == expectedUserLine &&
           actual.scs2 == expectedSCS2;
  }

  // other kinds are equal without further checking
  return true;
}


void test_getImplicitConversion(
  Env &env, SpecialExpr special, Type const *src, Type const *dest,
  int expectedKind, int expectedSCS, int expectedUserLine, int expectedSCS2)
{
  // grab existing error messages
  ErrorList existing;
  existing.takeMessages(env.errors);

  // run our function
  ImplicitConversion actual = getImplicitConversion(env, special, src, dest);

  // turn any resulting messags into warnings, so I can see their
  // results without causing the final exit status to be nonzero
  env.errors.markAllAsWarnings();
  
  // put the old messages back
  env.errors.takeMessages(existing);

  // did it behave as expected?
  bool matches = matchesExpectation(actual, expectedKind, expectedSCS,
                                            expectedUserLine, expectedSCS2);
  if (!matches || tracingSys("gIC")) {
    // construct a description of the actual result
    stringBuilder actualDesc;
    actualDesc << ImplicitConversion::kindNames[actual.kind];
    if (actual.kind == ImplicitConversion::IC_STANDARD ||
        actual.kind == ImplicitConversion::IC_USER_DEFINED) {
      actualDesc << "(" << toString(actual.scs);
      if (actual.kind == ImplicitConversion::IC_USER_DEFINED) {
        actualDesc << ", " << getLine(actual.user->loc)
                   << ", " << toString(actual.scs2);
      }
      actualDesc << ")";
    }

    // construct a description of the call site
    stringBuilder callDesc;
    callDesc << "getImplicitConversion("
             << toString(special) << ", `"
             << src->toString() << "', `"
             << dest->toString() << "')";

    if (!matches) {
      // construct a description of the expected result
      stringBuilder expectedDesc;
      xassert((unsigned)expectedKind <= (unsigned)ImplicitConversion::NUM_KINDS);
      expectedDesc << ImplicitConversion::kindNames[expectedKind];
      if (expectedKind == ImplicitConversion::IC_STANDARD ||
          expectedKind == ImplicitConversion::IC_USER_DEFINED) {
        expectedDesc << "(" << toString((StandardConversion)expectedSCS);
        if (expectedKind == ImplicitConversion::IC_USER_DEFINED) {
          expectedDesc << ", " << expectedUserLine
                       << ", " << toString((StandardConversion)expectedSCS2);
        }
        expectedDesc << ")";
      }

      env.error(stringc
        << callDesc << " yielded " << actualDesc
        << ", but I expected " << expectedDesc);
    }
    else {
      env.warning(stringc << callDesc << " yielded " << actualDesc);
    }
  }
}


// ----------------- CompressedImplicitConversion -----------------
void CompressedImplicitConversion::encode(ImplicitConversion const &ic)
{                                      
  // need at least 32 bits to work with
  STATIC_ASSERT(sizeof(unsigned) >= 4);

  kind_scs_scs2 = (unsigned)ic.kind | 
                  ((unsigned)ic.scs << 8) |
                  ((unsigned)ic.scs2 << 16);
  user = ic.user;
}

ImplicitConversion CompressedImplicitConversion::decode() const
{
  ImplicitConversion ic;

  ic.kind = (ImplicitConversion::Kind)(kind_scs_scs2 & 0xFF);
  xassert((unsigned)ic.kind < ImplicitConversion::NUM_KINDS);

  // it's difficult to check these for sanity because the encoding
  // is relatively dense
  ic.scs = (StandardConversion)((kind_scs_scs2 >> 8) & 0xFF);
  ic.scs2 = (StandardConversion)((kind_scs_scs2 >> 16) & 0xFF);

  ic.user = user;      
  
  return ic;
}


// EOF
