// template.cc
// template stuff, pulled out from cc_env.cc and cc_tcheck.cc, possibly
// among others

#include "cc_env.h"        // this module
#include "trace.h"         // tracingSys
#include "strtable.h"      // StringTable
#include "cc_lang.h"       // CCLang
#include "strutil.h"       // pluraln
#include "overload.h"      // selectBestCandidate_templCompoundType
#include "matchtype.h"     // MatchType
#include "typelistiter.h"  // TypeListIter
#include "cc_ast_aux.h"    // ASTTemplVisitor


TD_class Env::mode1Dummy(NULL, NULL); // arguments are meaningless

/* static */
TemplCandidates::STemplateArgsCmp TemplCandidates::compareSTemplateArgs
  (TypeFactory &tfac, STemplateArgument const *larg, STemplateArgument const *rarg)
{
  xassert(larg->kind == rarg->kind);

  switch(larg->kind) {
  default:
    xfailure("illegal TemplateArgument kind");
    break;

  case STemplateArgument::STA_NONE: // not yet resolved into a valid template argument
    xfailure("STA_NONE TemplateArgument kind");
    break;

  case STemplateArgument::STA_TYPE: // type argument
    {
    // check if left is at least as specific as right
    bool leftAtLeastAsSpec;
    {
      MatchTypes match(tfac, MatchTypes::MM_WILD);
      if (match.match_Type(larg->value.t, rarg->value.t)) {
        leftAtLeastAsSpec = true;
      } else {
        leftAtLeastAsSpec = false;
      }
    }
    // check if right is at least as specific as left
    bool rightAtLeastAsSpec;
    {
      MatchTypes match(tfac, MatchTypes::MM_WILD);
      if (match.match_Type(rarg->value.t, larg->value.t)) {
        rightAtLeastAsSpec = true;
      } else {
        rightAtLeastAsSpec = false;
      }
    }

    // change of basis matrix
    if (leftAtLeastAsSpec) {
      if (rightAtLeastAsSpec) {
        return STAC_EQUAL;
      } else {
        return STAC_LEFT_MORE_SPEC;
      }
    } else {
      if (rightAtLeastAsSpec) {
        return STAC_RIGHT_MORE_SPEC;
      } else {
        return STAC_INCOMPARABLE;
      }
    }

    }
    break;

  case STemplateArgument::STA_INT: // int or enum argument
    if (larg->value.i == rarg->value.i) {
      return STAC_EQUAL;
    }
    return STAC_INCOMPARABLE;
    break;

  case STemplateArgument::STA_REFERENCE: // reference to global object
  case STemplateArgument::STA_POINTER: // pointer to global object
  case STemplateArgument::STA_MEMBER: // pointer to class member
    if (larg->value.v == rarg->value.v) {
      return STAC_EQUAL;
    }
    return STAC_INCOMPARABLE;
    break;

  case STemplateArgument::STA_TEMPLATE: // template argument (not implemented)
    xfailure("STA_TEMPLATE TemplateArgument kind; not implemented");
    break;
  }
}


/* static */
int TemplCandidates::compareCandidatesStatic
  (TypeFactory &tfac, TemplateInfo const *lti, TemplateInfo const *rti)
{
  // I do not even put the primary into the set so it should never
  // show up.
  xassert(lti->isNotPrimary());
  xassert(rti->isNotPrimary());

  // they should have the same primary
  xassert(lti->getMyPrimaryIdem() == rti->getMyPrimaryIdem());

  // they should always have the same number of arguments; the number
  // of parameters is irrelevant
  xassert(lti->arguments.count() == rti->arguments.count());

  STemplateArgsCmp leaning = STAC_EQUAL;// which direction are we leaning?
  // for each argument pairwise
  ObjListIter<STemplateArgument> lIter(lti->arguments);
  ObjListIter<STemplateArgument> rIter(rti->arguments);
  for(;
      !lIter.isDone();
      lIter.adv(), rIter.adv()) {
    STemplateArgument const *larg = lIter.data();
    STemplateArgument const *rarg = rIter.data();
    STemplateArgsCmp cmp = compareSTemplateArgs(tfac, larg, rarg);
    switch(cmp) {
    default: xfailure("illegal STemplateArgsCmp"); break;
    case STAC_LEFT_MORE_SPEC:
      if (leaning == STAC_EQUAL) {
        leaning = STAC_LEFT_MORE_SPEC;
      } else if (leaning == STAC_RIGHT_MORE_SPEC) {
        leaning = STAC_INCOMPARABLE;
      }
      // left stays left and incomparable stays incomparable
      break;
    case STAC_RIGHT_MORE_SPEC:
      if (leaning == STAC_EQUAL) {
        leaning = STAC_RIGHT_MORE_SPEC;
      } else if (leaning == STAC_LEFT_MORE_SPEC) {
        leaning = STAC_INCOMPARABLE;
      }
      // right stays right and incomparable stays incomparable
      break;
    case STAC_EQUAL:
      // all stay same
      break;
    case STAC_INCOMPARABLE:
      leaning = STAC_INCOMPARABLE; // incomparable is an absorbing state
    }
  }
  xassert(rIter.isDone());      // we checked they had the same number of arguments earlier

  switch(leaning) {
  default: xfailure("illegal STemplateArgsCmp"); break;
  case STAC_LEFT_MORE_SPEC: return -1; break;
  case STAC_RIGHT_MORE_SPEC: return 1; break;
  case STAC_EQUAL:
    // FIX: perhaps this should be a user error?
    xfailure("Two template argument tuples are identical");
    break;
  case STAC_INCOMPARABLE: return 0; break;
  }
}


int TemplCandidates::compareCandidates(Variable const *left, Variable const *right)
{
  TemplateInfo *lti = const_cast<Variable*>(left)->templateInfo();
  xassert(lti);
  TemplateInfo *rti = const_cast<Variable*>(right)->templateInfo();
  xassert(rti);

  return compareCandidatesStatic(tfac, lti, rti);
}

// --------------------- InstContext -----------------

InstContext::InstContext
  (Variable *baseV0,
   Variable *instV0,
   Scope *foundScope0,
   SObjList<STemplateArgument> &sargs0)
    : baseV(baseV0)
    , instV(instV0)
    , foundScope(foundScope0)
    , sargs(cloneSArgs(sargs0))
{}


// --------------------- PartialScopeStack -----------------

void PartialScopeStack::stackIntoEnv(Env &env)
{
  for (int i=scopes.count()-1; i>=0; --i) {
    Scope *s = this->scopes.nth(i);
    env.scopes.prepend(s);
  }
}


void PartialScopeStack::unStackOutOfEnv(Env &env)
{
  for (int i=0; i<scopes.count(); ++i) {
    Scope *s = this->scopes.nth(i);
    Scope *removed = env.scopes.removeFirst();
    xassert(s == removed);
  }
}


// --------------------- FuncTCheckContext -----------------

FuncTCheckContext::FuncTCheckContext
  (Function *func0,
   Scope *foundScope0,
   PartialScopeStack *pss0)
    : func(func0)
    , foundScope(foundScope0)
    , pss(pss0)
{}



// ----------------------- Env ----------------------------

PartialScopeStack *Env::shallowClonePartialScopeStack(Scope *foundScope)
{
  xassert(foundScope);
  PartialScopeStack *pss = new PartialScopeStack();
  int scopesCount = scopes.count();
  xassert(scopesCount>=2);
  for (int i=0; i<scopesCount; ++i) {
    Scope *s0 = scopes.nth(i);
    Scope *s1 = scopes.nth(i+1);
    // NOTE: omit foundScope itself AND the template scope below it
    if (s1 == foundScope) {
      xassert(s0->scopeKind == SK_TEMPLATE);
      return pss;
    }
    xassert(s0->scopeKind != SK_TEMPLATE);
    pss->scopes.append(s0);
  }
  xfailure("failed to find foundScope in the scope stack");
  return NULL;
}

      
// sm: Currently, we are maintaining two stacks plus one weird global
// all for the purpose of computing the tcheck mode.  I think we should
// just maintain the mode explicitly.  Note that class Restorer, in
// smbase/macros.h, can make restoring previous values convenient.
#warning I think the tcheck mode stacks are overkill.

Env::TemplTcheckMode Env::getTemplTcheckMode() const {
  if (templateDeclarationStack.isEmpty()
      || templateDeclarationStack.topC() == &mode1Dummy) {
    return TTM_1NORMAL;
  }
  // we are in a template declaration
  if (funcDeclStack.isEmpty()) return TTM_3TEMPL_DEF;
  // we are in a function declaration somewhere
  if (funcDeclStack.topC()->isInitializer()) return TTM_3TEMPL_DEF;
  // we are in a function declaration and not in a default argument
  xassert(funcDeclStack.topC()->isD_func());
  return TTM_2TEMPL_FUNC_DECL;
}


// FIX: this lookup doesn't do very well for overloaded function
// templates where one primary is more specific than the other; the
// more specific one matches both itself and the less specific one,
// and this gets called ambiguous
Variable *Env::lookupPQVariable_primary_resolve(
  PQName const *name, LookupFlags flags, FunctionType *signature,
  MatchTypes::MatchMode matchMode)
{
  // only makes sense in one context; will push this spec around later
  xassert(flags & LF_TEMPL_PRIMARY);

  // first, call the version that does *not* do overload selection
  Variable *var = lookupPQVariable(name, flags);

  if (!var) return NULL;
  OverloadSet *oloadSet = var->getOverloadSet();
  if (oloadSet->count() > 1) {
    xassert(var->type->isFunctionType()); // only makes sense for function types
    // FIX: Somehow I think this isn't right
    var = findTemplPrimaryForSignature(oloadSet, signature, matchMode);
    if (!var) {
      // FIX: sometimes I just want to know if there is one; not sure
      // this is always an error.
//        error("function template specialization does not match "
//              "any primary in the overload set");
      return NULL;
    }
  }
  return var;
}


