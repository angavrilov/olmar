// implconv.cc                       see license.txt for copyright and terms of use
// code for implconv.h

#include "implconv.h"      // this module
#include "cc_env.h"        // Env
#include "variable.h"      // Variable
#include "overload.h"      // resolveOverload


// prototypes
StandardConversion tryCallCtor
  (Variable const *var, SpecialExpr special, Type const *src);



// ------------------- ImplicitConversion --------------------
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


// --------------------- getImplicitConversion ---------------
ImplicitConversion getImplicitConversion
  (Env &env, SpecialExpr special, Type const *src, Type const *dest)
{
  ImplicitConversion ret;

  // check for a standard sequence
  {
    StandardConversion scs = 
      getStandardConversion(NULL /*env*/, special, src, dest);
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
        // multiple ctors, resolve overloading
        GrowArray<ArgumentInfo> argTypes(1);
        argTypes[0] = ArgumentInfo(special, src);
        ctor = resolveOverload(env, ctor->overload->set, argTypes);
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
    if (ft->acceptsVarargs) {
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
  return getStandardConversion(NULL /*env*/, special, src, param->type);
}




