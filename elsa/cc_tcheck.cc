// cc_tcheck.cc            see license.txt for copyright and terms of use
// C++ typechecker, implemented as methods declared in cc_tcheck.ast

// Throughout, references are made to the ISO C++ Standard:
//
// International Organization for Standardization.
// ISO/IEC 14882:1998: Programming languages -- C++.
// International Organization for Standardization, Geneva,
// Switzerland, September 1998.
//
// These references are all marked with the string "cppstd".

#include "cc_ast.h"         // C++ AST
#include "cc_ast_aux.h"     // class ASTTemplVisitor
#include "cc_env.h"         // Env
#include "trace.h"          // trace
#include "cc_print.h"       // PrintEnv
#include "strutil.h"        // decodeEscapes
#include "cc_lang.h"        // CCLang
#include "stdconv.h"        // test_getStandardConversion
#include "implconv.h"       // test_getImplicitConversion
#include "overload.h"       // resolveOverload
#include "generic_amb.h"    // resolveAmbiguity, etc.
#include "implint.h"        // resolveImplIntAmbig
#include "ast_build.h"      // makeExprList1, etc.
#include "strutil.h"        // prefixEquals, pluraln
#include "macros.h"         // Restorer
#include "typelistiter.h"   // TypeListIter_FakeList

#include <stdlib.h>         // strtoul, strtod
#include <ctype.h>          // isdigit
#include <limits.h>         // INT_MAX, UINT_MAX, LONG_MAX

// D(): debug code
#ifdef NDEBUG
  #define D(stuff)
#else
  #define D(stuff) stuff
#endif


// forwards in this file
static Variable *outerResolveOverload_ctor
  (Env &env, SourceLoc loc, Type *type, FakeList<ArgExpression> *args, bool really);

static Variable *outerResolveOverload_explicitSet(
  Env &env,
  PQName * /*nullable*/ finalName,
  SourceLoc loc,
  StringRef varName,
  Type *receiverType,
  FakeList<ArgExpression> *args,
  SObjList<Variable> &candidates);

FakeList<ArgExpression> *tcheckArgExprList(FakeList<ArgExpression> *list, Env &env);

Type *resolveOverloadedUnaryOperator(
  Env &env,
  Expression *&replacement,
  //Expression *ths,
  Expression *expr,
  OverloadableOp op);



// return true if the list contains no disambiguating errors
bool noDisambErrors(ErrorList const &list)
{
  return !list.hasDisambErrors();
}


// 'ambiguousNodeName' is a template function in generic_amb.h, but
// declarators are special, since there's only one node type; the
// difference lies in the values of the fields (ah, the beauty of C++
// template specialization..)
string ambiguousNodeName(Declarator const *n)
{
  if (n->init) {
    return string("Declarator with initializer");
  }
  else {
    return string("Declarator without initializer");
  }
}

// and ASTTypeIds are also special
int countD_funcs(ASTTypeId const *n);
string ambiguousNodeName(ASTTypeId const *n)
{
  return stringc << "ASTTypeId with D_func depth " << countD_funcs(n);
}


// ------------------- TranslationUnit --------------------
void TranslationUnit::tcheck(Env &env)
{
  static int topForm = 0;
  FOREACH_ASTLIST_NC(TopForm, topForms, iter) {
    ++topForm;
    TRACE("topform", "--------- topform " << topForm <<
                     ", at " << toString(iter.data()->loc) <<
                     " --------");
    iter.setDataLink( iter.data()->tcheck(env) );
  }

  instantiateRemainingMethods(env, this);
}


// --------------------- TopForm ---------------------
TopForm *TopForm::tcheck(Env &env)
{
  if (!ambiguity) {
    itcheck(env);
    return this;
  }

  TopForm *ret = resolveImplIntAmbig(env, this);
  xassert(ret);
  return ret->tcheck(env);
}

void TF_decl::itcheck(Env &env)
{
  env.setLoc(loc);
  decl->tcheck(env, DC_TF_DECL);
}

void TF_func::itcheck(Env &env)
{
  env.setLoc(loc);
  f->tcheck(env);
}

void TF_template::itcheck(Env &env)
{
  env.setLoc(loc);
  td->tcheck(env);
}

void TF_explicitInst::itcheck(Env &env)
{
  env.setLoc(loc);
  
  d->tcheck(env, DC_TF_EXPLICITINST);
  
  // class instantiation?
  if (d->decllist->isEmpty()) {
    if (d->spec->isTS_elaborated()) {
      NamedAtomicType *nat = d->spec->asTS_elaborated()->atype;
      if (!nat) return;    // error recovery

      if (nat->isCompoundType() &&
          nat->asCompoundType()->isInstantiation()) {
        env.explicitlyInstantiate(nat->asCompoundType()->typedefVar);
      }
      else {
        // catch "template class C;"
        env.error("explicit instantiation (without declarator) is only for class instantiations");
      }
    }
    else {
      // catch "template C<int>;"
      env.error("explicit instantiation (without declarator) requires \"class ...\"");
    }
  }

  // function instantiation?
  else if (d->decllist->count() == 1) {
    // instantiation is handled by declarator::mid_tcheck
  }
  
  else {
    // other template declarations are limited to one declarator, so I
    // am simply assuming the same is true of explicit instantiations,
    // even though 14.7.2 doesn't say so explicitly...
    env.error("too many declarators in explicit instantiation");
  }
}

void TF_linkage::itcheck(Env &env)
{  
  env.setLoc(loc);

  // set the linkage type for the duration of this form
  Restorer<StringRef> restorer(env.currentLinkage, linkageType);

  forms->tcheck(env);
}

void TF_one_linkage::itcheck(Env &env)
{
  env.setLoc(loc);

  // set the linkage type for the duration of this form
  Restorer<StringRef> restorer(env.currentLinkage, linkageType);

  // we need to dig down into the form to apply 'extern'
  // [cppstd 7.5 para 7]
  ASTSWITCH(TopForm, form) {
    ASTCASE(TF_decl, d)   d->decl->dflags |= DF_EXTERN;
    ASTNEXT(TF_func, f)   f->f->dflags |= DF_EXTERN;   // legal?  let Function catch it if not
    ASTDEFAULT
      // template, or another 'extern "C"'!
      env.unimp(stringc
        << "weird use of 'extern \"" << linkageType << "\"'");
    ASTENDCASE
  }
  
  // typecheck the underlying form
  form->tcheck(env);
}

void TF_asm::itcheck(Env &env)
{
  env.setLoc(loc);

  StringRef t = text->text;
  if (prefixEquals(t, "\"collectLookupResults")) {
    // this activates an internal diagnostic that will collect
    // the E_variable lookup results as warnings, then at the
    // end of the program, compare them to this string
    env.collectLookupResults = t;
  }
}


void TF_namespaceDefn::itcheck(Env &env)
{
  env.setLoc(loc);

  // currently, what does the name refer to in this scope?
  Variable *existing = NULL;
  if (name) {
    existing = env.lookupVariable(name, LF_INNER_ONLY);
  }
  
  // violation of 7.3.1 para 2?
  if (existing && !existing->hasFlag(DF_NAMESPACE)) {
    env.error(loc, stringc
      << "attempt to redefine `" << name << "' as a namespace");

    // recovery: pretend it didn't have a name
    existing = NULL;
    name = NULL;                // dsw: this causes problems when you add it to the scope
  }
    
  Scope *s;
  if (existing) {
    // extend existing scope
    s = existing->scope;
  }
  else {
    // make an entry in the surrounding scope to refer to the new namespace
    Variable *v = env.makeVariable(loc, name, NULL /*type*/, DF_NAMESPACE);
    if (name) {
      env.addVariable(v);
    }

    // make new scope
    s = new Scope(SK_NAMESPACE, 0 /*changeCount; irrelevant*/, loc);
    s->namespaceVar = v;

    // point the variable at it so we can find it later
    v->scope = s;

    // hook it into the scope tree; this must be done before the
    // using edge is added, for anonymous scopes
    env.setParentScope(s);

    // if name is NULL, we need to add an "active using" edge from the
    // surrounding scope to 's'
    if (!name) {
      Scope *parent = env.scope();
      parent->addUsingEdge(s);
      parent->addUsingEdgeTransitively(env, s);
    }
  }

  // check the namespace body in its scope
  env.extendScope(s);
  FOREACH_ASTLIST_NC(TopForm, forms, iter) {
    iter.setDataLink( iter.data()->tcheck(env) );
  }
  env.retractScope(s);
}


void TF_namespaceDecl::itcheck(Env &env)
{
  env.setLoc(loc);
  decl->tcheck(env);
}


// --------------------- Function -----------------
void Function::tcheck(Env &env, Variable *instV)
{                               
  bool checkBody = env.checkFunctionBodies;

  if (env.secondPassTcheck) {
    // for the second pass, just force the use of the
    // variable computed in the first pass
    xassert(!instV);
    xassert(nameAndParams->var);
    instV = nameAndParams->var;

    if (checkBody) {
      instV->setFlag(DF_DEFINITION);
    }
  }

  // are we in a template function?
  bool inTemplate = env.scope()->hasTemplateParams();

  // only disambiguate, if template
  DisambiguateOnlyTemp disOnly(env, inTemplate /*disOnly*/);

  // get return type
  Type *retTypeSpec = retspec->tcheck(env, dflags);

  // supply DF_DEFINITION?
  DeclFlags dfDefn = (checkBody? DF_DEFINITION : DF_NONE);
  if (env.lang.treatExternInlineAsPrototype &&
      dflags >= (DF_EXTERN | DF_INLINE)) {
    // gcc treats extern-inline function definitions specially:
    //
    //   http://gcc.gnu.org/onlinedocs/gcc-3.4.1/gcc/Inline.html
    //
    // I will essentially ignore them (just treat them like a
    // prototype), thus modeling the dynamic semantics of gcc when
    // optimization is turned off.  My nominal stance is that any
    // program that has an extern-inline definition that is different
    // from the ordinary (external) definition has undefined behavior.
    // A possible future extension is to check that such definitions
    // agree.
    //
    // Ah, but I can't just say 'checkBody = false' here because if
    // there are ambiguities in the body then lots of things don't
    // like it.  And anyway, tchecking the body is a good idea.  So I
    // do something a little more subtle, I claim this isn't a
    // "definition".
    dfDefn = DF_NONE;
  }

  // construct the full type of the function; this will set
  // nameAndParams->var, which includes a type, but that type might
  // use different parameter names if there is already a prototype;
  // dt.type will come back with a function type which always has the
  // parameter names for this definition
  Declarator::Tcheck dt(retTypeSpec,
                        dflags | dfDefn,
                        DC_FUNCTION);
  dt.existingVar = instV;
  nameAndParams = nameAndParams->tcheck(env, dt);
  if (!nameAndParams->var) {
    return;                     // error should have already been reported
  }

  // this seems like it should be being done, but if I want this, I
  // should first write a test that fails if I don't do it
//    if (!nameAndParams->var->funcDefn) {
//      nameAndParams->var->funcDefn = this;
//    }

  if (! dt.type->isFunctionType() ) {
    env.error(stringc
      << "function declarator must be of function type, not `"
      << dt.type->toString() << "'",
      EF_DISAMBIGUATES);
    return;
  }

  // grab the definition type for later use
  funcType = dt.type->asFunctionType();

  if (checkBody) {      // so this is a definition and not just a declaration
    // force the parameter and return types to be complete (8.3.5 para 6)
    env.ensureCompleteType("use as return type", funcType->retType);
    SFOREACH_OBJLIST(Variable, funcType->params, iter) {
      env.ensureCompleteType("use as parameter type", iter.data()->type);
    }

    // NASTY (expletive) (expletive) HACK: Our STA_REFERENCE thing is
    // causing a problem for d0053.cc, because 'dt.type' is A2<17>
    // whereas 'nameAndParams->var->type' is A2<I>, 'I' being a
    // reference (to the param itself...).  So, do the same
    // force-complete to the nameAndParams->var->type.....
    {
      FunctionType *declFuncType = nameAndParams->var->type->asFunctionType();
      env.ensureCompleteType("use (in declFuncType) as return type", declFuncType->retType);
      SFOREACH_OBJLIST(Variable, declFuncType->params, iter) {
        env.ensureCompleteType("use (in declFuncType) as parameter type", iter.data()->type);
      }
    }
  }

  // record the definition scope for this template, since this
  // information is needed to instantiate it
  if (nameAndParams->var->templateInfo()) {
    Scope *s = env.nonTemplateScope();
    if (s->isPermanentScope()) {
      nameAndParams->var->templateInfo()->defnScope = s;
    }
    else {
      // If I just point 'defnScope' at 's' anyway, it might even
      // work, but it would violate my invariant that we only point at
      // permanent scopes.
      //
      // Actually, my grammar currently doesn't allow local templates,
      // so this is probably a non-issue.
      env.warning("local template instantiation is not currently implemented");
    }
  }

  if (checkBody) {
    tcheckBody(env);
  }
}

void Function::tcheckBody(Env &env)
{
  // FIX: not sure if this should be set first or last, but I don't
  // want it done twice, so we do it first for now
  hasBodyBeenTChecked = true;

  // once we get into the body of a function, if we end up triggering
  // additional instantiations, they should *not* see any prevailing
  // second-pass mode
  Restorer<bool> re(env.secondPassTcheck, false);

  // location for random purposes..
  SourceLoc loc = nameAndParams->var->loc;

  // if this function was originally declared in another scope
  // (main example: it's a class member function), then start
  // by extending that scope so the function body can access
  // the class's members
  ScopeSeq qualifierScopes;
  CompoundType *inClass = NULL;
  {
    Scope *s = nameAndParams->var->scope;
    if (s) {
      inClass = s->curCompound;   // might be NULL, that's ok

      // current scope must enclose 's':
      //   - if 's' is a namespace, 7.3.1.2 para 2 says so
      //   - if 's' is a class, 9.3 para 2 says so
      // example of violation: in/std/7.3.1.2b.cc, error 2
      bool encloses = env.currentScopeAboveTemplEncloses(s);
      if (!encloses) {
        if (dflags & DF_FRIEND) {
          // (t0291.cc) a friend definition is a little funky, and (IMO)
          // 11.4 isn't terribly clear on this point, so I'll just try
          // suppressing the error in this case
        }
        else {
          env.error(stringc
            << "function definition of `" << *(nameAndParams->getDeclaratorId())
            << "' must appear in a namespace that encloses the original declaration");
        }
      }

      // these two lines are the key to this whole block..  I'm
      // keeping the surrounding stuff, though, because it has that
      // error report above, and simply to avoid disturbing existing
      // (working) mechanism
      env.getQualifierScopes(qualifierScopes, nameAndParams->getDeclaratorId());
      env.extendScopeSeq(qualifierScopes);

      // the innermost scope listed in 'qualifierScopes'
      // should be the same one in which the variable was
      // declared (could this be triggered by user code?)
      if (encloses && qualifierScopes.isNotEmpty()) {
        // sm: 8/11/04: At one point this assertion was weakened to a
        // condition involving matching types.  That was wrong; the
        // innermost scope must be *exactly* the declaration scope of
        // the function, otherwise we'll be looking in the wrong scope
        // for members, etc.
        xassert(s == qualifierScopes.top());
      }
    }
  }

  // the parameters will have been entered into the parameter
  // scope, but that's gone now; make a new scope for the
  // function body and enter the parameters into that
  Scope *bodyScope = env.enterScope(SK_PARAMETER, "function parameter bindings");
  bodyScope->curFunction = this;
  SFOREACH_OBJLIST_NC(Variable, funcType->params, iter) {
    Variable *v = iter.data();
    if (v->name) {
      env.addVariable(v);
    }
  }

  // is this a nonstatic member function?
  if (funcType->isMethod()) {
    this->receiver = funcType->getReceiver();

    // this would be redundant--the parameter list already got
    // added to the environment, and it included '__receiver'
    //env.addVariable(receiver);
  }

  // while ctors do not appear to the caller to accept a receiver,
  // to the ctor itself there *is* a receiver; so synthesize one
  if (nameAndParams->var->name == env.constructorSpecialName) {
    xassert(inClass);

    xassert(!receiver);
    receiver = env.receiverParameter(loc, inClass, CV_NONE,
                                     NULL /*syntax*/);

    xassert(receiver->type->isReference());   // paranoia
    env.addVariable(receiver);
  }

  // have to check the member inits after adding the parameters
  // to the environment, because the initializing expressions
  tcheck_memberInits(env);

  // declare the __func__ variable
  if (env.lang.implicitFuncVariable ||
      env.lang.gccFuncBehavior == CCLang::GFB_variable) {
    // static char const __func__[] = "function-name";
    SourceLoc loc = body->loc;
    Type *charConst = env.getSimpleType(loc, ST_CHAR, CV_CONST);
    Type *charConstArr = env.makeArrayType(loc, charConst);

    if (env.lang.implicitFuncVariable) {
      Variable *funcVar = env.makeVariable(loc, env.string__func__,
                                           charConstArr, DF_STATIC);

      // I'm not going to add the initializer, because I'd need to make
      // an Expression AST node (which is no problem) but I don't have
      // anything to hang it off of, so it would leak.. I could add
      // a field to Function, but then I'd pay for that even when
      // 'implicitFuncVariable' is false..
      env.addVariable(funcVar);
    }

    // dsw: these two are also gcc; see
    // http://gcc.gnu.org/onlinedocs/gcc-3.4.1/gcc/Function-Names.html#Function%20Names
    if (env.lang.gccFuncBehavior == CCLang::GFB_variable) {
      env.addVariable(env.makeVariable(loc, env.string__FUNCTION__,
                                       charConstArr, DF_STATIC));
      env.addVariable(env.makeVariable(loc, env.string__PRETTY_FUNCTION__,
                                       charConstArr, DF_STATIC));
    }
  }

  // check the body in the new scope as well
  Statement *sel = body->tcheck(env);
  xassert(sel == body);     // compounds are never ambiguous

  if (handlers) {
    tcheck_handlers(env);
    
    // TODO: same checks for handlers that S_try::itcheck mentions in
    // its TODO ...
  }

  // close the new scope
  env.exitScope(bodyScope);

  // stop extending the named scope, if there was one
  env.retractScopeSeq(qualifierScopes);

  if (env.lang.treatExternInlineAsPrototype &&
      dflags >= (DF_EXTERN | DF_INLINE)) {
    // more extern-inline nonsense; skip 'funcDefn' setting
    return;
  }

  // this is a function definition; add a pointer from the
  // associated Variable
  //
  // WARNING: Due to the way function templates are instantiated it is
  // important to NOT move this line ABOVE this other line which is
  // above.
  //    if (!checkBody) {
  //      return;
  //    }
  // That is, it is important for the var of a function Declarator to
  // not have a funcDefn until after its whole body has been
  // typechecked.  See comment after 'if (!baseSyntax)' in
  // Env::instantiateTemplate()
  //
  // UPDATE: I've changed this invariant, as I need to point the
  // funcDefn at the definition even if the body has not been tchecked.
  if (nameAndParams->var->funcDefn) {
    xassert(nameAndParams->var->funcDefn == this);
  } else {
    nameAndParams->var->funcDefn = this;
  }
}


CompoundType *Function::verifyIsCtor(Env &env, char const *context)
{
  // make sure this function is a class member
  CompoundType *enclosing = NULL;
  if (nameAndParams->var->scope) {
    enclosing = nameAndParams->var->scope->curCompound;
  }
  if (!enclosing) {
    env.error(stringc
      << context << " are only valid for class member "
      << "functions (constructors in particular)",
      EF_DISAMBIGUATES);
    return NULL;
  }

  // make sure this function is a constructor; should already have
  // been mapped to the special name
  if (nameAndParams->var->name != env.constructorSpecialName) {
    env.error(stringc
      << context << " are only valid for constructors",
      EF_DISAMBIGUATES);
    return NULL;
  }

  return enclosing;
}


bool hasDependentActualArgs(FakeList<ArgExpression> *args)
{
  FAKELIST_FOREACH(ArgExpression, args, iter) {
    // <dependent> or TypeVariable or PseudoInstantiation
    if (iter->expr->type->containsGeneralizedDependent()) {
      return true;
    }
  }
  return false;
}


// cppstd 12.6.2 covers member initializers
void Function::tcheck_memberInits(Env &env)
{
  if (inits ||
      nameAndParams->var->name == env.constructorSpecialName) {
    // this should be a constructor
    CompoundType *enclosing = verifyIsCtor(env, "ctor member inits");
    if (!enclosing) {
      return;
    }

    // ok, so far so good; now go through and check the member inits
    // themselves
    FAKELIST_FOREACH_NC(MemberInit, inits, iter) {
      iter->tcheck(env, enclosing);
    }
  }
  else {
    // no inits and doesn't have a ctor name, skip
  }
}

void MemberInit::tcheck(Env &env, CompoundType *enclosing)
{
  // resolve template arguments in 'name'
  name->tcheck(env);

  // typecheck the arguments
  // dsw: I do not want to typecheck the args twice, as it is giving
  // me problems, so I moved this
//    args = tcheckArgExprList(args, env);

  // check for a member variable, since they have precedence over
  // base classes [para 2]; member inits cannot have qualifiers
  if (!name->hasQualifiers()) {
    // look for the given name in the class; should be an immediate
    // member, not one that was inherited
    Variable *v =
      enclosing->lookupVariable(name->getName(), env, LF_INNER_ONLY);
    if (v && !v->hasFlag(DF_TYPEDEF)) {     // typedef -> fall down to next area (in/t0390.cc)
      // only "nonstatic data member"
      if (v->hasFlag(DF_STATIC) ||
          v->type->isFunctionType()) {
        env.error("you can't initialize static data "
                  "nor member functions in a ctor member init list");
        return;
      }

      // annotate the AST
      member = env.storeVar(v);

      args = tcheckArgExprList(args, env);
      if (!hasDependentActualArgs(args)) {     // in/t0270.cc
        // decide which of v's possible constructors is being used
        ctorVar = env.storeVar(
          outerResolveOverload_ctor(env, env.loc(), v->type, args,
                                    env.doOverload()));
        env.ensureFuncBodyTChecked(ctorVar);
      }

      // TODO: check that the passed arguments are consistent
      // with at least one constructor of the variable's type.
      // dsw: update: isn't this what the assertion that ctorVar is
      // non-null is doing, since the call to
      // outerResolveOverload_ctor() will not succeed otherwise?

      // TODO: make sure that we only initialize each member once.
      // dsw: update: see below; do this in
      // cc_elaborate.cc:completeNoArgMemberInits()

      // TODO: provide a warning if the order in which the
      // members are initialized is different from their
      // declaration order, since the latter determines the
      // order of side effects.
      // dsw: update: this would have to be done in
      // cc_elaborate.cc:completeNoArgMemberInits(), since I re-order
      // the MemberInits there.

      return;
    }
  }

  // not a member name.. what about the name of base class?
  // since the base class initializer can use any name which
  // denotes the base class [para 2], first look up the name
  // in the environment generally
  Variable *baseVar = env.lookupPQVariable(name);
  if (!baseVar ||
      !baseVar->hasFlag(DF_TYPEDEF) ||
      !baseVar->type->isCompoundType()) {
    env.error(stringc
              << "`" << *name << "' does not denote any class");
    return;
  }
  CompoundType *baseClass = baseVar->type->asCompoundType();

  // is this class a direct base, and/or an indirect virtual base?
  bool directBase = false;
  bool directVirtual = false;
  bool indirectVirtual = false;
  FOREACH_OBJLIST(BaseClass, enclosing->bases, baseIter) {
    BaseClass const *b = baseIter.data();

    // check for direct base
    if (b->ct == baseClass) {
      directBase = true;
      directVirtual = b->isVirtual;
    }

    // check for indirect virtual base by looking for virtual
    // base of a direct base class
    if (b->ct->hasVirtualBase(baseClass)) {
      indirectVirtual = true;
    }
  }

  // did we find anything?
  if (!directBase && !indirectVirtual) {
    // if there are qualifiers, then it can't possibly be an
    // attempt to initialize a data member
    char const *norData = name->hasQualifiers()? "" : ", nor a data member,";
    env.error(stringc
              << "`" << *name << "' is not a base class" << norData
              << " so it cannot be initialized here");
    return;
  }

  // check for ambiguity [para 2]
  if (directBase && !directVirtual && indirectVirtual) {
    env.error(stringc
              << "`" << *name << "' is both a direct non-virtual base, "
              << "and an indirect virtual base; therefore the initializer "
              << "is ambiguous (there's no quick fix--you have to change "
              << "your inheritance hierarchy or forego initialization)");
    return;
  }

  // annotate the AST
  base = baseClass;

  // TODO: verify correspondence between template arguments
  // in the initializer name and template arguments in the
  // base class list

  // TODO: check that the passed arguments are consistent
  // with the chosen constructor

  args = tcheckArgExprList(args, env);
  // determine which constructor is being called
  ctorVar = env.storeVar(
    outerResolveOverload_ctor(env, env.loc(),
                              baseVar->type,
                              args,
                              env.doOverload()));
  env.ensureFuncBodyTChecked(ctorVar);
}


void Function::tcheck_handlers(Env &env)
{
  if (!verifyIsCtor(env, "ctor exception handlers")) {
    return;
  }
  
  FAKELIST_FOREACH_NC(Handler, handlers, iter) {
    iter->tcheck(env);
  }
}


bool Function::uninstTemplatePlaceholder() const
{
  return !hasBodyBeenTChecked &&
         nameAndParams->var &&
         nameAndParams->var->templateInfo();
}


// MemberInit

// -------------------- Declaration -------------------
void Declaration::tcheck(Env &env, DeclaratorContext context)
{
  // if there are no declarators, the type specifier's tchecker
  // needs to know this (for e.g. 3.3.1 para 5)
  if (decllist->isEmpty() &&
      spec->isTS_elaborated()) {
    dflags |= DF_FORWARD;
  }

  // if we're declaring an anonymous type, and there are
  // some declarators, then give the type a name; we don't
  // give names to anonymous types with no declarators as
  // a special exception to allow anonymous unions
  if (decllist->isNotEmpty()) {
    if (spec->isTS_classSpec()) {
      TS_classSpec *cs = spec->asTS_classSpec();
      if (cs->name == NULL) {
        cs->name = new PQ_name(SL_UNKNOWN, env.getAnonName(cs->keyword));
      }
    }
    if (spec->isTS_enumSpec()) {
      TS_enumSpec *es = spec->asTS_enumSpec();
      if (es->name == NULL) {
        es->name = env.getAnonName(TI_ENUM);
      }
    }
  }

  // check the specifier in the prevailing environment
  Type *specType = spec->tcheck(env, dflags);

  // ---- the following code is adopted from (the old) tcheckFakeExprList ----
  // (I couldn't just use the same code, templatized as necessary,
  // because I need my Declarator::Tcheck objects computed anew for
  // each declarator..)
  if (decllist) {
    // check first declarator
    Declarator::Tcheck dt1(specType, dflags, context);
    decllist = FakeList<Declarator>::makeList(decllist->first()->tcheck(env, dt1));

    // check subsequent declarators
    Declarator *prev = decllist->first();
    while (prev->next) {
      // some analyses don't want the type re-used, so let
      // the factory clone it if it wants to
      Type *dupType = env.tfac.cloneType(specType);

      Declarator::Tcheck dt2(dupType, dflags, context);
      prev->next = prev->next->tcheck(env, dt2);

      prev = prev->next;
    }
  }
  // ---- end of code from tcheckFakeExprList ----
}


// -------------------- ASTTypeId -------------------
int countD_funcs(ASTTypeId const *n)
{
  IDeclarator const *p = n->decl->decl;
  int ct=0;
  while (p->isD_func()) {
    ct++;
    p = p->asD_funcC()->base;
  }
  return ct;
}

ASTTypeId *ASTTypeId::tcheck(Env &env, Tcheck &tc)
{
  if (!ambiguity) {
    mid_tcheck(env, tc);
    return this;
  }

  ASTTypeId *ret = resolveImplIntAmbig(env, this);
  if (ret) {
    return ret->tcheck(env, tc);
  }

  return resolveAmbiguity(this, env, "ASTTypeId", false /*priority*/, tc);
}

void ASTTypeId::mid_tcheck(Env &env, Tcheck &tc)
{
  #ifdef GNU_EXTENSION
  if (tc.context == DC_E_COMPOUNDLIT
      && spec->isTS_classSpec()
      && spec->asTS_classSpec()->keyword == TI_UNION
      && !spec->asTS_classSpec()->name) {
    // in an E_compoundLit, gcc does not do anonymous union scope
    // promotion, even in C++ mode; so make up a name for the union
    StringRef fakeName = env.getAnonName(TI_UNION);
    spec->asTS_classSpec()->name = new PQ_name(env.loc(), fakeName);
    TRACE("gnu", "substituted name " << fakeName << 
                 " in anon union at " << decl->getLoc());
  }
  #endif // GNU_EXTENSION

  // check type specifier
  Type *specType = spec->tcheck(env, DF_NONE);

  // pass contextual info to declarator
  Declarator::Tcheck dt(specType, tc.dflags, tc.context);
  
  xassert(!tc.newSizeExpr || tc.context == DC_E_NEW);

  // check declarator
  decl = decl->tcheck(env, dt);
                     
  // retrieve add'l info from declarator's tcheck struct
  if (tc.newSizeExpr) {
    *(tc.newSizeExpr) = dt.size_E_new;
  }
}


Type *ASTTypeId::getType() const
{
  xassert(decl->var);
  return decl->var->type;
}


// ---------------------- PQName -------------------
void tcheckTemplateArgumentList(ASTList<TemplateArgument> &list, Env &env)
{
  // loop copied/modified from S_compound::itcheck
  FOREACH_ASTLIST_NC(TemplateArgument, list, iter) {
    // have to potentially change the list nodes themselves
    iter.setDataLink( iter.data()->tcheck(env) );
  }
}

void PQ_qualifier::tcheck(Env &env, Scope *scope, LookupFlags lflags)
{
  if (lflags & LF_DEPENDENT) {
    // the previous qualifier was dependent; do truncated processing here
    if (!templateUsed()) {
      // without the "template" keyword, the dependent context may give
      // rise to ambiguity, so reject it
      env.error("dependent template scope name requires 'template' keyword",
                EF_DISAMBIGUATES | EF_STRONG);
    }
    tcheckTemplateArgumentList(targs, env);
    denotedScopeVar = env.dependentVar;
    rest->tcheck(env, scope, lflags);
    return;
  }

  // begin by looking up the bare name, igoring template arguments
  Variable *bareQualifierVar = env.lookupOneQualifier_bareName(scope, this, lflags);

  // now check the arguments
  tcheckTemplateArgumentList(targs, env);

  // pull them out into a list
  SObjList<STemplateArgument> sargs;
  if (!env.templArgsASTtoSTA(targs, sargs)) {
    // error already reported; just finish up and bail
    rest->tcheck(env, scope, lflags);
    return;
  }

  // scope that has my template params
  Scope *hasParamsForMe = NULL;

  if ((lflags & LF_DECLARATOR) &&                // declarator context
      bareQualifierVar &&                        // lookup succeeded
      bareQualifierVar->isTemplate() &&          // names a template
      targs.isNotEmpty()) {                      // arguments supplied
    // In a template declarator context, we want to stage the use of
    // the template parameters so as to ensure proper association
    // between parameters and qualifiers.  For example, if we have
    //
    //   template <class S>
    //   template <class T>
    //   int A<S>::foo(T *t) { ... }
    //
    // and we're about to tcheck "A<S>", the SK_TEMPLATE_PARAMS scope
    // containing T that is currently on the stack must be temporarily
    // removed so that the template arguments to A cannot see it.
    //
    // So, for this and other reasons, find the template argument or
    // parameter scope that corresponds to 'bareQualifierVar'.

    // But there is an exception to the rule; this is implied by
    // 14.5.4.3, as it gives the above rules only for partial
    // class specializations (so arguments are not all concrete) and
    // explicit specializations of *implicitly* specialized classes
    // (so the template does not have a matching explicit spec):
    //   - if all template args are concrete, and
    //   - they correspond to an explicit specialization
    //   - then "template <>" is *not* used (for that class)
    // t0248.cc tests a couple cases...
    if (!containsTypeVariables(sargs) &&
        bareQualifierVar->templateInfo()->getSpecialization(env.tfac, sargs)) {
      // do not associate 'bareQualifier' with any template scope
    }
    else {
      hasParamsForMe = env.findParameterizingScope(bareQualifierVar);
    }
  }

  // finish looking up this qualifier
  bool dependent;
  bool anyTemplates;        // will be ignored
  Scope *denotedScope = env.lookupOneQualifier_useArgs(
    bareQualifierVar,       // scope found w/o using args
    this,                   // qualifier (to look up)
    dependent, anyTemplates, lflags);

  // save the result in our AST annotation field, 'denotedScopeVar'
  if (!denotedScope) {
    if (dependent) {
      denotedScopeVar = env.dependentVar;
      lflags |= LF_DEPENDENT;
    }
    else {
      // error; recovery: just keep going (error already reported)
    }
  }
  else {
    if (denotedScope->curCompound) {
      denotedScopeVar = denotedScope->curCompound->typedefVar;

      // must be a complete type [ref?] (t0245.cc)
      env.ensureCompleteType("use as qualifier", denotedScopeVar->type);
      
      if (hasParamsForMe) {
        // cement the association now
        hasParamsForMe->setParameterizedEntity(denotedScope->curCompound->typedefVar);

        TRACE("templateParams",
          "associated " << hasParamsForMe->desc() <<
          " with " << denotedScope->curCompound->instName);
      }
    }
    else if (denotedScope->isGlobalScope()) {
      denotedScopeVar = env.globalScopeVar;
    }
    else {
      xassert(denotedScope->isNamespace());
      denotedScopeVar = denotedScope->namespaceVar;

      xassert(!hasParamsForMe);     // namespaces aren't templatized
    }
  }

  rest->tcheck(env, denotedScope, lflags);
}