Variable *Env::findTemplPrimaryForSignature
  (OverloadSet *oloadSet,
   FunctionType *signature,
   MatchTypes::MatchMode matchMode)
{
  if (!signature) {
    xfailure("This is one more place where you need to add a signature "
             "to the call to Env::lookupPQVariable() to deal with function "
             "template overload resolution");
    return NULL;
  }

  Variable *candidatePrim = NULL;
  SFOREACH_OBJLIST_NC(Variable, oloadSet->set, iter) {
    Variable *var0 = iter.data();
    // skip non-template members of the overload set
    if (!var0->isTemplate()) continue;

    TemplateInfo *tinfo = var0->templateInfo();
    xassert(tinfo);
    xassert(tinfo->isPrimary()); // can only have primaries at the top level

    // check that the function type could be a special case of the
    // template primary
    //
    // FIX: I don't know if this is really as precise a lookup as is
    // possible.
    MatchTypes match(tfac, matchMode);
    if (match.match_Type
        (signature,
         var0->type
         // FIX: I don't know if this should be top or not or if it
         // matters.
         )) {
      if (candidatePrim) {
        xfailure("ambiguous attempt to lookup "
                 "overloaded function template primary from specialization");
        return NULL;            // ambiguous
      } else {
        candidatePrim = var0;
      }
    }
  }
  return candidatePrim;
}


void Env::initArgumentsFromASTTemplArgs
  (TemplateInfo *tinfo,
   ASTList<TemplateArgument> const &templateArgs)
{
  xassert(tinfo);
  xassert(tinfo->arguments.count() == 0); // don't use this if there are already arguments
  FOREACH_ASTLIST(TemplateArgument, templateArgs, iter) {
    TemplateArgument const *targ = iter.data();
    xassert(targ->sarg.hasValue());
    tinfo->arguments.append(new STemplateArgument(targ->sarg));
  }
}


bool Env::checkIsoToASTTemplArgs
  (ObjList<STemplateArgument> &templateArgs0,
   ASTList<TemplateArgument> const &templateArgs1)
{
  MatchTypes match(tfac, MatchTypes::MM_ISO);
  ObjListIterNC<STemplateArgument> iter0(templateArgs0);
  FOREACH_ASTLIST(TemplateArgument, templateArgs1, iter1) {
    if (iter0.isDone()) return false;
    TemplateArgument const *targ = iter1.data();
    xassert(targ->sarg.hasValue());
    STemplateArgument sta(targ->sarg);
    if (!match.match_STA(iter0.data(), &sta, 2 /*matchDepth*/)) return false;
    iter0.adv();
  }
  return iter0.isDone();
}


Variable *Env::getInstThatMatchesArgs
  (TemplateInfo *tinfo, ObjList<STemplateArgument> &arguments, Type *type0)
{
  Variable *prevInst = NULL;
  SFOREACH_OBJLIST_NC(Variable, tinfo->getInstantiations(), iter) {
    Variable *instantiation = iter.data();
    if (type0 && !instantiation->getType()->equals(type0)) continue;
    MatchTypes match(tfac, MatchTypes::MM_ISO);
    bool unifies = match.match_Lists2
      (instantiation->templateInfo()->arguments, arguments, 2 /*matchDepth*/);
    if (unifies) {
      if (instantiation->templateInfo()->isMutant() &&
          prevInst->templateInfo()->isMutant()) {
        // if one mutant matches, more may, so just use the first one
        continue;
      }
      // any other combination of double matching is an error; that
      // is, I don't think you can get a non-mutant instance in more
      // than once
      xassert(!prevInst);
      prevInst = instantiation;
    }
  }
  return prevInst;
}


bool Env::loadBindingsWithExplTemplArgs(Variable *var, ASTList<TemplateArgument> const &args,
                                        MatchTypes &match)
{
  xassert(var->templateInfo());
  xassert(var->templateInfo()->isPrimary());

  SObjListIterNC<Variable> paramIter(var->templateInfo()->params);
  ASTListIter<TemplateArgument> argIter(args);
  while (!paramIter.isDone() && !argIter.isDone()) {
    Variable *param = paramIter.data();

    // no bindings yet and param names unique
    //
    // FIX: maybe I shouldn't assume that param names are
    // unique; then this would be a user error
    if (param->type->isTypeVariable()) {
      xassert(!match.bindings.getTypeVar(param->type->asTypeVariable()));
    } else {
      // for, say, int template parameters
      xassert(!match.bindings.getObjVar(param));
    }

    TemplateArgument const *arg = argIter.data();

    // FIX: when it is possible to make a TA_template, add
    // check for it here.
    //              xassert("Template template parameters are not implemented");

    if (param->hasFlag(DF_TYPEDEF) && arg->isTA_type()) {
      STemplateArgument *bound = new STemplateArgument;
      bound->setType(arg->asTA_typeC()->type->getType());
      match.bindings.putTypeVar(param->type->asTypeVariable(), bound);
    }
    else if (!param->hasFlag(DF_TYPEDEF) && arg->isTA_nontype()) {
      STemplateArgument *bound = new STemplateArgument;
      Expression *expr = arg->asTA_nontypeC()->expr;
      if (expr->getType()->isIntegerType()) {
        int staticTimeValue;
        bool constEvalable = expr->constEval(*this, staticTimeValue);
        if (!constEvalable) {
          // error already added to the environment
          return false;
        }
        bound->setInt(staticTimeValue);
      } else if (expr->getType()->isReference()) {
        // Subject: Re: why does STemplateArgument hold Variables?
        // From: Scott McPeak <smcpeak@eecs.berkeley.edu>
        // To: "Daniel S. Wilkerson" <dsw@eecs.berkeley.edu>
        // The Variable is there because STA_REFERENCE (etc.)
        // refer to (linker-visible) symbols.  You shouldn't
        // be making an STemplateArgument out of an expression
        // directly; if the template parameter is
        // STA_REFERENCE (etc.) then dig down to find the
        // Variable the programmer wants to use.
        //
        // haven't tried to exhaustively enumerate what kinds
        // of expressions this could be; handle other types as
        // they come up
        xassert(expr->isE_variable());
        bound->setReference(expr->asE_variable()->var);
      } else {
        unimp("unhandled kind of template argument");
        return false;
      }
      match.bindings.putObjVar(param, bound);
    }
    else {
      // mismatch between argument kind and parameter kind
      char const *paramKind = param->hasFlag(DF_TYPEDEF)? "type" : "non-type";
      char const *argKind = arg->isTA_type()? "type" : "non-type";
      // FIX: make a provision for template template parameters here

      // NOTE: condition this upon a boolean flag reportErrors if it
      // starts to fail while filtering functions for overload
      // resolution; see Env::inferTemplArgsFromFuncArgs() for an
      // example
      error(stringc
            << "`" << param->name << "' is a " << paramKind
            << " parameter, but `" << arg->argString() << "' is a "
            << argKind << " argument",
            EF_STRONG);
      return false;
    }
    paramIter.adv();
    argIter.adv();
  }
  return true;
}


bool Env::inferTemplArgsFromFuncArgs
  (Variable *var,
   TypeListIter &argListIter,
   MatchTypes &match,
   bool reportErrors)
{
  xassert(var->templateInfo());
  xassert(var->templateInfo()->isPrimary());

  if (tracingSys("template")) {
    cout << "Deducing template arguments from function arguments" << endl;
  }
  // FIX: make this work for vararg functions
  int i = 1;            // for error messages
  SFOREACH_OBJLIST_NC(Variable, var->type->asFunctionType()->params, paramIter) {
    Variable *param = paramIter.data();
    xassert(param);
    // we could run out of args and it would be ok as long as we
    // have default arguments for the rest of the parameters
    if (!argListIter.isDone()) {
      Type *curArgType = argListIter.data();
      bool argUnifies = match.match_Type(curArgType, param->type);
      if (!argUnifies) {
        if (reportErrors) {
          error(stringc << "during function template instantiation: "
                << " argument " << i << " `" << curArgType->toString() << "'"
                << " is incompatable with parameter, `"
                << param->type->toString() << "'");
        }
        return false;             // FIX: return a variable of type error?
      }
    } else {
      // we don't use the default arugments for template arugment
      // inference, but there should be one anyway.
      if (!param->value) {
        if (reportErrors) {
          error(stringc << "during function template instantiation: too few arguments to " <<
                var->name);
        }
        return false;
      }
    }
    ++i;
    // advance the argIterCur if there is one
    if (!argListIter.isDone()) argListIter.adv();
  }
  if (!argListIter.isDone()) {
    if (reportErrors) {
      error(stringc << "during function template instantiation: too many arguments to " <<
            var->name);
    }
    return false;
  }
  return true;
}


bool Env::getFuncTemplArgs
  (MatchTypes &match,
   SObjList<STemplateArgument> &sargs,
   PQName const *final,
   Variable *var,
   TypeListIter &argListIter,
   bool reportErrors)
{
  xassert(var->templateInfo()->isPrimary());

  // 'final' might be NULL in the case of doing overload resolution
  // for templatized ctors (that is, the ctor is templatized, but not
  // necessarily the class)
  if (final && final->isPQ_template()) {
    if (!loadBindingsWithExplTemplArgs(var, final->asPQ_templateC()->args, match)) {
      return false;
    }
  }

  if (!inferTemplArgsFromFuncArgs(var, argListIter, match, reportErrors)) {
    return false;
  }

  // put the bindings in a list in the right order; FIX: I am rather
  // suspicious that the set of STemplateArgument-s should be
  // everywhere represented as a map instead of as a list
  //
  // try to report more than just one at a time if we can
  bool haveAllArgs = true;
  SFOREACH_OBJLIST_NC(Variable, var->templateInfo()->params, templPIter) {
    Variable *param = templPIter.data();
    STemplateArgument *sta = NULL;
    if (param->type->isTypeVariable()) {
      sta = match.bindings.getTypeVar(param->type->asTypeVariable());
    } else {
      sta = match.bindings.getObjVar(param);
    }
    if (!sta) {
      if (reportErrors) {
        error(stringc << "No argument for parameter `" << templPIter.data()->name << "'");
      }
      haveAllArgs = false;
    }
    sargs.append(sta);
  }
  return haveAllArgs;
}


// lookup with template argument inference based on argument expressions
Variable *Env::lookupPQVariable_function_with_args(
  PQName const *name, LookupFlags flags,
  FakeList<ArgExpression> *funcArgs)
{
  // first, lookup the name but return just the template primary
  Scope *scope;      // scope where found
  Variable *var = lookupPQVariable(name, flags | LF_TEMPL_PRIMARY, scope);
  if (!var || !var->isTemplate()) {
    // nothing unusual to do
    return var;
  }

  if (!var->isTemplateFunction()) {
    // most of the time this can't happen, b/c the func/ctor ambiguity
    // resolution has already happened and inner2 is only called when
    // the 'var' looks ok; but if there are unusual ambiguities, e.g.
    // in/t0171.cc, then we get here even after 'var' has yielded an
    // error.. so just bail, knowing that an error has already been
    // reported (hence the ambiguity will be resolved as something else)
    return NULL;
  }

  // if we are inside a function definition, just return the primary
  TemplTcheckMode ttm = getTemplTcheckMode();
  // FIX: this can happen when in the parameter list of a function
  // template definition a class template is instantiated (as a
  // Mutant)
  // UPDATE: other changes should prevent this
  xassert(ttm != TTM_2TEMPL_FUNC_DECL);
  if (ttm == TTM_2TEMPL_FUNC_DECL || ttm == TTM_3TEMPL_DEF) {
    xassert(var->templateInfo()->isPrimary());
    return var;
  }

  PQName const *final = name->getUnqualifiedNameC();

  // duck overloading
  OverloadSet *oloadSet = var->getOverloadSet();
  if (oloadSet->count() > 1) {
    xassert(var->type->isFunctionType()); // only makes sense for function types
    // FIX: the correctness of this depends on someone doing
    // overload resolution later, which I don't think is being
    // done.
    return var;
    // FIX: this isn't right; do overload resolution later;
    // otherwise you get a null signature being passed down
    // here during E_variable::itcheck_x()
    //              var = oloadSet->findTemplPrimaryForSignature(signature);
    // // FIX: make this a user error, or delete it
    // xassert(var);
  }
  xassert(var->templateInfo()->isPrimary());

  // get the semantic template arguments
  SObjList<STemplateArgument> sargs;
  {
    TypeListIter_FakeList argListIter(funcArgs);
    MatchTypes match(tfac, MatchTypes::MM_BIND);
    if (!getFuncTemplArgs(match, sargs, final, var, argListIter, true /*reportErrors*/)) {
      return NULL;
    }
  }

  // apply the template arguments to yield a new type based on
  // the template; note that even if we had some explicit
  // template arguments, we have put them into the bindings now,
  // so we omit them here
  return instantiateTemplate(loc(), scope, var, NULL /*inst*/, NULL /*bestV*/, sargs);
}


static bool doesUnificationRequireBindings
  (TypeFactory &tfac,
   SObjList<STemplateArgument> &sargs,
   ObjList<STemplateArgument> &arguments)
{
  // re-unify and check that no bindings get added
  MatchTypes match(tfac, MatchTypes::MM_BIND);
  bool unifies = match.match_Lists(sargs, arguments, 2 /*matchDepth*/);
  xassert(unifies);             // should of course still unify
  // bindings should be trivial for a complete specialization
  // or instantiation
  return !match.bindings.isEmpty();
}


void Env::insertBindingsForPrimary
  (Variable *baseV, SObjList<STemplateArgument> &sargs)
{
  xassert(baseV);
  SObjListIter<Variable> paramIter(baseV->templateInfo()->params);
  SObjListIterNC<STemplateArgument> argIter(sargs);
  while (!paramIter.isDone()) {
    Variable const *param = paramIter.data();
    
    // if we have exhaused the explicit arguments, use a NULL 'sarg'
    // to indicate that we need to use the default arguments from
    // 'param' (if available)
    STemplateArgument *sarg = argIter.isDone()? NULL : argIter.data();

    if (sarg && sarg->isTemplate()) {
      xassert("Template template parameters are not implemented");
    }

    if (param->hasFlag(DF_TYPEDEF) &&
        (!sarg || sarg->isType())) {
      if (!sarg && !param->defaultParamType) {
        error(stringc
          << "too few template arguments to `" << baseV->name << "'");
        return;
      }

      // bind the type parameter to the type argument
      Type *t = sarg? sarg->getType() : param->defaultParamType;
      Variable *binding = makeVariable(param->loc, param->name,
                                       t, DF_TYPEDEF);
      addVariable(binding);
    }
    else if (!param->hasFlag(DF_TYPEDEF) &&
             (!sarg || sarg->isObject())) {
      if (!sarg && !param->value) {
        error(stringc
          << "too few template arguments to `" << baseV->name << "'");
        return;
      }

      // TODO: verify that the argument in fact matches the parameter type

      // bind the nontype parameter directly to the nontype expression;
      // this will suffice for checking the template body, because I
      // can const-eval the expression whenever it participates in
      // type determination; the type must be made 'const' so that
      // E_variable::constEval will believe it can evaluate it
      Type *bindType = tfac.applyCVToType(param->loc, CV_CONST,
                                          param->type, NULL /*syntax*/);
      Variable *binding = makeVariable(param->loc, param->name,
                                       bindType, DF_NONE);

      // set 'bindings->value', in some cases creating AST
      if (!sarg) {
        binding->value = param->value;

        // sm: the statement above seems reasonable, but I'm not at
        // all convinced it's really right... has it been tcheck'd?
        // has it been normalized?  are these things necessary?  so
        // I'll wait for a testcase to remove this assertion... before
        // this assertion *is* removed, someone should read over the
        // applicable parts of cppstd
        xfailure("unimplemented: default non-type argument");
      }
      else if (sarg->kind == STemplateArgument::STA_INT) {
        E_intLit *value0 = buildIntegerLiteralExp(sarg->getInt());
        // FIX: I'm sure we can do better than SL_UNKNOWN here
        value0->type = tfac.getSimpleType(SL_UNKNOWN, ST_INT, CV_CONST);
        binding->value = value0;
      } 
      else if (sarg->kind == STemplateArgument::STA_REFERENCE) {
        E_variable *value0 = new E_variable(NULL /*PQName*/);
        value0->var = sarg->getReference();
        binding->value = value0;
      }
      else {
        unimp(stringc << "STemplateArgument objects that are of kind other than "
              "STA_INT are not implemented: " << sarg->toString());
        return;
      }
      xassert(binding->value);
      addVariable(binding);
    }
    else {                 
      // mismatch between argument kind and parameter kind
      char const *paramKind = param->hasFlag(DF_TYPEDEF)? "type" : "non-type";
      // FIX: make a provision for template template parameters here
      char const *argKind = sarg->isType()? "type" : "non-type";
      error(stringc
            << "`" << param->name << "' is a " << paramKind
            << " parameter, but `" << sarg->toString() << "' is a "
            << argKind << " argument", EF_STRONG);
    }

    paramIter.adv();
    if (!argIter.isDone()) {
      argIter.adv();
    }
  }

  xassert(paramIter.isDone());
  if (!argIter.isDone()) {
    error(stringc
          << "too many template arguments to `" << baseV->name << "'", EF_STRONG);
  }
}


void Env::insertBindingsForPartialSpec
  (Variable *baseV, MatchBindings &bindings)
{
  for(SObjListIterNC<Variable> paramIter(baseV->templateInfo()->params);
      !paramIter.isDone();
      paramIter.adv()) {
    Variable *param = paramIter.data();
    STemplateArgument *arg = NULL;
    if (param->type->isTypeVariable()) {
      arg = bindings.getTypeVar(param->type->asTypeVariable());
    } else {
      arg = bindings.getObjVar(param);
    }
    if (!arg) {
      error(stringc
            << "during partial specialization parameter `" << param->name
            << "' not bound in inferred bindings", EF_STRONG);
      return;
    }

    if (param->hasFlag(DF_TYPEDEF) &&
        arg->isType()) {
      // bind the type parameter to the type argument
      Variable *binding =
        makeVariable(param->loc, param->name,
                     // FIX: perhaps this clone type is gratuitous
                     tfac.cloneType(arg->value.t),
                     DF_TYPEDEF);
      addVariable(binding);
    }
    else if (!param->hasFlag(DF_TYPEDEF) &&
             arg->isObject()) {
      if (param->type->isIntegerType() && arg->kind == STemplateArgument::STA_INT) {
        // bind the int parameter directly to the int expression; this
        // will suffice for checking the template body, because I can
        // const-eval the expression whenever it participates in type
        // determination; the type must be made 'const' so that
        // E_variable::constEval will believe it can evaluate it
        Type *bindType =
          // FIX: perhaps this clone type is gratuitous
          tfac.cloneType(tfac.applyCVToType
                         (param->loc, CV_CONST,
                          param->type, NULL /*syntax*/));
        Variable *binding = makeVariable(param->loc, param->name,
                                         bindType, DF_NONE);
        // we omit the text as irrelevant and expensive to reconstruct
        binding->value = new E_intLit(NULL);
        binding->value->asE_intLit()->i = arg->value.i;
        addVariable(binding);
      } else {
        unimp(stringc
              << "non-integer non-type non-template template arguments not implemented");
      }
    }
    else {                 
      // mismatch between argument kind and parameter kind
      char const *paramKind = param->hasFlag(DF_TYPEDEF)? "type" : "non-type";
      char const *argKind = arg->isType()? "type" : "non-type";
      error(stringc
            << "`" << param->name << "' is a " << paramKind
            // FIX: might do better than what toString() gives
            << " parameter, but `" << arg->toString() << "' is a "
            << argKind << " argument", EF_STRONG);
    }
  }

  // Note that it is not necessary here to attempt to detect too few
  // or too many arguments for the given parameters.  If the number
  // doesn't match then the unification will fail and we will attempt
  // to instantiate the primary, which will then report this error.
}