void PQ_name::tcheck(Env &env, Scope *, LookupFlags)
{}

void PQ_operator::tcheck(Env &env, Scope *, LookupFlags)
{
  o->tcheck(env);
}

void PQ_template::tcheck(Env &env, Scope *, LookupFlags lflags)
{                             
  // like above in PQ_qualifier::tcheck
  if ((lflags & LF_DEPENDENT) && !templateUsed()) {
    env.error("dependent template name requires 'template' keyword",
              EF_DISAMBIGUATES | EF_STRONG);
  }

  tcheckTemplateArgumentList(args, env);
}


// -------------- "dependent" name stuff ---------------
// This section has a few functions that are common to the AST
// nodes that have to deal with dependent name issues.  (14.6.2)

// lookup has found 'var', and we might want to stash it
// in 'nondependentVar' too
void maybeNondependent(Env &env, SourceLoc loc, Variable *&nondependentVar,
                       Variable *var)
{
  if (!var->hasFlag(DF_TYPEDEF) && var->type->isGeneralizedDependent()) {
    // if this was the name of a variable, but the type refers to some
    // template params, then this shouldn't be considered
    // non-dependent (t0276.cc)
    //
    // TODO: this can't be right... there is still a fundamental
    // flaw in how I regard names whose lookup is nondependent but
    // the thing they look up to is dependent
    return;
  }

  if (!var->scope && !var->isTemplateParam()) {
    // I'm pretty sure I don't need to remember names that are not
    // in named scopes, other than template params themselves (t0277.cc)
    return;
  }

  if (env.inUninstTemplate() &&               // we're in a template
      !var->type->isSimple(ST_DEPENDENT) &&   // lookup was non-dependent
      !var->type->isError() &&                // and not erroneous
      !var->isMemberOfTemplate()) {           // will be in scope in instantiation
    TRACE("dependent", toString(loc) << ": " <<
                       (nondependentVar? "replacing" : "remembering") <<
                       " non-dependent lookup of `" << var->name <<
                       "' found var at " << toString(var->loc));
    nondependentVar = var;
  }
}

// if we want to re-use 'nondependentVar', return it; otherwise return NULL
Variable *maybeReuseNondependent(Env &env, SourceLoc loc, LookupFlags &lflags,
                                 Variable *nondependentVar)
{
  // should I use 'nondependentVar'?
  if (nondependentVar && !env.inUninstTemplate()) {
    if (nondependentVar->isTemplateParam()) {
      // We don't actually want to use 'nondependentVar', because that
      // Variable was only meaningful to the template definition.  Instead
      // we want the *corresponding* Variable that is now bound to a
      // template argument, and LF_TEMPL_PARAM will find it (and skip any
      // other names).
      TRACE("dependent", toString(loc) <<
                         ": previously-remembered non-dependent lookup for `" <<
                         nondependentVar->name << "' was a template parameter");
      lflags |= LF_TEMPL_PARAM;
    }
    else {
      // use 'nondependentVar' directly
      TRACE("dependent", toString(loc) <<
                         ": using previously-remembered non-dependent lookup for `" <<
                         nondependentVar->name << "': at " <<
                         toString(nondependentVar->loc));
      return nondependentVar;
    }
  }

  // do the lookup without using 'nondependentVar'
  return NULL;
}


// --------------------- TypeSpecifier --------------
Type *TypeSpecifier::tcheck(Env &env, DeclFlags dflags)
{
  env.setLoc(loc);

  Type *t = itcheck(env, dflags);
  Type *ret = env.tfac.applyCVToType(loc, cv, t, this);
  if (!ret) {
    return env.error(t, stringc
      << "cannot apply const/volatile to type `" << t->toString() << "'");
  }
  return ret;
}


Type *TS_name::itcheck(Env &env, DeclFlags dflags)
{
  name->tcheck(env);

  ErrorFlags eflags = EF_NONE;
  LookupFlags lflags = LF_NONE;

  // should I use 'nondependentVar'?
  Variable *v = maybeReuseNondependent(env, loc, lflags, nondependentVar);
  if (v) {
    var = v;
    return var->type;
  }

  if (typenameUsed) {
    if (!name->hasQualifiers()) {
      // cppstd 14.6 para 5, excerpt:
      //   "The keyword typename shall only be applied to qualified
      //    names, but those names need not be dependent."
      env.error("the `typename' keyword can only be used with a qualified name");
    }

    lflags |= LF_TYPENAME;
  }
  else {
    // if the user uses the keyword "typename", then the lookup errors
    // are non-disambiguating, because the syntax is unambiguous;
    // otherwise, they are disambiguating (which is the usual case)
    eflags |= EF_DISAMBIGUATES;
  }

  if (!env.lang.isCplusplus) {
    // in C, we never look at a class scope unless the name is
    // preceded by "." or "->", which it certainly is not in a TS_name
    // (in/c/dC0013.c)
    lflags |= LF_SKIP_CLASSES;
  }

  v = env.lookupPQVariable(name, lflags);
  if (!v) {
    // NOTE:  Since this is marked as disambiguating, but the same
    // error message in E_variable::itcheck is not marked as such, it
    // means we prefer to report the error as if the interpretation as
    // "variable" were the only one.
    return env.error(stringc
      << "there is no type called `" << *name << "'", eflags);
  }

  if (!v->hasFlag(DF_TYPEDEF)) {
    if (v->type && v->type->isSimple(ST_DEPENDENT)) {
      // more informative error message (in/d0111.cc, error 1)
      return env.error(stringc
        << "dependent name `" << *name
        << "' used as a type, but the 'typename' keyword was not supplied", eflags);
    }
    else {
      return env.error(stringc
        << "variable name `" << *name << "' used as if it were a type", eflags);
    }
  }

  // TODO: access control check

  var = env.storeVar(v);    // annotation

  // should I remember this non-dependent lookup?
  maybeNondependent(env, loc, nondependentVar, var);

  // 7/27/04: typedefAliases used to be maintained at this point, but
  // now it is gone

  // there used to be a call to applyCV here, but that's redundant
  // since the caller (tcheck) calls it too
  return var->type;
}


Type *TS_simple::itcheck(Env &env, DeclFlags dflags)
{ 
  // This is the one aspect of the implicit-int solution that cannot
  // be confined to implint.h: having selected an interpretation that
  // uses implicit int, change it to ST_INT so that analyses don't
  // have to know about any of this parsing nonsense.
  if (id == ST_IMPLINT) {
    id = ST_INT;
  }

  return env.getSimpleType(loc, id, cv);
}


// This function maps syntax like
//
//   "class A"                            // ordinary class
//   "class A<int>"                       // existing instantiation or specialization
//   "template <class T> class A"         // template class decl
//   "template <class T> class A<T*>"     // partial specialization decl
//   "template <> class A<int>"           // complete specialization decl
//
// to a particular CompoundType.  If there are extant template
// parameters, then 'templateParams' will be set below to non-NULL
// (but possibly empty).  If the name has template arguments applied
// to it, then 'templateArgs' will be non-NULL.
//
// There are three syntactic contexts for this, identified by the
// 'dflags' present.  Within each of these I categorize w.r.t. the
// presence of template parameters and arguments:
//
//   "class A ;"          // DF_FORWARD=1  DF_DEFINITION=0
//
//     no-params no-args    ordinary forward declaration
//     params    no-args    fwd decl of template primary
//     no-params args       old-style explicit instantiation request [ref?]
//     params    args       fwd decl of explicit specialization
//
//   "class A *x ;"       // DF_FORWARD=0  DF_DEFINITION=0
//
//     no-params no-args    fwd decl, but might push into outer scope (3.3.1 para 5)
//     params    no-args    illegal [ref?]
//     no-params args       use of a template instantiation or specialization
//     params    args       illegal
//
//   "class A { ... } ;"  // DF_FORWARD=0  DF_DEFINITION=1
//
//     no-params no-args    ordinary class definition
//     params    no-args    template primary definition
//     no-params args       illegal
//     params    args       template specialization definition
//
// Note that the first and third cases are fairly parallel, one being
// a declaration and the other a definition.  The second case is the
// odd one out, though it is more like case 1 than case 3.  It is also
// quite rare, as such uses are almost always written without the
// "class" keyword.
CompoundType *checkClasskeyAndName(
  Env &env,
  SourceLoc loc,             // location of type specifier
  DeclFlags dflags,          // syntactic and semantic declaration modifiers
  TypeIntr keyword,          // keyword used
  PQName *name)              // name, with qualifiers and template args (if any)
{
  // context flags
  bool forward = (dflags & DF_FORWARD);
  bool definition = (dflags & DF_DEFINITION);
  xassert(!( forward && definition ));

  // handle 'friend' the same way I do other case 2 decls, even though
  // that isn't quite right
  if (dflags & DF_FRIEND) {
    if (definition) {
      env.error("no friend definitions, please");
      return NULL;
    }
    forward = false;
  }

  // template params?
  SObjList<Variable> *templateParams =
    env.scope()->hasTemplateParams()? &(env.scope()->templateParams) : NULL;

  // template args?
  ASTList<TemplateArgument> *templateArgs = NULL;
  if (name) {
    PQName *unqual = name->getUnqualifiedName();
    if (unqual->isPQ_template()) {
      // get the arguments
      templateArgs = &(unqual->asPQ_template()->args);
    }
  }

  // reject illegal combinations
  if (!forward && !definition && templateParams) {
    // Actually, this requirement holds regardless of whether there is
    // a definition, but 'forward' only signals the presence of
    // declarators in non-definition cases.  So this error should be
    // caught elsewhere.  The code below will not run into trouble in
    // this case, so just let it go.
    //return env.error("templatized class declarations cannot have declarators");
  }
  if (definition && !templateParams && templateArgs) {
    env.error("class specifier name can have template arguments "
              "only in a templatized definition");
    return NULL;
  }
  if (keyword==TI_UNION && (templateParams || templateArgs)) {
    env.error("template unions are not allowed");
    return NULL;
  }

  // see if the environment already has this name
  CompoundType *ct = NULL;
  if (name) {
    // decide how the lookup should be performed
    LookupFlags lflags = LF_NONE;
    if (!name->hasQualifiers() && (forward || definition)) {
      lflags |= LF_INNER_ONLY;
    }
    if (templateParams) {
      lflags |= LF_TEMPL_PRIMARY;
    }

    // do it
    ct = env.lookupPQCompound(name, lflags);

    // failed lookup is cause for immediate abort in a couple of cases
    if (!ct &&
        (name->hasQualifiers() || templateArgs)) {
      env.error(stringc << "no such " << toString(keyword)
                        << ": `" << *name << "'");
      return NULL;
    }
  }
  CompoundType *primary = ct;       // used for specializations

  // if we have template args and params, refine 'ct' down to the
  // specialization of interest (if already declared)
  if (templateParams && templateArgs) {
    // this is supposed to be a specialization
    TemplateInfo *primaryTI = primary->templateInfo();
    if (!primaryTI) {
      env.error("attempt to specialize a non-template");
      return NULL;
    }

    // get the template arguments
    SObjList<STemplateArgument> sargs;
    if (!env.templArgsASTtoSTA(*templateArgs, sargs)) {
      return NULL;         // already reported the error
    }

    // does this specialization already exist?
    Variable *spec = primaryTI->getSpecialization(env.tfac, sargs);
    if (spec) {
      ct = spec->type->asCompoundType();
    }
    else {
      ct = NULL;
    }
  }

  // if already declared, compare to that decl
  if (ct) {
    // check that the keywords match
    if ((int)ct->keyword != (int)keyword) {
      // it's ok for classes and structs to be confused (7.1.5.3 para 3)
      if ((keyword==TI_UNION) != (ct->keyword==CompoundType::K_UNION)) {
        env.error(stringc
          << "there is already a " << ct->keywordAndName()
          << ", but here you're defining a " << toString(keyword)
          << " " << *name);
        return NULL;
      }

      // let the definition keyword (of a template primary only)
      // override any mismatching prior decl
      if (definition && !templateArgs) {
        TRACE("env", "changing " << ct->keywordAndName() <<
                     " to a " << toString(keyword) << endl);
        ct->keyword = (CompoundType::Keyword)keyword;
      }
    }

    // definition of something previously declared?
    if (definition) {
      if (ct->templateInfo()) {
        TRACE("template", "template class " <<
                          (templateArgs? "specialization " : "") <<
                          "definition: " << ct->templateInfo()->templateName());
      }

      if (ct->forward) {
        // now it is no longer a forward declaration
        ct->forward = false;
      }
      else {
        env.error(stringc << ct->keywordAndName() << " has already been defined");
      }
    }

    if (!templateParams && templateArgs) {
      // this is more like an instantiation than a declaration
    }
    else {
      // check correspondence between extant params and params on 'ct'
      env.verifyCompatibleTemplateParameters(ct);
    }
  }

  // if not already declared, make a new CompoundType
  else {
    // get the raw name for use in creating the CompoundType
    StringRef stringName = name? name->getName() : NULL;

    // making an ordinary compound, or a template primary?
    if (!templateArgs) {
      Scope *destScope = (forward || definition) ?
        // if forward=true, 3.3.1 para 5 says:
        //   the elaborated-type-specifier declares the identifier to be a
        //   class-name in the scope that contains the declaration
        // if definition=true, I think it works the same [ref?]
        env.typeAcceptingScope() :
        // 3.3.1 para 5 says:
        //   the identifier is declared in the smallest non-class,
        //   non-function-prototype scope that contains the declaration
        env.outerScope() ;

      // this sets the parameterized primary of the scope
      env.makeNewCompound(ct, destScope, stringName, loc, keyword,
                          !definition /*forward*/);

      if (templateParams) {
        TRACE("template", "template class " << (definition? "defn" : "decl") <<
                          ": " << ct->templateInfo()->templateName());
      }
    }

    // making a template specialization
    else {
      // get the template arguments (again)
      SObjList<STemplateArgument> sargs;
      if (!env.templArgsASTtoSTA(*templateArgs, sargs)) {
        xfailure("what what what?");     // already got them above!
      }

      // make a new type, since a specialization is a distinct template
      // [cppstd 14.5.4 and 14.7]; but don't add it to any scopes
      env.makeNewCompound(ct, NULL /*scope*/, stringName, loc, keyword,
                          !definition /*forward*/);

      // dsw: need to register it at least, even if it isn't added to
      // the scope, otherwise I can't print out the name of the type
      // because at the top scope I don't know the scopeKind
      env.typeAcceptingScope()->registerVariable(ct->typedefVar);

      // similarly, the parentScope should be set properly
      env.setParentScope(ct);

      // 'makeNewCompound' will already have put the template *parameters*
      // into 'specialTI', but not the template arguments
      TemplateInfo *ctTI = ct->templateInfo();
      ctTI->copyArguments(sargs);

      // synthesize an instName to aid in debugging
      ct->instName = env.str(stringc << ct->name << sargsToString(ctTI->arguments));

      // add this type to the primary's list of specializations; we are not
      // going to add 'ct' to the environment, so the only way to find the
      // specialization is to go through the primary template
      TemplateInfo *primaryTI = primary->templateInfo();
      primaryTI->addSpecialization(ct->getTypedefVar());

      // the template parameters parameterize the primary
      //
      // 8/09/04: moved this below 'makeNewCompound' so the params
      // aren't regarded as inherited
      env.scope()->setParameterizedEntity(ct->typedefVar);

      TRACE("template", (definition? "defn" : "decl") <<
                        " of specialization of template class" <<
                        primary->typedefVar->fullyQualifiedName() <<
                        ", " << ct->instName);
    }
  }

  // record the definition scope, for instantiation to use
  if (templateParams && definition) {
    ct->templateInfo()->defnScope = env.nonTemplateScope();
  }

  return ct;
}


Type *TS_elaborated::itcheck(Env &env, DeclFlags dflags)
{
  env.setLoc(loc);

  name->tcheck(env);

  if (keyword == TI_ENUM) {
    EnumType *et = env.lookupPQEnum(name);
    if (!et) {
      if (!env.lang.allowIncompleteEnums ||
          name->hasQualifiers()) {
        return env.error(stringc << "there is no enum called `" << *name << "'",
                         EF_DISAMBIGUATES);
      }
      else {
        // make a forward-declared enum (gnu/d0083.c)
        et = new EnumType(name->getName());
        return env.declareEnum(loc, et);
      }
    }

    this->atype = et;          // annotation
    return env.makeType(loc, et);
  }
                                     
  CompoundType *ct =
    checkClasskeyAndName(env, loc, dflags, keyword, name);
  if (!ct) {
    ct = env.errorCompoundType;
  }

  this->atype = ct;              // annotation

  return ct->typedefVar->type;
}


Type *TS_classSpec::itcheck(Env &env, DeclFlags dflags)
{
  env.setLoc(loc);
  dflags |= DF_DEFINITION;

  // if we're on the second pass, then skip almost everything
  if (env.secondPassTcheck) {
    // just get the function bodies
    env.extendScope(ctype);
    tcheckFunctionBodies(env);
    env.retractScope(ctype);
    return ctype->typedefVar->type;
  }

  // lookup scopes in the name, if any
  ScopeSeq qualifierScopes;
  if (name) {                                      
    // 2005-02-18: passing LF_DECLARATOR fixes in/t0191.cc
    name->tcheck(env, NULL /*scope*/, LF_DECLARATOR);
  }
  env.getQualifierScopes(qualifierScopes, name);
  env.extendScopeSeq(qualifierScopes);

  // figure out which class the (keyword, name) pair refers to
  CompoundType *ct =
    checkClasskeyAndName(env, loc, dflags, keyword, name);
  if (!ct) {
    // error already reported
    env.retractScopeSeq(qualifierScopes);
    this->ctype = env.errorCompoundType;
    return this->ctype->typedefVar->type;
  }

  this->ctype = ct;           // annotation

  // check the body of the definition
  tcheckIntoCompound(env, dflags, ct);

  // 8/14/04: er... ahh.. what?  if we needed it before this we
  // would have already emitted an error!  nothing is accomplished
  // by this...
  //env.instantiateForwardClasses(ct->typedefVar);

  env.retractScopeSeq(qualifierScopes);

  return ct->typedefVar->type;
}


// filter that keeps only strong messages
bool strongMsgFilter(ErrorMsg *msg)
{
  if (msg->flags & (EF_STRONG | EF_STRONG_WARNING)) {
    // keep it
    return true;
  }
  else {
    // drop it
    TRACE("template", "dropping error arising from uninst template: " <<
                     msg->msg);
    return false;
  }
}


// type check once we know what 'ct' is; this is also called
// to check newly-cloned AST fragments for template instantiation
void TS_classSpec::tcheckIntoCompound(
  Env &env, DeclFlags dflags,    // as in tcheck
  CompoundType *ct)              // compound into which we're putting declarations
{
  // should have set the annotation by now
  xassert(ctype);

  // are we an inner class?
  CompoundType *containingClass = env.acceptingScope()->curCompound;
  if (env.lang.noInnerClasses) {
    // nullify the above; act as if it's an outer class
    containingClass = NULL;
  }

  // let me map from compounds to their AST definition nodes
  ct->syntax = this;

  // only report serious errors while checking the class,
  // in the absence of actual template arguments
  DisambiguateOnlyTemp disOnly(env, ct->isTemplate() /*disOnly*/);

  // we should not be in an ambiguous context, because that would screw
  // up the environment modifications; if this fails, it could be that
  // you need to do context isolation using 'DisambiguateNestingTemp'
  xassert(env.disambiguationNestingLevel == 0);

  // 9/21/04: d0102.cc demonstrates that certain errors that are
  // marked 'disambiguating' can still be superfluous because of being
  // in uninstantiated template code.  So I'm going to use a big
  // hammer here, and throw away all non-EF_STRONG errors once
  // tchecking of this template finishes.  For the moment, that means
  // I need to save the existing errors.
  ErrorList existingErrors;
  if (ct->isTemplate()) {
    existingErrors.takeMessages(env.errors);
  }

  // open a scope, and install 'ct' as the compound which is
  // being built; in fact, 'ct' itself is a scope, so we use
  // that directly
  //
  // 8/19/04: Do this before looking at the base class specs, because
  // any prevailing template params are now attached to 'ct' and hence
  // only visible there (t0271.cc).
  env.extendScope(ct);

  // look at the base class specifications
  if (bases) {
    FAKELIST_FOREACH_NC(BaseClassSpec, bases, iter) {
      // resolve any template arguments in the base class name
      iter->name->tcheck(env);

      // cppstd 10, para 1: ignore non-types when looking up
      // base class names
      Variable *baseVar = env.lookupPQVariable(iter->name, LF_ONLY_TYPES);
      if (!baseVar) {
        env.error(stringc
          << "no class called `" << *(iter->name) << "' was found",
          EF_NONE);
        continue;
      }
      xassert(baseVar->hasFlag(DF_TYPEDEF));    // that's what LF_ONLY_TYPES means

      // special case for template parameters
      if (baseVar->type->isTypeVariable()) {
        // let it go.. we're doing the pseudo-check of a template;
        // but there's nothing we can add to the base class list,
        // and it wouldn't help even if we could, so do nothing
        continue;
      }

      // cppstd 10, para 1: must be a class type
      CompoundType *base = baseVar->type->ifCompoundType();
      if (!base) {
        env.error(stringc
          << "`" << *(iter->name) << "' is not a class or "
          << "struct or union, so it cannot be used as a base class");
        continue;
      }

      // also 10 para 1: must be complete type
      if (!env.ensureCompleteType("use as base class", baseVar->type)) {
        continue;
      }

      // fill in the default access mode if the user didn't provide one
      // [cppstd 11.2 para 2]
      AccessKeyword acc = iter->access;
      if (acc == AK_UNSPECIFIED) {
        acc = (ct->keyword==CompoundType::K_CLASS? AK_PRIVATE : AK_PUBLIC);
      }                                  
      
      // add this to the class's list of base classes
      ct->addBaseClass(new BaseClass(base, acc, iter->isVirtual));
      
      // annotate the AST with the type we found
      iter->type = base;
    }

    // we're finished constructing the inheritance hierarchy
    if (tracingSys("printHierarchies")) {
      string h1 = ct->renderSubobjHierarchy();
      cout << "// ----------------- " << ct->name << " -------------------\n";
      cout << h1;

      // for debugging; this checks that the 'visited' flags are being
      // cleared properly, among other things
      string h2 = ct->renderSubobjHierarchy();
      if (!h1.equals(h2)) {
        cout << "WARNING: second rendering doesn't match first!\n";
      }
    }
  }

  // look at members: first pass is to enter them into the environment
  {
    // don't check function bodies
    Restorer<bool> r(env.checkFunctionBodies, false);

    FOREACH_ASTLIST_NC(Member, members->list, iter) {
      Member *member = iter.data();
      member->tcheck(env);
      
      #if 0     // old
      // dsw: The invariant that I have chosen for function members of
      // templatized classes is that we typecheck the declaration but
      // not the definition; the definition is typechecked when the
      // function is called or at some other later time.  This works
      // fine for out-of-line function definitions, as when the
      // definition is found, without typechecking the body, we point
      // the variable created by typechecking the declaration at the
      // definition, which will be typechecked later.  This means that
      // for in-line functions we need to do this as well.
      if (member->isMR_func()) {
        // an inline member function definition
        MR_func *mrfunc = member->asMR_func();
        Variable *mrvar = mrfunc->f->nameAndParams->var;
        xassert(mrvar);
        xassert(!mrvar->funcDefn);
        mrvar->funcDefn = mrfunc->f;
      }
      #endif // 0
    }
  }

  // 2005-02-17: check default arguments first so they are available
  // to all function bodies (hmmm... what if a default argument
  // expression invokes a function that itself has default args,
  // but appears later in the file?  how could that ever be handled
  // cleanly?)
  //
  // 2005-02-18: Do this here instead of in 'tcheckFunctionBodies'
  // for greater uniformity with template instantiation.  Also, must
  // do it above 'addCompilerSuppliedDecls', since the presence of
  // default args affects whether (e.g.) a copy ctor exists.
  {
    DefaultArgumentChecker checker(env);
    traverse(checker);
  }

  // default ctor, copy ctor, operator=; only do this for C++.
  if (env.lang.isCplusplus) {
    addCompilerSuppliedDecls(env, loc, ct);
  }

  // let the CompoundType build additional indexes if it wants
  ct->finishedClassDefinition(env.conversionOperatorName);

  // second pass: check function bodies
  // (only if we're not in a context where this is supressed)
  if (env.checkFunctionBodies) {
    tcheckFunctionBodies(env);
  }

  // now retract the class scope from the stack of scopes; do
  // *not* destroy it!
  env.retractScope(ct);

  if (containingClass) {
    // set the constructed scope's 'parentScope' pointer now that
    // we've removed 'ct' from the Environment scope stack; future
    // (unqualified) lookups in 'ct' will thus be able to see
    // into the containing class [cppstd 3.4.1 para 8]
    ct->parentScope = containingClass;
  }

  env.addedNewCompound(ct);
  
  // finish up the error filtering started above
  if (ct->isTemplate()) {
    // remove all messages that are not 'strong'
    env.errors.filter(strongMsgFilter);

    // now put back the saved messages
    env.errors.prependMessages(existingErrors);
  }
}


// This is pass 2 of tchecking a class.  It implements 3.3.6 para 1
// bullet 1, which specifies that the scope of a class member includes
// all function bodies.  That means that we have to wait until all
// the members have been added to the class scope before checking any
// of the function bodies.  Pass 1 does the former, pass 2 the latter.
void TS_classSpec::tcheckFunctionBodies(Env &env)
{ 
  xassert(env.checkFunctionBodies);

  CompoundType *ct = env.scope()->curCompound;
  xassert(ct);

  // inform the members that they are being checked on the second
  // pass through a class definition
  Restorer<bool> r(env.secondPassTcheck, true);
  
  // check function bodies and elaborate ctors and dtors of member
  // declarations
  FOREACH_ASTLIST_NC(Member, members->list, iter) {
    Member *member = iter.data();
    member->tcheck(env);
  }
}


Type *TS_enumSpec::itcheck(Env &env, DeclFlags dflags)
{
  env.setLoc(loc);

  EnumType *et = NULL;
  Type *ret = NULL;

  if (env.lang.allowIncompleteEnums && name) {
    // is this referring to an existing forward-declared enum?
    et = env.lookupEnum(name);
    if (et) {
      ret = env.makeType(loc, et);
      if (!et->valueIndex.isEmpty()) {
        // if it has values, it's definitely been defined already
        env.error(stringc << "mulitply defined enum `" << name << "'");
        return ret;      // ignore this defn
      }
    }
  }

  if (!et) {
    // declare the new enum
    et = new EnumType(name);
    ret = env.declareEnum(loc, et);
  }

  FAKELIST_FOREACH_NC(Enumerator, elts, iter) {
    iter->tcheck(env, et, ret);
  }

  this->etype = et;           // annotation
  return ret;
}


// BaseClass
// MemberList

// ---------------------- Member ----------------------
// cppstd 9.2 para 6:
//   "A member shall not be auto, extern, or register."
void checkMemberFlags(Env &env, DeclFlags flags)
{
  if (flags & (DF_AUTO | DF_EXTERN | DF_REGISTER)) {
    env.error("class members cannot be marked `auto', `extern', "
              "or `register'");
  }   
}

void MR_decl::tcheck(Env &env)
{
  env.setLoc(loc);

  if (env.secondPassTcheck) {
    // TS_classSpec is only thing of interest
    if (d->spec->isTS_classSpec()) {
      d->spec->asTS_classSpec()->tcheck(env, d->dflags);
    }
    return;
  }

  // the declaration knows to add its variables to
  // the curCompound
  d->tcheck(env, DC_MR_DECL);

  checkMemberFlags(env, d->dflags);
}

void MR_func::tcheck(Env &env)
{
  env.setLoc(loc);

  if (env.scope()->curCompound->keyword == CompoundType::K_UNION) {
    // TODO: is this even true?  
    // apparently not, as Mozilla has them; would like to find 
    // a definitive answer
    //env.error("unions cannot have member functions");
    //return;
  }

  // mark the function as inline, whether or not the
  // user explicitly did so
  f->dflags = (DeclFlags)(f->dflags | DF_INLINE);

  // we check the bodies in a second pass, after all the class
  // members have been added to the class, so that the potential
  // scope of all class members includes all function bodies
  // [cppstd sec. 3.3.6]
  //
  // the check-body suppression is now handled via a flag in 'env', so
  // this call site doesn't directly reflect that that is happening
  f->tcheck(env);

  checkMemberFlags(env, f->dflags);
}

void MR_access::tcheck(Env &env)
{
  if (env.secondPassTcheck) { return; }

  env.setLoc(loc);

  env.scope()->curAccess = k;
}

void MR_publish::tcheck(Env &env)
{
  if (env.secondPassTcheck) { return; }

  env.setLoc(loc);

  name->tcheck(env);

  if (!name->hasQualifiers()) {
    env.error(stringc
      << "in superclass publication, you have to specify the superclass");
  }
  else {
    // TODO: actually verify the superclass has such a member, and make
    // it visible in this class
  }
}

void MR_usingDecl::tcheck(Env &env)
{
  if (env.secondPassTcheck) { return; }

  env.setLoc(loc);
  decl->tcheck(env);
}

void MR_template::tcheck(Env &env)
{
  // maybe this will "just work"? crossing fingers..
  env.setLoc(loc);
  d->tcheck(env);
}


// -------------------- Enumerator --------------------
void Enumerator::tcheck(Env &env, EnumType *parentEnum, Type *parentType)
{
  var = env.makeVariable(loc, name, env.tfac.cloneType(parentType), DF_ENUMERATOR);

  enumValue = parentEnum->nextValue;
  if (expr) {
    expr->tcheck(env, expr);

    // will either set 'enumValue', or print (add) an error message
    expr->constEval(env, enumValue);
  }

  parentEnum->addValue(name, enumValue, var);
  parentEnum->nextValue = enumValue + 1;

  // cppstd sec. 3.3.1:
  //   "The point of declaration for an enumerator is immediately after
  //   its enumerator-definition. [Example:
  //     const int x = 12;
  //     { enum { x = x }; }
  //   Here, the enumerator x is initialized with the value of the
  //   constant x, namely 12. ]"
  if (!env.addVariable(var)) {
    env.error(stringc
      << "enumerator " << name << " conflicts with an existing variable "
      << "or typedef by the same name");
  }
}