void Env::insertBindings(Variable *baseV, SObjList<STemplateArgument> &sargs)
{
  if (baseV->templateInfo()->isPartialSpec()) {
    // unify again to compute the bindings again since we forgot
    // them already
    MatchTypes match(tfac, MatchTypes::MM_BIND);
    bool matches = match.match_Lists(sargs, baseV->templateInfo()->arguments, 2 /*matchDepth*/);
    xassert(matches);           // this is bad if it fails
    // FIX: this
    //        if (tracingSys("template")) {
    //          cout << "bindings generated by unification with partial-specialization "
    //            "specialization-pattern" << endl;
    //          printBindings(match.bindings);
    //        }
    insertBindingsForPartialSpec(baseV, match.bindings);
  } else {
    xassert(baseV->templateInfo()->isPrimary());
    insertBindingsForPrimary(baseV, sargs);
  }
}


// go over the list of arguments, and make a list of semantic
// arguments
void Env::templArgsASTtoSTA
  (ASTList<TemplateArgument> const &arguments,
   SObjList<STemplateArgument> &sargs)
{
  FOREACH_ASTLIST(TemplateArgument, arguments, iter) {
    // the caller wants me not to modify the list, and I am not going
    // to, but I need to put non-const pointers into the 'sargs' list
    // since that's what the interface to 'equalArguments' expects..
    // this is a problem with const-polymorphism again
    TemplateArgument *ta = const_cast<TemplateArgument*>(iter.data());
    if (!ta->sarg.hasValue()) {
      error(stringc << "TemplateArgument has no value " << ta->argString(), EF_STRONG);
      return;
    }
    sargs.append(&(ta->sarg));
  }
}


Variable *Env::instantiateTemplate_astArgs
  (SourceLoc loc, Scope *foundScope, 
   Variable *baseV, Variable *instV,
   ASTList<TemplateArgument> const &astArgs)
{
  SObjList<STemplateArgument> sargs;
  templArgsASTtoSTA(astArgs, sargs);
  return instantiateTemplate(loc, foundScope, baseV, instV, NULL /*bestV*/, sargs);
}


MatchTypes::MatchMode Env::mapTcheckModeToTypeMatchMode(TemplTcheckMode tcheckMode)
{
  // map the typechecking mode to the type matching mode
  MatchTypes::MatchMode matchMode = MatchTypes::MM_NONE;
  switch(tcheckMode) {
  default: xfailure("bad mode"); break;
  case TTM_1NORMAL:
    matchMode = MatchTypes::MM_BIND;
    break;
  case TTM_2TEMPL_FUNC_DECL:
    matchMode = MatchTypes::MM_ISO;
    break;
  case TTM_3TEMPL_DEF:
    xfailure("mapTcheckModeToTypeMatchMode: shouldn't be here TTM_3TEMPL_DEF mode");
    break;
  }
  return matchMode;
}


Variable *Env::findMostSpecific(Variable *baseV, SObjList<STemplateArgument> &sargs)
{
  // baseV should be a template primary
  xassert(baseV->templateInfo()->isPrimary());
  if (tracingSys("template")) {
    cout << "Env::instantiateTemplate: "
         << "template instantiation, searching instantiations of primary "
         << "for best match to the template arguments"
         << endl;
    baseV->templateInfo()->debugPrint();
  }

  MatchTypes::MatchMode matchMode = mapTcheckModeToTypeMatchMode(getTemplTcheckMode());

  // iterate through all of the instantiations and build up an
  // ObjArrayStack<Candidate> of candidates
  TemplCandidates templCandidates(tfac);
  SFOREACH_OBJLIST_NC(Variable, baseV->templateInfo()->getInstantiations(), iter) {
    Variable *var0 = iter.data();
    TemplateInfo *templInfo0 = var0->templateInfo();
    xassert(templInfo0);        // should have templateness
    // Sledgehammer time.
    if (templInfo0->isMutant()) continue;
    // see if this candidate matches
    MatchTypes match(tfac, matchMode);
    if (match.match_Lists(sargs, templInfo0->arguments, 2 /*matchDepth*/)) {
      templCandidates.candidates.push(var0);
    }
  }

  // there are no candidates so we just use the primary
  if (templCandidates.candidates.isEmpty()) {
    return baseV;
  }

  // if there are any candidates to try, select the best
  Variable *bestV = selectBestCandidate_templCompoundType(templCandidates);

  // if there is not best candidiate, then the call is ambiguous and
  // we should deal with that error
  if (!bestV) {
    // FIX: what is the right thing to do here?
    error(stringc << "ambiguous attempt to instantiate template", EF_STRONG);
    return NULL;
  }
  // otherwise, use the best one

  // if the best is an instantiation/complete specialization check
  // that no bindings get generated during unification
  if (bestV->templateInfo()->isCompleteSpecOrInstantiation()) {
    xassert(!doesUnificationRequireBindings
            (tfac, sargs, bestV->templateInfo()->arguments));
  }
  // otherwise it is a partial specialization

  return bestV;
}


// remove scopes from the environment until the innermost
// scope on the scope stack is the same one that the template
// definition appeared in; template definitions can see names
// visible from their defining scope only [cppstd 14.6 para 1]
//
// update: (e.g. t0188.cc) pop scopes until we reach one that
// *contains* (or equals) the defining scope
//
// 4/20/04: Even more (e.g. t0204.cc), we need to push scopes
// to dig back down to the defining scope.  So the picture now
// looks like this:
//
//       global                   /---- this is "foundScope"
//         |                     /
//         namespace1           /   }
//         | |                 /    }- 2. push these: "pushedScopes"
//         | namespace11  <---/     }
//         |   |
//         |   template definition
//         |
//         namespace2               }
//           |                      }- 1. pop these: "poppedScopes"
//           namespace21            }
//             |
//             point of instantiation
//
// actually, it's *still* not right, because
//   - this allows the instantiation to see names declared in
//     'namespace11' that are below the template definition, and
//   - it's entirely wrong for dependent names, a concept I
//     largely ignore at this time
// however, I await more examples before continuing to refine
// my approximation to the extremely complex lookup rules
Scope *Env::prepArgScopeForTemlCloneTcheck
  (ObjList<Scope> &poppedScopes, SObjList<Scope> &pushedScopes, Scope *foundScope)
{
  xassert(foundScope);
  // pop scope scopes
  while (!scopes.first()->enclosesOrEq(foundScope)) {
    poppedScopes.prepend(scopes.removeFirst());
    if (scopes.isEmpty()) {
      xfailure("emptied scope stack searching for defining scope");
    }
  }

  // make a list of the scopes to push; these form a path from our
  // current scope to the 'foundScope'
  Scope *s = foundScope;
  while (s != scopes.first()) {
    pushedScopes.prepend(s);
    s = s->parentScope;
    if (!s) {
      if (scopes.first()->isGlobalScope()) {
        // ok, hit the global scope in the traversal
        break;
      }
      else {
        xfailure("missed the current scope while searching up "
                 "from the defining scope");
      }
    }
  }

  // now push them in list order, which means that 'foundScope'
  // will be the last one to be pushed, and hence the innermost
  // (I waited until now b/c this is the opposite order from the
  // loop above that fills in 'pushedScopes')
  SFOREACH_OBJLIST_NC(Scope, pushedScopes, iter) {
    // Scope 'iter.data()' is now on both lists, but neither owns
    // it; 'scopes' does not own Scopes that are named, as explained
    // in the comments near its declaration (cc_env.h)
    scopes.prepend(iter.data());
  }

  // make a new scope for the template arguments
  Scope *argScope = enterScope(SK_TEMPLATE, "template argument bindings");

  return argScope;
}


void Env::unPrepArgScopeForTemlCloneTcheck
  (Scope *argScope, ObjList<Scope> &poppedScopes, SObjList<Scope> &pushedScopes)
{
  // remove the argument scope
  exitScope(argScope);

  // restore the original scope structure
  pushedScopes.reverse();
  while (pushedScopes.isNotEmpty()) {
    // make sure the ones we're removing are the ones we added
    xassert(scopes.first() == pushedScopes.first());
    scopes.removeFirst();
    pushedScopes.removeFirst();
  }

  // re-add the inner scopes removed above
  while (poppedScopes.isNotEmpty()) {
    scopes.prepend(poppedScopes.removeFirst());
  }
}