// ----------------- declareNewVariable ---------------   
// This block of helpers, especially 'declareNewVariable', is the
// heart of the type checker, and the most complicated.

// This function is called whenever a constructed type is passed to a
// lower-down IDeclarator which *cannot* accept member function types.
// (sm 7/10/03: I'm now not sure exactly what that means...)
//
// sm 8/10/04: the issue is that someone has to call 'doneParams', but
// that can't be done in one central location, so this does it unless
// it has already been done, and is called in several places;
// 'dt.funcSyntax' is used as a flag to tell when it's happened
void possiblyConsumeFunctionType(Env &env, Declarator::Tcheck &dt, 
                                 bool reportErrors = true)
{
  if (dt.funcSyntax) {
    if (dt.funcSyntax->cv != CV_NONE && reportErrors) {
      env.error("cannot have const/volatile on nonmember functions");
    }
    dt.funcSyntax = NULL;

    // close the parameter list
    env.doneParams(dt.type->asFunctionType());
  }
}


// given a 'dt.type' that is a function type, and a 'dt.funcSyntax'
// that's carrying information about the function declarator syntax,
// and 'inClass' the class that the function will be considered a
// member of, attach a 'this' parameter to the function type, and
// close its parameter list
void makeMemberFunctionType(Env &env, Declarator::Tcheck &dt,
                            NamedAtomicType *inClassNAT, SourceLoc loc)
{
  // make the implicit 'this' parameter
  xassert(dt.funcSyntax);
  CVFlags thisCV = dt.funcSyntax->cv;
  Variable *receiver = env.receiverParameter(loc, inClassNAT, thisCV, dt.funcSyntax);

  // add it to the function type
  FunctionType *ft = dt.type->asFunctionType();
  ft->addReceiver(receiver);

  // close it
  dt.funcSyntax = NULL;
  env.doneParams(ft);
}


// Check some restrictions regarding the use of 'operator'; might
// add some errors to the environment, but otherwise doesn't
// change anything.  Parameters are same as declareNewVariable, plus
// 'scope', the scope into which the name will be inserted.
void checkOperatorOverload(Env &env, Declarator::Tcheck &dt,
                           SourceLoc loc, PQName const *name,
                           Scope *scope)
{
  if (!dt.type->isFunctionType()) {
    env.error(loc, "operators must be functions");
    return;
  }
  FunctionType *ft = dt.type->asFunctionType();

  // caller guarantees this will work
  OperatorName const *oname = name->getUnqualifiedNameC()->asPQ_operatorC()->o;
  char const *strname = oname->getOperatorName();

  if (scope->curCompound) {
    // All the operators mentioned in 13.5 must be non-static if they
    // are implemented by member functions.  (Actually, 13.5.7 does
    // not explicitly require non-static, but it's clearly intended.)
    //
    // That leaves only operators new and delete, which (12.5 para 1)
    // are always static *even if not explicitly declared so*.
    if (oname->isON_newDel()) {
      // actually, this is now done elsewhere (search for "12.5 para 1")
      //dt.dflags |= DF_STATIC;      // force it to be static
    }
    else if (dt.dflags & DF_STATIC) {
      env.error(loc, "operator member functions (other than new/delete) cannot be static");
    }
  }

  // describe the operator
  enum OperatorDesc {
    OD_NONE    = 0x00,
    NONMEMBER  = 0x01,      // can be a non-member function (anything can be a member function)
    ONEPARAM   = 0x02,      // can accept one parameter
    TWOPARAMS  = 0x04,      // can accept two parameters
    ANYPARAMS  = 0x08,      // can accept any number of parameters
    INCDEC     = 0x10,      // it's ++ or --
  };
  OperatorDesc desc = OD_NONE;

  ASTSWITCHC(OperatorName, oname) {
    ASTCASEC(ON_newDel, n)
      PRETEND_USED(n);
      // don't check anything.. I haven't done anything with these yet
      return;

    ASTNEXTC(ON_operator, o)
      static int/*OperatorDesc*/ const map[] = {
        // each group of similar operators is prefixed with a comment
        // that says which section of cppstd specifies the restrictions
        // that are enforced here

        // 13.5.1
        NONMEMBER | ONEPARAM,                       // OP_NOT
        NONMEMBER | ONEPARAM,                       // OP_BITNOT

        // 13.5.7
        NONMEMBER | ONEPARAM | TWOPARAMS | INCDEC,  // OP_PLUSPLUS
        NONMEMBER | ONEPARAM | TWOPARAMS | INCDEC,  // OP_MINUSMINUS

        // 13.5.1, 13.5.2
        NONMEMBER | ONEPARAM | TWOPARAMS,           // OP_PLUS
        NONMEMBER | ONEPARAM | TWOPARAMS,           // OP_MINUS
        NONMEMBER | ONEPARAM | TWOPARAMS,           // OP_STAR

        // 13.5.2
        NONMEMBER | TWOPARAMS,                      // OP_DIV
        NONMEMBER | TWOPARAMS,                      // OP_MOD
        NONMEMBER | TWOPARAMS,                      // OP_LSHIFT
        NONMEMBER | TWOPARAMS,                      // OP_RSHIFT
        NONMEMBER | TWOPARAMS | ONEPARAM/*13.5p2*/, // OP_BITAND
        NONMEMBER | TWOPARAMS,                      // OP_BITXOR
        NONMEMBER | TWOPARAMS,                      // OP_BITOR

        // 13.5.3
        TWOPARAMS,                                  // OP_ASSIGN
        
        // 13.5.2 (these are handled as ordinary binary operators (13.5 para 9))
        NONMEMBER | TWOPARAMS,                      // OP_PLUSEQ
        NONMEMBER | TWOPARAMS,                      // OP_MINUSEQ
        NONMEMBER | TWOPARAMS,                      // OP_MULTEQ
        NONMEMBER | TWOPARAMS,                      // OP_DIVEQ
        NONMEMBER | TWOPARAMS,                      // OP_MODEQ
        NONMEMBER | TWOPARAMS,                      // OP_LSHIFTEQ
        NONMEMBER | TWOPARAMS,                      // OP_RSHIFTEQ
        NONMEMBER | TWOPARAMS,                      // OP_BITANDEQ
        NONMEMBER | TWOPARAMS,                      // OP_BITXOREQ
        NONMEMBER | TWOPARAMS,                      // OP_BITOREQ

        // 13.5.2
        NONMEMBER | TWOPARAMS,                      // OP_EQUAL
        NONMEMBER | TWOPARAMS,                      // OP_NOTEQUAL
        NONMEMBER | TWOPARAMS,                      // OP_LESS
        NONMEMBER | TWOPARAMS,                      // OP_GREATER
        NONMEMBER | TWOPARAMS,                      // OP_LESSEQ
        NONMEMBER | TWOPARAMS,                      // OP_GREATEREQ

        // 13.5.2
        NONMEMBER | TWOPARAMS,                      // OP_AND
        NONMEMBER | TWOPARAMS,                      // OP_OR

        // 13.5.6
        ONEPARAM,                                   // OP_ARROW

        // 13.5.2
        NONMEMBER | TWOPARAMS,                      // OP_ARROW_STAR

        // 13.5.5
        TWOPARAMS,                                  // OP_BRACKETS

        // 13.5.4
        ANYPARAMS,                                  // OP_PARENS

        // 13.5.2
        NONMEMBER | TWOPARAMS,                      // OP_COMMA
        
        OD_NONE,                                    // OP_QUESTION (not used)
      };
      ASSERT_TABLESIZE(map, NUM_OVERLOADABLE_OPS);
      xassert(validCode(o->op));      
      
      // the table is declared int[] so that I can bitwise-OR
      // enumerated values without a cast; and overloading operator|
      // like I do elsewhere is nonportable b/c then an initializing
      // expression (which is supposed to be a literal) involves a
      // function call, at least naively...
      desc = (OperatorDesc)map[o->op];

      break;

    ASTNEXTC(ON_conversion, c)
      PRETEND_USED(c);
      desc = ONEPARAM;

    ASTENDCASECD
  }

  xassert(desc & (ONEPARAM | TWOPARAMS | ANYPARAMS));
            
  bool isMember = scope->curCompound != NULL;
  if (!isMember && !(desc & NONMEMBER)) {
    env.error(loc, stringc << strname << " must be a member function");
  }

  if (!(desc & ANYPARAMS)) {
    // count and check parameters
    int params = ft->params.count();     // includes implicit receiver
    bool okOneParam = desc & ONEPARAM;
    bool okTwoParams = desc & TWOPARAMS;

    if ((okOneParam && params==1) ||
        (okTwoParams && params==2)) {
      // ok
    }
    else {
      char const *howmany =
        okOneParam && okTwoParams? "one or two parameters" :
                      okTwoParams? "two parameters" :
                                   "one parameter" ;
      env.error(loc, stringc << strname << " must have " << howmany);
    }

    if ((desc & INCDEC) && (params==2)) {
      // second parameter must have type 'int'
      Type *t = ft->params.nth(1)->type;
      if (!t->isSimple(ST_INT) ||
          t->getCVFlags()!=CV_NONE) {
        env.error(loc, stringc
          << (isMember? "" : "second ")
          << "parameter of " << strname
          << " must have type `int', not `"
          << t->toString() << "', if it is present");
      }
    }

    // cannot have any default arguments
    SFOREACH_OBJLIST(Variable, ft->params, iter) {
      if (iter.data()->value != NULL) {
        env.error(loc, stringc << strname << " cannot have default arguments");
      }
    }
  }
}


// This function is perhaps the most complicated in this entire
// module.  It has the responsibility of adding a variable called
// 'name' to the environment.  But to do this it has to implement the
// various rules for when declarations conflict, overloading,
// qualified name lookup, etc.
//
// Update: I've now broken some of this mechanism apart and implemented
// the pieces in Env, so it's perhaps a bit less complicated now.
//
// 8/11/04: I renamed it from D_name_tcheck, to reflect that it is no
// longer done at the bottom of the IDeclarator chain, but instead is
// done right after processing the IDeclarator,
// Declarator::mid_tcheck.
static Variable *declareNewVariable(
  // environment in which to do general lookups
  Env &env,

  // contains various information about 'name', notably its type
  Declarator::Tcheck &dt,

  // true if we're a D_grouping is innermost to a D_pointer/D_reference
  bool inGrouping,

  // source location where 'name' appeared
  SourceLoc loc,

  // name being declared
  PQName const *name)
{
  // this is used to refer to a pre-existing declaration of the same
  // name; I moved it up top so my error subroutines can use it
  Variable *prior = NULL;

  // the unqualified part of 'name', mapped if necessary for
  // constructor names
  StringRef unqualifiedName = name? name->getName() : NULL;

  // false until I somehow call doneParams() for function types
  bool consumedFunction = false;

  // scope in which to insert the name, and to look for pre-existing
  // declarations
  Scope *scope = env.acceptingScope(dt.dflags);

  goto realStart;

  // This code has a number of places where very different logic paths
  // lead to the same conclusion.  So, I'm going to put the code for
  // these conclusions up here (like mini-subroutines), and 'goto'
  // them when appropriate.  I put them at the top instead of the
  // bottom since g++ doesn't like me to jump forward over variable
  // declarations.  They aren't put into real subroutines because they
  // want to access many of this function's parameters and locals, and
  // it'd be a hassle to pass them all each time.  In any case, they
  // would all be tail calls, since once I 'goto' somewhere I don't
  // come back.

  // an error has been reported, but for error recovery purposes,
  // return something reasonable
  makeDummyVar:
  {
    if (!consumedFunction) {
      possiblyConsumeFunctionType(env, dt);
    }

    // the purpose of this is to allow the caller to have a workable
    // object, so we can continue making progress diagnosing errors
    // in the program; this won't be entered in the environment, even
    // though the 'name' is not NULL
    Variable *ret = env.makeVariable(loc, unqualifiedName, dt.type, dt.dflags);

    // set up the variable's 'scope' field as if it were properly
    // entered into the scope; this is for error recovery, in particular
    // for going on to check the bodies of methods
    scope->registerVariable(ret);

    return ret;
  }

realStart:
  if (!name) {
    // no name, nothing to enter into environment
    possiblyConsumeFunctionType(env, dt);
    return env.makeVariable(loc, NULL, dt.type, dt.dflags);
  }

  // C linkage?
  if (env.linkageIs_C()) {
    dt.dflags |= DF_EXTERN_C;
  }

  // friend?
  bool isFriend = (dt.dflags & DF_FRIEND);
  if (isFriend) {
    // TODO: somehow remember the access control implications
    // of the friend declaration

    if (name->hasQualifiers()) {
      // we're befriending something that either is already declared,
      // or will be declared before it is used; no need to contemplate
      // adding a declaration, so just make the required Variable
      // and be done with it                                            
      //
      // TODO: can't befriend cv members, e.g. in/t0333.cc
      possiblyConsumeFunctionType(env, dt, false /*reportErrors*/);
      return env.makeVariable(loc, unqualifiedName, dt.type, dt.dflags);
    }
    else {
      // the main effect of 'friend' in my implementation is to
      // declare the variable in the innermost non-class, non-
      // template scope (this isn't perfect; see cppstd 11.4)
      scope = env.outerScope();

      // turn off the decl flag because it shouldn't end up
      // in the final Variable
      dt.dflags = dt.dflags & ~DF_FRIEND;
    }
  }

  // ambiguous grouped declarator in a paramter list?
  if (dt.hasFlag(DF_PARAMETER) && inGrouping) {
    // the name must *not* correspond to an existing type; this is
    // how I implement cppstd 8.2 para 7
    Variable *v = env.lookupPQVariable(name);
    if (v && v->hasFlag(DF_TYPEDEF)) {
      TRACE("disamb", "discarding grouped param declarator of type name");
      env.error(stringc
        << "`" << *name << "' is the name of a type, but was used as "
        << "a grouped parameter declarator; ambiguity resolution should "
        << "pick a different interpretation, so if the end user ever "
        << "sees this message then there's a bug in my typechecker",
        EF_DISAMBIGUATES);
      goto makeDummyVar;
    }
  }

  // member of an anonymous union that is not in an E_compoundLit ?
  if (scope->curCompound &&
      scope->curCompound->keyword == CompoundType::K_UNION &&
      scope->curCompound->name == NULL) {
    // we're declaring a field of an anonymous union, which actually
    // goes in the enclosing scope
    scope = env.enclosingScope();
  }

  // constructor?
  bool isConstructor = dt.type->isFunctionType() &&
                       dt.type->asFunctionTypeC()->isConstructor();
  if (isConstructor) {
    // if I just use the class name as the name of the constructor,
    // then that will hide the class's name as a type, which messes
    // everything up.  so, I'll kludge together another name for
    // constructors (one which the C++ programmer can't type) and
    // just make sure I always look up constructors under that name
    unqualifiedName = env.constructorSpecialName;
  }

  // are we in a class member list?  we can't be in a member
  // list if the name is qualified (and if it's qualified then
  // a class scope has been pushed, so we'd be fooled)
  //
  // TODO: this is wrong because qualified names *can* appear in
  // class member lists.. (according to gcc)
  //
  // 8/13/04: Actually, 8.3 para 1 says (in part):
  //   "A declarator-id shall not be qualified except for the
  //    definition of a member function [etc.] ... *outside* of
  //    its class, ..." (emphasis mine)
  // So maybe what I do is actually right?
  //
  // Essentially, what I'm doing is saying that if you have
  // qualifiers then you're a definition outside the class body,
  // otherwise you're inside it.  But of course that is not the
  // way to tell if you're outside a class body!  But the fix is
  // still not perfectly clear to me, so it remains a TODO.
  // (When I fix this, I may be able to remove the 'enclosingClass'
  // argument from 'createDeclaration'.)
  CompoundType *enclosingClass =
    name->hasQualifiers()? NULL : scope->curCompound;

  // if we're in the scope of a class at all then we're DF_MEMBER
  if (scope->curCompound && !isFriend) {
    dt.dflags |= DF_MEMBER;
  }

  // if we're not in a class member list, and the type is not a
  // function type, and 'extern' is not specified, then this is
  // a definition
  if (!enclosingClass &&
      !dt.type->isFunctionType() &&
      !(dt.dflags & DF_EXTERN)) {
    dt.dflags |= DF_DEFINITION;
  }

  // has this variable already been declared?
  //Variable *prior = NULL;    // moved to the top

  if (name->hasQualifiers()) {
    // TODO: I think this is wrong, but I'm not sure how.  For one
    // thing, it's very similar to what happens below for unqualified
    // names; could those be unified?  Second, the thing above about
    // how class member declarations can be qualified, but I don't
    // allow it ...

    // the name has qualifiers, which means it *must* be declared
    // somewhere; now, Declarator::tcheck will have already pushed the
    // qualified scope, so we just look up the name in the now-current
    // environment, which will include that scope
    prior = scope->lookupVariable(unqualifiedName, env, LF_INNER_ONLY);
    if (!prior) {
      env.error(stringc
        << "undeclared identifier `" << *name << "'");
      goto makeDummyVar;
    }

    // ok, so we found a prior declaration; but if it's a member of
    // an overload set, then we need to pick the right one now for
    // several reasons:
    //   - the DF_DEFINITION flag is per-member, not per-set
    //   - below we'll be checking for type equality again
    if (prior->overload) {
      // only functions can be overloaded
      if (!dt.type->isFunctionType()) {
        env.error(dt.type, stringc
          << "the name `" << *name << "' is overloaded, but the type `"
          << dt.type->toString() << "' isn't even a function; it must "
          << "be a function and match one of the overloadings");
        goto makeDummyVar;
      }
      FunctionType *dtft = dt.type->asFunctionType();

      // 'dtft' is incomplete for the moment, because we don't know
      // yet whether it's supposed to be a static member or a
      // nonstatic member; this is determined by finding a function
      // whose signature (ignoring 'this' parameter, if any) matches
      int howMany = prior->overload->set.count();
      prior = env.findInOverloadSet(prior->overload, dtft, dt.funcSyntax->cv);
      if (!prior) {
        env.error(stringc
          << "the name `" << *name << "' is overloaded, but the type `"
          << dtft->toString_withCV(dt.funcSyntax->cv) 
          << "' doesn't match any of the "
          << howMany << " declared overloaded instances",
          EF_STRONG);
        goto makeDummyVar;
      }
    }

    if (prior->hasFlag(DF_MEMBER)) {
      // this intends to be the definition of a class member; make sure
      // the code doesn't try to define a nonstatic data member
      if (!prior->type->isFunctionType() &&
          !prior->hasFlag(DF_STATIC)) {
        env.error(stringc
          << "cannot define nonstatic data member `" << *name << "'");
        goto makeDummyVar;
      }
    }
  }
  else {
    // has this name already been declared in the innermost scope?
    prior = env.lookupVariableForDeclaration(scope, unqualifiedName, dt.type,
      dt.funcSyntax? dt.funcSyntax->cv : CV_NONE);
  }

  if (scope->curCompound &&
      !isFriend &&
      name->getUnqualifiedNameC()->isPQ_operator() &&
      name->getUnqualifiedNameC()->asPQ_operatorC()->o->isON_newDel()) {
    // 12.5 para 1: new/delete member functions are static even if
    // they are not explicitly declared so
    dt.dflags |= DF_STATIC;
  }

  // is this a nonstatic member function?
  //
  // TODO: This can't be right in the presence of overloaded functions,
  // since we're just testing the static-ness of the first element of
  // the overload set!
  if (dt.type->isFunctionType()) {
    if (scope->curCompound &&
        !isFriend &&
        !isConstructor &&               // ctors don't have a 'this' param
        !(dt.dflags & DF_STATIC) &&
        (!name->hasQualifiers() ||
         !prior->type->isFunctionType() ||
         prior->type->asFunctionTypeC()->isMethod())) {
      TRACE("memberFunc", "nonstatic member function: " << *name);

      // add the implicit 'this' parameter
      makeMemberFunctionType(env, dt, scope->curCompound, loc);
    }
    else {
      TRACE("memberFunc", "static or non-member function: " << *name);
      possiblyConsumeFunctionType(env, dt);
    }
  }
  consumedFunction = true;

  // check restrictions on operator overloading
  if (name->getUnqualifiedNameC()->isPQ_operator()) {
    checkOperatorOverload(env, dt, loc, name, scope);
  }

  // check for overloading
  OverloadSet *overloadSet =
    name->hasQualifiers() ? NULL /* I don't think this is right! */ :
    env.getOverloadForDeclaration(prior, dt.type);

  // 8/11/04: Big block of template code obviated by
  // Declarator::mid_tcheck.

  // make a new variable; see implementation for details
  return env.createDeclaration(loc, unqualifiedName, dt.type, dt.dflags,
                               scope, enclosingClass, prior, overloadSet);
}


// -------------------- Declarator --------------------
Declarator *Declarator::tcheck(Env &env, Tcheck &dt)
{
  if (!ambiguity) {
    mid_tcheck(env, dt);
    return this;
  }

  // As best as I can tell from the standard, cppstd sections 6.8 and
  // 8.2, we always prefer a Declarator interpretation which has no
  // initializer (if that's valid) to one that does.  I'm not
  // completely sure because, ironically, the English text there ("the
  // resolution is to consider any construct that could possibly be a
  // declaration a declaration") is ambiguous in my opinion.  See the
  // examples of ambiguous syntax in cc.gr, nonterminal
  // InitDeclarator.
  if (this->init == NULL &&
      ambiguity->init != NULL &&
      ambiguity->ambiguity == NULL) {
    // already in priority order
    return resolveAmbiguity(this, env, "Declarator", true /*priority*/, dt);
  }
  else if (this->init != NULL &&
           ambiguity->init == NULL &&
           ambiguity->ambiguity == NULL) {
    // reverse priority order; swap them
    Declarator *withInit = this;
    Declarator *noInit = ambiguity;

    noInit->ambiguity = withInit;    // 'noInit' is first
    withInit->ambiguity = NULL;      // 'withInit' is second

    // run with priority
    return resolveAmbiguity(noInit, env, "Declarator", true /*priority*/, dt);
  }
  else {
    // if both have an initialzer or both lack an initializer, then
    // we'll resolve without ambiguity; otherwise we'll probably fail
    // to resolve, which will be reported as such
    return resolveAmbiguity(this, env, "Declarator", false /*priority*/, dt);
  }
}


// array initializer case
//   static int y[] = {1, 2, 3};
// or in this case (a gnu extention):
// http://gcc.gnu.org/onlinedocs/gcc-3.3/gcc/Compound-Literals.html#Compound%20Literals
//   static int y[] = (int []) {1, 2, 3};
// which is equivalent to:
//   static int y[] = {1, 2, 3};
Type *Env::computeArraySizeFromCompoundInit(SourceLoc tgt_loc, Type *tgt_type,
                                            Type *src_type, Initializer *init)
{
  // If we start at a reference, we have to go down to the raw
  // ArrayType and then back up to a reference.
  bool tgt_type_isRef = tgt_type->isReferenceType();
  tgt_type = tgt_type->asRval();
  if (tgt_type->isArrayType() &&
      init->isIN_compound()) {
    ArrayType *at = tgt_type->asArrayType();
    IN_compound const *cpd = init->asIN_compoundC();
                   
    // count the initializers; this is done via the environment
    // so the designated-initializer extension can intercept
    int initLen = countInitializers(loc(), src_type, cpd);

    if (!at->hasSize()) {
      // replace the computed type with another that has
      // the size specified; the location isn't perfect, but
      // getting the right one is a bit of work
      tgt_type = tfac.setArraySize(tgt_loc, at, initLen);
    }
    else {
      // TODO: cppstd wants me to check that there aren't more
      // initializers than the array's specified size, but I
      // don't want to do that check since I might have an error
      // in my const-eval logic which could break a Mozilla parse
      // if my count is short
    }
  }
  return tgt_type_isRef ? makeReferenceType(SL_UNKNOWN, tgt_type): tgt_type;
}

// provide a well-defined size for the array from the size of the
// initializer, such as in this case:
//   char sName[] = "SOAPPropertyBag";
Type *computeArraySizeFromLiteral(Env &env, Type *tgt_type, Initializer *init)
{
  // If we start at a reference, we have to go down to the raw
  // ArrayType and then back up to a reference.
  bool tgt_type_isRef = tgt_type->isReferenceType();
  tgt_type = tgt_type->asRval();
  if (tgt_type->isArrayType() &&
      !tgt_type->asArrayType()->hasSize() &&
      init->isIN_expr() &&
      init->asIN_expr()->e->type->asRval()->isArrayType() &&
      init->asIN_expr()->e->type->asRval()->asArrayType()->hasSize()
      ) {
    tgt_type = env.tfac.cloneType(tgt_type);
    tgt_type->asArrayType()->size =
      init->asIN_expr()->e->type->asRval()->asArrayType()->size;
    xassert(tgt_type->asArrayType()->hasSize());
  }
  return tgt_type_isRef ? env.makeReferenceType(SL_UNKNOWN, tgt_type): tgt_type;
}

// true if the declarator corresponds to a local/global variable declaration
bool isVariableDC(DeclaratorContext dc)
{
  return dc==DC_TF_DECL ||    // global
         dc==DC_S_DECL ||     // local
         dc==DC_CN_DECL;      // local in a Condition
}

// determine if a complete type is required, and if so, check that it is
void checkCompleteTypeRules(Env &env, Declarator::Tcheck &dt, Initializer *init)
{
  // TODO: According to 15.4 para 1, not only must the type in
  // DC_EXCEPTIONSPEC be complete (which this code enforces), but if
  // it is a pointer or reference type, the pointed-to thing must be
  // complete too!

  if (dt.context == DC_D_FUNC) {
    // 8.3.5 para 6: ok to be incomplete unless this is a definition;
    // I'll just allow it (here) in all cases (t0048.cc)
    return;
  }

  if (dt.context == DC_TA_TYPE) {
    // mere appearance of a type in an argument list is not enough to
    // require that it be complete; maybe the function definition will
    // need it, but that is detected later
    return;
  }

  if (dt.dflags & (DF_TYPEDEF | DF_EXTERN)) {
    // 7.1.3 does not say that the type named by a typedef must be
    // complete, so I will allow it to be incomplete (t0079.cc)
    //
    // I'm not sure where the exception for 'extern' is specified, but
    // it clearly exists.... (t0170.cc)
    return;
  }        

  if (dt.context == DC_MR_DECL &&
      (dt.dflags & DF_STATIC)) {
    // static members don't have to be complete types
    return;
  }

  if (dt.type->isArrayType()) {
    if (init) {
      // The array type might be incomplete now, but the initializer
      // will take care of it.  (If I instead moved this entire block
      // below where the init is tchecked, I think I would run into
      // problems when tchecking the initializer wants a ctor to exist.)
      // (t0077.cc)
      return;
    }
                                       
    if (dt.context == DC_MR_DECL &&
        !env.lang.strictArraySizeRequirements) {
      // Allow incomplete array types, so-called "open arrays".
      // Usually, such things only go at the *end* of a structure, but
      // we do not check that.
      return;
    }

    #ifdef GNU_EXTENSION
    if (dt.context == DC_E_COMPOUNDLIT) {
      // dsw: arrays in ASTTypeId's of compound literals are
      // allowed to not have a size and not have an initializer:
      // (gnu/g0014.cc)
      return;
    }
    #endif // GNU_EXTENSION
  }

  // ok, we're not in an exceptional circumstance, so the type
  // must be complete; if we have an error, what will we say?
  char const *action =
    dt.context==DC_EXCEPTIONSPEC? "name in exception spec" :
    dt.context==DC_E_CAST?        "cast to" :
           /* catch-all */        "create an object of" ;

  // check it
  if (!env.ensureCompleteType(action, dt.type)) {
    dt.type = env.errorType();        // recovery
  }
}

void Declarator::mid_tcheck(Env &env, Tcheck &dt)
{
  // true if we're immediately in a class body
  Scope *enclosingScope = env.scope();
  bool inClassBody = !!(enclosingScope->curCompound);
           
  // is this declarator in a templatizable context?  this prevents 
  // confusion when processing the template arguments themselves (which
  // contain declarators), among other things
  bool templatizableContext = 
    dt.context == DC_FUNCTION ||   // could be in MR_func or TD_func
    dt.context == DC_TD_DECL ||
    dt.context == DC_MR_DECL;

  // cppstd sec. 3.4.3 para 3:
  //    "In a declaration in which the declarator-id is a
  //    qualified-id, names used before the qualified-id
  //    being declared are looked up in the defining
  //    namespace scope; names following the qualified-id
  //    are looked up in the scope of the member's class
  //    or namespace."
  //
  // to implement this, I'll find the declarator's qualified
  // scope ahead of time and add it to the scope stack
  //
  // 4/21/04: in fact, I need the entire sequence of scopes
  // in the qualifier, since all of them need to be visible
  //
  // dsw points out that this furthermore requires that the underlying
  // PQName be tchecked, so we can use the template arguments (if
  // any).  Hence, down in D_name::tcheck, we no longer tcheck the
  // name since it's now always done out here.
  ScopeSeq qualifierScopes;
  PQName *name = decl->getDeclaratorId();
  if (name) {
    name->tcheck(env, NULL /*scope*/, LF_DECLARATOR);
  }
  env.getQualifierScopes(qualifierScopes, name);
  env.extendScopeSeq(qualifierScopes);

  if (init) dt.dflags |= DF_INITIALIZED;

  // get the type from the IDeclarator
  decl->tcheck(env, dt);

  // declarators usually require complete types
  checkCompleteTypeRules(env, dt, init);

  // this this a specialization?
  if (templatizableContext &&                      // e.g. toplevel
      enclosingScope->isTemplateParamScope() &&    // "template <...>" above
      !enclosingScope->parameterizedEntity) {      // that's mine, not my class' (need to wait until after name->tcheck to test this)
    if (dt.type->isFunctionType()) {
      // complete specialization?
      if (enclosingScope->templateParams.isEmpty()) {    // just "template <>"
        xassert(!dt.existingVar);
        dt.existingVar = env.makeExplicitFunctionSpecialization
                           (decl->loc, dt.dflags, name, dt.type->asFunctionType());
        if (dt.existingVar) {
          enclosingScope->setParameterizedEntity(dt.existingVar);
        }
      }

      else {
        // either a partial specialization, or a primary; since the
        // former doesn't exist for functions, there had better not
        // be any template arguments on the function name
        if (name->getUnqualifiedName()->isPQ_template()) {
          env.error("function template partial specialization is not allowed",
                    EF_STRONG);
        }
      }
    }
    else {
      // for class specializations, we should not get here, as the syntax
      //
      //   template <>
      //   class A<int> { ... }  /*declarator goes here*/  ;
      //
      // does not have (and cannot have) any declarators
      env.error("can only specialize functions", EF_STRONG);
    }
  }

  // explicit instantiation?
  if (dt.context == DC_TF_EXPLICITINST) {
    dt.existingVar = env.explicitFunctionInstantiation(name, dt.type);
  }

  bool callerPassedInstV = false;
  if (!dt.existingVar) {
    // make a Variable, add it to the environment
    var = env.storeVar(
      declareNewVariable(env, dt, decl->hasInnerGrouping(), decl->loc, name));
  }
  else {
    // caller already gave me a Variable to use
    var = dt.existingVar;
    callerPassedInstV = true;

    // declareNewVariable is normally responsible for adding the receiver
    // param to 'dt.type', but since I skipped it, I have to do it
    // here too
    if (var->type->isFunctionType() &&
        var->type->asFunctionType()->isMethod()) {
      TRACE("memberFunc", "nonstatic member function: " << var->name);

      // add the implicit 'this' parameter
      makeMemberFunctionType(env, dt,
        var->type->asFunctionType()->getNATOfMember(), decl->loc);
    }
    else {
      TRACE("memberFunc", "static or non-member function: " << var->name);
      possiblyConsumeFunctionType(env, dt);
    }
  }

  if (!var) {
    env.retractScopeSeq(qualifierScopes);
    return;      // an error was found and reported; bail rather than die later
  }
  type = env.tfac.cloneType(dt.type);
  context = dt.context;

  // handle function template declarations ....
  TemplateInfo *templateInfo = NULL;
  if (callerPassedInstV) {
    // don't try to take it; dt.var probably already has it, etc.
  }
  else if (templatizableContext) {
    if (dt.type->isFunctionType()) {
      templateInfo = env.takeFTemplateInfo();
    }
    else {
      // non-templatizable entity
      //
      // TODO: I should allow static members of template classes
      // to be templatizable too
    }
  }
  else {
    // other contexts: don't try to take it, you're not a
    // templatizable declarator
  }

  if (templateInfo) {
    TRACE("template", "template func " << 
                      ((dt.dflags & DF_DEFINITION) ? "defn" : "decl")
                      << ": " << dt.type->toCString(var->fullyQualifiedName()));

    if (!var->templateInfo()) {
      // this is a new template decl; attach it to the Variable
      var->setTemplateInfo(templateInfo);

      // (what was I about to do here?)

    }
    else {
      // TODO: merge default arguments

      if (dt.dflags & DF_DEFINITION) {
        // save this templateInfo for use with the definition  
        //
        // TODO: this really should just be TemplateParams, not
        // a full TemplateInfo ...
        var->templateInfo()->definitionTemplateInfo = templateInfo;
      }
      else {
        // discard this templateInfo
        delete templateInfo;
      }
    }

    // no such thing as a function partial specialization, so this
    // is automatically the primary
    if (enclosingScope->isTemplateParamScope() &&
        !enclosingScope->parameterizedEntity) {
      enclosingScope->setParameterizedEntity(var);
    }

    // sm: I'm not sure what this is doing.  It used to only be done when
    // 'var' had no templateInfo to begin with.  Now I'm doing it always,
    // which might not be right.
    if (getDeclaratorId() &&
        getDeclaratorId()->isPQ_template()) {
      env.initArgumentsFromASTTemplArgs(var->templateInfo(),
                                        getDeclaratorId()->asPQ_templateC()->args);
    }
  }
  
  // cppstd, sec. 3.3.1:
  //   "The point of declaration for a name is immediately after
  //   its complete declarator (clause 8) and before its initializer
  //   (if any), except as noted below."
  // (where "below" talks about enumerators, class members, and
  // class names)
  //
  // However, since the bottom of the recursion for IDeclarators
  // is always D_name, it's equivalent to add the name to the
  // environment then instead of here.

  // want only declarators corresp. to local/global variables
  // (it is disturbing that I have to check for so many things...)
  if (env.lang.isCplusplus &&
      !dt.hasFlag(DF_EXTERN) &&                 // not an extern decl
      !dt.hasFlag(DF_TYPEDEF) &&                // not a typedef
      isVariableDC(dt.context) &&               // local/global variable
      var->type->isCompoundType()) {            // class-valued
    if (!init) {
      // cppstd 8.5 paras 7,8,9: treat
      //   C c;
      // like
      //   C c();
      // except that the latter is not actually allowed since it would
      // be interpreted as a declaration of a function
      init = new IN_ctor(decl->loc, NULL /*args*/);
    }
    else if (init->isIN_expr()) {
      // cppstd reference? treat
      //   C c = 5;
      // like
      //   C c(5);
      // except the latter isn't always syntactically allowed (e.g. CN_decl)

      // take out the IN_expr
      IN_expr *inexpr = init->asIN_expr();
      Expression *e = inexpr->e;
      inexpr->e = NULL;
      delete inexpr;

      // put in an IN_ctor
      init = new IN_ctor(decl->loc, makeExprList1(e));
    }
  }

  // tcheck the initializer, unless we're inside a class, in which
  // case wait for pass two
  //
  // 8/06/04: dsw: why wait until pass 2?  I need to change it to pass
  // 1 to get in/d0088.cc to work and all the other elsa and oink
  // tests also work
  //
  // sm: You're right; I had thought 3.3.6 said that member scopes
  // included *all* initializers, but it does not, so such scopes only
  // include *subsequent* initializers, hence pass 1 is the right time.
  //
  // 2005-02-17:  I think I Now I understand.  3.3.6 para 1 bullet 1
  // says that *default arguments* must be tchecked in pass 2, and
  // that may have been the original intent.  I am changing it so
  // default arguments are skipped here (checked by
  // DefaultArgumentChecker); *member initializers* will continue to
  // be tcheck'd in pass 1.  Testcase: in/t0369.cc.
  if (dt.context == DC_D_FUNC &&
      !env.checkFunctionBodies /*pass 1*/) {
    // skip it
  }
  else if (init) {
    // TODO: check the initializer for compatibility with
    // the declared type

    // TODO: check compatibility with dflags; e.g. we can't allow
    // an initializer for a global variable declared with 'extern'

    tcheck_init(env);
  }
            
  // pull the scope back out of the stack; if this is a
  // declarator attached to a function definition, then
  // Function::tcheck will re-extend it for analyzing
  // the function body
  env.retractScopeSeq(qualifierScopes);

  // If it is a function, is it virtual?
  if (inClassBody
      && var->type->isMethod()
      && !var->hasFlag(DF_VIRTUAL)) {
    FunctionType *varft = var->type->asFunctionType();

//      printf("var->name: %s\n", var->name);
//      printf("env.scope->curCompound->name: %s\n", env.scope()->curCompound->name);

    // find the next variable up the hierarchy
    FOREACH_OBJLIST(BaseClass, env.scope()->curCompound->bases, base_iter) {
      // FIX: Should I skip it for private inheritance?  Hmm,
      // experiments on g++ indicate that private inheritance does not
      // prevent the virtuality from coming through.  Ben agrees.

      // FIX: deal with ambiguity if we find more than one
      // FIX: is there something to do for virtual inheritance?
      //        printf("iterating over base base_iter.data()->ct->name: %s\n",
      //               base_iter.data()->ct->name);
      Variable *var2 = base_iter.data()->ct->lookupVariable(var->name, env);
      xassert(var2 != var);
      if (var2 &&
          var2->type->isMethod()) {
        FunctionType *var2ft = var2->type->asFunctionType();

        // one could write this without the case split, but I don't want
        // to call getOverloadSet() (which makes new OverloadSet objects
        // if they are not already present) when I don't need to

        if (!var2->overload) {
          // see if it's virtual and has the same signature
          if (var2->hasFlag(DF_VIRTUAL) &&
              var2ft->equalOmittingReceiver(varft) &&
              var2ft->getReceiverCV() == varft->getReceiverCV()) {
            var->setFlag(DF_VIRTUAL);
          }
        }

        else {
          // check for member of overload set with same signature
          // and marked 'virtual'
          if (Variable *var_overload = env.findInOverloadSet(
                var2->overload, var->type->asFunctionType())) {
            xassert(var_overload != var);
            xassert(var_overload->type->isFunctionType());
            xassert(var_overload->type->asFunctionType()->isMethod());
            if (var_overload->hasFlag(DF_VIRTUAL)) {
              // then we inherit the virtuality
              var->setFlag(DF_VIRTUAL);
              break;
            }
          }
        }
      }
    }
  }
}


// pulled out so it could be done in pass 1 or pass 2
void Declarator::tcheck_init(Env &env)
{
  xassert(init);

  // record that we are in an initializer
  Restorer<Env::TemplTcheckMode> changeMode(env.tcheckMode,
    env.tcheckMode==Env::TTM_1NORMAL ? env.tcheckMode : Env::TTM_3TEMPL_DEF);

  init->tcheck(env, type);

  // TODO: check the initializer for compatibility with
  // the declared type

  // TODO: check compatibility with dflags; e.g. we can't allow
  // an initializer for a global variable declared with 'extern'

  // remember the initializing value, for const values
  if (init->isIN_expr()) {
    var->value = init->asIN_exprC()->e;
  }

  // use the initializer size to refine array types
  // array initializer case
  var->type = env.computeArraySizeFromCompoundInit(var->loc, var->type, type, init);
  // array compound literal initializer case
  var->type = computeArraySizeFromLiteral(env, var->type, init);

  // update 'type' if necessary
  if (type->asRval()->isArrayType()) {
    type->asRval()->asArrayType()->size = var->type->asRval()->asArrayType()->size;
  }
}


// ------------------ IDeclarator ------------------
void D_name::tcheck(Env &env, Declarator::Tcheck &dt)
{
  env.setLoc(loc);

  // 7/27/04: This has been disabled because Declarator::mid_tcheck
  // takes care of tchecking the name in advance.
  //if (name) {
  //  name->tcheck(env);
  //}

  // do *not* call 'possiblyConsumeFunctionType', since declareNewVariable
  // will do so if necessary, and in a different way
}


// cppstd, 8.3.2 para 4:
//   "There shall be no references to references, no arrays of
//    references, and no pointers to references."

void D_pointer::tcheck(Env &env, Declarator::Tcheck &dt)
{
  env.setLoc(loc);
  possiblyConsumeFunctionType(env, dt);

  if (dt.type->isReference()) {
    env.error("cannot create a pointer to a reference");
  }
  else {
    // apply the pointer type constructor
    if (!dt.type->isError()) {
      dt.type = env.tfac.syntaxPointerType(loc, cv, dt.type, this);
    }
  }

  // recurse
  base->tcheck(env, dt);
}

// almost identical to D_pointer ....
void D_reference::tcheck(Env &env, Declarator::Tcheck &dt)
{
  env.setLoc(loc);
  possiblyConsumeFunctionType(env, dt);

  if (dt.type->isReference()) {
    env.error("cannot create a reference to a reference");
  }
  else {
    // apply the reference type constructor
    if (!dt.type->isError()) {
      dt.type = env.tfac.syntaxReferenceType(loc, dt.type, this);
    }
  }

  // recurse
  base->tcheck(env, dt);
}


// this code adapted from (the old) tcheckFakeExprList; always passes NULL
// for the 'sizeExpr' argument to ASTTypeId::tcheck
FakeList<ASTTypeId> *tcheckFakeASTTypeIdList(
  FakeList<ASTTypeId> *list, Env &env, DeclFlags dflags, DeclaratorContext context)
{
  if (!list) {
    return list;
  }

  // context for checking (ok to share these across multiple ASTTypeIds)
  ASTTypeId::Tcheck tc(dflags, context);

  // check first ASTTypeId
  FakeList<ASTTypeId> *ret
    = FakeList<ASTTypeId>::makeList(list->first()->tcheck(env, tc));

  // check subsequent expressions, using a pointer that always
  // points to the node just before the one we're checking
  ASTTypeId *prev = ret->first();
  while (prev->next) {
    prev->next = prev->next->tcheck(env, tc);

    prev = prev->next;
  }

  return ret;
}

// implement cppstd 8.3.5 para 3:
//   "array of T" -> "pointer to T"
//   "function returning T" -> "pointer to function returning T"
// also, since f(int) and f(int const) are the same function (not
// overloadings), strip toplevel cv qualifiers
static Type *normalizeParameterType(Env &env, SourceLoc loc, Type *t)
{
  if (t->isArrayType()) {
    return env.makePtrType(loc, t->asArrayType()->eltType);
  }
  if (t->isFunctionType()) {
    return env.makePtrType(loc, t);
  }
  return t;
}


void D_func::tcheck(Env &env, Declarator::Tcheck &dt)
{
  // record that we are in a function declaration
  Restorer<Env::TemplTcheckMode> changeMode(env.tcheckMode,
    env.tcheckMode==Env::TTM_1NORMAL ? env.tcheckMode : Env::TTM_2TEMPL_FUNC_DECL);

  env.setLoc(loc);
  possiblyConsumeFunctionType(env, dt);

  FunctionFlags specialFunc = FF_NONE;

  // handle "fake" return type ST_CDTOR
  if (dt.type->isSimple(ST_CDTOR)) {
    if (env.lang.isCplusplus) {
      // get the name being declared
      D_name *dname;
      PQName *name;
      {
        // get the D_name one level down (skip D_groupings)
        IDeclarator *idecl = base;
        while (idecl->isD_grouping()) {
          idecl = idecl->asD_grouping()->base;
        }
        xassert(idecl->isD_name());    // grammar should ensure this
        dname = idecl->asD_name();

        // skip qualifiers
        name = dname->name->getUnqualifiedName();
      }

      // conversion operator
      if (name->isPQ_operator()) {
        if (name->asPQ_operator()->o->isON_conversion()) {
          ON_conversion *conv = name->asPQ_operator()->o->asON_conversion();

          // compute the named type; this becomes the return type
          ASTTypeId::Tcheck tc(DF_NONE, DC_ON_CONVERSION);
          conv->type = conv->type->tcheck(env, tc);
          dt.type = conv->type->getType();
          specialFunc = FF_CONVERSION;
        }
        else {
          if (!env.lang.allowImplicitIntForOperators) {
            env.error(stringc << "cannot declare `" << name->toString() << "' with no return type");
          }
          dt.type = env.getSimpleType(SL_UNKNOWN, ST_INT);     // recovery
        }
      }

      // constructor or destructor
      else {
        StringRef nameString = name->asPQ_name()->name;
        CompoundType *inClass = env.acceptingScope()->curCompound;

        // destructor
        if (nameString[0] == '~') {
          if (!inClass) {
            env.error("destructors must be class members");
          }
          else if (0!=strcmp(nameString+1, inClass->name)) {
            env.error(stringc
              << "destructor name `" << nameString
              << "' must match the class name `" << inClass->name << "'");
          }

          // return type is 'void'
          dt.type = env.getSimpleType(dname->loc, ST_VOID);
          specialFunc = FF_DTOR;
        }

        // constructor
        else {
          if (!inClass) {
            if (!env.lang.allowImplicitInt &&
                env.lang.allowImplicitIntForMain &&
                nameString == env.str("main")) {
              // example: g0018.cc
              env.warning("obsolete use of implicit int in declaration of main()");

              // change type to 'int'
              dt.type = env.getSimpleType(loc, ST_INT);
            }
            else {
              env.error("constructors must be class members (and implicit int is not supported)");
              return;    // otherwise would segfault below..
            }
          }
          else {
            if (nameString != inClass->name) {
              // I'm not sure if this can occur...
              env.error(stringc
                << "constructor name `" << nameString
                << "' must match the class name `" << inClass->name << "'");
            }

            // return type is same as class type
            dt.type = env.makeType(dname->loc, inClass);
            specialFunc = FF_CTOR;
          }
        }
      }
    }
    
    else {     // C
      if (env.lang.allowImplicitInt) {
        // surely this is not adequate, as implicit-int applies to
        // all declarations, not just those that appear in function
        // definitions... I think the rest of the implementation is
        // in Oink?
        dt.type = env.getSimpleType(loc, ST_INT);
      }
      else {
        env.error("support for omitted return type is currently off");
      }
    }
  }

  // make a new scope for the parameter list
  Scope *paramScope = env.enterScope(SK_PARAMETER, "D_func parameter list scope");
//    cout << "D_func::tcheck env.locStr() " << env.locStr() << endl;
//    if (templateInfo) templateInfo->debugPrint();
//    env.gdbScopes();

  // typecheck the parameters; this disambiguates any ambiguous type-ids,
  // and adds them to the environment
  params = tcheckFakeASTTypeIdList(params, env, DF_PARAMETER, DC_D_FUNC);
//    if (params->first()) {
//      cout << "immediately after typechecking: params->first()->gdb()" << endl;
//      params->first()->gdb();
//    }

  // build the function type; I do this after type checking the parameters
  // because it's convenient if 'syntaxFunctionType' can use the results
  // of checking them
  FunctionType *ft = env.tfac.syntaxFunctionType(loc, dt.type, this, env.tunit);
  ft->flags = specialFunc;
  dt.funcSyntax = this;
  // moved below where dt.var exists
//    ft->templateInfo = templateInfo;

  // add them, now that the list has been disambiguated
  int ct=0;
  FAKELIST_FOREACH_NC(ASTTypeId, params, iter) {
    ct++;
    Variable *v = iter->decl->var;

    if (v->type->isSimple(ST_VOID)) {
      if (ct == 1 &&
          !iter->next &&
          !v->name &&
          !iter->decl->init) {
        // special case: only parameter is "void" and it doesn't have a name:
        // same as empty param list
        break;
      }
      env.error("cannot have parameter of type `void', unless it is "
                "the only parameter, has no parameter name, and has "
                "no default value");
      continue;
    }

    if (v->type->isSimple(ST_ELLIPSIS)) {
      // no need for as careful checking as for ST_VOID, since the
      // grammar ensures it's last if it appears at all
      ft->flags |= FF_VARARGS;
      break;
    }

    v->type = normalizeParameterType(env, loc, v->type);

    // get the default argument, if any
    if (iter->decl->init) {
      Initializer *i = iter->decl->init;
      if (!i->isIN_expr()) {
        env.error("function parameter default value must be a simple initializer, "
                  "not a compound (e.g. \"= { ... }\") or constructor "
                  "(e.g. \"int x(3)\") initializer");
      }
      else {
        // this is obsolete, now that Variable has a 'value' field
        //Expression *e = i->asIN_expr()->e;
        //p->defaultArgument = new DefaultArgument(e, e->exprToString());
      }
    }

    // dsw: You didn't implement adding DF_PARAMETER to variables that
    // are parameters; This seems to be the best place to put it.
    v->setFlag(DF_PARAMETER);
    ft->addParam(v);
  }

  // dsw: in K&R C, an empty parameter list means that the number of
  // arguments is not specified
  if (env.lang.emptyParamsMeansNoInfo && params->isEmpty()) {
    ft->flags |= FF_NO_PARAM_INFO;
  }

  if (specialFunc == FF_CONVERSION) {
    if (ft->params.isNotEmpty() || ft->acceptsVarargs()) {
      env.error("conversion operator cannot accept arguments");
    }
  }

  // the verifier will type-check the pre/post at this point
  env.checkFuncAnnotations(ft, this);

  env.exitScope(paramScope);

  if (exnSpec) {
    ft->exnSpec = exnSpec->tcheck(env);
  }

  // call this after attaching the exception spec, if any
  //env.doneParams(ft);
  // update: doneParams() is done by 'possiblyConsumeFunctionType'
  // or 'declareNewVariable', depending on what declarator is next in
  // the chain

  // now that we've constructed this function type, pass it as
  // the 'base' on to the next-lower declarator
  dt.type = ft;

  base->tcheck(env, dt);
}


void D_array::tcheck(Env &env, Declarator::Tcheck &dt)
{
  env.setLoc(loc);
  possiblyConsumeFunctionType(env, dt);

  // check restrictions in cppstd 8.3.4 para 1
  if (dt.type->isReference()) {
    env.error("cannot create an array of references");
    return;
  }
  if (dt.type->isSimple(ST_VOID)) {
    env.error("cannot create an array of void");
    return;
  }
  if (dt.type->isFunctionType()) {
    env.error("cannot create an array of functions");
    return;
  }
  // TODO: check for abstract classes

  // cppstd 8.3.4 para 1:
  //   "cv-qualifier array of T" -> "array of cv-qualifier T"
  // hmmm.. I don't know what syntax would give rise to
  // the former, or at least my AST can't represent it.. oh well

  if (size) {
    // typecheck the 'size' expression
    size->tcheck(env, size);
  }

  ArrayType *at;

  if (dt.context == DC_E_NEW) {
    // we're in a new[] (E_new) type-id
    if (!size) {
      env.error("new[] must have an array size specified");
      at = env.makeArrayType(loc, dt.type);    // error recovery
    }
    else {
      if (base->isD_name()) {
        // this is the final size expression, it need not be a
        // compile-time constant; this 'size' is not part of the type
        // of the objects being allocated, rather it is a dynamic
        // count of the number of objects to allocate
        dt.size_E_new = size;
        this->isNewSize = true;     // annotation

        // now just call into the D_name to finish off the type; dt.type
        // is left unchanged, because this D_array contributed nothing
        // to the *type* of the objects we're allocating
        base->tcheck(env, dt);
        return;
      }
      else {
        // this is an intermediate size, so it must be a compile-time
        // constant since it is part of a description of a C++ type
        int sz;
        if (!size->constEval(env, sz)) {
          // error has already been reported; this is for error recovery
          at = env.makeArrayType(loc, dt.type);
        }
        else {
          // constuct the type
          at = env.makeArrayType(loc, dt.type, sz);
        }
      }
    }
  }

  else {
    // we're not in an E_new, so add the size to the type if it's
    // specified; there are some contexts which require a type (like
    // definitions), but we'll report those errors elsewhere
    if (size) {
      // try to evaluate the size to a constant
      int sz;
      string errorMsg;
      if (!size->constEval(errorMsg, sz)) {
        // size didn't evaluate to a constant
        sz = ArrayType::NO_SIZE;
        if ((dt.context == DC_S_DECL ||
             // dsw: if it is a struct declared local to a function,
             // then gcc in C mode allows it to have dynamic size
             (dt.context == DC_MR_DECL && env.enclosingKindScope(SK_FUNCTION))) &&
            env.lang.allowDynamicallySizedArrays) {
          // allow it anyway
          sz = ArrayType::DYN_SIZE;
        }
        else if (errorMsg.length() > 0) {
          // report the error
          env.error(errorMsg);
        }
      }
      else {
        // check restrictions on array size (c.f. cppstd 8.3.4 para 1)
        if (env.lang.strictArraySizeRequirements) {
          if (sz <= 0) {
            env.error(loc, "array size must be positive");
          }
        }
        else {
          if (sz < 0) {
            env.error(loc, "array size must be nonnegative");
          }
        }
      }

      at = env.makeArrayType(loc, dt.type, sz);
    }
    else {
      // no size
      at = env.makeArrayType(loc, dt.type);
    }
  }

  // having added this D_array's contribution to the type, pass
  // that on to the next declarator
  dt.type = at;
  base->tcheck(env, dt);
}


void D_bitfield::tcheck(Env &env, Declarator::Tcheck &dt)
{
  env.setLoc(loc);
  possiblyConsumeFunctionType(env, dt);

  if (name) {
    // shouldn't be necessary, but won't hurt
    name->tcheck(env);
  }

  // fix: I hadn't been type-checking this...
  bits->tcheck(env, bits);

  // check that the expression is a compile-time constant
  int n;
  if (!bits->constEval(env, n)) {
    env.error("bitfield size must be a constant",
              EF_NONE);
  }

  // TODO: record the size of the bit field somewhere; but
  // that size doesn't influence type checking very much, so
  // fixing this will be a low priority for some time.  I think
  // the way to do it is to make another kind of Type which
  // stacks a bitfield size on top of another Type, and
  // construct such an animal here.
}


void D_ptrToMember::tcheck(Env &env, Declarator::Tcheck &dt)
{
  env.setLoc(loc);                   
  
  // typecheck the nested name
  nestedName->tcheck(env);

  // enforce [cppstd 8.3.3 para 3]
  if (dt.type->isReference()) {
    env.error("you can't make a pointer-to-member refer to a reference type");
    return;
    
    // there used to be some recovery code here, but I decided it was
    // better to just bail entirely rather than tcheck into 'base' with
    // broken invariants
  }

  if (dt.type->isVoid()) {
    env.error("you can't make a pointer-to-member refer to `void'");
    return;
  }

  // find the compound to which it refers
  // (previously I used lookupPQCompound, but I believe that is wrong
  // because this is a lookup of a nested-name-specifier (8.3.3) and
  // such lookups are done in the variable space (3.4.3))
  Variable *ctVar = env.lookupPQVariable(nestedName, LF_ONLY_TYPES);
  if (!ctVar) {
    env.error(stringc
      << "cannot find type `" << nestedName->toString()
      << "' for pointer-to-member");
    return;
  }

  // NOTE dsw: don't do this; if it is a typevar, we handle it below
  // if (ctVar->type->isDependent())
  if (ctVar->type->isSimple(ST_DEPENDENT)) {
    // e.g. t0186.cc; propagate the dependentness
    TRACE("dependent", "ptr-to-member: propagating dependentness of " <<
                       nestedName->toString());
    dt.type = env.getSimpleType(SL_UNKNOWN, ST_DEPENDENT);
    base->tcheck(env, dt);
    return;
  }

  // allow the pointer to point to a member of a class (CompoundType),
  // *or* a TypeVariable (for templates)
  NamedAtomicType *nat = ctVar->type->ifNamedAtomicType();
  if (!nat) {
    env.error(stringc
      << "in ptr-to-member, `" << nestedName->toString()
      << "' does not refer to a class nor is a type variable");
    return;
  }

  if (dt.type->isFunctionType()) {
    // add the 'this' parameter to the function type, so the
    // full type becomes "pointer to *member* function"
    makeMemberFunctionType(env, dt, nat, loc);
  }

  // build the ptr-to-member type constructor
  dt.type = env.tfac.syntaxPointerToMemberType(loc, nat, cv, dt.type, this);

  // recurse
  base->tcheck(env, dt);
}


void D_grouping::tcheck(Env &env, Declarator::Tcheck &dt)
{
  env.setLoc(loc);

  // don't call 'possiblyConsumeFunctionType', since the
  // D_grouping is supposed to be transparent

  base->tcheck(env, dt);
}


bool IDeclarator::hasInnerGrouping() const
{
  bool ret = false;

  IDeclarator const *p = this;
  while (p) {
    switch (p->kind()) {
      // turn off the flag because innermost so far is now
      // a pointer type constructor
      case D_POINTER:
        ret = false;
        break;
      case D_REFERENCE:
        ret = false;
        break;
      case D_PTRTOMEMBER:
        ret = false;
        break;

      // turn on the flag b/c innermost is now grouping
      case D_GROUPING:
        ret = true;
        break;
              
      // silence warning..
      default:
        break;
    }

    p = p->getBaseC();
  }

  return ret;
}


// ------------------- ExceptionSpec --------------------
FunctionType::ExnSpec *ExceptionSpec::tcheck(Env &env)
{
  FunctionType::ExnSpec *ret = new FunctionType::ExnSpec;

  // typecheck the list, disambiguating it
  types = tcheckFakeASTTypeIdList(types, env, DF_NONE, DC_EXCEPTIONSPEC);

  // add the types to the exception specification
  FAKELIST_FOREACH_NC(ASTTypeId, types, iter) {
    ret->types.append(iter->getType());
  }

  return ret;
}


// ------------------ OperatorName ----------------
char const *ON_newDel::getOperatorName() const
{
  // changed the names so that they can be printed out with these
  // names and it will be the correct syntax; it means the identifier
  // has a space in it, which isn't exactly ideal, but the alternative
  // (ad-hoc decoding) isn't much better
  return (isNew && isArray)? "operator new[]" :
         (isNew && !isArray)? "operator new" :
         (!isNew && isArray)? "operator delete[]" :
                              "operator delete";
}

char const *ON_operator::getOperatorName() const
{
  xassert(validCode(op));
  return operatorFunctionNames[op];
}

char const *ON_conversion::getOperatorName() const
{
  // this is the sketchy one..
  // update: but it seems to be fitting into the design just fine
  return "conversion-operator";
}


void OperatorName::tcheck(Env &env)
{
  if (isON_conversion()) {
    ON_conversion *conv = asON_conversion();
    
    // check the "return" type
    ASTTypeId::Tcheck tc(DF_NONE, DC_ON_CONVERSION);
    conv->type->tcheck(env, tc);
  }
  else {
    // nothing to do for other kinds of OperatorName
  }
}


// ---------------------- Statement ---------------------
Statement *Statement::tcheck(Env &env)
{
  env.setLoc(loc);

  int dummy;
  if (!ambiguity) {
    // easy case
    mid_tcheck(env, dummy);
    return this;
  }

  Statement *ret = resolveImplIntAmbig(env, this);
  if(ret) {
    return ret->tcheck(env);
  }

  // the only ambiguity for Statements I know if is S_decl vs. S_expr,
  // and this one is always resolved in favor of S_decl if the S_decl
  // is a valid interpretation [cppstd, sec. 6.8]
  if (this->isS_decl() && ambiguity->isS_expr() &&
      ambiguity->ambiguity == NULL) {
    // S_decl is first, run resolver with priority enabled
    return resolveAmbiguity(this, env, "Statement", true /*priority*/, dummy);
  }
  else if (this->isS_expr() && ambiguity->isS_decl() &&
           ambiguity->ambiguity == NULL) {
    // swap the expr and decl
    S_expr *expr = this->asS_expr();
    S_decl *decl = ambiguity->asS_decl();

    expr->ambiguity = NULL;
    decl->ambiguity = expr;

    // now run it with priority
    return resolveAmbiguity(static_cast<Statement*>(decl), env,
                            "Statement", true /*priority*/, dummy);
  }
  
  // unknown ambiguity situation
  env.error("unknown statement ambiguity", EF_NONE);
  return this;
}


// dsw: it is too slow to have emacs reload cc.ast.gen.h just to display
// the body of this method when I'm looking around in the stack in gdb
void Statement::mid_tcheck(Env &env, int &)
{
  itcheck(env);
}


void S_skip::itcheck(Env &env)
{}


void S_label::itcheck(Env &env)
{
  // this is a prototypical instance of typechecking a
  // potentially-ambiguous subtree; we have to change the
  // pointer to whatever is returned by the tcheck call
  s = s->tcheck(env);
  
  // TODO: check that the label is not a duplicate
}


void S_case::itcheck(Env &env)
{                    
  expr->tcheck(env, expr);
  s = s->tcheck(env);

  // TODO: check that the expression is of a type that makes
  // sense for a switch statement, and that this isn't a
  // duplicate case

  // UPDATE: dsw: whatever you do here, do it in
  // gnu.cc:S_rangeCase::itcheck() as well
                           
  // compute case label value
  expr->constEval(env, labelVal);
}


void S_default::itcheck(Env &env)
{
  s = s->tcheck(env);
  
  // TODO: check that there is only one 'default' case
}


void S_expr::itcheck(Env &env)
{
  expr->tcheck(env);
}


void S_compound::itcheck(Env &env)
{ 
  Scope *scope = env.enterScope(SK_FUNCTION, "compound statement");

  FOREACH_ASTLIST_NC(Statement, stmts, iter) {
    // have to potentially change the list nodes themselves
    iter.setDataLink( iter.data()->tcheck(env) );
  }

  env.exitScope(scope);
}


// Given a (reference to) a pointer to a statement, make it into an
// S_compound if it isn't already, so that it will be treated as having
// its own local scope (cppstd 6.4 para 1, 6.5 para 2).  Note that we
// don't need this for try-blocks, because their substatements are 
// *required* to be compound statements already.
void implicitLocalScope(Statement *&stmt)
{
  if (!stmt->isS_compound()) {
    Statement *orig = stmt;
    stmt = new S_compound(orig->loc, new ASTList<Statement>(orig));
  }
}


void S_if::itcheck(Env &env)
{
  // if 'cond' declares a variable, its scope is the
  // body of the "if"
  Scope *scope = env.enterScope(SK_FUNCTION, "condition in an 'if' statement");

  // 6.4 para 1
  implicitLocalScope(thenBranch);
  implicitLocalScope(elseBranch);

  cond = cond->tcheck(env);
  thenBranch = thenBranch->tcheck(env);
  elseBranch = elseBranch->tcheck(env);

  env.exitScope(scope);
}


void S_switch::itcheck(Env &env)
{
  Scope *scope = env.enterScope(SK_FUNCTION, "condition in a 'switch' statement");

  // 6.4 para 1
  implicitLocalScope(branches);

  cond = cond->tcheck(env);
  branches = branches->tcheck(env);

  env.exitScope(scope);
}


void S_while::itcheck(Env &env)
{
  Scope *scope = env.enterScope(SK_FUNCTION, "condition in a 'while' statement");

  // 6.5 para 2
  implicitLocalScope(body);

  cond = cond->tcheck(env);
  body = body->tcheck(env);

  env.exitScope(scope);
}


void S_doWhile::itcheck(Env &env)
{
  // 6.5 para 2
  implicitLocalScope(body);

  body = body->tcheck(env);
  expr->tcheck(env);

  // TODO: verify that 'expr' makes sense in a boolean context
}


void S_for::itcheck(Env &env)
{
  Scope *scope = env.enterScope(SK_FUNCTION, "condition in a 'for' statement");

  // 6.5 para 2
  implicitLocalScope(body);

  init = init->tcheck(env);
  cond = cond->tcheck(env);
  after->tcheck(env);
  body = body->tcheck(env);

  env.exitScope(scope);
}