static bool doSTemplArgsContainVars(SObjList<STemplateArgument> &sargs)
{
  SFOREACH_OBJLIST_NC(STemplateArgument, sargs, iter) {
    if (iter.data()->containsVariables()) return true;
  }
  return false;
}


// see comments in cc_env.h
//
// sm: TODO: I think this function should be split into two, one for
// classes and one for functions.  To the extent they share mechanism
// it would be better to encapsulate the mechanism than to share it
// by threading two distinct flow paths through the same code.
//
//
// dsw: There are three dimensions to the space of paths through this
// code.
//
// 1 - class or function template
// 
// 2 - baseForward: true exactly when we are typechecking a forward
// declaration and not a definition.
//
// 3 - instV: non-NULL when
//   a) a template primary was forward declared
//   b) it has now been defined
//   c) it has forwarded instantiations (& specializations?)  which
//   when they were instantiated, did not find a specialization and so
//   they are now being instantiated
// Note the test 'if (!instV)' below.  In this case, we are always
// going to instantiate the primary and not the specialization,
// because if the specialization comes after the instantiation point,
// it is not relevant anyway.
//
//
// dsw: We have three typechecking modes:
//
//   1) normal: template instantiations are done fully;
//
//   2) template function declaration: declarations are instantiated
//   but not definitions;
//
//   3) template definition: template instantiation request just result
//   in the primary being returned.
//
// Further, inside template definitions, default arguments are not
// typechecked and function template calls do not attempt to infer
// their template arguments from their function arguments.
//
// Two stacks in Env allow us to distinguish these modes record when
// the type-checker is typechecking a node below a TemplateDeclaration
// and a D_func/Initializer respectively.  See the implementation of
// Env::getTemplTcheckMode() for the details.
Variable *Env::instantiateTemplate
  (SourceLoc loc,
   Scope *foundScope,
   Variable *baseV,
   Variable *instV,
   Variable *bestV,
   SObjList<STemplateArgument> &sargs,
   Variable *funcFwdInstV)
{
  // NOTE: There is an ordering bug here, e.g. it fails t0209.cc.  The
  // problem here is that we choose an instantiation (or create one)
  // based only on the arguments explicitly present.  Since default
  // arguments are delayed until later, we don't realize that C<> and
  // C<int> are the same class if it has a default argument of 'int'.
  //
  // The fix I think is to insert argument bindings first, *then*
  // search for or create an instantiation.  In some cases this means
  // we bind arguments and immediately throw them away (i.e. when an
  // instantiation already exists), but I don't see any good
  // alternatives.

  xassert(baseV->templateInfo()->isPrimary());
  // if we are in a template definition typechecking mode just return
  // the primary
  TemplTcheckMode tcheckMode = getTemplTcheckMode();
  if (tcheckMode == TTM_3TEMPL_DEF) {
    return baseV;
  }

  // maintain the inst loc stack (ArrayStackPopper is in smbase/array.h)
  ArrayStackPopper<SourceLoc> pusher_popper(instantiationLocStack, loc /*pushVal*/);

  // 1 **** what template are we going to instanitate?  We now find
  // the instantiation/specialization/primary most specific to the
  // template arguments we have been given.  If it is a complete
  // specialization, we use it as-is.  If it is a partial
  // specialization, we reassign baseV.  Otherwise we fall back to the
  // primary.  At the end of this section if we have not returned then
  // baseV is the template to instantiate.

  Variable *oldBaseV = baseV;   // save baseV for other uses later
  // has this class already been instantiated?
  if (!instV) {
    // Search through the instantiations of this primary and do an
    // argument/specialization pattern match.
    if (!bestV) {
      // FIX: why did I do this?  It doesn't work.
//        if (tcheckMode == TTM_2TEMPL_FUNC_DECL) {
//          // in mode 2, just instantiate the primary
//          bestV = baseV;
//        }
      bestV = findMostSpecific(baseV, sargs);
      // if no bestV then the lookup was ambiguous, error has been reported
      if (!bestV) return NULL;
    }
    if (bestV->templateInfo()->isCompleteSpecOrInstantiation()) {
      return bestV;
    }
    baseV = bestV;              // baseV is saved in oldBaseV above
  }
  // there should be something non-trivial to instantiate
  xassert(baseV->templateInfo()->argumentsContainVariables()
          // or the special case of a primary, which doesn't have
          // arguments but acts as if it did
          || baseV->templateInfo()->isPrimary()
          );

  // render the template arguments into a string that we can use
  // as the name of the instantiated class; my plan is *not* that
  // this string serve as the unique ID, but rather that it be
  // a debugging aid only
  //
  // dsw: UPDATE: now it's used during type construction for
  // elaboration, so has to be just the base name
  StringRef instName;
  bool baseForward = false;
  bool sTemplArgsContainVars = doSTemplArgsContainVars(sargs);
  if (baseV->type->isCompoundType()) {
    if (tcheckMode == TTM_2TEMPL_FUNC_DECL) {
      // This ad-hoc condition is to prevent a class template
      // instantiation in mode 2 that happens to not have any
      // quantified template variables from being treated as
      // forwareded.  Otherwise, what will happen is that if the
      // same template arguments are seen in mode 3, the class will
      // not be re-instantiated and a normal mode 3 class will have
      // no body.  To see this happen turn of the next line and run
      // in/d0060.cc and watch it fail.  Now comment out the 'void
      // g(E<A> &a)' in d0060.cc and run again and watch it pass.
      if (sTemplArgsContainVars) {
        // don't instantiate the template in a function template
        // definition parameter list
        baseForward = true;
      }
    } else {
      baseForward = baseV->type->asCompoundType()->forward;
    }
    instName = baseV->type->asCompoundType()->name;
  } else {
    xassert(baseV->type->isFunctionType());
    // we should never get here in TTM_2TEMPL_FUNC_DECL mode
    xassert(tcheckMode != TTM_2TEMPL_FUNC_DECL);
    Declaration *declton = baseV->templateInfo()->declSyntax;
    if (declton && !baseV->funcDefn) {
      Declarator *decltor = declton->decllist->first();
      xassert(decltor);
      // FIX: is this still necessary?
      baseForward = (decltor->context == DC_TD_PROTO);
      // lets find out
      xassert(baseForward);
    } else {
      baseForward = false;
    }
    instName = baseV->name;     // FIX: just something for now
  }

  // A non-NULL instV is marked as no longer forwarded before being
  // passed in.
  if (instV) {
    xassert(!baseForward);
  }
  TRACE("template",    (baseForward ? "(forward) " : "")
                    << "instantiating class: "
                    << instName << sargsToString(sargs));

  // 2 **** Prepare the argument scope for template instantiation

  ObjList<Scope> poppedScopes;
  SObjList<Scope> pushedScopes;
  Scope *argScope = NULL;
  // don't mess with it if not going to tcheck anything; we need it in
  // either case for function types
  if (!(baseForward && baseV->type->isCompoundType())) {
    argScope = prepArgScopeForTemlCloneTcheck(poppedScopes, pushedScopes, foundScope);
    insertBindings(baseV, sargs);
  }

  // 3 **** make a copy of the template definition body AST (unless
  // this is a forward declaration)

  TS_classSpec *copyCpd = NULL;
  TS_classSpec *cpdBaseSyntax = NULL; // need this below
  Function *copyFun = NULL;
  if (baseV->type->isCompoundType()) {
    cpdBaseSyntax = baseV->type->asCompoundType()->syntax;
    if (baseForward) {
      copyCpd = NULL;
    } else {
      xassert(cpdBaseSyntax);
      copyCpd = cpdBaseSyntax->clone();
    }
  } else {
    xassert(baseV->type->isFunctionType());
    Function *baseSyntax = baseV->funcDefn;
    if (baseForward) {
      copyFun = NULL;
    } else {
      xassert(baseSyntax);
      copyFun = baseSyntax->clone();
    }
  }

  // FIX: merge these
  if (copyCpd && tracingSys("cloneAST")) {
    cout << "--------- clone of " << instName << " ------------\n";
    copyCpd->debugPrint(cout, 0);
  }
  if (copyFun && tracingSys("cloneAST")) {
    cout << "--------- clone of " << instName << " ------------\n";
    copyFun->debugPrint(cout, 0);
  }

  // 4 **** make the Variable that will represent the instantiated
  // class during typechecking of the definition; this allows template
  // classes that refer to themselves and recursive function templates
  // to work.  The variable is assigned to instV; if instV already
  // exists, this work does not need to be done: the template was
  // previously forwarded and that forwarded definition serves the
  // purpose.

  if (!instV) {
    // copy over the template arguments so we can recognize this
    // instantiation later
    StringRef name = baseV->type->isCompoundType()
      ? baseV->type->asCompoundType()->name    // no need to call 'str', 'name' is already a StringRef
      : NULL;
    TemplateInfo *instTInfo = new TemplateInfo(name, loc);
    instTInfo->instantiatedFrom = baseV;
    SFOREACH_OBJLIST(STemplateArgument, sargs, iter) {
      instTInfo->arguments.append(new STemplateArgument(*iter.data()));
    }

    // record the location which provoked this instantiation; this
    // information will be useful if it turns out we can only to a
    // forward-declaration here, since later when we do the delayed
    // instantiation, 'loc' will be only be available because we
    // stashed it here
    //instTInfo->instLoc = loc;
    // actually, this is now part of the constructor argument above, 
    // but I'll leave the comment

    // FIX: Scott, its almost as if you just want to clone the type
    // here.
    //
    // sm: No, I want to type-check the instantiated cloned AST.  The
    // resulting type will be quite different, since it will refer to
    // concrete types instead of TypeVariables.
    if (baseV->type->isCompoundType()) {
      // 1/21/03: I had been using 'instName' as the class name, but
      // that causes problems when trying to recognize constructors
      CompoundType *instVCpdType = tfac.makeCompoundType(baseV->type->asCompoundType()->keyword,
                                                         baseV->type->asCompoundType()->name);
      instVCpdType->instName = instName; // stash it here instead
      instVCpdType->forward = baseForward;

      // wrap the compound in a regular type
      SourceLoc copyLoc = copyCpd ? copyCpd->loc : SL_UNKNOWN;
      Type *type = makeType(copyLoc, instVCpdType);

      // make a fake implicit typedef; this class and its typedef
      // won't actually appear in the environment directly, but making
      // the implicit typedef helps ensure uniformity elsewhere; also
      // must be done before type checking since it's the typedefVar
      // field which is returned once the instantiation is found via
      // 'instantiations'
      instV = makeVariable(copyLoc, instName, type,
                           DF_TYPEDEF | DF_IMPLICIT);
      instV->type->asCompoundType()->typedefVar = instV;

      if (lang.compoundSelfName) {
        // also make the self-name, which *does* go into the scope
        // (testcase: t0167.cc)
        Variable *var2 = makeVariable(copyLoc, instName, type,
                                      DF_TYPEDEF | DF_SELFNAME);
        instV->type->asCompoundType()->addUniqueVariable(var2);
        addedNewVariable(instV->type->asCompoundType(), var2);
      }
    } else {
      xassert(baseV->type->isFunctionType());
      // sm: It seems to me the sequence should be something like this:
      //   1. bind template parameters to concrete types
      //        dsw: this has been done already above
      //   2. tcheck the declarator portion, thus yielding a FunctionType
      //      that refers to concrete types (not TypeVariables), and
      //      also yielding a Variable that can be used to name it
      //        dsw: this is done here
      if (copyFun) {
        xassert(!baseForward);
        // NOTE: 1) the whole point is that we don't check the body,
        // and 2) it is very important that we do not add the variable
        // to the namespace, otherwise the primary is masked if the
        // template body refers to itself
        copyFun->tcheck(*this, false /*checkBody*/,
                        false /*reallyAddVariable*/, funcFwdInstV /*prior*/);
        instV = copyFun->nameAndParams->var;
        //   3. add said Variable to the list of instantiations, so if the
        //      function recursively calls itself we'll be ready
        //        dsw: this is done below
        //   4. tcheck the function body
        //        dsw: this is done further below.
      } else {
        xassert(baseForward);
        // We do have to clone the forward declaration before
        // typechecking.
        Declaration *fwdDecl = baseV->templateInfo()->declSyntax;
        xassert(fwdDecl);
        // use the same context again but make sure it is well defined
        xassert(fwdDecl->decllist->count() == 1);
        DeclaratorContext ctxt = fwdDecl->decllist->first()->context;
        xassert(ctxt != DC_UNKNOWN);
        Declaration *copyDecl = fwdDecl->clone();
        xassert(argScope);
        xassert(!funcFwdInstV);
        copyDecl->tcheck(*this, ctxt,
                         false /*reallyAddVariable*/, NULL /*prior*/);
        xassert(copyDecl->decllist->count() == 1);
        Declarator *copyDecltor = copyDecl->decllist->first();
        instV = copyDecltor->var;
      }
    }

    xassert(instV);
    xassert(foundScope);
    foundScope->registerVariable(instV);

    // tell the base template about this instantiation; this has to be
    // done before invoking the type checker, to handle cases where the
    // template refers to itself recursively (which is very common)
    //
    // dsw: this is the one place where I use oldBase instead of base;
    // looking at the other code above, I think it is the only one
    // where I should, but I'm not sure.
    xassert(oldBaseV->templateInfo() && oldBaseV->templateInfo()->isPrimary());
    // dsw: this had to be moved down here as you can't get the
    // typedefVar until it exists
    instV->setTemplateInfo(instTInfo);
    if (funcFwdInstV) {
      // addInstantiation would have done this
      instV->templateInfo()->setMyPrimary(oldBaseV->templateInfo());
    } else {
      // NOTE: this can mutate instV if instV is a duplicated mutant
      // in the instantiation list; FIX: perhaps find another way to
      // prevent creating duplicate mutants in the first place; this
      // is quite wasteful if we have cloned an entire class of AST
      // only to throw it away again
      Variable *newInstV = oldBaseV->templateInfo()->
        addInstantiation(instV, true /*suppressDup*/);
      if (newInstV != instV) {
        // don't do stage 5 below; just use the newInstV and be done
        // with it
        copyCpd = NULL;
        copyFun = NULL;
        instV = newInstV;
      }
      xassert(instV->templateInfo()->getMyPrimaryIdem() == oldBaseV->templateInfo());
    }
    xassert(instTInfo->isNotPrimary());
    xassert(instTInfo->getMyPrimaryIdem() == oldBaseV->templateInfo());
  }

  // 5 **** typecheck the cloned AST

  if (copyCpd || copyFun) {
    xassert(!baseForward);
    xassert(argScope);

    // we are about to tcheck the cloned AST.. it might be that we
    // were in the context of an ambiguous expression when the need
    // for instantiated arose, but we want the tchecking below to
    // proceed as if it were not ambiguous (it isn't ambiguous, it
    // just might be referenced from a context that is)
    //
    // so, the following object will temporarily set the level to 0,
    // and restore its original value when the block ends
    {
      DisambiguateNestingTemp nestTemp(*this, 0);

      if (instV->type->isCompoundType()) {
        // FIX: unlike with function templates, we can't avoid
        // typechecking the compound type clone when not in 'normal'
        // mode because otherwise implicit members such as ctors don't
        // get elaborated into existence
//          if (getTemplTcheckMode() == TTM_1NORMAL) { . . .
        xassert(copyCpd);
        // invoke the TS_classSpec typechecker, giving to it the
        // CompoundType we've built to contain its declarations; for
        // now I don't support nested template instantiation
        copyCpd->ctype = instV->type->asCompoundType();
        // preserve the baseV and sargs so that when the member
        // function bodies are typechecked later we have them
        InstContext *instCtxt = new InstContext(baseV, instV, foundScope, sargs);
        copyCpd->tcheckIntoCompound
          (*this,
           DF_NONE,
           copyCpd->ctype,
           false /*inTemplate*/,
           // that's right, really check the member function bodies
           // exactly when we are instantiating something that is not
           // a complete specialization; if it *is* a complete
           // specialization, then the tcheck will be done later, say,
           // when the function is used
           sTemplArgsContainVars /*reallyTcheckFunctionBodies*/,
           instCtxt,
           foundScope,
           NULL /*containingClass*/);
        // this is turned off because it doesn't work: somewhere the
        // mutants are actually needed; Instead we just avoid them
        // above.
        //      deMutantify(baseV);

        // find the funcDefn's for all of the function declarations
        // and then instantiate them; FIX: do the superclass members
        // also?
        FOREACH_ASTLIST_NC(Member, cpdBaseSyntax->members->list, memIter) {
          Member *mem = memIter.data();
          if (!mem->isMR_decl()) continue;
          Declaration *decltn = mem->asMR_decl()->d;
          Declarator *decltor = decltn->decllist->first();
          // this seems to happen with anonymous enums
          if (!decltor) continue;
          if (!decltor->var->type->isFunctionType()) continue;
          xassert(decltn->decllist->count() == 1);
          Function *funcDefn = decltor->var->funcDefn;
          // skip functions that haven't been defined yet; hopefully
          // they'll be defined later and if they are, their
          // definitions will be patched in then
          if (!funcDefn) continue;
          // I need to instantiate this funcDefn.  This means: 1)
          // clone it; [NOTE: this is not being done: 2) all the
          // arguments are already in the scope, but if the definition
          // named them differently, it won't work, so throw out the
          // current template scope and make another]; then 3) just
          // typecheck it.
          Function *copyFuncDefn = funcDefn->clone();
          // find the instantiation of the cloned declaration that
          // goes with it; FIX: this seems brittle to me; I'd rather
          // have something like a pointer from the cloned declarator
          // to the one it was cloned from.
          Variable *funcDefnInstV = instV->type->asCompoundType()->lookupVariable
            (decltor->var->name, *this,
             LF_INNER_ONLY |
             // this shouldn't be necessary, but if it turns out to be
             // a function template, we don't want it instantiated
             LF_TEMPL_PRIMARY);
          xassert(funcDefnInstV);
          xassert(strcmp(funcDefnInstV->name, decltor->var->name)==0);
          if (funcDefnInstV->templateInfo()) {
            // FIX: I don't know what to do with function template
            // members of class templates just now, but what we do
            // below is probably wrong, so skip them
            continue;
          }
          xassert(!funcDefnInstV->funcDefn);

          // FIX: I wonder very much if the right thing is happening
          // to the scopes at this point.  I think all the current
          // scopes up to the global scope need to be removed, and
          // then the template scope re-created, and then typecheck
          // this.  The extra scopes are probably harmless, but
          // shouldn't be there.

          // save all the scopes from the foundScope down to the
          // current scope; Scott: I think you are right that I didn't
          // need to save them all, but I can't see how to avoid
          // saving these.  How else would I re-create them?  Note
          // that this is idependent of the note above about fixing
          // the scope stack: we save whatever is there before
          // entering Function::tcheck()
          xassert(instCtxt);
          xassert(foundScope);
          FuncTCheckContext *tcheckCtxt = new FuncTCheckContext
            (copyFuncDefn, foundScope, shallowClonePartialScopeStack(foundScope));

          // urk, there's nothing to do here.  The declaration has
          // already been tchecked, and we don't want to tcheck the
          // definition yet, so we can just point the var at the
          // cloned definition; I'll run the tcheck with
          // checkBody=false anyway just for uniformity.
          copyFuncDefn->tcheck(*this,
                               false /*checkBody*/,
                               false /*reallyAddVariable*/,
                               funcDefnInstV /*prior*/);
          // pase in the definition for later use
          xassert(!funcDefnInstV->funcDefn);
          funcDefnInstV->funcDefn = copyFuncDefn;

          // preserve the instantiation context
          xassert(funcDefnInstV = copyFuncDefn->nameAndParams->var);
          copyFuncDefn->nameAndParams->var->setInstCtxts(instCtxt, tcheckCtxt);
        }
      } else {
        xassert(instV->type->isFunctionType());
        // if we are in a template definition, don't typecheck the
        // cloned function body
        if (tcheckMode == TTM_1NORMAL) {
          xassert(copyFun);
          copyFun->funcType = instV->type->asFunctionType();
          xassert(scope()->isGlobalTemplateScope());
          // NOTE: again we do not add the variable to the namespace
          copyFun->tcheck(*this, true /*checkBody*/,
                          false /*reallyAddVariable*/, instV /*prior*/);
          xassert(instV->funcDefn == copyFun);
        }
      }
    }

    if (tracingSys("cloneTypedAST")) {
      cout << "--------- typed clone of " 
           << instName << sargsToString(sargs) << " ------------\n";
      if (copyCpd) copyCpd->debugPrint(cout, 0);
      if (copyFun) copyFun->debugPrint(cout, 0);
    }
  }
  // this else case can now happen if we find that there was a
  // duplicated instV and therefore jump out during stage 4
//    } else {
//      xassert(baseForward);
//    }

  // 6 **** Undo the scope, reversing step 2
  if (argScope) {
    unPrepArgScopeForTemlCloneTcheck(argScope, poppedScopes, pushedScopes);
    argScope = NULL;
  }
  // make sure we haven't forgotten these
  xassert(poppedScopes.isEmpty() && pushedScopes.isEmpty());

  return instV;
}