void S_break::itcheck(Env &env)
{
  // TODO: verify we're in the context of a 'switch'
}


void S_continue::itcheck(Env &env)
{
  // TODO: verify we're in the context of a 'switch'
}


void S_return::itcheck(Env &env)
{
  if (expr) {
    expr->tcheck(env);
    
    // TODO: verify that 'expr' is compatible with the current
    // function's declared return type
  }

  else {
    // TODO: check that the function is declared to return 'void'
  }
}


void S_goto::itcheck(Env &env)
{
  // TODO: verify the target is an existing label
}


void S_decl::itcheck(Env &env)
{
  decl->tcheck(env, DC_S_DECL);
}


void S_try::itcheck(Env &env)
{
  body->tcheck(env);
  
  FAKELIST_FOREACH_NC(Handler, handlers, iter) {
    iter->tcheck(env);
  }
  
  // TODO: verify the handlers make sense in sequence:
  //   - nothing follows a "..." specifier
  //   - no duplicates
  //   - a supertype shouldn't be caught after a subtype
}


void S_asm::itcheck(Env &)
{}


void S_namespaceDecl::itcheck(Env &env)
{
  decl->tcheck(env);
}


// ------------------- Condition --------------------
Condition *Condition::tcheck(Env &env)
{
  int dummy;
  if (!ambiguity) {
    // easy case
    mid_tcheck(env, dummy);
    return this;
  }

  // generic resolution: whatever tchecks is selected
  return resolveAmbiguity(this, env, "Condition", false /*priority*/, dummy);
}

void CN_expr::itcheck(Env &env)
{
  expr->tcheck(env);

  // TODO: verify 'expr' makes sense in a boolean or switch context
}


void CN_decl::itcheck(Env &env)
{
  ASTTypeId::Tcheck tc(DF_NONE, DC_CN_DECL);
  typeId = typeId->tcheck(env, tc);
  
  // TODO: verify the type of the variable declared makes sense
  // in a boolean or switch context
}


// ------------------- Handler ----------------------
void Handler::tcheck(Env &env)
{
  Scope *scope = env.enterScope(SK_FUNCTION, "exception handler");

  // originally, I only did this for the non-isEllpsis() case, to
  // avoid creating a type with ST_ELLIPSIS in it.. but cc_qual
  // finds it convenient, so now we tcheck 'typeId' always
  //
  // dsw: DF_PARAMETER: we think of the handler as an anonymous inline
  // function that is simultaneously defined and called in the same
  // place
  ASTTypeId::Tcheck tc(DF_PARAMETER, DC_HANDLER);
  typeId = typeId->tcheck(env, tc);

  body->tcheck(env);

  env.exitScope(scope);
}


// ------------------- Expression tcheck -----------------------

// There are several things going on with the replacement pointer.
//
// First, since Expressions can be ambiguous, when we select among
// ambiguous Expressions, the replacement is used to tell which one
// to use.  The caller then stores that instead of the original
// pointer.
//
// Second, to support elaboration of implicit function calls, an
// Expression node can decide to replace itself with a different
// kind of node (e.g. overloaded operators), or to insert another
// Expression above it (e.g. user-defined conversion functions).
//
// Finally, the obvious design would call for 'replacement' being
// a return value from 'tcheck', but I found that it was too easy
// to forget to update the original pointer.  So I changed the
// interface so that the original pointer cannot be forgotten, since
// a reference to it is now a parameter.


void Expression::tcheck(Env &env, Expression *&replacement)
{
  // the replacement point should always start out in agreement with
  // the receiver object of the 'tcheck' call; consequently,
  // Expressions can leave it as-is and no replacement will happen
  xassert(replacement == this);

  if (!ambiguity) {
    mid_tcheck(env, replacement);
    return;
  }

  // There is one very common ambiguity, between E_funCall and
  // E_constructor, and this ambiguity happens to frequently stack
  // upon itself, leading to worst-case exponential tcheck time.
  // Since it can be resolved easily in most cases, I special-case the
  // resolution.
  if ( ( (this->isE_funCall() &&
          this->ambiguity->isE_constructor() ) ||
         (this->isE_constructor() &&
          this->ambiguity->isE_funCall()) ) &&
      this->ambiguity->ambiguity == NULL) {
    E_funCall *call;
    E_constructor *ctor;
    if (this->isE_funCall()) {
      call = this->asE_funCall();
      ctor = ambiguity->asE_constructor();
    }
    else {
      ctor = this->asE_constructor();
      call = ambiguity->asE_funCall();
    }

    // The code that follows is essentially resolveAmbiguity(),
    // specialized to two particular kinds of nodes, but only
    // tchecking the first part of each node to disambiguate.
    IFDEBUG( SourceLoc loc = env.loc(); )
    TRACE("disamb", toString(loc) << ": ambiguous: E_funCall vs. E_constructor");

    // grab errors
    ErrorList existing;
    existing.takeMessages(env.errors);

    // common case: function call
    TRACE("disamb", toString(loc) << ": considering E_funCall");
    LookupSet candidates;
    call->inner1_itcheck(env, candidates);
    if (noDisambErrors(env.errors)) {
      // ok, finish up; it's safe to assume that the E_constructor
      // interpretation would fail if we tried it
      TRACE("disamb", toString(loc) << ": selected E_funCall");
      env.errors.prependMessages(existing);
      call->type = env.tfac.cloneType(call->inner2_itcheck(env, candidates));
      call->ambiguity = NULL;
      replacement = call;
      return;
    }

    // grab the errors from trying E_funCall
    ErrorList funCallErrors;
    funCallErrors.takeMessages(env.errors);

    // try the E_constructor interpretation
    TRACE("disamb", toString(loc) << ": considering E_constructor");
    ctor->inner1_itcheck(env);
    if (noDisambErrors(env.errors)) {
      // ok, finish up
      TRACE("disamb", toString(loc) << ": selected E_constructor");
      env.errors.prependMessages(existing);
      
      // a little tricky because E_constructor::inner2_itcheck is
      // allowed to yield a replacement AST node
      replacement = ctor;
      Type *t = ctor->inner2_itcheck(env, replacement);

      replacement->type = env.tfac.cloneType(t);
      replacement->ambiguity = NULL;
      return;
    }

    // both failed.. just leave the errors from the function call
    // interpretation since that's the more likely intent
    env.errors.deleteAll();
    env.errors.takeMessages(existing);
    env.errors.takeMessages(funCallErrors);

    // 10/20/04: Need to give a type anyway.
    this->type = env.errorType();

    // finish up
    replacement = this;     // redundant but harmless
    return;
  }

  // some other ambiguity, use the generic mechanism; the return value
  // is ignored, because the selected alternative will be stored in
  // 'replacement'
  resolveAmbiguity(this, env, "Expression", false /*priority*/, replacement);
}


bool const CACHE_EXPR_TCHECK = false;

void Expression::mid_tcheck(Env &env, Expression *&replacement)
{
  if (CACHE_EXPR_TCHECK && type && !type->isError()) {
    // this expression has already been checked
    //
    // we also require that 'type' not be ST_ERROR, because if the
    // error was disambiguating then we need to check it again to
    // insert the disambiguating message into the environment again;
    // see cc.in/cc.in59
    //
    // update: I've modified the ambiguity resolution engine to
    // fix this itself, by nullifying the 'type' field of any
    // failing subtree
    //
    // update update: but that doesn't work (see markAsFailed, above)
    // so now I'm back to presuming that every node marks itself
    // as ST_ERROR if it should be re-checked in additional
    // contexts (which is almost everywhere)
    return;
  }

  // during ambiguity resolution, 'replacement' is set to whatever
  // the original (first in the ambiguity list) Expression pointer
  // was; reset it to 'this', as that will be our "replacement"
  // unless the Expression wants to do something else
  replacement = this;

  // check it, and store the result
  Type *t = itcheck_x(env, replacement);

  // elaborate the AST by storing the computed type, *unless*
  // we're only disambiguating (because in that case many of
  // the types will be ST_ERROR anyway)
  //if (!env.onlyDisambiguating()) {
  //  type = t;
  //}
  //
  // update: made it unconditional again because after tcheck()
  // the callers expect to be able to dig in and find the type;
  // I guess I'll at some point have to write a visitor to
  // clear the computed types if I want to actually check the
  // template bodies after arguments are presented

  // dsw: putting the cloneType here means I can remove *many* of them
  // elsewhere, namely wherever an itcheck_x() returns.  This causes a
  // bit of garbage, since some of the types returned are partially
  // from somewhere else and partially made right there, such as
  // "return makeLvalType(t)" where "t" is an already existing type.
  // The only way I can think of to optimize that is to turn it in to
  // "return makeLvalType(env.tfac.cloneType(t))" and get rid of the
  // clone here; but that would be error prone and labor-intensive, so
  // I don't do it.
  type = env.tfac.cloneType(t);
}


Type *E_boolLit::itcheck_x(Env &env, Expression *&replacement)
{
  // cppstd 2.13.5 para 1
  return env.getSimpleType(SL_UNKNOWN, ST_BOOL);
}


// return true if type 'id' can represent value 'i'
bool canRepresent(SimpleTypeId id, unsigned long i)
{
  // I arbitrarily choose to make this determination according to the
  // representations available under the compiler that compiles Elsa,
  // since that's convenient and likely to correspond with what
  // happens when the source code in question is "really" compiled.

  switch (id) {
    default: xfailure("bad type id");

    case ST_INT:                 return i <= INT_MAX;
    case ST_UNSIGNED_INT:        return i <= UINT_MAX;
    case ST_LONG_INT:            return i <= LONG_MAX;

    case ST_UNSIGNED_LONG_INT:
    case ST_LONG_LONG:
    case ST_UNSIGNED_LONG_LONG:
      // I don't want to force the host compiler to have 'long long',
      // so I'm just going to claim that every value is representable
      // by these three types.  Also, given that I passed 'i' in as
      // 'unsigned long', it's pretty much a given that it will be
      // representable.
      return true;
  }
}

Type *E_intLit::itcheck_x(Env &env, Expression *&replacement)
{
  // cppstd 2.13.1 para 2

  char const *p = text;

  // what radix? (and advance 'p' past it)
  int radix = 10;
  if (*p == '0') {
    p++;
    if (*p == 'x' || *p == 'X') {
      radix = 16;
      p++;
    }
    else {
      radix = 8;
    }
  }

  // what value? (tacit assumption: host compiler's 'unsigned long'
  // is big enough to make these distinctions)
  i = strtoul(p, NULL /*endp*/, radix);

  // what suffix?
  while (isdigit(*p)) {
    p++;
  }
  bool hasU = false;
  int hasL = 0;
  if (*p == 'u' || *p == 'U') {
    hasU = true;
    p++;
  }
  if (*p == 'l' || *p == 'L') {
    hasL = 1;
    p++;
    if (*p == 'l' || *p == 'L') {
      hasL = 2;
      p++;
    }
  }
  if (*p == 'u' || *p == 'U') {
    hasU = true;
  }

  // The radix and suffix determine a sequence of types, and we choose
  // the smallest from the sequence that can represent the given value.

  // There are three nominal sequences, one containing unsigned types,
  // one with only signed types, and one with both.  In the tables
  // below, I will represent a sequence by pointing into one or the
  // other nominal sequence; the pointed-to element is the first
  // element in the effective sequence.  Sequences terminate with
  // ST_VOID.
  static SimpleTypeId const signedSeq[] = {
    ST_INT,                 // 0
    ST_LONG_INT,            // 1
    ST_LONG_LONG,           // 2
    ST_VOID
  };
  static SimpleTypeId const unsignedSeq[] = {
    ST_UNSIGNED_INT,        // 0
    ST_UNSIGNED_LONG_INT,   // 1
    ST_UNSIGNED_LONG_LONG,  // 2
    ST_VOID
  };
  static SimpleTypeId const mixedSeq[] = {
    ST_INT,                 // 0
    ST_UNSIGNED_INT,        // 1
    ST_LONG_INT,            // 2
    ST_UNSIGNED_LONG_INT,   // 3
    ST_LONG_LONG,           // 4
    ST_UNSIGNED_LONG_LONG,  // 5
    ST_VOID
  };

  // The following table layout is inspired by the table in C99
  // 6.4.4.1 para 5.

  // C99: (hasU + 2*hasL) x isNotDecimal -> typeSequence
  static SimpleTypeId const * const c99Map[6][2] = {
      // radix:  decimal              hex/oct
    // suffix:
    /* none */ { signedSeq+0,         mixedSeq+0 },
    /* U */    { unsignedSeq+0,       unsignedSeq+0 },
    /* L */    { signedSeq+1,         mixedSeq+2 },
    /* UL */   { unsignedSeq+1,       unsignedSeq+1 },
    /* LL */   { signedSeq+2,         mixedSeq+4 },
    /* ULL */  { unsignedSeq+2,       unsignedSeq+2 }
  };

  // The difference is in C++, the radix is only relevant when there
  // is no suffix.  Also, cppstd doesn't specify anything for 'long
  // long' (since that type does not exist in that language), so I'm
  // extrapolating its rules to that case.  Entries in the cppMap
  // that differ from c99Map are marked with "/*d*/".

  // C++: (hasU + 2*hasL) x isNotDecimal -> typeSequence
  static SimpleTypeId const * const cppMap[6][2] = {
      // radix:  decimal              hex/oct
    // suffix:
    /* none */ { signedSeq+0,         mixedSeq+0 },
    /* U */    { unsignedSeq+0,       unsignedSeq+0 },
    /* L */    { mixedSeq+2/*d*/,     mixedSeq+2 },
    /* UL */   { unsignedSeq+1,       unsignedSeq+1 },
    /* LL */   { mixedSeq+4/*d*/,     mixedSeq+4 },
    /* ULL */  { unsignedSeq+2,       unsignedSeq+2 }
  };

  // compute the sequence to use
  SimpleTypeId const *seq =
    env.lang.isCplusplus? cppMap[hasU + 2*hasL][radix!=10] :
                          c99Map[hasU + 2*hasL][radix!=10] ;

  // At this point, we pick the type that is the first type in 'seq'
  // that can represent the value.
  SimpleTypeId id = *seq;
  while (*(seq+1) != ST_VOID &&
         !canRepresent(id, i)) {
    seq++;
    id = *seq;
  }

  return env.getSimpleType(SL_UNKNOWN, id);
}


Type *E_floatLit::itcheck_x(Env &env, Expression *&replacement)
{
  // TODO: wrong; see cppstd 2.13.3 para 1
  d = strtod(text, NULL /*endp*/);
  return env.getSimpleType(SL_UNKNOWN, ST_FLOAT);
}


Type *E_stringLit::itcheck_x(Env &env, Expression *&replacement)
{
  // cppstd 2.13.4 para 1

  // wide character?
  SimpleTypeId id = text[0]=='L'? ST_WCHAR_T : ST_CHAR;

  // TODO: this is wrong because I'm not properly tracking the string
  // size if it has escape sequences
  int len = 0;
  E_stringLit *p = this;
  while (p) {
    len += strlen(p->text) - 2;   // don't include surrounding quotes
    if (p->text[0]=='L') len--;   // don't count 'L' if present
    p = p->continuation;
  }

  CVFlags stringLitCharCVFlags = CV_NONE;
  if (env.lang.stringLitCharsAreConst) {
    stringLitCharCVFlags = CV_CONST;
  }
  Type *charConst = env.getSimpleType(SL_UNKNOWN, id, stringLitCharCVFlags);
  return env.makeArrayType(SL_UNKNOWN, charConst, len+1);    // +1 for implicit final NUL
}


void quotedUnescape(string &dest, int &destLen, char const *src,
                    char delim, bool allowNewlines)
{
  // strip quotes or ticks
  decodeEscapes(dest, destLen, string(src+1, strlen(src)-2),
                delim, allowNewlines);
}

Type *E_charLit::itcheck_x(Env &env, Expression *&replacement)
{
  // cppstd 2.13.2 paras 1 and 2

  SimpleTypeId id = ST_CHAR;
  
  if (!env.lang.isCplusplus) {
    // nominal type of character literals in C is int, not char
    id = ST_INT;
  }

  int tempLen;
  string temp;

  char const *srcText = text;
  if (*srcText == 'L') {
    id = ST_WCHAR_T;
    srcText++;
  }

  quotedUnescape(temp, tempLen, srcText, '\'',
                 false /*allowNewlines*/);
  if (tempLen == 0) {
    return env.error("character literal with no characters");
  }
  else if (tempLen > 1) {
    // below I only store the first byte
    //
    // technically, this is ok, since multicharacter literal values
    // are implementation-defined; but Elsa is not so much its own
    // implementation as an approximation of some nominal "real"
    // compiler, which will no doubt do something smarter, so Elsa
    // should too
    env.warning("multicharacter literals not properly implemented");
    if (id == ST_CHAR) {
      // multicharacter non-wide character literal has type int
      id = ST_INT;
    }
  }

  c = (unsigned int)temp[0];

  if (!env.lang.isCplusplus && id == ST_WCHAR_T) {
    // in C, 'wchar_t' is not built-in, it is defined; so we
    // have to look it up
    Variable *v = env.globalScope()->lookupVariable(env.str("wchar_t"), env);
    if (!v) {
      return env.error("you must #include <stddef.h> before using wchar_t");
    }
    else {
      return v->type;
    }
  }

  return env.getSimpleType(SL_UNKNOWN, id);
}


Type *makeLvalType(TypeFactory &tfac, Type *underlying)
{
  if (underlying->isLval()) {
    // this happens for example if a variable is declared to
    // a reference type
    return underlying;
  }
  else if (underlying->isFunctionType()) {
    // don't make references to functions
    return underlying;

    // at one point I added a similar prohibition against
    // references to arrays, but that was wrong, e.g.:
    //   int (&a)[];
  }
  else {
    return tfac.makeReferenceType(SL_UNKNOWN, underlying);
  }
}

Type *makeLvalType(Env &env, Type *underlying)
{
  return makeLvalType(env.tfac, underlying);
}


Type *E_this::itcheck_x(Env &env, Expression *&replacement)
{
  // we should be in a method with a receiver parameter
  receiver = env.lookupVariable(env.receiverName);
  if (!receiver) {
    return env.error("can only use 'this' in a nonstatic method");
  }

  // 14.6.2.2 para 2
  if (receiver->type->containsGeneralizedDependent()) {
    return env.dependentType();
  }

  // compute type: *pointer* to the thing 'receiverVar' is
  // a *reference* to
  //
  // (sm: it seems to me that type cloning should be unnecessary since
  // this is intended to be the *same* object as __receiver)
  return env.makePointerType(env.loc(), CV_NONE,
                             receiver->type->asRval());
}


E_fieldAcc *wrapWithImplicitThis(Env &env, Variable *var, PQName *name)
{
  // make *this
  E_this *ths = new E_this;
  Expression *thisRef = new E_deref(ths);
  thisRef->tcheck(env, thisRef);

  // sm: this assertion can fail if the method we are in right now
  // is static; the error has been reported, so just proceed
  //xassert(ths->receiver);

  // no need to tcheck as the variable has already been looked up
  E_fieldAcc *efieldAcc = new E_fieldAcc(thisRef, name);
  efieldAcc->field = var;

  // E_fieldAcc::itcheck_fieldAcc() does something a little more
  // complicated, but we don't need that since the situation is
  // more constrained here
  efieldAcc->type = makeLvalType(env, env.tfac.cloneType(var->type));

  return efieldAcc;
}


Type *E_variable::itcheck_x(Env &env, Expression *&replacement)
{
  return itcheck_var(env, replacement, LF_NONE);
}

Type *E_variable::itcheck_var(Env &env, Expression *&replacement, LookupFlags flags)
{
  LookupSet dummy;
  return itcheck_var_set(env, replacement, flags, dummy);
}

Type *E_variable::itcheck_var_set(Env &env, Expression *&replacement, 
                                  LookupFlags flags, LookupSet &candidates)
{
  name->tcheck(env);

  // re-use dependent?
  Variable *v = maybeReuseNondependent(env, name->loc, flags, nondependentVar);
  if (v) {
    var = v;
    
    // 2005-02-20: 'add' instead of 'adds', because when we remember a
    // non-dependent lookup, we do *not* want to re-do overload
    // resolution
    candidates.addIf(v, flags);
  }
  else {
    // do lookup normally
    v = env.lookupPQVariable_set(candidates, name, flags);

    if (v && v->hasFlag(DF_TYPEDEF)) {
      return env.error(name->loc, stringc
        << "`" << *name << "' used as a variable, but it's actually a type",
        EF_DISAMBIGUATES);
    }

    // 2005-02-18: cppstd 14.2 para 2: if template arguments are
    // supplied, then the name must look up to a template name
    if (name->getUnqualifiedName()->isPQ_template()) {
      if (!v || !v->namesTemplateFunction()) {
        // would disambiguate use of '<' as less-than
        env.error(name->loc, stringc
          << "explicit template arguments were provided after `"
          << name->toString_noTemplArgs()
          << "', but that is not the name of a template function",
          EF_DISAMBIGUATES);
      }
    }

    if (!v) {
      // dsw: In K and R C it seems to be legal to call a function
      // variable that has never been declareed.  At this point we
      // notice this fact and if we are in K and R C we insert a
      // variable with signature "int ()(...)" which is what I recall as
      // the correct signature for such an implicit variable.
      if (env.lang.allowImplicitFunctionDecls && (flags & LF_FUNCTION_NAME)) {
        if (env.lang.allowImplicitFunctionDecls == B3_WARN) {
          env.warning(name->loc, stringc << "implicit declaration of `" << *name << "'");
        }

        // this should happen in C mode only so name must be a PQ_name
        v = env.makeImplicitDeclFuncVar(name->asPQ_name()->name);
      }
      else {
        if (flags & LF_SUPPRESS_NONEXIST) {
          // return a special type and do not insert an error message;
          // this is like a pending error that the caller should
          // resolve, either by making it a real error or (by using
          // argument dependent lookup) fix; ST_NOTFOUND shoult never
          // appear in the output of type checking
          return env.getSimpleType(name->loc, ST_NOTFOUND);
        }
        else {
          // 10/23/02: I've now changed this to non-disambiguating,
          // prompted by the need to allow template bodies to call
          // undeclared functions in a "dependent" context [cppstd 14.6
          // para 8].  See the note in TS_name::itcheck.
          return env.error(name->loc, stringc
                           << "there is no variable called `" << *name << "'",
                           EF_NONE);
        }
      }
    }
    xassert(v);

    // TODO: access control check

    var = env.storeVarIfNotOvl(v);

    // should I remember this non-dependent lookup?
    maybeNondependent(env, name->loc, nondependentVar, var);
  }

  if (var->isTemplateArg/*wrong*/()) {
    // this variable is actually a bound meta-variable (template
    // argument), so it is *not* to be regarded as a reference
    // (14.1 para 6)
    //
    // TODO: The correct query here is 'isTemplateParamOrArg', but
    // when I put that in it runs smack into the STA_REFERENCE
    // problem, so I am leaving it wrong for now.
    return var->type;
  }

  // elaborate 'this->'
  if (!(flags & LF_NO_IMPL_THIS) &&
      var->isMember() && 
      !var->isStatic()) {
    replacement = wrapWithImplicitThis(env, var, name);
  }

  // return a reference because this is an lvalue
  return makeLvalType(env, var->type);
}


// ------------- BEGIN: outerResolveOverload ------------------
// return true iff all the variables are non-methods; this is
// temporary, otherwise I'd make it a method on class OverloadSet
static bool allNonMethods(SObjList<Variable> &set)
{
  SFOREACH_OBJLIST(Variable, set, iter) {
    if (iter.data()->type->asFunctionType()->isMethod()) return false;
  }
  return true;
}

#if 0     // not needed
// return true iff all the variables are methods; this is temporary,
// otherwise I'd make it a method on class OverloadSet
static bool allMethods(SObjList<Variable> &set)
{
  SFOREACH_OBJLIST(Variable, set, iter) {
//      cout << "iter.data()->type->asFunctionType() " << iter.data()->type->asFunctionType()->toCString() << endl;
    if (!iter.data()->type->asFunctionType()->isMethod()) return false;
  }
  return true;
}                      
#endif // 0


// set an ArgumentInfo according to an expression
void getArgumentInfo(Env &env, ArgumentInfo &ai, Expression *e)
{
  Variable *ovlVar = env.getOverloadedFunctionVar(e);
  if (ovlVar) {
    ai.ovlVar = ovlVar;
  }
  else {
    ai.special = e->getSpecial(env.lang);
    ai.type = e->type;
  }
}


// Given a Variable that might denote an overloaded set of functions,
// and the syntax of the arguments that are to be passed to the
// function ultimately chosen, pick one of the functions using
// overload resolution; return NULL if overload resolution fails for
// any reason. 'loc' is the location of the call site, for error
// reporting purposes.  (The arguments have already been tcheck'd.)
//
// This function mediates between the type checker, which knows about
// syntax and context, and the overload module's 'resolveOverload',
// which knows about neither.  In essence, it's everything that is
// common to overload resolution needed in various AST locations
// that isn't already covered by 'resolveOverload' itself.
//
// Note that it is up to the caller to do AST rewriting as necessary
// to reflect the chosen function.  Rationale: the caller is in a
// better position to know what things need to be rewritten, since it
// is fully aware of the syntactic context.
//
// The contract w.r.t. errors is: the caller must provide a non-NULL
// 'var', and if I return NULL, I will also add an error message, so
// the caller does not have to do so.
static Variable *outerResolveOverload(Env &env,
                                      PQName * /*nullable*/ finalName,
                                      SourceLoc loc, Variable *var,
                                      Type *receiverType, FakeList<ArgExpression> *args)
{
  // if no overload set, nothing to resolve
  if (!var->overload) {
    return var;
  }

  return outerResolveOverload_explicitSet(env, finalName, loc, var->name,
                                          receiverType, args, var->overload->set);
}

static Variable *outerResolveOverload_explicitSet(
  Env &env,
  PQName * /*nullable*/ finalName,
  SourceLoc loc,
  StringRef varName,
  Type *receiverType,
  FakeList<ArgExpression> *args,
  SObjList<Variable> &candidates)
{
  // special case: does 'finalName' directly name a particular
  // conversion operator?  e.g. in/t0226.cc
  if (finalName &&
      finalName->isPQ_operator() &&
      finalName->asPQ_operator()->o->isON_conversion()) {
    ON_conversion *conv = finalName->asPQ_operator()->o->asON_conversion();
    Type *namedType = conv->type->getType();

    // find the operator in the overload set
    SFOREACH_OBJLIST_NC(Variable, candidates, iter) {
      Type *iterRet = iter.data()->type->asFunctionType()->retType;
      if (iterRet->equals(namedType)) {
        return iter.data();
      }
    }

    env.error(stringc << "cannot find conversion operator yielding `"
                      << namedType->toString() << "'");
    return NULL;
  }

  OVERLOADINDTRACE(::toString(loc)
        << ": overloaded(" << candidates.count()
        << ") call to " << varName);      // if I make this fully qualified, d0053.cc fails...

  // are any members of the set (nonstatic) methods?
  bool anyMethods = !allNonMethods(candidates);

  // fill an array with information about the arguments
  GrowArray<ArgumentInfo> argInfo(args->count() + (anyMethods?1:0) );
  {
    int index = 0;

    if (anyMethods) {
      argInfo[index].special = SE_NONE;
      if (receiverType) {
        // TODO: take into account whether the receiver is an rvalue
        // or an lvalue
        argInfo[index].type = makeLvalType(env, receiverType);
      }
      else {
        // let a NULL receiver type indicate the absence of a receiver
        // argument; this will make all nonstatic methods not viable
        argInfo[index].type = NULL;
      }

      index++;
    }

    FAKELIST_FOREACH_NC(ArgExpression, args, iter) {
      getArgumentInfo(env, argInfo[index], iter->expr);
      index++;
    }
  }

  // 2005-02-23: There used to be code here that would bail on
  // overload resolution if any of the arguments was dependent.
  // However, that caused a problem (e.g. in/t0386.cc) because if the
  // receiver object is implicit, it does not matter if it is
  // dependent, but we cannot tell from here whether it was implicit.
  // Therefore I have moved the obligation of skipping overload
  // resolution to the caller, who *can* tell.

  // resolve overloading
  bool wasAmbig;     // ignored, since error will be reported
  return resolveOverload(env, loc, &env.errors,
                         anyMethods? OF_METHODS : OF_NONE,
                         candidates, finalName, argInfo, wasAmbig);
}


// version of 'outerResolveOverload' for constructors; 'type' is the
// type of the object being constructed
static Variable *outerResolveOverload_ctor
  (Env &env, SourceLoc loc, Type *type, FakeList<ArgExpression> *args, bool really)
{
  Variable *ret = NULL;
  // dsw: FIX: should I be generating error messages if I get a weird
  // type here, or if I get a weird var below?
  //
  // sm: at one point this code said 'asRval' but I think that is wrong
  // since we should not be treating the construction of a reference
  // the same as the construction of an object
  if (type->isCompoundType()) {
    CompoundType *ct = type->asCompoundType();
    Variable *ctor = ct->getNamedField(env.constructorSpecialName, env, LF_INNER_ONLY);
    xassert(ctor);
    if (really) {
      Variable *chosen = outerResolveOverload(env,
                                              NULL, // finalName; none for a ctor
                                              loc,
                                              ctor,
                                              NULL, // not a method call, so no 'this' object
                                              args);
      if (chosen) {
        ret = chosen;
      } else {
        ret = ctor;
      }
    } else {                    // if we aren't really doing overloading
      ret = ctor;
    }
  }
  // dsw: Note var can be NULL here for ctors for built-in types like
  // int; see t0080.cc
  return ret;
}
// ------------- END: outerResolveOverload ------------------


// this is the old code, and served as the prototypical way to tcheck
// a FakeList of potentially ambiguous elements; but now FakeList<Expression>
// is not used, so this function has become much simpler
#if 0
FakeList<Expression> *tcheckFakeExprList(FakeList<Expression> *list, Env &env)
{
  if (!list) {
    return list;
  }

  // check first expression
  Expression *firstExp = list->first();
  firstExp->tcheck(env, firstExp);
  FakeList<Expression> *ret = FakeList<Expression>::makeList(firstExp);

  // check subsequent expressions, using a pointer that always
  // points to the node just before the one we're checking
  Expression *prev = ret->first();
  while (prev->next) {
    Expression *tmp = prev->next;
    tmp->tcheck(env, tmp);
    prev->next = tmp;

    prev = prev->next;
  }

  return ret;
}
#endif // 0

// here's the new code that serves the same role as the old
FakeList<ArgExpression> *tcheckArgExprList(FakeList<ArgExpression> *list, Env &env)
{
  if (!list) {
    return list;
  }

  // check it recursively from inside ArgExpression::tcheck
  return FakeList<ArgExpression>::makeList(list->first()->tcheck(env));
}

ArgExpression *ArgExpression::tcheck(Env &env)
{
  // modeled after Statement::tcheck

  int dummy;
  if (!ambiguity) {
    mid_tcheck(env, dummy);
    return this;
  }

  return resolveAmbiguity(this, env, "ArgExpression", false /*priority*/, dummy);
}

void ArgExpression::mid_tcheck(Env &env, int &)
{
  expr->tcheck(env, expr);

  // recursively check the tail of the list
  if (next) {
    next = next->tcheck(env);
  }
}