// given a name that was found without qualifiers or template arguments,
// see if we're currently inside the scope of a template definition
// with that name
CompoundType *Env::findEnclosingTemplateCalled(StringRef name)
{
  FOREACH_OBJLIST(Scope, scopes, iter) {
    Scope const *s = iter.data();

    if (s->curCompound &&
        s->curCompound->templateInfo() &&
        s->curCompound->templateInfo()->baseName == name) {
      return s->curCompound;
    }
  }
  return NULL;     // not found
}


void Env::provideDefForFuncTemplDecl
  (Variable *forward, TemplateInfo *primaryTI, Function *f)
{
  xassert(forward);
  xassert(primaryTI);
  xassert(forward->templateInfo()->getMyPrimaryIdem() == primaryTI);
  xassert(primaryTI->isPrimary());
  Variable *fVar = f->nameAndParams->var;
  // update things in the declaration; I copied this from
  // Env::createDeclaration()
  TRACE("odr", "def'n of " << forward->name
        << " at " << toString(f->getLoc())
        << " overrides decl at " << toString(forward->loc));
  forward->loc = f->getLoc();
  forward->setFlag(DF_DEFINITION);
  forward->clearFlag(DF_EXTERN);
  forward->clearFlag(DF_FORWARD); // dsw: I added this
  // make this idempotent
  if (forward->funcDefn) {
    xassert(forward->funcDefn == f);
  } else {
    forward->funcDefn = f;
    if (tracingSys("template")) {
      cout << "definition of function template " << fVar->toString()
           << " attached to previous forwarded declaration" << endl;
      primaryTI->debugPrint();
    }
  }
  // make this idempotent
  if (fVar->templateInfo()->getMyPrimaryIdem()) {
    xassert(fVar->templateInfo()->getMyPrimaryIdem() == primaryTI);
  } else {
    fVar->templateInfo()->setMyPrimary(primaryTI);
  }
}