void compareArgsToParams(Env &env, FunctionType *ft, FakeList<ArgExpression> *args)
{
  if (!env.doCompareArgsToParams ||
      (ft->flags & FF_NO_PARAM_INFO)) {
    return;
  }

  SObjListIterNC<Variable> paramIter(ft->params);
  int paramIndex = 1;
  FakeList<ArgExpression> *argIter = args;

  // for now, skip receiver object
  //
  // TODO (elaboration, admission): consider the receiver object as well
  if (ft->isMethod()) {
    paramIter.adv();
  }

  // iterate over both lists
  for (; !paramIter.isDone() && !argIter->isEmpty();
       paramIter.adv(), paramIndex++, argIter = argIter->butFirst()) {
    Variable *param = paramIter.data();
    ArgExpression *arg = argIter->first();

    // is the argument the name of an overloaded function? [cppstd 13.4]
    Variable *ovlVar = env.getOverloadedFunctionVar(arg->expr);
    if (ovlVar) {
      // pick the one that matches the target type
      ovlVar = env.pickMatchingOverloadedFunctionVar(ovlVar, param->type);
      if (ovlVar) {
        // modify 'arg' accordingly
        env.setOverloadedFunctionVar(arg->expr, ovlVar);
      }
    }

    // try to convert the argument to the parameter
    ImplicitConversion ic = getImplicitConversion(env,
      arg->getSpecial(env.lang),
      arg->getType(),
      param->type,
      false /*destIsReceiver*/);
    if (!ic) {
      env.error(arg->getType(), stringc
        << "cannot convert argument type `" << arg->getType()->toString()
        << "' to parameter " << paramIndex 
        << " type `" << param->type->toString() << "'");
    }

    // TODO (elaboration): if 'ic' involves a user-defined
    // conversion, then modify the AST to make that explicit
  }

  if (argIter->isEmpty()) {
    // check that all remaining parameters have default arguments
    for (; !paramIter.isDone(); paramIter.adv(), paramIndex++) {
      if (!paramIter.data()->value) {
        env.error(stringc
          << "no argument supplied for parameter " << paramIndex);
      }
      else {
        // TODO (elaboration): modify the call site to explicitly
        // insert the default-argument expression (is that possible?
        // might it run into problems with evaluation contexts?)
      }
    }
  }
  else if (paramIter.isDone() && !ft->acceptsVarargs()) {
    env.error("too many arguments supplied");
  }
}


static bool hasNamedFunction(Expression *e)
{
  return e->isE_variable() || e->isE_fieldAcc();
}

static Variable *getNamedFunction(Expression *e)
{
  if (e->isE_variable()) { return e->asE_variable()->var; }
  if (e->isE_fieldAcc()) { return e->asE_fieldAcc()->field; }
  xfailure("no named function");
  return NULL;   // silence warning
}
  
// fwd
static Type *internalTestingHooks
  (Env &env, StringRef funcName, FakeList<ArgExpression> *args);


// true if the type has no destructor because it's not a compound type
// nor an array of (an array of ..) compound types
static bool hasNoopDtor(Type *t)
{
  // if it's an array type, then whether it has a no-op dtor depends
  // entirely on whether the element type has a no-op dtor
  while (t->isArrayType()) {
    t = t->asArrayType()->eltType;
  }

  return !t->isCompoundType();
}

Type *E_funCall::itcheck_x(Env &env, Expression *&replacement)
{
  LookupSet candidates;
  inner1_itcheck(env, candidates);

  // special case: if someone explicitly called the destructor
  // of a non-class type, e.g.:
  //   typedef unsigned uint;
  //   uint x;
  //   x.~uint();
  // then change it into a void-typed simple evaluation:
  //   (void)x;
  // since the call itself is a no-op
  if (func->isE_fieldAcc()) {
    E_fieldAcc *fa = func->asE_fieldAcc();
    if (fa->fieldName->getName()[0] == '~' &&
        hasNoopDtor(fa->obj->type->asRval())) {
      if (args->isNotEmpty()) {
        env.error("call to dtor must have no arguments");
      }
      ASTTypeId *voidId =
        new ASTTypeId(new TS_simple(SL_UNKNOWN, ST_VOID),
                      new Declarator(new D_name(SL_UNKNOWN, NULL /*name*/),
                                     NULL /*init*/));
      replacement = new E_cast(voidId, fa->obj);
      replacement->tcheck(env, replacement);
      return replacement->type;
    }
  }

  return inner2_itcheck(env, candidates);
}

#if 0      // don't need this now, but I might want it again later
Expression *&skipGroupsRef(Expression *&e)
{
  if (e->isE_grouping()) {
    return skipGroupsRef(e->asE_grouping()->expr);
  }
  else {
    return e;
  }
}
#endif // 0

void E_funCall::inner1_itcheck(Env &env, LookupSet &candidates)
{
  // dsw: I need to know the arguments before I'm asked to instantiate
  // the function template
  //
  // check the argument list
  //
  // sm: No!  That defeats the entire purpose of inner1/2.  See the
  // comments above the block that calls inner1/2, and the comments
  // near the declarations of inner1/2 in cc_tcheck.ast.
  //args = tcheckArgExprList(args, env);

  // 2005-02-11: Doing this simplifies a number of things.  In general
  // I think I should move towards a strategy of eliminating
  // E_groupings wherever I can do so.  Eventually, I'd like it to be
  // the case that no E_groupings survive type checking.
  func = func->skipGroups();

  // nominal flags if we're calling a named function
  LookupFlags specialFlags = 
    LF_TEMPL_PRIMARY |       // we will do template instantiation later
    LF_FUNCTION_NAME |       // we might allow an implicit declaration
    LF_NO_IMPL_THIS;         // don't add 'this->' (must do overload resol'n first)

  if (func->isE_variable()) {
    // tell the E_variable *not* to do instantiation of
    // templates, because that will need to wait until
    // we see the argument types
    //
    // what follows is essentially Expression::tcheck
    // specialized to E_variable, but with a special lookup
    // flag passed
    E_variable *evar = func->asE_variable();
    xassert(!evar->ambiguity);

    if (!evar->name->hasQualifiers()) {
      // Unqualified name being looked up in the context of a function
      // call; cppstd 3.4.2 applies, which is implemented in
      // inner2_itcheck.  So, here, we don't report an error because
      // inner2 will do another lookup and report an error if that one
      // fails too.
      specialFlags |= LF_SUPPRESS_NONEXIST;
    }

    // fill in 'candidates'
    specialFlags |= LF_LOOKUP_SET;

    func->type = env.tfac.cloneType(
      evar->itcheck_var_set(env, func, specialFlags, candidates));
  }
  else if (func->isE_fieldAcc()) {
    // similar handling for member function templates
    E_fieldAcc *eacc = func->asE_fieldAcc();
    xassert(!eacc->ambiguity);
    func->type = env.tfac.cloneType(
      eacc->itcheck_fieldAcc(env, specialFlags));
  }
  else {
    // do the general thing
    func->tcheck(env, func);
    return;
  }
}


static bool shouldUseArgDepLookup(E_variable *evar)
{
  if (evar->type->isSimple(ST_NOTFOUND)) {
    // lookup failed; use it
    return true;
  }

  if (!evar->type->isFunctionType()) {
    // We found a non-function, like an (function) object or a
    // function pointer.  The standard seems to say that even in
    // this case we do arg-dependent lookup, but I think that is a
    // little strange, and it does not work in the current
    // implementation because we end up doing overload resolution
    // with the object name as a candidate, and that messes up
    // everything (how should it work???).  So I am just going to
    // let it go.  A testcase is in/t0360.cc.
    return false;
  }

  if (evar->var->isMember()) {
    // found a member function; we do *not* use arg-dep lookup
    return false;
  }

  return true;
}

void possiblyWrapWithImplicitThis(Env &env, Expression *&func,
                                  E_variable *&fevar, E_fieldAcc *&feacc)
{
  if (fevar &&
      fevar->var &&
      fevar->var->isMember() &&
      !fevar->var->isStatic()) {
    feacc = wrapWithImplicitThis(env, fevar->var, fevar->name);
    func = feacc;
    fevar = NULL;
  }
}

Type *E_funCall::inner2_itcheck(Env &env, LookupSet &candidates)
{
  // check the argument list
  args = tcheckArgExprList(args, env);

  // do any of the arguments have types that are dependent on template params?
  bool dependentArgs = hasDependentActualArgs(args);

  // inner1 skipped E_groupings already
  xassert(!func->isE_grouping());

  // is 'func' an E_variable?  a number of special cases kick in if so
  E_variable *fevar = func->isE_variable()? func->asE_variable() : NULL;

  // similarly for E_fieldAcc
  E_fieldAcc *feacc = func->isE_fieldAcc()? func->asE_fieldAcc() : NULL;

  // abbreviated processing for dependent lookups
  if (dependentArgs) {
    if (fevar && fevar->nondependentVar) {
      // kill the 'nondependent' lookup
      TRACE("dependent", toString(fevar->name->loc) <<
        ": killing the nondependency of " << fevar->nondependentVar->name);
      fevar->nondependentVar = NULL;
    }

    // 14.6.2 para 1, 14.6.2.2 para 1
    return env.dependentType();
  }
  else if (feacc && feacc->type->isGeneralizedDependent()) {
    return env.dependentType();
  }

  // did we already decide to re-use a previous nondependent lookup
  // result?  (in/t0387.cc)
  bool const alreadyDidLookup =
    !env.inUninstTemplate() &&
    fevar && 
    fevar->nondependentVar && 
    fevar->nondependentVar == fevar->var;

  // 2005-02-18: rewrote function call site name lookup; see doc/lookup.txt
  if (env.lang.allowOverloading &&
      !alreadyDidLookup &&
      (func->type->isSimple(ST_NOTFOUND) ||
       func->type->asRval()->isFunctionType()) &&
      (fevar || (feacc &&
                 // in the case of a call to a compiler-synthesized
                 // destructor, the 'field' is currently NULL (that might
                 // change); but in that case overloading is not possible,
                 // so this code can safely ignore it (e.g. in/t0091.cc)
                 feacc->field))) {
    PQName *pqname = fevar? fevar->name : feacc->fieldName;

    // what is the set of names obtained by inner1?
    //LookupSet candidates;   // use passed-in list
    if (fevar) {
      // passed-in 'candidates' is already correct
    }
    else {
      xassert(candidates.isEmpty());
      if (feacc->field) {
        feacc->field->getOverloadList(candidates);
      }
    }

    // augment with arg-dep lookup?
    if (fevar &&                              // E_variable
        !pqname->hasQualifiers() &&           // unqualified
        shouldUseArgDepLookup(fevar)) {       // (some other tests pass)
      // get additional candidates from associated scopes
      ArrayStack<Type*> argTypes(args->count());
      FAKELIST_FOREACH(ArgExpression, args, iter) {
        argTypes.push(iter->getType());
      }
      env.associatedScopeLookup(candidates, pqname->getName(),
                                argTypes, LF_NONE);
    }

    if (candidates.isEmpty()) {
      return fevar->type =
        env.error(pqname->loc,
                  stringc << "there is no function called `"
                          << pqname->getName() << "'",
                  EF_NONE);
    }

    // template args supplied?
    PQ_template *targs = NULL;
    if (pqname->getUnqualifiedName()->isPQ_template()) {
      targs = pqname->getUnqualifiedName()->asPQ_template();
    }

    // refine candidates by instantiating templates, etc.
    char const *lastRemovalReason = "(none removed yet)";
    SObjListMutator<Variable> mut(candidates);
    while (!mut.isDone()) {
      Variable *v = mut.data();
      bool const considerInherited = false;
      InferArgFlags const iflags = IA_NO_ERRORS;

      // filter out non-templates if we have template arguments
      if (targs && !v->isTemplate(considerInherited)) {
        mut.remove();
        lastRemovalReason = "non-template given template arguments";
        continue;
      }

      // instantiate templates
      if (v->isTemplate(considerInherited)) {
        // initialize the bindings with those explicitly provided
        MatchTypes match(env.tfac, MatchTypes::MM_BIND, Type::EF_DEDUCTION);
        if (targs) {
          if (!env.loadBindingsWithExplTemplArgs(v, targs->args, match, iflags)) {
            mut.remove();
            lastRemovalReason = "incompatible explicit template args";
            continue;
          }
        }

        // deduce the rest from the call site (expression) args
        TypeListIter_FakeList argsIter(args);
        if (!env.inferTemplArgsFromFuncArgs(v, argsIter, match, iflags)) {
          mut.remove();
          lastRemovalReason = "incompatible call site args";
          continue;
        }

        // use them to instantiate the template
        Variable *inst = env.instantiateFunctionTemplate(pqname->loc, v, match);
        if (!inst) {
          mut.remove();
          lastRemovalReason = "could not deduce all template params";
          continue;
        }

        // replace the template primary with its instantiation
        mut.dataRef() = inst;
      }
      
      mut.adv();
    }

    // do we still have any candidates?
    if (candidates.isEmpty()) {
      return env.error(pqname->loc, stringc
               << "call site name lookup failed to yield any candidates; "
               << "last candidate was removed because: " << lastRemovalReason);
    }

    // throw the whole mess at overload resolution
    Variable *chosen;
    if (candidates.count() > 1) {
      // here's a cute hack: I (may) need to pass the receiver object as
      // one of the arguments to participate in overload resolution, so
      // just make a temporary ArgExpression grafted onto the front of
      // the real arguments list
      ArgExpression tmpReceiver(feacc? feacc->obj : NULL);
      FakeList<ArgExpression> *ovlArgs = args;
      if (feacc) {
        ovlArgs = ovlArgs->prepend(&tmpReceiver);
      }

      // pick the best candidate
      chosen = outerResolveOverload_explicitSet
        (env, pqname, pqname->loc, pqname->getName(),
         fevar? env.implicitReceiverType() : feacc->obj->type,
         args, candidates);

      // now disconnect the temporary object so it can be safely
      // disposed of
      tmpReceiver.expr = NULL;
      tmpReceiver.next = NULL;
    }
    else {
      chosen = candidates.first();
    }

    if (chosen) {
      // rewrite AST to reflect choice
      //
      // the stored type will be the dealiased type in hopes this
      // achieves 7.3.3 para 13, "This has no effect on the type of
      // the function, and in all other respects the function
      // remains a member of the base class."
      chosen = env.storeVar(chosen);
      if (fevar) {
        fevar->var = chosen;
        maybeNondependent(env, pqname->loc, fevar->nondependentVar, chosen);   // in/t0385.cc
      }
      else {
        feacc->field = chosen;
        
        // TODO: I'm pretty sure I need nondependent names for E_fieldAcc too
        //maybeNondependent(env, pqname->loc, feacc->nondependentVar, chosen);
      }
      func->type = env.tfac.cloneType(chosen->type);
    }
    else {
      return env.errorType();
    }
  }

  // the type of the function that is being invoked
  Type *t = func->type->asRval();

  // check for operator()
  CompoundType *ct = t->ifCompoundType();
  if (ct) {
    // the insertion of implicit 'this->' below will not be reached,
    // so do it here too
    possiblyWrapWithImplicitThis(env, func, fevar, feacc);

    Variable *funcVar = ct->getNamedField(env.functionOperatorName, env);
    if (funcVar) {
      // resolve overloading
      funcVar = outerResolveOverload(env, NULL /*finalName*/, env.loc(), funcVar,
                                     func->type, args);
      if (funcVar) {
        // rewrite AST to reflect use of 'operator()'
        Expression *object = func;
        E_fieldAcc *fa = new E_fieldAcc(object,
          new PQ_operator(SL_UNKNOWN, new ON_operator(OP_PARENS), env.functionOperatorName));
        fa->field = funcVar;
        fa->type = env.tfac.cloneType(funcVar->type);
        func = fa;

        return funcVar->type->asFunctionType()->retType;
      }
      else {
        return env.errorType();
      }
    }
    else {
      return env.error(stringc
        << "object of type `" << t->toString() << "' used as a function, "
        << "but it has no operator() declared");
    }
  }

  // for internal testing
  if (fevar) {
    Type *ret = internalTestingHooks(env, fevar->name->getName(), args);
    if (ret) {
      return ret;
    }
  }

  // fulfill the promise that inner1 made when it passed
  // LF_NO_IMPL_THIS, namely that we would add 'this->' later
  // if needed; here is "later"
  possiblyWrapWithImplicitThis(env, func, fevar, feacc);

  // make sure this function has been typechecked
  if (fevar) {
    // if it is a pointer to a function that function should get
    // instantiated when its address is taken
    env.ensureFuncBodyTChecked(fevar->var);
  }
  else if (feacc) {
    env.ensureFuncBodyTChecked(feacc->field);
  }

  // automatically coerce function pointers into functions
  if (t->isPointerType()) {
    t = t->asPointerTypeC()->atType;
    // if it is an E_variable then its overload set will be NULL so we
    // won't be doing overload resolution in this case
  }

  if (!t->isFunctionType()) {
    return env.error(func->type->asRval(), stringc
      << "you can't use an expression of type `" << func->type->toString()
      << "' as a function");
  }

  FunctionType *ft = t->asFunctionType();
  env.instantiateTemplatesInParams(ft);

  // receiver object?
  if (env.doCompareArgsToParams && ft->isMethod()) {
    Type *receiverType = NULL;
    if (feacc) {
      // explicit receiver via '.'
      receiverType = feacc->obj->type;
    }
    else if (func->isE_binary() &&
             func->asE_binary()->op == BIN_DOT_STAR) {
      // explicit receiver via '.*'
      receiverType = func->asE_binary()->e1->type;
    }
    else if (func->isE_binary() &&
             func->asE_binary()->op == BIN_ARROW_STAR) {
      // explicit receiver via '->*'
      receiverType = func->asE_binary()->e1->type->asRval();
      if (!receiverType->isPointerType()) {
        // this message is partially redundant; the error(1) in in/t0298.cc
        // also yields the rather vague "no viable candidate"
        env.error("LHS of ->* must be a pointer");
        receiverType = NULL;
      }
      else {
        receiverType = receiverType->asPointerType()->atType;
      }
    }
    else {
      // now that we wrap with 'this->' explicitly, this code should
      // not be reachable
      xfailure("got to implicit receiver code; should not be possible!");

      // implicit receiver
      Variable *receiver = env.lookupVariable(env.receiverName);
      if (!receiver) {
        return env.error("must supply a receiver object to invoke a method");
      }
      else {
        receiverType = receiver->type;
      }
    }

    if (receiverType) {
      // check that the receiver object matches the receiver parameter
      if (!getImplicitConversion(env,
             SE_NONE,
             receiverType,
             ft->getReceiver()->type,
             true /*destIsReceiver*/)) {
        env.error(stringc
          << "cannot convert argument type `" << receiverType->toString()
          << "' to receiver parameter type `" << ft->getReceiver()->type->toString()
          << "'");
      }
    }
    else {
      // error already reported
    }
  }

  // compare argument types to parameters (not guaranteed by overload
  // resolution since it might not have been done, and even if done,
  // uses more liberal rules)
  compareArgsToParams(env, ft, args);

  // type of the expr is type of the return value
  return ft->retType;
}


// special hooks for testing internal algorithms; returns a type
// for the entire E_funCall expression if it recognizes the form
// and typechecks it in its entirety
//
// actually, the arguments have already been tchecked ...
static Type *internalTestingHooks
  (Env &env, StringRef funcName, FakeList<ArgExpression> *args)
{
  // check the type of an expression
  if (funcName == env.special_checkType) {
    if (args->count() == 2) {
      Type *t1 = args->nth(0)->getType();
      Type *t2 = args->nth(1)->getType();
      if (t1->equals(t2)) {
        // ok
      }
      else {
        env.error(stringc << "checkType: `" << t1->toString() 
                          << "' != `" << t2->toString() << "'");
      }
    }
    else {
      env.error("invalid call to __checkType");
    }
  }

  // test vector for 'getStandardConversion'
  if (funcName == env.special_getStandardConversion) {
    int expect;
    if (args->count() == 3 &&
        args->nth(2)->constEval(env, expect)) {
      test_getStandardConversion
        (env,
         args->nth(0)->getSpecial(env.lang),     // is it special?
         args->nth(0)->getType(),                // source type
         args->nth(1)->getType(),                // dest type
         expect);                                // expected result
    }
    else {
      env.error("invalid call to __getStandardConversion");
    }
  }

  // test vector for 'getImplicitConversion'
  if (funcName == env.special_getImplicitConversion) {
    int expectKind;
    int expectSCS;
    int expectUserLine;
    int expectSCS2;
    if (args->count() == 6 &&
        args->nth(2)->constEval(env, expectKind) &&
        args->nth(3)->constEval(env, expectSCS) &&
        args->nth(4)->constEval(env, expectUserLine) &&
        args->nth(5)->constEval(env, expectSCS2)) {
      test_getImplicitConversion
        (env,
         args->nth(0)->getSpecial(env.lang),     // is it special?
         args->nth(0)->getType(),                // source type
         args->nth(1)->getType(),                // dest type
         expectKind, expectSCS, expectUserLine, expectSCS2);   // expected result
    }
    else {
      env.error("invalid call to __getImplicitConversion");
    }
  }

  // test overload resolution
  if (funcName == env.special_testOverload) {
    int expectLine;
    if (args->count() == 2 &&
        args->nth(1)->constEval(env, expectLine)) {

      if (args->first()->expr->isE_funCall() &&
          hasNamedFunction(args->first()->expr->asE_funCall()->func)) {
        // resolution yielded a function call
        Variable *chosen = getNamedFunction(args->first()->expr->asE_funCall()->func);
        int actualLine = sourceLocManager->getLine(chosen->loc);
        if (expectLine != actualLine) {
          env.error(stringc
            << "expected overload to choose function on line "
            << expectLine << ", but it chose line " << actualLine);
        }
      }
      else if (expectLine != 0) {
        // resolution yielded something else
        env.error("expected overload to choose a function, but it "
                  "chose a non-function");
      }

      // propagate return type
      return args->first()->getType();
    }
    else {
      env.error("invalid call to __testOverload");
    }
  }

  // test vector for 'computeLUB'
  if (funcName == env.special_computeLUB) {
    int expect;
    if (args->count() == 4 &&
        args->nth(3)->constEval(env, expect)) {
      test_computeLUB
        (env,
         args->nth(0)->getType(),        // T1
         args->nth(1)->getType(),        // T2
         args->nth(2)->getType(),        // LUB
         expect);                        // expected result
    }
    else {
      env.error("invalid call to __computeLUB");
    }
  }

  // given a call site, check that the function it calls is
  // (a) defined, and (b) defined at a particular line
  if (funcName == env.special_checkCalleeDefnLine) {
    int expectLine;
    if (args->count() == 2 &&
        args->nth(1)->constEval(env, expectLine)) {

      if (args->first()->expr->isE_funCall() &&
          hasNamedFunction(args->first()->expr->asE_funCall()->func)) {
        // resolution yielded a function call
        Variable *chosen = getNamedFunction(args->first()->expr->asE_funCall()->func);
        if (!chosen->funcDefn) {
          env.error("expected to be calling a defined function");
        }
        else {
          int actualLine = sourceLocManager->getLine(chosen->funcDefn->getLoc());
          if (expectLine != actualLine) {
            env.error(stringc
              << "expected to call function on line "
              << expectLine << ", but it chose line " << actualLine);
          }
        }
      }
      else if (expectLine != 0) {
        // resolution yielded something else
        env.error("expected overload to choose a function, but it "
                  "chose a non-function");
      }

      // propagate return type
      return args->first()->getType();
    }
    else {
      env.error("invalid call to __testOverload");
    }
  }

  // E_funCall::itcheck should continue, and tcheck this normally
  return NULL;
}


Type *E_constructor::itcheck_x(Env &env, Expression *&replacement)
{
  inner1_itcheck(env);

  return inner2_itcheck(env, replacement);
}

void E_constructor::inner1_itcheck(Env &env)
{
  type = spec->tcheck(env, DF_NONE);
}

Type *E_constructor::inner2_itcheck(Env &env, Expression *&replacement)
{
  xassert(replacement == this);

  // inner1_itcheck sets the type, so if it signaled an error then bail
  if (type->isError()) {
    return type;
  }

  // simplify some gratuitous uses of E_constructor
  if (!type->isLikeCompoundType() && !type->isDependent()) {
    // you can make a temporary for an int like this (from
    // in/t0014.cc)
    //   x = int(6);
    // or you can use a typedef to get any other type like this (from
    // t0059.cc)
    //   typedef char* char_ptr;
    //   typedef unsigned long ulong;
    //   return ulong(char_ptr(mFragmentIdentifier)-char_ptr(0));
    // in those cases, there isn't really any ctor to call, so just
    // turn it into a cast

    // there had better be exactly one argument to this ctor
    //
    // oops.. actually there can be zero; "int()" is valid syntax,
    // yielding an integer with indeterminate value
    if (args->count() > 1) {
      return env.error(stringc
        << "function-style cast to `" << type->toString()
        << "' must have zere or one argument (not " 
        << args->count() << ")");
    }
    
    // change it into a cast; this code used to do some 'buildASTTypeId'
    // contortions, but that's silly since the E_constructor already
    // carries a perfectly good type specifier ('this->spec'), and so
    // all we need to do is add an empty declarator
    ASTTypeId *typeSyntax = new ASTTypeId(this->spec,
      new Declarator(new D_name(this->spec->loc, NULL /*name*/), NULL /*init*/));
    if (args->count() == 1) {
      replacement =
        new E_cast(typeSyntax, args->first()->expr);
    }
    else {   /* zero args */
      // The correct semantics (e.g. from a verification point of
      // view) would be to yield a value about which nothing is known,
      // but there is no simple way to do that in the existing AST.  I
      // just want to hack past this for now, since I think it will be
      // very unlikely to cause a real problem, so my solution is to
      // pretend it's always the value 0.
      replacement =
        new E_cast(typeSyntax, env.buildIntegerLiteralExp(0));
    }
    replacement->tcheck(env, replacement);
    return replacement->type;
  }

  // check arguments
  args = tcheckArgExprList(args, env);

  if (!env.ensureCompleteType("construct", type)) {
    return type;     // recovery: skip what follows
  }

  Variable *ctor = outerResolveOverload_ctor(env, env.loc(), type, args,
                                             env.doOverload());
  ctorVar = env.storeVar(ctor);
  env.ensureFuncBodyTChecked(ctor);

  // make sure the argument types are compatible
  // with the constructor parameters
  if (ctor) {
    compareArgsToParams(env, ctor->type->asFunctionType(), args);
  }

  return type;
}


// cppstd sections: 5.2.5 and 3.4.5
Type *E_fieldAcc::itcheck_x(Env &env, Expression *&replacement)
{
  return itcheck_fieldAcc(env, LF_NONE);
}

Type *E_fieldAcc::itcheck_fieldAcc(Env &env, LookupFlags flags)
{
  obj->tcheck(env, obj);

  // Note: must delay tchecking 'fieldName' until we decide
  // what scope to do its qualifier lookups in

  bool isPseudoDestructor = fieldName->getName()[0] == '~';

  // dependent?
  if (obj->type->containsGeneralizedDependent()) {
    if (isPseudoDestructor) {
      // 14.6.2.2 para 4
      return env.getSimpleType(SL_UNKNOWN, ST_VOID);
    }
    else {
      // 14.6.2.2 para 1
      return env.dependentType();
    }
  }

  // get the type of 'obj', and make sure it's a compound
  Type *rt = obj->type->asRval();
  CompoundType *ct = rt->ifCompoundType();
  if (!ct) {
    // strange case, just lookup qualifiers globally
    fieldName->tcheck(env);

    // maybe it's a type variable because we're accessing a field
    // of a template parameter?
    if (rt->isTypeVariable()) {
      // call it a reference to the 'dependent' variable instead of
      // leaving it NULL; this helps the Cqual++ type checker a little
      field = env.dependentTypeVar;
      return field->type;
    }

    if (isPseudoDestructor) {
      // invoking destructor explicitly, which is allowed for all
      // types; most of the time, the rewrite in E_funCall::itcheck
      // will replace this, but in the case of a type which is an
      // array of objects, this will leave the E_fieldAcc's 'field'
      // member NULL ...
      return env.makeDestructorFunctionType(SL_UNKNOWN, NULL /*ct*/);
    }

    // TODO: 3.4.5 para 2 suggests 'rt' might be a pointer to scalar
    // type... but how could that possibly be legal?

    return env.error(rt, stringc
      << "non-compound `" << rt->toString()
      << "' doesn't have fields to access");
  }

  // make sure the type has been completed
  if (!env.ensureCompleteType("access a member of", rt)) {
    return env.errorType();
  }

  // look for the named field [cppstd 3.4.5]
  Variable *f;
  if (!fieldName->hasQualifiers()) {
    // lookup in class only [cppstd 3.4.5 para 2]
    fieldName->tcheck(env, ct);
    f = ct->lookupPQVariable(fieldName, env, flags);
  }
  else {
    PQ_qualifier *firstQ = fieldName->asPQ_qualifier();
    if (!firstQ->qualifier) {      // empty first qualifier
      // lookup in complete scope only [cppstd 3.4.5 para 5]
      fieldName->tcheck(env);
      f = env.lookupPQVariable(fieldName, flags);
    }
    else {
      // lookup in both scopes [cppstd 3.4.5 para 4]

      // class scope
      fieldName->tcheck(env, ct, LF_SUPPRESS_ERROR);
      f = ct->lookupPQVariable(fieldName, env, flags);

      // global scope
      fieldName->tcheck(env, NULL, LF_SUPPRESS_ERROR);
      Variable *globalF = env.lookupPQVariable(fieldName, flags);

      if (f && globalF) {
        // must both refer to same entity
        if (f != globalF) {
          // unfortunately, a lookup bug elsewhere prevents this message
          // from being reached even when it should be, e.g. in/t0330.cc
          return env.error(stringc
            << "in qualified member lookup of `" << *fieldName
            << "', lookup in class scope found entity at " << f->loc
            << " but lookup in global scope found entity at " << globalF->loc
            << "; they must lookup to the same entity");
        }
        
        // Note: cppstd requires that 'f' and 'globalF' name the same
        // entity, but allows the intermediate qualifiers to be
        // different.  That means that the scope annotations on
        // 'fieldName' are in essence undefined.  I choose to leave
        // them as they are now, corresponding to the (successful)
        // lookup in the global scope.
      }
      else if (!f) {
        f = globalF;        // keep whichever is not NULL, if either
      }
      else if (!globalF) {
        // 'f' was good but 'globalF' not; re-tcheck 'fieldName' in
        // the scope that worked, so its scope annotations are right
        fieldName->tcheck(env, ct);
      }
    }

    // if not NULL, 'f' should refer to a member of class 'ct'
    if (f) {
      if (!( f->scope &&
             f->scope->curCompound &&
             ct->hasBaseClass(f->scope->curCompound) )) {
        return env.error(f->type, stringc
          << "in qualified member lookup of `" << *fieldName
          << "', the found entity is not a member of " << ct->name);
      }
    }
  }

  if (!f) {
    return env.error(rt, stringc
      << "there is no member called `" << *fieldName
      << "' in " << rt->toString());
  }

  // should not be a type
  if (f->hasFlag(DF_TYPEDEF)) {
    return env.error(rt, stringc
      << "member `" << *fieldName << "' is a typedef!");
  }
       
  // TODO: access control check

  if (!(flags & LF_TEMPL_PRIMARY)) {
    env.ensureFuncBodyTChecked(f);
  }
  field = env.storeVarIfNotOvl(f);

  // type of expression is type of field; possibly as an lval
  if (obj->type->isLval() &&
      !field->type->isFunctionType()) {
    return makeLvalType(env, field->type);
  }
  else {
    return field->type;
  }
}


Type *E_arrow::itcheck_x(Env &env, Expression *&replacement)
{   
  // check LHS
  obj->tcheck(env, obj);

  // overloaded?  if so, replace 'obj'
  Type *t = obj->type;
  while (t && !t->isDependent()) {
    t = resolveOverloadedUnaryOperator(env, obj, obj, OP_ARROW);
    // keep sticking in 'operator->' until the LHS is not a class
  }
  
  // now replace with '*' and '.' and proceed
  replacement = new E_fieldAcc(new E_deref(obj), fieldName);
  replacement->tcheck(env, replacement);
  return replacement->type;
}


Type *E_sizeof::itcheck_x(Env &env, Expression *&replacement)
{
  expr->tcheck(env, expr);

  try {
    size = expr->type->asRval()->reprSize();
    TRACE("sizeof", "sizeof(" << expr->exprToString() <<
                    ") is " << size);
  }
  catch (XReprSize &e) {
    // dsw: You are allowed to take the size of an array that has dynamic
    // size; FIX: is this the right place to handle it?  Perhaps
    // ArrayType::reprSize() would be better.
    //
    // sm: This seems like an ok place to me.  
    if (expr->type->asRval()->isArrayType()
        && expr->type->asRval()->asArrayType()->size == ArrayType::DYN_SIZE) {
      // sm: don't do this; 'size' is interpreted as an integer and
      // no one expects to interpret it as an ArrayType::size, so this
      // just sets the size to -2!
      //size = ArrayType::DYN_SIZE;
      env.warning("taking the sizeof a dynamically-sized array");
      TRACE("sizeof", "sizeof(" << expr->exprToString() <<
                      ") is dynamic..");
    } else {
      return env.error(e.why());  // jump out with an error
    }
  }

  // TODO: is this right?
  return expr->type->isError()?
           expr->type : env.getSimpleType(SL_UNKNOWN, ST_UNSIGNED_INT);
}