void Env::ensureFuncMemBodyTChecked(Variable *v)
{
  if (getTemplTcheckMode() != TTM_1NORMAL) return;
  xassert(v);
  xassert(v->getType()->isFunctionType());

  // FIX: perhaps these should be assertion failures instead
  TemplateInfo *vTI = v->templateInfo();
  if (vTI) {
    // NOTE: this is pretty weak, because it only applies to function
    // templates not to function members of class templates, and the
    // instantiation of function templates is never delayed.
    // Therefore, we might as well say "return" here.
    if (vTI->isMutant()) return;
    if (!vTI->isCompleteSpecOrInstantiation()) return;
  }

  Function *funcDefn0 = v->funcDefn;

  InstContext *instCtxt = v->instCtxt;
  // anything that wasn't part of a template instantiation
  if (!instCtxt) {
    // if this function is defined, it should have been typechecked by now;
    // UPDATE: unless it is defined later in the same class
//      if (funcDefn0) {
//        xassert(funcDefn0->hasBodyBeenTChecked);
//      }
    return;
  }

  FuncTCheckContext *tcheckCtxt = v->tcheckCtxt;
  xassert(tcheckCtxt);

  // if we are a member of a templatized class and we have not seen
  // the funcDefn, then it must come later, but this is not
  // implemented
  if (!funcDefn0) {
    unimp("class template function memeber invocation "
          "before function definition is unimplemented");
    return;
  }

  // if has been tchecked, we are done
  if (funcDefn0->hasBodyBeenTChecked) {
    return;
  }

  // OK, seems it can be a partial specialization as well
//    xassert(instCtxt->baseV->templateInfo()->isPrimary());
  xassert(instCtxt->baseV->templateInfo()->isPrimary() ||
          instCtxt->baseV->templateInfo()->isPartialSpec());
  // this should only happen for complete specializations
  xassert(instCtxt->instV->templateInfo()->isCompleteSpecOrInstantiation());

  // set up the scopes the way instantiateTemplate() would
  ObjList<Scope> poppedScopes;
  SObjList<Scope> pushedScopes;
  Scope *argScope = prepArgScopeForTemlCloneTcheck
    (poppedScopes, pushedScopes, instCtxt->foundScope);
  insertBindings(instCtxt->baseV, *(instCtxt->sargs));

  // mess with the scopes the way typechecking would between when it
  // leaves instantiateTemplate() and when it arrives at the function
  // tcheck
  xassert(scopes.count() >= 2);
  xassert(scopes.nth(1) == tcheckCtxt->foundScope);
  xassert(scopes.nth(0)->scopeKind == SK_TEMPLATE);
  tcheckCtxt->pss->stackIntoEnv(*this);

  xassert(funcDefn0 == tcheckCtxt->func);
  funcDefn0->tcheck(*this,
                    true /*checkBody*/,
                    false /*reallyAddVariable*/,
                    v /*prior*/);
  // should still be true
  xassert(v->funcDefn == funcDefn0);

  tcheckCtxt->pss->unStackOutOfEnv(*this);

  if (argScope) {
    unPrepArgScopeForTemlCloneTcheck(argScope, poppedScopes, pushedScopes);
    argScope = NULL;
  }
  // make sure we haven't forgotten these
  xassert(poppedScopes.isEmpty() && pushedScopes.isEmpty());
}


void Env::instantiateForwardFunctions(Variable *forward, Variable *primary)
{
  TemplateInfo *primaryTI = primary->templateInfo();
  xassert(primaryTI);
  xassert(primaryTI->isPrimary());

  // temporarily supress TTM_3TEMPL_DEF and return to TTM_1NORMAL for
  // purposes of instantiating the forward function templates
  StackMaintainer<TemplateDeclaration> sm1(templateDeclarationStack, &mode1Dummy);

  // Find all the places where this declaration was instantiated,
  // where this function template specialization was
  // called/instantiated after it was declared but before it was
  // defined.  In each, now instantiate with the same template
  // arguments and fill in the funcDefn; UPDATE: see below, but now we
  // check that the definition was already provided rather than
  // providing one.
  SFOREACH_OBJLIST_NC(Variable, primaryTI->getInstantiations(), iter) {
    Variable *instV = iter.data();
    TemplateInfo *instTIinfo = instV->templateInfo();
    xassert(instTIinfo);
    xassert(instTIinfo->isNotPrimary());
    if (instTIinfo->instantiatedFrom != forward) continue;
    // should not have a funcDefn as it is instantiated from a forward
    // declaration that does not yet have a definition and we checked
    // that above already
    xassert(!instV->funcDefn);

    // instantiate this definition
    SourceLoc instLoc = instTIinfo->instLoc;

    Variable *instWithDefn = instantiateTemplate
      (instLoc,
       // FIX: I don't know why it is possible for the primary here to
       // not have a scope, but it is.  Since 1) we are in the scope
       // of the definition that we want to instantiate, and 2) from
       // experiments with gdb, I have to put the definition of the
       // template in the same scope as the declaration, I conclude
       // that I can use the same scope as we are in now
//         primary->scope,    // FIX: ??
       // UPDATE: if the primary is in the global scope, then I
       // suppose its scope might be NULL; I don't remember the rule
       // for that.  So, maybe it's this:
//         primary->scope ? primary->scope : env.globalScope()
       scope(),
       primary,
       NULL /*instV; only used by instantiateForwardClasses; this
              seems to be a fundamental difference*/,
       forward /*bestV*/,
       reinterpret_cast< SObjList<STemplateArgument>& >  // hack..
         (instV->templateInfo()->arguments),
       // don't actually add the instantiation to the primary's
       // instantiation list; we will do that below
       instV
       );
    // previously the instantiation of the forward declaration
    // 'forward' produced an instantiated declaration; we now provide
    // a definition for it; UPDATE: we now check that when the
    // definition of the function was typechecked that a definition
    // was provided for it, since we now re-use the declaration's var
    // in the defintion declartion (the first pass when
    // checkBody=false).
    //
    // FIX: I think this works for functions the definition of which
    // is going to come after the instantiation request of the
    // containing class template since I think both of these will be
    // NULL; we will find out
    xassert(instV->funcDefn == instWithDefn->funcDefn);
  }
}