// do operator overload resolution for a unary operator; return
// non-NULL if we've replaced this node, and want the caller to
// return that value
Type *resolveOverloadedUnaryOperator(
  Env &env,                  // environment
  Expression *&replacement,  // OUT: replacement node
  //Expression *ths,           // expression node that is being resolved (not used)
  Expression *expr,          // the subexpression of 'this' (already type-checked)
  OverloadableOp op          // which operator this node is
) {
  // 14.6.2.2 para 1
  if (expr->type->containsGeneralizedDependent()) {
    return env.dependentType();
  }

  if (!env.doOperatorOverload()) {
    return NULL;
  }

  if (expr->type->asRval()->isCompoundType() ||
      expr->type->asRval()->isEnumType()) {
    OVERLOADINDTRACE("found overloadable unary " << toString(op) <<
                     " near " << env.locStr());
    StringRef opName = env.operatorName[op];

    // argument information
    GrowArray<ArgumentInfo> args(1);
    getArgumentInfo(env, args[0], expr);

    // prepare resolver
    OverloadResolver resolver(env, env.loc(), &env.errors,
                              OF_NONE,
                              NULL, // I assume operators can't have explicit template arguments
                              args);

    // user-defined candidates
    resolver.addUserOperatorCandidates(expr->type, NULL /*rhsType*/, opName);

    // built-in candidates
    resolver.addBuiltinUnaryCandidates(op);

    // pick the best candidate
    bool dummy;
    Candidate const *winnerCand = resolver.resolveCandidate(dummy);
    if (winnerCand) {
      Variable *winner = winnerCand->var;

      if (!winner->hasFlag(DF_BUILTIN)) {
        OperatorName *oname = new ON_operator(op);
        PQ_operator *pqo = new PQ_operator(SL_UNKNOWN, oname, opName);

        if (winner->hasFlag(DF_MEMBER)) {
          // replace '~a' with 'a.operator~()'
          replacement = new E_funCall(
            new E_fieldAcc(expr, pqo),               // function
            FakeList<ArgExpression>::emptyList()     // arguments
          );
        }
        else {
          // replace '~a' with '<scope>::operator~(a)'
          replacement = new E_funCall(
            // function to invoke
            new E_variable(env.makeFullyQualifiedName(winner->scope, pqo)),
            // arguments
            makeExprList1(expr)
          );
        }

        // for now, just re-check the whole thing
        replacement->tcheck(env, replacement);
        return replacement->type;
      }

      else {
        // chose a built-in operator

        // TODO: need to replace the arguments according to their
        // conversions (if any)

        // get the correct return value, at least
        Type *ret = resolver.getReturnType(winnerCand);
        OVERLOADINDTRACE("computed built-in operator return type `" <<
                         ret->toString() << "'");

        return ret;
      }
    }
  }

  // not replaced
  return NULL;
}


// similar, but for binary
Type *resolveOverloadedBinaryOperator(
  Env &env,                  // environment
  Expression *&replacement,  // OUT: replacement node
  //Expression *ths,           // expression node that is being resolved (not used)
  Expression *e1,            // left subexpression of 'this' (already type-checked)
  Expression *e2,            // right subexpression, or NULL for postfix inc/dec
  OverloadableOp op          // which operator this node is
) {
  // 14.6.2.2 para 1
  if (e1->type->containsGeneralizedDependent() ||
      e2 && e2->type->containsGeneralizedDependent()) {
    return env.dependentType();
  }

  if (!env.doOperatorOverload()) {
    return NULL;
  }

  // check for operator overloading
  if (e1->type->asRval()->isCompoundType() ||
      e1->type->asRval()->isEnumType() ||
      e2 && e2->type->asRval()->isCompoundType() ||
      e2 && e2->type->asRval()->isEnumType()) {
    OVERLOADINDTRACE("found overloadable binary " << toString(op) <<
                     " near " << env.locStr());
    StringRef opName = env.operatorName[op];

    // collect argument information
    GrowArray<ArgumentInfo> args(2);
    getArgumentInfo(env, args[0], e1);
    if (e2) {
      getArgumentInfo(env, args[1], e2);
    }
    else {
      // for postfix inc/dec, the second parameter is 'int'
      args[1] = ArgumentInfo(SE_NONE, env.getSimpleType(SL_UNKNOWN, ST_INT));
    }

    // prepare the overload resolver
    OverloadResolver resolver(env, env.loc(), &env.errors,
                              OF_NONE,
                              NULL, // I assume operators can't have explicit template arguments
                              args, 10 /*numCand*/);
    if (op == OP_COMMA) {
      // 13.3.1.2 para 9: no viable -> use built-in
      resolver.emptyCandidatesIsOk = true;
    }

    // user-defined candidates
    resolver.addUserOperatorCandidates(args[0].type, args[1].type, opName);

    // built-in candidates
    resolver.addBuiltinBinaryCandidates(op, args[0], args[1]);

    // pick one
    bool dummy;
    Candidate const *winnerCand = resolver.resolveCandidate(dummy);
    if (winnerCand) {
      Variable *winner = winnerCand->var;

      if (!e2) {
        // synthesize and tcheck a 0 for the second argument to postfix inc/dec
        e2 = new E_intLit(env.str("0"));
        e2->tcheck(env, e2);
      }

      if (!winner->hasFlag(DF_BUILTIN)) {
        PQ_operator *pqo = new PQ_operator(SL_UNKNOWN, new ON_operator(op), opName);
        if (winner->hasFlag(DF_MEMBER)) {
          // replace 'a+b' with 'a.operator+(b)'
          replacement = new E_funCall(
            // function to invoke
            new E_fieldAcc(e1, pqo),
            // arguments
            makeExprList1(e2)
          );
        }
        else {
          // replace 'a+b' with '<scope>::operator+(a,b)'
          replacement = new E_funCall(
            // function to invoke
            new E_variable(env.makeFullyQualifiedName(winner->scope, pqo)),
            // arguments
            makeExprList2(e1, e2)
          );
        }

        // for now, just re-check the whole thing
        replacement->tcheck(env, replacement);
        return replacement->type;
      }

      else {
        // chose a built-in operator

        // TODO: need to replace the arguments according to their
        // conversions (if any)

        if (op == OP_BRACKETS) {
          // just let the calling code replace it with * and +; that
          // should get the right type automatically
          return NULL;
        }
        else {
          // get the correct return value, at least
          Type *ret = resolver.getReturnType(winnerCand);
          OVERLOADINDTRACE("computed built-in operator return type `" <<
                           ret->toString() << "'");
                           
          // I'm surprised that Oink doesn't complain about this not
          // being cloned... but since it appears unnecessary, there is
          // no point, so I will leave it un-cloned.
          return ret;
        }
      }
    }
  }

  // not replaced
  return NULL;
}


bool isArithmeticOrEnumType(Type *t)
{
  if (t->isSimpleType()) {
    return isArithmeticType(t->asSimpleTypeC()->type);
  }
  return t->isEnumType();
}


Type *E_unary::itcheck_x(Env &env, Expression *&replacement)
{
  expr->tcheck(env, expr);

  // consider the possibility of operator overloading
  Type *ovlRet = resolveOverloadedUnaryOperator(
    env, replacement, /*this,*/ expr, toOverloadableOp(op));
  if (ovlRet) {
    return ovlRet;
  }

  Type *t = expr->type->asRval();

  // make sure 'expr' is compatible with given operator
  switch (op) {
    default:
      xfailure("bad operator kind");

    case UNY_PLUS:
      // 5.3.1 para 6
      if (isArithmeticOrEnumType(t)) {
        return env.getSimpleType(SL_UNKNOWN, applyIntegralPromotions(t));
      }
      if (t->isPointerType()) {
        return t;
      }
      return env.error(t, stringc
        << "argument to unary + must be of arithmetic, enumeration, or pointer type, not `"
        << t->toString() << "'");

    case UNY_MINUS:
      // 5.3.1 para 7
      if (isArithmeticOrEnumType(t)) {
        return env.getSimpleType(SL_UNKNOWN, applyIntegralPromotions(t));
      }
      return env.error(t, stringc
        << "argument to unary - must be of arithmetic or enumeration type, not `"
        << t->toString() << "'");

    case UNY_NOT: {
      // 5.3.1 para 8
      Type *t_bool = env.getSimpleType(SL_UNKNOWN, ST_BOOL);
      if (!getImplicitConversion(env, expr->getSpecial(env.lang), t, t_bool)) {
        env.error(t, stringc
          << "argument to unary ! must be convertible to bool; `"
          << t->toString() << "' is not");
      }
      return t_bool;
    }

    case UNY_BITNOT:
      // 5.3.1 para 9
      if (t->isIntegerType() || t->isEnumType()) {
        return env.getSimpleType(SL_UNKNOWN, applyIntegralPromotions(t));
      }
      return env.error(t, stringc
        << "argument to unary ~ must be of integer or enumeration type, not `"
        << t->toString() << "'");
        
      // 5.3.1 para 9 also mentions an ambiguity with "~X()", which I
      // tried to exercise in in/t0343.cc, but I can't seem to get it;
      // so either Elsa already does what is necessary, or I do not
      // understand the syntax adequately ....
  }
}


Type *E_effect::itcheck_x(Env &env, Expression *&replacement)
{
  expr->tcheck(env, expr);

  // consider the possibility of operator overloading
  Type *ovlRet = isPrefix(op)?
    resolveOverloadedUnaryOperator(
      env, replacement, /*this,*/ expr, toOverloadableOp(op)) :
    resolveOverloadedBinaryOperator(
      env, replacement, /*this,*/ expr, NULL, toOverloadableOp(op)) ;
  if (ovlRet) {
    return ovlRet;
  }

  // TODO: make sure 'expr' is compatible with given operator

  // dsw: FIX: check that the argument is an lvalue.

  // Cppstd 5.2.6 "Increment and decrement: The value obtained by
  // applying a postfix ++ ... .  The result is an rvalue."; Cppstd
  // 5.3.2 "Increment and decrement: The operand of prefix ++ ... .
  // The value ... is an lvalue."
  Type *ret = expr->type->asRval();
  if (op == EFF_PREINC || op == EFF_PREDEC) {
    ret = env.makeReferenceType(env.loc(), ret);
  }
  return ret;
}


Type *E_binary::itcheck_x(Env &env, Expression *&replacement)
{
  e1->tcheck(env, e1);
  e2->tcheck(env, e2);

  // help disambiguate t0182.cc
  //
  // This is not exactly the right test.  The right thing to do is
  // check for 'e1' being a name which refers to a template function,
  // and then reject.  However, that is slightly more complicated
  // (must consider both E_variable and E_fieldAcc), and I can't think
  // of any test that would reveal the difference.
  if (op == BIN_LESS && e1->type->isFunctionType()) {
    return env.error("cannot apply '<' to a function", EF_DISAMBIGUATES);
  }

  // check for operator overloading
  if (isOverloadable(op)) {
    Type *ovlRet = resolveOverloadedBinaryOperator(
      env, replacement, /*this,*/ e1, e2, toOverloadableOp(op));
    if (ovlRet) {
      return ovlRet;
    }
  }

  // TODO: much of what follows is obviated by the fact that
  // 'resolveOverloadedBinaryOperator' now returns non-NULL for
  // built-in operator candidates ...

  if (op == BIN_BRACKETS) {
    // built-in a[b] is equivalent to *(a+b)
    replacement = new E_deref(new E_binary(e1, BIN_PLUS, e2));
    replacement->tcheck(env, replacement);
    return replacement->type;
  }

  // get types of arguments, converted to rval
  Type *lhsType = env.operandRval(e1->type);
  Type *rhsType = env.operandRval(e2->type);
  
  // if the LHS is an array, coerce it to a pointer
  if (lhsType->isArrayType()) {
    lhsType = env.makePtrType(SL_UNKNOWN, lhsType->asArrayType()->eltType);
  }

  switch (op) {
    default: xfailure("illegal op code"); break;

    case BIN_EQUAL:               // ==
    case BIN_NOTEQUAL:            // !=
    case BIN_LESS:                // <
    case BIN_GREATER:             // >
    case BIN_LESSEQ:              // <=
    case BIN_GREATEREQ:           // >=

    case BIN_AND:                 // &&
    case BIN_OR:                  // ||
    case BIN_IMPLIES:             // ==>
    case BIN_EQUIVALENT:          // <==>
      return env.getSimpleType(SL_UNKNOWN, ST_BOOL);

    case BIN_COMMA:
      // dsw: I changed this to allow the following: &(3, a);
      return e2->type/*rhsType*/;

    case BIN_PLUS:                // +
      // dsw: deal with pointer arithmetic correctly; Note that the case
      // p + 1 is handled correctly by the default behavior; this is the
      // case 1 + p.
      if (lhsType->isIntegerType() && rhsType->isPointerType()) {
        return rhsType;         // a pointer type, that is
      }
      // default behavior of returning the left side is close enough for now.
      break;

    case BIN_MINUS:               // -
      // dsw: deal with pointer arithmetic correctly; this is the case
      // p1 - p2
      if (lhsType->isPointerType() && rhsType->isPointerType() ) {
        return env.getSimpleType(SL_UNKNOWN, ST_INT);
      }
      // default behavior of returning the left side is close enough for now.
      break;

    case BIN_MULT:                // *
    case BIN_DIV:                 // /
    case BIN_MOD:                 // %
    case BIN_LSHIFT:              // <<
    case BIN_RSHIFT:              // >>
    case BIN_BITAND:              // &
    case BIN_BITXOR:              // ^
    case BIN_BITOR:               // |
      // default behavior of returning the left side is close enough for now.
      break;

    // BIN_ASSIGN can't appear in E_binary

    case BIN_DOT_STAR:            // .*
    case BIN_ARROW_STAR:          // ->*
      // [cppstd 5.5]
      if (op == BIN_ARROW_STAR) {
        // left side should be a pointer to a class
        if (!lhsType->isPointer()) {
          return env.error(stringc
            << "left side of ->* must be a pointer, not `"
            << lhsType->toString() << "'");
        }
        lhsType = lhsType->asPointerType()->atType;
      }

      // left side should be a class
      CompoundType *lhsClass = lhsType->ifCompoundType();
      if (!lhsClass) {
        return env.error(op==BIN_DOT_STAR?
          "left side of .* must be a class or reference to a class" :
          "left side of ->* must be a pointer to a class");
      }

      // right side should be a pointer to a member
      if (!rhsType->isPointerToMemberType()) {
        return env.error("right side of .* or ->* must be a pointer-to-member");
      }
      PointerToMemberType *ptm = rhsType->asPointerToMemberType();

      // actual LHS class must be 'ptm->inClass()', or a
      // class unambiguously derived from it
      int subobjs = lhsClass->countBaseClassSubobjects(ptm->inClass());
      if (subobjs == 0) {
        return env.error(stringc
          << "the left side of .* or ->* has type `" << lhsClass->name
          << "', but this is not equal to or derived from `" << ptm->inClass()->name
          << "', the class whose members the right side can point at");
      }
      else if (subobjs > 1) {
        return env.error(stringc
          << "the left side of .* or ->* has type `" << lhsClass->name
          << "', but this is derived from `" << ptm->inClass()->name
          << "' ambiguously (in more than one way)");
      }

      // the return type is essentially the 'atType' of 'ptm'
      Type *ret = ptm->atType;

      // 8.3.3 para 3: "A pointer to member shall not point to ... a
      // member with reference type."  Scott says this can't happen
      // here.
      xassert(!ret->isReference());

      // [cppstd 5.5 para 6]
      // but it might be an lvalue if it is a pointer to a data
      // member, and either
      //   - op==BIN_ARROW_STAR, or
      //   - op==BIN_DOT_STAR and 'e1->type' is an lvalue
      if (op==BIN_ARROW_STAR ||
          /*must be DOT_STAR*/ e1->type->isLval()) {
        // this internally handles 'ret' being a function type
        ret = makeLvalType(env, ret);
      }

      return ret;
  }

  // TODO: make sure 'expr' is compatible with given operator

  return lhsType;
}


// someone took the address of 'e_var', and we must compute
// the PointerToMemberType of that construct
static Type *makePTMType(Env &env, E_variable *e_var)
{
  // shouldn't even get here unless e_var->name is qualified
  xassert(e_var->name->hasQualifiers());

  // dsw: It is inelegant to recompute the var here, but I don't want
  // to just ignore the typechecking that already computed a type for
  // the expr and use the var exclusively, which is what would happen
  // if I just passed in the var.
  Variable *var0 = e_var->var;
  xassert(var0);
  xassert(var0->scope);

  // cppstd: 8.3.3 para 3, can't be static
  xassert(!var0->hasFlag(DF_STATIC));
  
  // this is essentially a consequence of not being static
  if (e_var->type->asRval()->isFunctionType()) {
    xassert(e_var->type->asRval()->asFunctionType()->isMethod());
  }

  // cppstd: 8.3.3 para 3, can't be cv void
  if (e_var->type->isVoid()) {
    return env.error(var0->loc, "attempted to make a pointer to member to void");
  }
  // cppstd: 8.3.3 para 3, can't be a reference;
  // NOTE: This does *not* say e_var->type->isReference(), since an
  // E_variable expression will have reference type when the variable
  // itself is not a reference.
  if (var0->type->isReference()) {
    return env.error(var0->loc, "attempted to make a pointer to member to a reference");
  }

  CompoundType *inClass0 = var0->scope->curCompound;
  xassert(inClass0);

  return env.tfac.makePointerToMemberType
    (SL_UNKNOWN, inClass0, CV_NONE, e_var->type->asRval());
}

Type *E_addrOf::itcheck_x(Env &env, Expression *&replacement)
{
  // might we be forming a pointer-to-member?
  bool possiblePTM = false;
  E_variable *evar = NULL;
  // NOTE: do *not* unwrap any layers of parens:
  // cppstd 5.3.1 para 3: "A pointer to member is only formed when
  // an explicit & is used and its operand is a qualified-id not
  // enclosed in parentheses."
  if (expr->isE_variable()) {
    evar = expr->asE_variable();

    // cppstd 5.3.1 para 3: Nor is &unqualified-id a pointer to
    // member, even within the scope of the unqualified-id's
    // class.
    // dsw: Consider the following situation: How do you know you
    // &x isn't making a pointer to member?  Only because the name
    // isn't fully qualified.
    //   struct A {
    //     int x;
    //     void f() {int *y = &x;}
    //   };
    if (evar->name->hasQualifiers()) {
      possiblePTM = true;
    }
  }

  // check 'expr'
  if (possiblePTM) {
    // suppress elaboration of 'this'; the situation where 'this'
    // would be inserted is exactly the situation where a
    // pointer-to-member is formed
    Type *t = evar->itcheck_var(env, expr, LF_NO_IMPL_THIS);
    evar->type = env.tfac.cloneType(t);
    xassert(evar == expr);     // do not expect replacement here
  }
  else {
    expr->tcheck(env, expr);
  }

  if (expr->type->isError()) {
    // skip further checking because the tree is not necessarily
    // as the later stages expect; e.g., an E_variable might have
    // a NULL 'var' field
    return expr->type;
  }

  // 14.6.2.2 para 1
  if (expr->type->isGeneralizedDependent()) {
    return env.dependentType();
  }

  if (possiblePTM) {
    xassert(evar->var);

    // make sure we instantiate any functions that have their address
    // taken
    env.ensureFuncBodyTChecked(evar->var);

    if (evar->var->isMember() &&
        !evar->var->isStatic()) {
      return makePTMType(env, evar);
    }
  }
  // Continuing, the same paragraph points out that we are correct in
  // creating a pointer to member only when the address-of operator
  // ("&") is explicit:
  // cppstd 5.3.1 para 3: Neither does qualified-id, because there is no
  // implicit conversion from a qualified-id for a nonstatic member
  // function the the type "pointer to member function" as there is
  // from an lvalue of a function type to the type "pointer to
  // function" (4.3).

  // ok to take addr of function; special-case it so as not to weaken
  // what 'isLval' means
  if (expr->type->isFunctionType()) {
    return env.makePtrType(SL_UNKNOWN, expr->type);
  }

  if (!expr->type->isLval()) {
    return env.error(expr->type, stringc
      << "cannot take address of non-lvalue `" 
      << expr->type->toString() << "'");
  }
  ReferenceType *rt = expr->type->asReferenceType();

  // change the "&" into a "*"
  return env.makePtrType(SL_UNKNOWN, rt->atType);
}


Type *E_deref::itcheck_x(Env &env, Expression *&replacement)
{
  ptr->tcheck(env, ptr);

  // check for overloading
  {
    Type *ovlRet = resolveOverloadedUnaryOperator(
      env, replacement, /*this,*/ ptr, OP_STAR);
    if (ovlRet) {
      return ovlRet;
    }
  }

  // clone the type as it's taken out of one AST node, so it
  // can then be used as the type of another AST node (this one)
  Type *rt = ptr->type->asRval();

  if (rt->isFunctionType()) {
    return rt;                         // deref is idempotent on FunctionType-s
  }

  if (rt->isPointerType()) {
    PointerType *pt = rt->asPointerType();

    // dereferencing yields an lvalue
    return makeLvalType(env, pt->atType);
  }

  // implicit coercion of array to pointer for dereferencing
  if (rt->isArrayType()) {
    return makeLvalType(env, rt->asArrayType()->eltType);
  }

  // check for "operator*" (and "operator[]" since I currently map
  // 'x[y]' into '*(x+y)')
  if (rt->isCompoundType()) {
    CompoundType *ct = rt->asCompoundType();
    if (ct->lookupVariable(env.operatorName[OP_STAR], env)) {
      // replace this Expression node with one that looks like
      // an explicit call to the overloaded operator*
      replacement = new E_funCall(
        // function: ptr.operator*
        new E_fieldAcc(ptr, new PQ_operator(SL_UNKNOWN, new ON_operator(OP_STAR),
                                            env.operatorName[OP_STAR])),
        // arguments: ()
        FakeList<ArgExpression>::emptyList()
      );

      // now, tcheck this new Expression
      replacement->tcheck(env, replacement);
      return replacement->type;
    }

    // this is an older hack..
    if (ct->lookupVariable(env.operatorName[OP_BRACKETS], env)) {
      // ok.. gee what type?  would have to do the full deal, and
      // would likely get it wrong for operator[] since I don't have
      // the right info to do an overload calculation.. well, if I
      // make it ST_ERROR then that will suppress further complaints
      return env.getSimpleType(SL_UNKNOWN, ST_ERROR);    // TODO: fix this!
    }
  }

  if (env.lang.complainUponBadDeref) {
    // TODO: "dereference" is misspelled; not fixing right now since
    // we're in the middle of binning errors
    return env.error(rt, stringc
      << "cannot derefence non-pointer `" << rt->toString() << "'");
  }
  else {
    // unfortunately, I get easily fooled by overloaded functions and
    // end up concluding the wrong types.. so I'm simply going to turn
    // off the error message for now..
    return env.getSimpleType(SL_UNKNOWN, ST_ERROR);
  }
}

Type *E_cast::itcheck_x(Env &env, Expression *&replacement)
{
  ASTTypeId::Tcheck tc(DF_NONE, DC_E_CAST);
  ctype = ctype->tcheck(env, tc);
  expr->tcheck(env, expr);
  
  // TODO: check that the cast makes sense

  Type *ret = ctype->getType();

  // This is a gnu extension: in C mode, if the expr is an lvalue,
  // make the returned type an lvalue.  This is in direct
  // contradiction to the C99 spec: Section 6.5.4, footnote 85: "A
  // cast does not yield an lvalue".
  // http://gcc.gnu.org/onlinedocs/gcc-3.1/gcc/Lvalues.html
  if (env.lang.lvalueFlowsThroughCast) {
    if (expr->getType()->isReference() && !ret->isReference()) {
      ret = env.makeReferenceType(env.loc(), ret);
    }
  }

  return ret;
}


// try to convert t1 to t2 as per the rules in 5.16 para 3; return
// NULL if conversion is not possible, otherwise return type to which
// 't1' is converted (it is not always exactly 't2') and set 'ic'
// accordingly
Type *attemptCondConversion(Env &env, ImplicitConversion &ic /*OUT*/,
                            Type *t1, Type *t2)
{
  // bullet 1: attempt direct conversion to lvalue
  if (t2->isLval()) {
    ic = getImplicitConversion(env, SE_NONE, t1, t2);
    if (ic) {
      return t2;
    }
  }

  // bullet 2
  CompoundType *t1Class = t1->asRval()->ifCompoundType();
  CompoundType *t2Class = t2->asRval()->ifCompoundType();
  if (t1Class && t2Class &&
      (t1Class->hasBaseClass(t2Class) ||
       t2Class->hasBaseClass(t1Class))) {
    // bullet 2.1
    if (t1Class->hasBaseClass(t2Class) &&
        t2->asRval()->getCVFlags() >= t1->asRval()->getCVFlags()) {
      ic.addStdConv(SC_IDENTITY);
      return t2->asRval();
    }
    else {
      return NULL;
    }
  }
  else {
    // bullet 2.2
    t2 = t2->asRval();
    ic = getImplicitConversion(env, SE_NONE, t1, t2);
    if (ic) {
      return t2;
    }
  }

  return NULL;
}


// "array-to-pointer (4.2) and function-to-pointer (4.3) standard
// conversions"; (for the moment) functionally identical to
// 'normalizeParameterType' but specified by a different part of
// cppstd...
Type *arrAndFuncToPtr(Env &env, Type *t)
{
  if (t->isArrayType()) {
    return env.makePtrType(SL_UNKNOWN, t->asArrayType()->eltType);
  }
  if (t->isFunctionType()) {
    return env.makePtrType(SL_UNKNOWN, t);
  }
  return t;
}


// cppstd 5.9 "composite pointer type" computation
Type *compositePointerType
  (Env &env, PointerType *t1, PointerType *t2)
{
  // if either is pointer to void, result is void
  if (t1->atType->isVoid() || t2->atType->isVoid()) {
    CVFlags cv = t1->atType->getCVFlags() | t2->atType->getCVFlags();

    // reuse t1 or t2 if possible
    if (t1->atType->isVoid() && cv == t1->atType->getCVFlags()) {
      return t1;
    }
    if (t2->atType->isVoid() && cv == t2->atType->getCVFlags()) {
      return t2;
    }

    return env.tfac.makePointerType(SL_UNKNOWN, CV_NONE,
      env.tfac.getSimpleType(SL_UNKNOWN, ST_VOID, cv));
  }

  // types must be similar... aw hell.  I don't understand what the
  // standard wants, and my limited understanding is at odds with
  // what both gcc and icc do so.... hammer time
  bool whatev;
  return computeLUB(env, t1, t2, whatev);
}

// cppstd 5.16 computation similar to composite pointer but
// for pointer-to-member
Type *compositePointerToMemberType
  (Env &env, PointerToMemberType *t1, PointerToMemberType *t2)
{
  // hammer time
  bool whatev;
  return computeLUB(env, t1, t2, whatev);
}


// cppstd 5.16
Type *E_cond::itcheck_x(Env &env, Expression *&replacement)
{
  cond->tcheck(env, cond);

  // para 1: 'cond' converted to bool
  if (!cond->type->isGeneralizedDependent() &&
      !getImplicitConversion(env, cond->getSpecial(env.lang), cond->type,
                             env.getSimpleType(SL_UNKNOWN, ST_BOOL))) {
    env.error(cond->type, stringc
      << "cannot convert `" << cond->type->toString() 
      << "' to bool for conditional of ?:");
  }
  // TODO (elaboration): rewrite AST if a user-defined conversion was used

  th->tcheck(env, th);
  el->tcheck(env, el);

  // 14.6.2.2 para 1
  //
  // Note that I'm interpreting 14.6.2.2 literally, to the point of
  // regarding the entire ?: dependent if the condition is, even
  // though the type of the condition cannot affect the type of the ?:
  // expression.
  if (cond->type->isGeneralizedDependent() ||
      th->type->isGeneralizedDependent() ||
      el->type->isGeneralizedDependent()) {
    return env.dependentType();
  }

  // pull out the types; during the processing below, we might change
  // these to implement "converted operand used in place of ..."
  Type *thType = th->type;
  Type *elType = el->type;

  Type *thRval = th->type->asRval();
  Type *elRval = el->type->asRval();

  if (!env.lang.isCplusplus) {
    // ANSI C99 mostly requires that the types be the same, but gcc
    // doesn't seem to enforce anything, so I won't either; and if
    // they are different, it isn't clear what the type should be ...

    bool bothLvals = thType->isLval() && elType->isLval();

    // for in/gnu/d0095.c
    if (thRval->isPointerType() &&
        thRval->asPointerType()->atType->isVoid()) {
      return bothLvals? elType : elRval;
    }

    return bothLvals? thType : thRval;
  }

  // para 2: if one or the other has type 'void'
  {
    bool thVoid = thRval->isSimple(ST_VOID);
    bool elVoid = elRval->isSimple(ST_VOID);
    if (thVoid || elVoid) {
      if (thVoid && elVoid) {
        return thRval;         // result has type 'void'
      }

      // the void-typed expression must be a 'throw' (can it be
      // parenthesized?  a strict reading of the standard would say
      // no..), and the whole ?: has the non-void type
      if (thVoid) {
        if (!th->isE_throw()) {
          env.error("void-typed expression in ?: must be a throw-expression");
        }
        return arrAndFuncToPtr(env, elRval);
      }
      if (elVoid) {
        if (!el->isE_throw()) {
          env.error("void-typed expression in ?: must be a throw-expression");
        }
        return arrAndFuncToPtr(env, thRval);
      }
    }
  }

  // para 3: different types but at least one is a class type
  if (!thRval->equals(elRval) &&
      (thRval->isCompoundType() ||
       elRval->isCompoundType())) {
    // try to convert each to the other
    ImplicitConversion ic_thToEl;
    Type *thConv = attemptCondConversion(env, ic_thToEl, thType, elType);
    ImplicitConversion ic_elToTh;
    Type *elConv = attemptCondConversion(env, ic_elToTh, elType, thType);

    if (thConv && elConv) {
      return env.error("class-valued argument(s) to ?: are ambiguously inter-convertible");
    }

    if (thConv) {
      if (ic_thToEl.isAmbiguous()) {
        return env.error("in ?:, conversion of second arg to type of third is ambiguous");
      }

      // TODO (elaboration): rewrite AST according to 'ic_thToEl'
      thType = thConv;
      thRval = th->type->asRval();
    }

    if (elConv) {
      if (ic_elToTh.isAmbiguous()) {
        return env.error("in ?:, conversion of third arg to type of second is ambiguous");
      }

      // TODO (elaboration): rewrite AST according to 'ic_elToTh'
      elType = elConv;
      elRval = el->type->asRval();
    }
  }

  // para 4: same-type lval -> result is that type
  if (thType->isLval() &&
      elType->isLval() &&
      thType->equals(elType)) {
    return thType;
  }

  // para 5: overload resolution
  if (!thRval->equals(elRval) &&
      (thRval->isCompoundType() || elRval->isCompoundType())) {
    // collect argument info
    GrowArray<ArgumentInfo> args(2);
    args[0].special = th->getSpecial(env.lang);
    args[0].type = thType;
    args[1].special = el->getSpecial(env.lang);
    args[1].type = elType;

    // prepare the overload resolver
    OverloadResolver resolver(env, env.loc(), &env.errors,
                              OF_NONE,
                              NULL, // no template arguments
                              args, 2 /*numCand*/);

    // built-in candidates
    resolver.addBuiltinBinaryCandidates(OP_QUESTION, args[0], args[1]);

    // pick one
    bool dummy;
    Candidate const *winnerCand = resolver.resolveCandidate(dummy);
    if (winnerCand) {
      Variable *winner = winnerCand->var;
      xassert(winner->hasFlag(DF_BUILTIN));

      // TODO (elaboration): need to replace the arguments according
      // to their conversions (if any)

      // get the correct return value, at least
      Type *ret = resolver.getReturnType(winnerCand);
      OVERLOADINDTRACE("computed built-in operator return type `" <<
                       ret->toString() << "'");

      // para 5 ends by saying (in effect) that 'ret' should be used
      // as the new 'thType' and 'elType' and then keep going on to
      // para 6; but I can't see how that would be different from just
      // returning here...
      return ret;
    }
  }
  
  // para 6: final standard conversions (conversion to rvalues has
  // already been done, so use the '*Rval' variables)
  {
    thRval = arrAndFuncToPtr(env, thRval);
    elRval = arrAndFuncToPtr(env, elRval);
    
    // bullet 1
    if (thRval->equals(elRval)) {
      return thRval;
    }
    
    // bullet 2
    if (isArithmeticOrEnumType(thRval) &&
        isArithmeticOrEnumType(elRval)) {
      return usualArithmeticConversions(env.tfac, thRval, elRval);
    }

    // bullet 3
    if (thRval->isPointerType() && el->getSpecial(env.lang) == SE_ZERO) {
      return thRval;
    }
    if (elRval->isPointerType() && th->getSpecial(env.lang) == SE_ZERO) {
      return elRval;
    }
    if (thRval->isPointerType() && elRval->isPointerType()) {
      Type *ret = compositePointerType(env,
        thRval->asPointerType(), elRval->asPointerType());
      if (!ret) { goto incompatible; }
      return ret;
    }

    // bullet 4
    if (thRval->isPointerToMemberType() && el->getSpecial(env.lang) == SE_ZERO) {
      return thRval;
    }
    if (elRval->isPointerToMemberType() && th->getSpecial(env.lang) == SE_ZERO) {
      return elRval;
    }
    if (thRval->isPointerToMemberType() && elRval->isPointerToMemberType()) {
      Type *ret = compositePointerToMemberType(env,
        thRval->asPointerToMemberType(), elRval->asPointerToMemberType());
      if (!ret) { goto incompatible; }
      return ret;
    }
  }

incompatible:
  return env.error(stringc
    << "incompatible ?: argument types `" << thRval->toString()
    << "' and `" << elRval->toString() << "'");
}