void Env::instantiateForwardClasses(Scope *scope, Variable *baseV)
{
  // temporarily supress TTM_3TEMPL_DEF and return to TTM_1NORMAL for
  // purposes of instantiating the forward classes
  StackMaintainer<TemplateDeclaration> sm(templateDeclarationStack, &mode1Dummy);

  SFOREACH_OBJLIST_NC(Variable, baseV->templateInfo()->getInstantiations(), iter) {
    Variable *instV = iter.data();
    xassert(instV->templateInfo());
    CompoundType *inst = instV->type->asCompoundType();
    // this assumption is made below
    xassert(inst->templateInfo() == instV->templateInfo());

    if (inst->forward) {
      TRACE("template", "instantiating previously forward " << inst->name);
      inst->forward = false;
      
      // this location was stored back when the template was
      // forward-instantiated
      SourceLoc instLoc = inst->templateInfo()->instLoc;

      instantiateTemplate(instLoc,
                          scope,
                          baseV,
                          instV /*use this one*/,
                          NULL /*bestV*/,
                          reinterpret_cast< SObjList<STemplateArgument>& >  // hack..
                            (inst->templateInfo()->arguments)
                          );
    }
    else {
      // this happens in e.g. t0079.cc, when the template becomes
      // an instantiation of itself because the template body
      // refers to itself with template arguments supplied
      
      // update: maybe not true anymore?
    }
  }
}


// --------------------- from cc_tcheck.cc ----------------------
// go over all of the function bodies and make sure they have been
// typechecked
class EnsureFuncBodiesTcheckedVisitor : public ASTTemplVisitor {
  Env &env;
public:
  EnsureFuncBodiesTcheckedVisitor(Env &env0) : env(env0) {}
  bool visitFunction(Function *f);
  bool visitDeclarator(Declarator *d);
};

bool EnsureFuncBodiesTcheckedVisitor::visitFunction(Function *f) {
  xassert(f->nameAndParams->var);
  env.ensureFuncMemBodyTChecked(f->nameAndParams->var);
  return true;
}

bool EnsureFuncBodiesTcheckedVisitor::visitDeclarator(Declarator *d) {
//    printf("EnsureFuncBodiesTcheckedVisitor::visitDeclarator d:%p\n", d);
  // a declarator can lack a var if it is just the declarator for the
  // ASTTypeId of a type, such as in "operator int()".
  if (!d->var) {
    // I just want to check something that will make sure that this
    // didn't happen because the enclosing function body never got
    // typechecked
    xassert(d->context == DC_UNKNOWN);
//      xassert(d->decl);
//      xassert(d->decl->isD_name());
//      xassert(!d->decl->asD_name()->name);
  } else {
    if (d->var->type->isFunctionType()) {
      env.ensureFuncMemBodyTChecked(d->var);
    }
  }
  return true;
}


void instantiateRemainingMethods(Env &env, TranslationUnit *tunit)
{
  // Given the current architecture, it is impossible to ensure that
  // all called functions have had their body tchecked.  This is
  // because if implicit calls to existing functions.  That is, one
  // user-written (not implicitly defined) function f() that is
  // tchecked implicitly calls another function g() that is
  // user-written, but the call is elaborated into existence.  This
  // means that we don't see the call to g() until the elaboration
  // stage, but by then typechecking is over.  However, since the
  // definition of g() is user-written, not implicitly defined, it
  // does indeed need to be tchecked.  Therefore, at the end of a
  // translation unit, I simply typecheck all of the function bodies
  // that have not yet been typechecked.  If this doesn't work then we
  // really need to change something non-trivial.
  //
  // It seems that we are ok for now because it is only function
  // members of templatized classes that we are delaying the
  // typechecking of.  Also, I expect that the definitions will all
  // have been seen.  Therefore, I can just typecheck all of the
  // function bodies at the end of typechecking the translation unit.
  EnsureFuncBodiesTcheckedVisitor visitor(env);
  tunit->traverse(visitor);
}


// this is defined just below, but I want it there (not here)
bool mergeParameterLists(Env &env, CompoundType *prior,
                         TemplateParams *dest, TemplateParams const *src);

// we (may) have just encountered some syntax which declares
// some template parameters, but found that the declaration
// matches a prior declaration with (possibly) some other template
// parameters; verify that they match (or complain), and then
// discard the ones stored in the environment (if any)
//
// return false if there is some problem, true if it's all ok
// (however, this value is ignored at the moment)
bool verifyCompatibleTemplates(Env &env, CompoundType *prior)
{
  Scope *scope = env.scope();
  if (!scope->curTemplateParams && !prior->isTemplate()) {
    // neither talks about templates, forget the whole thing
    return true;
  }

  if (!scope->curTemplateParams && prior->isTemplate()) {
    env.error(stringc
      << "prior declaration of " << prior->keywordAndName()
      << " at " << prior->typedefVar->loc
      << " was templatized with parameters "
      << prior->templateInfo()->paramsToCString()
      << " but the this one is not templatized",
      EF_DISAMBIGUATES);
    return false;
  }

  if (scope->curTemplateParams && !prior->isTemplate()) {
    env.error(stringc
      << "prior declaration of " << prior->keywordAndName()
      << " at " << prior->typedefVar->loc
      << " was not templatized, but this one is, with parameters "
      << scope->curTemplateParams->paramsToCString(),
      EF_DISAMBIGUATES);
    delete scope->curTemplateParams;
    scope->curTemplateParams = NULL;
    return false;
  }

  // now we know both declarations have template parameters;
  // check them for naming equivalent types
  //
  // furthermore, fix the names in 'prior' in case they differ
  // with those of 'scope->curTemplateParams'
  //
  // even more, merge their default arguments
  bool ret = mergeParameterLists(
    env, prior,
    prior->templateInfo(),       // dest
    scope->curTemplateParams);   // src

  // clean up 'curTemplateParams' regardless
  delete scope->curTemplateParams;
  scope->curTemplateParams = NULL;

  return ret;
}


// context: I have previously seen a (forward) template
// declaration, such as
//   template <class S> class C;             // dest
//                   ^
// and I want to modify it to use the same names as another
// declaration later on, e.g.
//   template <class T> class C { ... };     // src
//                   ^
// since in fact I am about to discard the parameters that
// come from 'src' and simply keep using the ones from
// 'dest' for future operations, including processing the
// template definition associated with 'src'
bool mergeParameterLists(Env &env, CompoundType *prior,
                         TemplateParams *dest, TemplateParams const *src)
{
  TRACE("template", "mergeParameterLists: prior=" << prior->name
    << ", dest=" << dest->paramsToCString()
    << ", src=" << src->paramsToCString());

  // keep track of whether I've made any naming changes
  // (alpha conversions)
  bool anyNameChanges = false;

  SObjListIterNC<Variable> destIter(dest->params);
  SObjListIter<Variable> srcIter(src->params);
  for (; !destIter.isDone() && !srcIter.isDone();
       destIter.adv(), srcIter.adv()) {
    Variable *dest = destIter.data();
    Variable const *src = srcIter.data();

    // are the types equivalent?
    if (!dest->type->equals(src->type)) {
      env.error(stringc
        << "prior declaration of " << prior->keywordAndName()
        << " at " << prior->typedefVar->loc
        << " was templatized with parameter `"
        << dest->name << "' of type `" << dest->type->toString()
        << "' but this one has parameter `"
        << src->name << "' of type `" << src->type->toString()
        << "', and these are not equivalent",
        EF_DISAMBIGUATES);
      return false;
    }

    // what's up with their default arguments?
    if (dest->value && src->value) {
      // this message could be expanded...
      env.error("cannot specify default value of template parameter more than once");
      return false;
    }

    // there is a subtle problem if the prior declaration has a
    // default value which refers to an earlier template parameter,
    // but the name of that parameter has been changed
    if (anyNameChanges &&              // earlier param changed
        dest->value) {                 // prior has a value
      // leave it as a to-do for now; a proper implementation should
      // remember the name substitutions done so far, and apply them
      // inside the expression for 'dest->value'
      xfailure("unimplemented: alpha conversion inside default values"
               " (workaround: use consistent names in template parameter lists)");
    }

    // merge their default values
    if (src->value && !dest->value) {
      dest->value = src->value;
    }

    // do they use the same name?
    if (dest->name != src->name) {
      // make the names the same
      TRACE("template", "changing parameter " << dest->name
        << " to " << src->name);
      anyNameChanges = true;
      dest->name = src->name;
    }
  }

  if (srcIter.isDone() && destIter.isDone()) {
    return true;   // ok
  }
  else {
    env.error(stringc
      << "prior declaration of " << prior->keywordAndName()
      << " at " << prior->typedefVar->loc
      << " was templatized with " 
      << pluraln(dest->params.count(), "parameter")
      << ", but this one has "
      << pluraln(src->params.count(), "parameter"),
      EF_DISAMBIGUATES);
    return false;
  }
}


// EOF