Type *E_sizeofType::itcheck_x(Env &env, Expression *&replacement)
{
  ASTTypeId::Tcheck tc(DF_NONE, DC_E_SIZEOFTYPE);
  atype = atype->tcheck(env, tc);
  Type *t = atype->getType();
  try {
    size = t->reprSize();
  }
  catch (XReprSize &e) {
    t = env.error(e.why());
  }

  return t->isError()? t : env.getSimpleType(SL_UNKNOWN, ST_UNSIGNED_INT);
}


Type *E_assign::itcheck_x(Env &env, Expression *&replacement)
{
  target->tcheck(env, target);
  src->tcheck(env, src);
  
  // check for operator overloading
  {
    Type *ovlRet = resolveOverloadedBinaryOperator(
      env, replacement, /*this,*/ target, src, 
      toOverloadableOp(op, true /*assignment*/));
    if (ovlRet) {
      return ovlRet;
    }
  }

  // TODO: make sure 'target' and 'src' make sense together with 'op'

  return target->type;
}


Type *E_new::itcheck_x(Env &env, Expression *&replacement)
{
  placementArgs = tcheckArgExprList(placementArgs, env);

  // TODO: check the environment for declaration of an operator 'new'
  // which accepts the given placement args

  // typecheck the typeid in E_new context; it returns the
  // array size for new[] (if any)
  ASTTypeId::Tcheck tc(DF_NONE, DC_E_NEW);
  tc.newSizeExpr = &arraySize;
  atype = atype->tcheck(env, tc);

  // grab the type of the objects to allocate
  Type *t = atype->getType();
      
  // cannot allocate incomplete types
  if (!env.ensureCompleteType("create an object of", t)) {
    return env.makePtrType(SL_UNKNOWN, t);     // error recovery
  }

  // The AST has the capability of recording whether argument parens
  // (the 'new-initializer' in the terminology of cppstd)
  // syntactically appeared, and this does matter for static
  // semantics; see cppstd 5.3.4 para 15.  However, for our purposes,
  // it will likely suffice to simply pretend that anytime 't' refers
  // to a class type, missing parens were actually present.
  if (t->isCompoundType() && !ctorArgs) {
    ctorArgs = new ArgExpressionListOpt(NULL /*list*/);
  }

  if (ctorArgs) {
    ctorArgs->list = tcheckArgExprList(ctorArgs->list, env);
    Variable *ctor0 = 
      outerResolveOverload_ctor(env, env.loc(), t, ctorArgs->list,
                                env.doOverload());
    // ctor0 can be null when the type is a simple type, such as an
    // int; I assume that ctor0 being NULL is the correct behavior in
    // that case
    ctorVar = env.storeVar(ctor0);
    env.ensureFuncBodyTChecked(ctor0);
  }

  return env.makePtrType(SL_UNKNOWN, t);
}


Type *E_delete::itcheck_x(Env &env, Expression *&replacement)
{
  expr->tcheck(env, expr);

  Type *t = expr->type->asRval();
  if (!t->isPointer()) {
    env.error(t, stringc
      << "can only delete pointers, not `" << t->toString() << "'");
  }
  
  return env.getSimpleType(SL_UNKNOWN, ST_VOID);
}


Type *E_throw::itcheck_x(Env &env, Expression *&replacement)
{
  if (expr) {
    expr->tcheck(env, expr);
  }
  else {
    // TODO: make sure that we're inside a 'catch' clause
  }

  return env.getSimpleType(SL_UNKNOWN, ST_VOID);
}


Type *E_keywordCast::itcheck_x(Env &env, Expression *&replacement)
{
  ASTTypeId::Tcheck tc(DF_NONE, DC_E_KEYWORDCAST);
  ctype = ctype->tcheck(env, tc);
  expr->tcheck(env, expr);

  // TODO: make sure that 'expr' can be cast to 'type'
  // using the 'key'-style cast

  return ctype->getType();
}


Type *E_typeidExpr::itcheck_x(Env &env, Expression *&replacement)
{
  expr->tcheck(env, expr);
  return env.type_info_const_ref();
}


Type *E_typeidType::itcheck_x(Env &env, Expression *&replacement)
{
  ASTTypeId::Tcheck tc(DF_NONE, DC_E_TYPEIDTYPE);
  ttype = ttype->tcheck(env, tc);
  return env.type_info_const_ref();
}


Type *E_grouping::itcheck_x(Env &env, Expression *&replacement)
{
  expr->tcheck(env, expr);
  return expr->type;
}


// --------------------- Expression constEval ------------------
// TODO: Currently I do not implement the notion of "value-dependent
// expression" (cppstd 14.6.2.3).  But, I believe a good way to do
// that is to extend 'constEval' so it can return a code indicating
// that the expression *is* constant, but is also value-dependent.

bool Expression::constEval(Env &env, int &result) const
{
  string msg;
  if (constEval(msg, result)) {
    return true;
  }
  
  if (msg.length() > 0) {
    env.error(msg);
  }
  return false;
}


bool Expression::constEval(string &msg, int &result) const
{
  xassert(!ambiguity);

  if (type->isError()) {
    // don't try to const-eval an expression that failed
    // to typecheck
    return false;
  }

  ASTSWITCHC(Expression, this) {
    // Handle this idiom for finding a member offset:
    // &((struct scsi_cmnd *)0)->b.q
    ASTCASEC(E_addrOf, eaddr)
      return eaddr->expr->constEvalAddr(msg, result);
      
    ASTNEXTC(E_boolLit, b)
      result = b->b? 1 : 0;
      return true;

    ASTNEXTC(E_intLit, i)
      result = i->i;
      return true;

    ASTNEXTC(E_charLit, c)
      result = c->c;
      return true;

    ASTNEXTC(E_variable, v)
      if (v->var->isEnumerator()) {
        result = v->var->getEnumeratorValue();
        return true;
      }

      if (v->var->type->isCVAtomicType() &&
          (v->var->type->asCVAtomicTypeC()->cv & CV_CONST) &&
          v->var->value) {
        // const variable
        return v->var->value->constEval(msg, result);
      }

      msg = stringc
        << "can't const-eval non-const variable `" << v->var->name << "'";
      return false;

    ASTNEXTC(E_constructor, c)
      if (type->isIntegerType()) {
        // allow it; should only be 1 arg, and that will be value
        return c->args->first()->constEval(msg, result);
      }
      else {
        msg = "can only const-eval E_constructors for integer types";
        return false;
      }

    ASTNEXTC(E_sizeof, s)
      result = s->size;
      return true;

    ASTNEXTC(E_unary, u)
      if (!u->expr->constEval(msg, result)) return false;
      switch (u->op) {
        default: xfailure("bad code");
        case UNY_PLUS:   result = +result;  return true;
        case UNY_MINUS:  result = -result;  return true;
        case UNY_NOT:    result = !result;  return true;
        case UNY_BITNOT: result = ~result;  return true;
      }

    ASTNEXTC(E_binary, b)
      if (b->op == BIN_COMMA) {
        // avoid trying to eval the LHS
        return b->e2->constEval(msg, result);
      }

      int v1, v2;
      if (!b->e1->constEval(msg, v1) ||
          !b->e2->constEval(msg, v2)) return false;

      if (v2==0 && (b->op == BIN_DIV || b->op == BIN_MOD)) {
        msg = "division by zero in constant expression";
        return false;
      }

      switch (b->op) {
        case BIN_EQUAL:     result = (v1 == v2);  return true;
        case BIN_NOTEQUAL:  result = (v1 != v2);  return true;
        case BIN_LESS:      result = (v1 < v2);  return true;
        case BIN_GREATER:   result = (v1 > v2);  return true;
        case BIN_LESSEQ:    result = (v1 <= v2);  return true;
        case BIN_GREATEREQ: result = (v1 >= v2);  return true;

        case BIN_MULT:      result = (v1 * v2);  return true;
        case BIN_DIV:       result = (v1 / v2);  return true;
        case BIN_MOD:       result = (v1 % v2);  return true;
        case BIN_PLUS:      result = (v1 + v2);  return true;
        case BIN_MINUS:     result = (v1 - v2);  return true;
        case BIN_LSHIFT:    result = (v1 << v2);  return true;
        case BIN_RSHIFT:    result = (v1 >> v2);  return true;
        case BIN_BITAND:    result = (v1 & v2);  return true;
        case BIN_BITXOR:    result = (v1 ^ v2);  return true;
        case BIN_BITOR:     result = (v1 | v2);  return true;
        case BIN_AND:       result = (v1 && v2);  return true;
        case BIN_OR:        result = (v1 || v2);  return true;
        // BIN_COMMA handled above

        default:         // BIN_BRACKETS, etc.
          return false;
      }

    ASTNEXTC(E_cast, c)
      if (!c->expr->constEval(msg, result)) return false;

      Type *t = c->ctype->getType();
      if (t->isIntegerType() ||
          t->isPointer()) {             // for Linux kernel
        return true;       // ok
      }
      else {
        // TODO: this is probably not the right rule..
        msg = stringc
          << "in constant expression, can only cast to integer or pointer types, not `"
          << t->toString() << "'";
        return false;
      }

    ASTNEXTC(E_cond, c)
      if (!c->cond->constEval(msg, result)) return false;

      if (result) {
        return c->th->constEval(msg, result);
      }
      else {
        return c->el->constEval(msg, result);
      }

    ASTNEXTC(E_sizeofType, s)
      result = s->size;
      return true;

    ASTNEXTC(E_grouping, e)
      return e->expr->constEval(msg, result);

    ASTDEFAULTC
      return extConstEval(msg, result);

    ASTENDCASEC
  }
}

// The intent of this function is to provide a hook where extensions
// can handle the 'constEval' message for their own AST syntactic
// forms, by overriding this function.  The non-extension nodes use
// the switch statement above, which is more compact.
bool Expression::extConstEval(string &msg, int &result) const
{
  msg = stringc << kindName() << " is not constEval'able";
  return false;
}

bool Expression::constEvalAddr(string &msg, int &result) const
{
  result = 0;                   // FIX: this is wrong
  int result0;                  // dummy for use below
  // FIX: I'm sure there are cases missing from this
  ASTSWITCHC(Expression, this) {
    // These two are dereferences, so they recurse back to constEval().
    ASTCASEC(E_deref, e)
      return e->ptr->constEval(msg, result0);
      break;

    ASTNEXTC(E_arrow, e)
      return e->obj->constEval(msg, result0);
      break;

    // These just recurse on constEvalAddr().
    ASTNEXTC(E_fieldAcc, e)
      return e->obj->constEvalAddr(msg, result0);
      break;

    ASTNEXTC(E_cast, e)
      return e->expr->constEvalAddr(msg, result0);
      break;

    ASTNEXTC(E_keywordCast, e)
      // FIX: haven't really thought about these carefully
      switch (e->key) {
      default:
        xfailure("bad CastKeyword");
      case CK_DYNAMIC:
        return false;
        break;
      case CK_STATIC: case CK_REINTERPRET: case CK_CONST:
        return e->expr->constEvalAddr(msg, result0);
        break;
      }
      break;

    ASTNEXTC(E_grouping, e)
      return e->expr->constEvalAddr(msg, result);
      break;

    ASTDEFAULTC
      return false;
      break;

    ASTENDCASEC
  }
}


bool Expression::hasUnparenthesizedGT() const
{
  // recursively dig down into any subexpressions which syntactically
  // aren't enclosed in parentheses or brackets
  ASTSWITCHC(Expression, this) {
    ASTCASEC(E_funCall, f)
      return f->func->hasUnparenthesizedGT();

    ASTNEXTC(E_fieldAcc, f)
      return f->obj->hasUnparenthesizedGT();

    ASTNEXTC(E_unary, u)
      return u->expr->hasUnparenthesizedGT();

    ASTNEXTC(E_effect, e)
      return e->expr->hasUnparenthesizedGT();

    ASTNEXTC(E_binary, b)
      if (b->op == BIN_GREATER) {
        // all this just to find one little guy..
        return true;
      }

      return b->e1->hasUnparenthesizedGT() ||
             b->e2->hasUnparenthesizedGT();

    ASTNEXTC(E_addrOf, a)
      return a->expr->hasUnparenthesizedGT();

    ASTNEXTC(E_deref, d)
      return d->ptr->hasUnparenthesizedGT();
      
    ASTNEXTC(E_cast, c)
      return c->expr->hasUnparenthesizedGT();
      
    ASTNEXTC(E_cond, c)
      return c->cond->hasUnparenthesizedGT() ||
             c->th->hasUnparenthesizedGT() ||
             c->el->hasUnparenthesizedGT();

    ASTNEXTC(E_assign, a)
      return a->target->hasUnparenthesizedGT() ||
             a->src->hasUnparenthesizedGT();
             
    ASTNEXTC(E_delete, d)
      return d->expr->hasUnparenthesizedGT();
      
    ASTNEXTC(E_throw, t)
      return t->expr && t->expr->hasUnparenthesizedGT();

    ASTDEFAULTC
      // everything else, esp. E_grouping, is false
      return extHasUnparenthesizedGT();

    ASTENDCASEC
  }
}

bool Expression::extHasUnparenthesizedGT() const
{
  return false;
}


// can 0 be cast to 't' and still be regarded as a null pointer
// constant?
bool allowableNullPtrCastDest(CCLang &lang, Type *t)
{
  // C++ (4.10, 5.19)
  if (t->isIntegerType() || t->isEnumType()) {
    return true;
  }

  // C99 6.3.2.3: in C, void* casts are also allowed
  if (!lang.isCplusplus && 
      t->isPointerType() &&
      t->asPointerType()->atType->isVoid()) {
    return true;
  }
  
  return false;
}

SpecialExpr Expression::getSpecial(CCLang &lang) const
{
  ASTSWITCHC(Expression, this) {
    ASTCASEC(E_intLit, i)
      return i->i==0? SE_ZERO : SE_NONE;

    ASTNEXTC1(E_stringLit)
      return SE_STRINGLIT;

    // 9/23/04: oops, forgot this
    ASTNEXTC(E_grouping, e)
      return e->expr->getSpecial(lang);

    // 9/24/04: cppstd 4.10: null pointer constant is integral constant
    // expression (5.19) that evaluates to 0.
    // cppstd 5.19: "... only type conversions to integral or enumeration
    // types can be used ..."
    ASTNEXTC(E_cast, e)
      if (allowableNullPtrCastDest(lang, e->ctype->getType()) &&
          e->expr->getSpecial(lang) == SE_ZERO) {
        return SE_ZERO;
      }
      return SE_NONE;

    ASTDEFAULTC
      return SE_NONE;

    ASTENDCASEC
  }
}


// ------------------- FullExpression -----------------------
void FullExpression::tcheck(Env &env)
{
  expr->tcheck(env, expr);
}


// ExpressionListOpt

// ----------------------- Initializer --------------------
// TODO: all the initializers need to be checked for compatibility
// with the types they initialize

void IN_expr::tcheck(Env &env, Type *)
{
  e->tcheck(env, e);
}


void IN_compound::tcheck(Env &env, Type* type)
{
  // NOTE: I ignore the FullExpressionAnnot *annot
  FOREACH_ASTLIST_NC(Initializer, inits, iter) {
    // TODO: This passes the wrong type; 'type' should be e.g. a class,
    // and this code ought to dig into the class and pass the types of
    // successive fields.  It doesn't matter right now, however, since
    // the type is eventually ignored anyway.
    iter.data()->tcheck(env, type);
  }
}


void IN_ctor::tcheck(Env &env, Type *type)
{
  args = tcheckArgExprList(args, env);
  ctorVar = env.storeVar(
    outerResolveOverload_ctor(env, loc, type, args, env.doOverload()));
  env.ensureFuncBodyTChecked(ctorVar);
}


// -------------------- TemplateDeclaration ---------------
void TemplateDeclaration::tcheck(Env &env)
{
  // if this is a complete specialization put nothing on the stack as
  // we are still in normal code
  bool inCompleteSpec = params->isEmpty();
  // Second star to the right, and straight on till morning
  Restorer<Env::TemplTcheckMode> changeMode(env.tcheckMode,
    inCompleteSpec? env.tcheckMode : Env::TTM_3TEMPL_DEF);

  // make a new scope to hold the template parameters
  Scope *paramScope = env.enterScope(SK_TEMPLATE_PARAMS, "template declaration parameters");

  // check each of the parameters, i.e. enter them into the scope
  // and its 'templateParams' list
  FAKELIST_FOREACH_NC(TemplateParameter, params, iter) {
    iter->tcheck(env, paramScope->templateParams);
  }

  // mark the new scope as unable to accept new names, so
  // that the function or class being declared will be entered
  // into the scope above us
  paramScope->canAcceptNames = false;

  // in what follows, ignore errors that are not disambiguating
  //bool prev = env.setDisambiguateOnly(true);
  //
  // update: moved this inside Function::tcheck and TS_classSpec::tcheck
  // so that the declarators would still get full checking

  // check the particular declaration
  itcheck(env);

  // restore prior error mode
  //env.setDisambiguateOnly(prev);

  // remove the template argument scope
  env.exitScope(paramScope);
}


void TD_func::itcheck(Env &env)
{
  // check the function definition; internally this will get
  // the template parameters attached to the function type
  f->tcheck(env);
                                                                            
  // instantiate any instantiations that were requested but delayed
  // due to not having the definition
  env.instantiateForwardFunctions(f->nameAndParams->var);

  // 8/11/04: The big block of template code that used to be here
  // was superceded by activity in Declarator::mid_tcheck.
}


void TD_decl::itcheck(Env &env)
{
  if (d->dflags & DF_FRIEND) {
    // for now I just want to ignore friend declarations altogether...
    return;
  }

  if (env.secondPassTcheck) {
    // TS_classSpec is only thing of interest
    if (d->spec->isTS_classSpec()) {
      d->spec->asTS_classSpec()->tcheck(env, d->dflags);
    }
    return;
  }

  // cppstd 14 para 3: there can be at most one declarator
  // in a template declaration
  if (d->decllist->count() > 1) {
    env.error("there can be at most one declarator in a template declaration");
  }

  // is this like the old TD_class?
  bool likeTD_class = d->spec->isTS_classSpec() || d->spec->isTS_elaborated();

  // check the declaration; works like TD_func because D_func is the
  // place we grab template parameters, and that's shared by both
  // definitions and prototypes
  DisambiguateOnlyTemp disOnly(env, !likeTD_class);
  d->tcheck(env, DC_TD_DECL);
}


void TD_tmember::itcheck(Env &env)
{
  // just recursively introduce the next level of parameters; the
  // new take{C,F}TemplateInfo functions know how to get parameters
  // out of multiple layers of nested template scopes
  d->tcheck(env);
}


// ------------------- TemplateParameter ------------------
void TP_type::tcheck(Env &env, SObjList<Variable> &tparams)
{
  // cppstd 14.1 is a little unclear about whether the type
  // name is visible to its own default argument; but that
  // would make no sense, so I'm going to check the
  // default type first
  if (defaultType) {
    ASTTypeId::Tcheck tc(DF_NONE, DC_TP_TYPE);
    defaultType = defaultType->tcheck(env, tc);
  }

  // the standard is not clear about whether the user code should
  // be able to do this:
  //   template <class T>
  //   int f(class T &t)      // can use "class T" instead of just "T"?
  //   { ... }
  // my approach of making a TypeVariable, instead of calling
  // it a CompoundType with a flag for 'is type variable', rules
  // out the subsequent use of "class T" ...

  // make a type variable for this thing
  TypeVariable *tvar = new TypeVariable(name);
  CVAtomicType *fullType = env.makeType(loc, tvar);

  // make a typedef variable for this type
  Variable *var = env.makeVariable(loc, name, fullType, 
                                   DF_TYPEDEF | DF_TEMPL_PARAM);
  tvar->typedefVar = var;
  if (defaultType) {
    var->defaultParamType = defaultType->getType();
  }

  if (name) {
    // introduce 'name' into the environment
    if (!env.addVariable(var)) {
      env.error(stringc
        << "duplicate template parameter `" << name << "'",
        EF_NONE);
    }
  }

  // annotate this AST node with the type
  this->type = fullType;

  // add this parameter to the list of them
  tparams.append(var);
}


void TP_nontype::tcheck(Env &env, SObjList<Variable> &tparams)
{                                                   
  // TODO: I believe I want to remove DF_PARAMETER.
  ASTTypeId::Tcheck tc(DF_PARAMETER | DF_TEMPL_PARAM, DC_TP_NONTYPE);

  // check the parameter; this actually adds it to the
  // environment too, so we don't need to do so here
  param = param->tcheck(env, tc);

  // add to the parameter list
  tparams.append(param->decl->var);
}


// -------------------- TemplateArgument ------------------
TemplateArgument *TemplateArgument::tcheck(Env &env)
{
  int dummy;
  if (!ambiguity) {
    // easy case
    mid_tcheck(env, dummy);
    return this;
  }

  // generic resolution: whatever tchecks is selected
  return resolveAmbiguity(this, env, "TemplateArgument", false /*priority*/, dummy);
}


void TA_type::itcheck(Env &env)
{
  ASTTypeId::Tcheck tc(DF_NONE, DC_TA_TYPE);
  type = type->tcheck(env, tc);

  Type *t = type->getType();
//    if (t->isCVAtomicType() && t->asCVAtomicType()->isTypeVariable()) {
//      TypeVariable *tv = t->asCVAtomicType()->asTypeVariable();
//      Variable *v0 = tv->typedefVar;
//      xassert(v0);
//      cout << "env.locStr() " << env.locStr()
//           << " v0->name '" << v0->name
//           << "' v0->serialNumber " << v0->serialNumber
//           << endl;
//    }
  
  // dsw: it would be a gratuitious non-orthgonality to guard the
  // following with a test such as
  //   if (!t->isTypeVariable())
  sarg.setType(t);
}

static void setSTemplArgFromExpr
  (Env &env, STemplateArgument &sarg, Expression *expr, int recursionCount)
{
  // see cppstd 14.3.2 para 1

  if (expr->type->isIntegerType() ||
      expr->type->isBool() ||
      expr->type->isEnumType()) {
    int i;
    string msg;   // discarded
    if (expr->constEval(msg, i)) {
      sarg.setInt(i);
    }
    else if (env.inUninstTemplate()) {
      // assume that the cause is evaluating an expression that refers
      // to a (non-type) template argument (TODO: confirm)
      sarg.setDepExpr();
    }
    else {
      env.error(stringc
        << "cannot evaluate `" << expr->exprToString()
        << "' as a template integer argument");
    }
  }

  else if (expr->type->isReference()) {
    if (expr->isE_variable()) {
      // lookup the variable in the scope and see if it has a value
      // and replace it with that
      Env::TemplTcheckMode mode = env.getTemplTcheckMode();
      if (mode == Env::TTM_2TEMPL_FUNC_DECL
          || mode == Env::TTM_3TEMPL_DEF
          || recursionCount > 0) {
        sarg.setReference(expr->asE_variable()->var);
      } else if (mode == Env::TTM_1NORMAL) {
        Variable *var0 = env.lookupPQVariable(expr->asE_variable()->name);
        if (!var0) {
          env.error(stringc
                    << "`" << expr->exprToString() << "' must lookup to a variable "
                    << "for it to be a variable template reference argument");
          return;
        }
        if (var0->isEnumerator()) {      // in/t0394.cc
          sarg.setInt(var0->getEnumeratorValue());
          return;
        }
        if (!var0->value) {
          env.error(stringc
                    << "`" << expr->exprToString() << "' must lookup to a variable with a value "
                    << "for it to be a variable template reference argument");
          return;
        }
        // FIX: I suppose we should check here that the type of
        // var0->value is what we expect from the expr
        setSTemplArgFromExpr(env, sarg, var0->value, 1 /*recursionCount*/);
      } else {
        xfailure("bad");
      }
    }
    else {
      env.error(stringc
        << "`" << expr->exprToString() << "' must be a simple variable "
        << "for it to be a template reference argument");
    }
  }

  else if (expr->type->isPointer()) {
    if (expr->isE_addrOf() &&
        expr->asE_addrOf()->expr->isE_variable()) {
      sarg.setPointer(expr->asE_addrOf()->asE_variable()->var);
    }
    else {
      env.error(stringc
        << "`" << expr->exprToString() << " must be the address of a "
        << "simple variable for it to be a template pointer argument");
    }
  }

  else if (expr->type->isPointerToMemberType()) {
    // this check is identical to the case above, but combined with
    // the inferred type it checks for a different syntax
    if (expr->isE_addrOf() &&
        expr->asE_addrOf()->expr->isE_variable()) {
      sarg.setMember(expr->asE_addrOf()->asE_variable()->var);
    }
    else {
      env.error(stringc
        << "`" << expr->exprToString() << " must be the address of a "
        << "class member for it to be a template pointer argument");
    }
  }

  // do I need an explicit exception for this?
  //else if (expr->type->isTypeVariable()) {

  else {
    env.error(expr->type, stringc
      << "`" << expr->exprToString() << "' has type `"
      << expr->type->toString() << "' but that's not an allowable "
      << "type for a template argument");
  }
}


void TA_nontype::itcheck(Env &env)
{
  expr->tcheck(env, expr);
  setSTemplArgFromExpr(env, sarg, expr, 0 /*recursionCount*/);
}


void TA_templateUsed::itcheck(Env &env)
{
  // nothing to do; 'sarg' is already STA_NONE, and the plan is
  // that no one will ever look at it anyway
}


// -------------------------- NamespaceDecl -------------------------
void ND_alias::tcheck(Env &env)
{
  // lookup the qualifiers in the name
  original->tcheck(env);

  // find the namespace we're talking about
  Variable *origVar = env.lookupPQVariable(original, LF_ONLY_NAMESPACES);
  if (!origVar) {
    env.error(stringc
      << "could not find namespace `" << *original << "'");
    return;
  }
  xassert(origVar->isNamespace());   // meaning of LF_ONLY_NAMESPACES

  // is the alias already bound to something?
  Variable *existing = env.lookupVariable(alias, LF_INNER_ONLY);
  if (existing) {
    // 7.3.2 para 3: redefinitions are allowed only if they make it
    // refer to the same thing
    if (existing->isNamespace() &&
        existing->scope == origVar->scope) {
      return;     // ok; nothing needs to be done
    }
    else {
      env.error(stringc
        << "redefinition of namespace alias `" << alias
        << "' not allowed because the new definition isn't the same as the old");
      return;
    }
  }

  // make a new namespace variable entry
  Variable *v = env.makeVariable(env.loc(), alias, NULL /*type*/, DF_NAMESPACE);
  env.addVariable(v);

  // make it refer to the same namespace as the original one
  v->scope = origVar->scope;
  
  // note that, if one cares to, the alias can be distinguished from
  // the original name in that the scope's 'namespaceVar' still points
  // to the original one (only)
}


void ND_usingDecl::tcheck(Env &env)
{
  if (!name->hasQualifiers()) {
    env.error("a using-declaration requires a qualified name");
    return;
  }

  if (name->getUnqualifiedName()->isPQ_template()) {
    // cppstd 7.3.3 para 5
    env.error("you can't use a template-id (template name + template arguments) "
              "in a using-declaration");
    return;
  }

  // lookup the qualifiers in the name
  name->tcheck(env);

  // find what we're referring to; if this is a template, then it
  // names the template primary, not any instantiated version
  Variable *origVar = env.lookupPQVariable(name, LF_TEMPL_PRIMARY);
  if (!origVar) {
    env.error(stringc
      << "undeclared identifier: `" << *name << "'");
    return;
  }

  if (!origVar->overload) {
    env.makeUsingAliasFor(name->loc, origVar);
  }
  else {
    SFOREACH_OBJLIST_NC(Variable, origVar->overload->set, iter) {
      env.makeUsingAliasFor(name->loc, iter.data());
    }
  }

  // the eighth example in 7.3.3 implies that the structure and enum
  // tags come along for the ride too
  {
    Scope *origScope = origVar->scope? origVar->scope : env.globalScope();

    CompoundType *origCt = origScope->lookupCompound(origVar->name);
    if (origCt) {
      // alias the structure tag
      env.addCompound(origCt);
      
      // if it has been shadowed, we need that too
      if (env.isShadowTypedef(origCt->typedefVar)) {
        env.makeUsingAliasFor(name->loc, origCt->typedefVar);
      }
    }

    EnumType *origEnum = origScope->lookupEnum(origVar->name, env);
    if (origEnum) {
      // alias the enum tag
      env.addEnum(origEnum);

      if (env.isShadowTypedef(origEnum->typedefVar)) {
        env.makeUsingAliasFor(name->loc, origEnum->typedefVar);
      }
    }
  }
}


void ND_usingDir::tcheck(Env &env)
{
  // lookup the qualifiers in the name
  name->tcheck(env);

  // find the namespace we're talking about
  Variable *targetVar = env.lookupPQVariable(name, LF_ONLY_NAMESPACES);
  if (!targetVar) {
    env.error(stringc
      << "could not find namespace `" << *name << "'");
    return;
  }
  xassert(targetVar->isNamespace());   // meaning of LF_ONLY_NAMESPACES
  Scope *target = targetVar->scope;
  
  // to implement transitivity of 'using namespace', add a "using"
  // edge from the current scope to the target scope, if the current
  // one has a name (and thus could be the target of another 'using
  // namespace')
  //
  // update: for recomputation to work, I need to add the edge
  // regardless of whether 'cur' has a name
  Scope *cur = env.scope();
  cur->addUsingEdge(target);

  if (cur->usingEdgesRefct == 0) {
    // add the effect of a single "using" edge, which includes
    // a transitive closure computation
    cur->addUsingEdgeTransitively(env, target);
  }
  else {
    // someone is using me, which means their transitive closure
    // is affected, etc.  let's just recompute the whole network
    // of active-using edges
    env.refreshScopeOpeningEffects();
  }
}


// EOF
