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
#include "ast_build.h"      // makeExprList1, etc.
#include "strutil.h"        // prefixEquals, pluraln
#include "macros.h"         // Restorer

#include <stdlib.h>         // strtoul, strtod

// D(): debug code
#ifdef NDEBUG
  #define D(stuff)
#else
  #define D(stuff) stuff
#endif


// forwards in this file
static Variable *outerResolveOverload_ctor
  (Env &env, SourceLoc loc, Type *type, FakeList<ArgExpression> *args, bool really);
static bool reallyDoOverload(Env &env, FakeList<ArgExpression> *args);
FakeList<ArgExpression> *tcheckArgExprList(FakeList<ArgExpression> *list, Env &env);


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
    TRACE("topform", topForm);
    iter.data()->tcheck(env);
  }

  instantiateRemainingMethods(env, this);
}


// --------------------- TopForm ---------------------
void TF_decl::tcheck(Env &env)
{
  env.setLoc(loc);
  decl->tcheck(env, DC_TF_DECL);
}

void TF_func::tcheck(Env &env)
{
  env.setLoc(loc);
  f->tcheck(env, true /*checkBody*/);
}

void TF_template::tcheck(Env &env)
{
  env.setLoc(loc);
  td->tcheck(env);
}

void TF_explicitInst::tcheck(Env &env)
{
  env.setLoc(loc);
  d->tcheck(env);
  
  #warning this is not right
}

void TF_linkage::tcheck(Env &env)
{  
  env.setLoc(loc);

  // set the linkage type for the duration of this form
  Restorer<StringRef> restorer(env.currentLinkage, linkageType);

  forms->tcheck(env);
}

void TF_one_linkage::tcheck(Env &env)
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

void TF_asm::tcheck(Env &env)
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


void TF_namespaceDefn::tcheck(Env &env)
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
  // dsw: I predicated this on name so that you don't try to add a
  // variable with a NULL name to the namespace
  #warning anonymous namespaces are now broken
  else if (name) {
    // make an entry in the surrounding scope to refer to the new namespace
    Variable *v = env.makeVariable(loc, name, NULL /*type*/, DF_NAMESPACE);
    env.addVariable(v);

    // make new scope
    s = new Scope(SK_NAMESPACE, 0 /*changeCount; irrelevant*/, loc);
    s->namespaceVar = v;

    // point the variable at it so we can find it later
    v->scope = s;
    
    // TODO: if name is NULL, we need to add an "active using" edge
    // from the surrounding scope to s
    
    env.setParentScope(s);
  }

  // check the namespace body in its scope
  env.extendScope(s);
  FOREACH_ASTLIST_NC(TopForm, forms, iter) {
    iter.data()->tcheck(env);
  }
  env.retractScope(s);
}


void TF_namespaceDecl::tcheck(Env &env)
{
  env.setLoc(loc);
  decl->tcheck(env);
}


// --------------------- Function -----------------
void Function::tcheck(Env &env, bool checkBody)
{
  // are we in a template function?
  bool inTemplate = env.scope()->hasTemplateParams();

  // only disambiguate, if template
  DisambiguateOnlyTemp disOnly(env, inTemplate /*disOnly*/);

  // get return type
  Type *retTypeSpec = retspec->tcheck(env, dflags);

  // construct the full type of the function; this will set
  // nameAndParams->var, which includes a type, but that type might
  // use different parameter names if there is already a prototype;
  // dt.type will come back with a function type which always has the
  // parameter names for this definition
  Declarator::Tcheck dt(retTypeSpec,
                        dflags | (checkBody? DF_DEFINITION : DF_NONE),
                        DC_FUNCTION);
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

  if (checkBody) {
    tcheckBody(env);
  }
}

void Function::tcheckBody(Env &env)
{
  // FIX: not sure if this should be set first or last, but I don't
  // want it done twice, so we do it first for now
  hasBodyBeenTChecked = true;

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
        env.error(stringc
          << "function definition of `" << *(nameAndParams->getDeclaratorId())
          << "' must appear in a namespace that encloses the original declaration");
      }

      // these two lines are the key to this whole block..  I'm
      // keeping the surrounding stuff, though, because it has that
      // error report above, and simply to avoid disturbing existing
      // (working) mechanism
      env.getQualifierScopes(qualifierScopes, nameAndParams->getDeclaratorId(),
                             LF_DECLARATOR);
      env.extendScopeSeq(qualifierScopes);

      // the innermost scope listed in 'qualifierScopes'
      // should be the same one in which the variable was
      // declared (could this be triggered by user code?)
      if (encloses && qualifierScopes.isNotEmpty()) {
        // I had to weaken this assertion to deal with the difference
        // between the template scope and its instantiation which
        // occurs when a function member of a class template is
        // instantiated and typechecked
//          xassert(s == qualifierScopes.top());
        if (s != qualifierScopes.top()) {
          MatchTypes match(env.tfac, MatchTypes::MM_WILD);
          CompoundType *sCpd = s->curCompound;
          xassert(sCpd);
          CompoundType *qCpd = qualifierScopes.top()->curCompound;
          xassert(qCpd);
          xassert(match.match_Atomic(sCpd, qCpd, 0 /*matchDepth*/));
        }
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
  // funcDefn at the definition even if the body has not bee tchecked.
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
    if (v) {
      // only "nonstatic data member"
      if (v->hasFlag(DF_TYPEDEF) ||
          v->hasFlag(DF_STATIC) ||
          v->type->isFunctionType()) {
        env.error("you can't initialize types, nor static data, "
                  "nor member functions, in a ctor member init list");
        return;
      }

      // annotate the AST
      member = env.storeVar(v);

      args = tcheckArgExprList(args, env);
      // decide which of v's possible constructors is being used
      ctorVar = env.storeVar(
        outerResolveOverload_ctor(env, env.loc(), v->type, args,
                                  reallyDoOverload(env, args)));

      if (ctorVar && ctorVar->type->isFunctionType()) {
        env.ensureFuncMemBodyTChecked(ctorVar);
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
                              reallyDoOverload(env, args)));
  if (ctorVar->type->isFunctionType()) {
    env.ensureFuncMemBodyTChecked(ctorVar);
  }
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


// MemberInit

// -------------------- Declaration -------------------
void Declaration::tcheck(Env &env, DeclaratorContext context)
{
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

    if (dt1.var && dt1.var->templInfo) {
      // this could have been a forward declaration of a templatized
      // function, for which we will need to remember the syntax of
      // the declaration so it can be re-tchecked with parameters
      // bound to arguments
      dt1.var->templInfo->declSyntax = this;
      
      // cppstd 14 para 3: there can be at most one declarator
      // in a template declaration; this justifies not repeating
      // this 'declSyntax' action in the loop below
      if (decllist->count() > 1) {
        env.error("there can be at most one declarator in a template declaration");
      }

      // is this a function template?
      if (dt1.var->type->isFunctionType()) {
        xassert(!dt1.var->templInfo->isMutant());
        // lookup the Declarator
        Declarator *declarator = decllist->first();
        xassert(declarator->var == dt1.var);
        PQ_name dt1_varName(dt1.var->loc, dt1.var->name);
        Variable *previous = env.lookupPQVariable_primary_resolve
          // FIX: this test doesn't work for special names like for
          // constructors
//            (declarator->decl->getDeclaratorId(),
          // FIX: this won't work if you need anything other than the
          // base name.  I'm hoping we are still in the same scope.
          (&dt1_varName,
           LF_TEMPL_PRIMARY,
           dt1.var->type->asFunctionType(),
           MatchTypes::MM_WILD);
        xassert(previous);
        xassert(previous->templateInfo()->isPrimary());
        if (dt1.var->templInfo->isPrimary()) {
          // if this is a forward declaration for a primary, it should
          // already be in the namespace and should be isomorphic
          MatchTypes match(env.tfac, MatchTypes::MM_ISO);
          xassert(match.match_Type(dt1.var->type, previous->type));
        } else {
          // if this is a forward declaration for a specialization /
          // instantiation, it should not be in the namespace but
          // should be put into the instantiation list of 'previous',
          // but only if it is not already there
          Variable *var0 = previous->templateInfo()->
            getInstantiationOfVar(env.tfac, dt1.var);
          if (!var0) {
            xassert(!dt1.var->funcDefn); // pure declaration, not definition
            previous->templateInfo()->addInstantiation(env.tfac, dt1.var);
          }
        }
      }
    }

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

  return resolveAmbiguity(this, env, "ASTTypeId", false /*priority*/, tc);
}

void ASTTypeId::mid_tcheck(Env &env, Tcheck &tc)
{
  #ifdef GNU_EXTENSION
  if (tc.context == DC_E_COMPOUNDLIT
      && spec->isTS_classSpec()
      // dsw: I really only need this for unions, but I think I'll do
      // the more general thing just for simplicity
      && spec->asTS_classSpec()->keyword == TI_UNION
      && !spec->asTS_classSpec()->name) {
    spec->asTS_classSpec()->tcheckAsIfNamed = true;
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

void PQ_qualifier::tcheck(Env &env)
{
  tcheckTemplateArgumentList(targs, env);
  rest->tcheck(env);
}

void PQ_name::tcheck(Env &env)
{}

void PQ_operator::tcheck(Env &env)
{
  o->tcheck(env);
}

void PQ_template::tcheck(Env &env)
{
  tcheckTemplateArgumentList(args, env);
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

  if (typenameUsed && !name->hasQualifiers()) {
    // cppstd 14.6 para 5, excerpt:
    //   "The keyword typename shall only be applied to qualified
    //    names, but those names need not be dependent."
    env.error("the `typename' keyword can only be used with a qualified name");
  }

  // if the user uses the keyword "typename", then the lookup errors
  // are non-disambiguating, because the syntax is unambiguous
  ErrorFlags eflags = (typenameUsed? EF_NONE : EF_DISAMBIGUATES);

  LookupFlags lflags = (typenameUsed? LF_TYPENAME : LF_NONE);
  Variable *v = env.lookupPQVariable(name, lflags);
  if (!v) {
    // NOTE:  Since this is marked as disambiguating, but the same
    // error message in E_variable::itcheck is not marked as such, it
    // means we prefer to report the error as if the interpretation as
    // "variable" were the only one.
    return env.error(stringc
      << "there is no type called `" << *name << "'", eflags);
  }

  if (!v->hasFlag(DF_TYPEDEF)) {
    return env.error(stringc
      << "variable name `" << *name << "' used as if it were a type", eflags);
  }

  // TODO: access control check

  var = env.storeVar(v);    // annotation

  // 7/27/04: typedefAliases used to be maintained at this point, but
  // now it is gone

  // there used to be a call to applyCV here, but that's redundant
  // since the caller (tcheck) calls it too
  return var->type;
}


Type *TS_simple::itcheck(Env &env, DeclFlags dflags)
{
  return env.getSimpleType(loc, id, cv);
}


// implemented in template.cc
bool verifyCompatibleTemplates(Env &env, CompoundType *prior);

Type *TS_elaborated::itcheck(Env &env, DeclFlags dflags)
{
  env.setLoc(loc);

  name->tcheck(env);

  if (keyword == TI_ENUM) {
    EnumType *et = env.lookupPQEnum(name);
    if (!et) {
      if (env.lang.isCplusplus) {
        return env.error(stringc
                         << "there is no enum called `" << *name << "'",
                         EF_DISAMBIGUATES);
      } else {
        // dsw: gcc allows you in C mode to make a typedef to an enum
        // that doesn't exist yet; this code copied from
        // TS_enumSpec::itcheck(), but I'm not sure if it warrants
        // factoring out as I had to modify it a bit.
        StringRef name0 = name->asPQ_name()->name;
        et = new EnumType(name0);
        Type *ret = env.makeType(loc, et);
        env.addEnum(et);
        // make the implicit typedef
        Variable *tv = env.makeVariable(loc, name0, ret, DF_TYPEDEF | DF_IMPLICIT);
        et->typedefVar = tv;
        // FIX: in TS_enumSpec::itcheck() Scott has an 'if' wrapped
        // around this that has no consequences
        env.addVariable(tv);
        return ret;
      }
    }

    this->atype = et;          // annotation
    return env.makeType(loc, et);
  }

  CompoundType *ct = NULL;
  if (!name->hasQualifiers() &&
      (dflags & DF_FORWARD) &&
      !(dflags & DF_FRIEND)) {
    // cppstd 3.3.1 para 5:
    //   "for an elaborated-type-specifier of the form
    //      class-key identifier ;
    //    the elaborated-type-specifier declares the identifier to be a
    //    class-name in the scope that contains the declaration"
    ct = env.lookupCompound(name->getName(), LF_INNER_ONLY);
    if (!ct) {
      // make a forward declaration
      Type *ret =
         env.makeNewCompound(ct, env.acceptingScope(), name->getName(),
                             loc, keyword, true /*forward*/);
      this->atype = ct;        // annotation
      return ret;
    }
    else {
      // redundant, nothing to do (what the hey, I'll check keywords)
      if ((keyword==TI_UNION) != (ct->keyword==CompoundType::K_UNION)) {
    keywordComplaint:
        return env.error(stringc
          << "you asked for a " << toString(keyword) << " called `"
          << *name << "', but that's actually a " << toString(ct->keyword));
      }
      this->atype = ct;        // annotation
      return env.makeType(loc, ct);
    }
  }

  ct = env.lookupPQCompound(name);
  if (!ct) {
    if (name->hasQualifiers()) {
      return env.error(stringc
        << "there is no " << toString(keyword) << " called `" << *name << "'");
    }

    // cppstd 3.3.1 para 5, continuing from above:
    //   "for an elaborated-type-specifier of the form
    //      class-key identifier
    //    if the elaborated-type-specifier is used in the decl-specifier-seq
    //    or parameter-declaration-clause of a function defined in namespace
    //    scope, the identifier is declared as a class-name in the namespace
    //    that contains the declaration; otherwise, except as a friend
    //    declaration, the identifier is declared in the smallest non-class,
    //    non-function-prototype scope that contains the declaration."
    //
    // my interpretation: create a forward declaration, but in the innermost
    // scope which is not:
    //   - a function parameter list scope, nor
    //   - a class scope, nor
    //   - a template parameter list scope (perhaps a concept unique to my impl.)
    //
    // Note that due to the exclusion of DF_FRIEND above I'm actually
    // handling 'friend' here, despite what the standard says..
    Scope *scope = env.outerScope();
    Type *ret =
       env.makeNewCompound(ct, scope, name->getName(), loc, keyword, 
                           true /*forward*/);
    this->atype = ct;           // annotation
    return ret;
  }

  // check that the keywords match; these 'keyword's are different
  // types, but they agree for the three relevant values
  if ((int)keyword != (int)ct->keyword) {
    // dsw: I made the struct/class confusion only be a warning
    // because gcc allows it
    if ( (keyword==TI_STRUCT && ct->keyword==CompoundType::K_CLASS)
         || (keyword==TI_CLASS && ct->keyword==CompoundType::K_STRUCT) ) {
      env.warning(stringc
        << "you asked for a " << toString(keyword) << " called `"
        << *name << "', but that's actually a " << toString(ct->keyword));
    } else {
      goto keywordComplaint;
    }
  }

  if (name->getUnqualifiedName()->isPQ_template()) {
    // this is like
    //   friend class Foo<T>;
    // inside some other templatized class.. I'm not sure
    // how to properly enforce the correspondence between
    // the declarations..
    // TODO: fix his

    // at least discard the template params as
    // 'verifyCompatibleTemplates' would have done..
    //
    // 7/31/04: params are no longer discarded by anyone
  }
  else {
    verifyCompatibleTemplates(env, ct);
  }

  this->atype = ct;              // annotation
  return env.makeType(loc, ct);
}


Type *TS_classSpec::itcheck(Env &env, DeclFlags dflags)
{
  env.setLoc(loc);

  // was the previous declaration a forward declaration?
  bool prevWasForward = false;

  // are we in a template?
  bool inTemplate = env.scope()->hasTemplateParams();

  // are we an inner class?
  CompoundType *containingClass = env.acceptingScope()->curCompound;
  if (env.lang.noInnerClasses) {
    // nullify the above; act as if it's an outer class
    containingClass = NULL;
  }

  // check restrictions on the form of the name
  ASTList<TemplateArgument> *templateArgs = NULL;
  if (name) {
    PQName *unqual = name->getUnqualifiedName();
    if (unqual->isPQ_template()) {
      if (!inTemplate) {
        return env.error("class specifier name can have template arguments "
                         "only in a templatized declaration");
      }
      else {
        PQ_template *t = unqual->asPQ_template();

        // typecheck the arguments
        tcheckTemplateArgumentList(t->args, env);
        templateArgs = &(t->args);
      }
    }
  }

  // get the raw name
  StringRef stringName = name? name->getName() : NULL;

  // see if the environment already has this name
  CompoundType *ct =
    stringName? env.lookupPQCompound(name, LF_INNER_ONLY) : NULL;
  Type *ret;
  if (ct && !templateArgs) {
    // check that the keywords match
    if ((int)ct->keyword != (int)keyword) {
      // apparently this isn't an error, because my streambuf.h
      // violates it..
      //return env.error(stringc
      //  << "there is already a " << ct->keywordAndName()
      //  << ", but here you're defining a " << toString(keyword)
      //  << " " << name);

      // but certainly we wouldn't allow changing a union to a
      // non-union, or vice-versa
      if ((ct->keyword == (CompoundType::K_UNION)) !=
          (keyword == TI_UNION)) {
        return env.error(stringc
          << "there is already a " << ct->keywordAndName()
          << ", but here you're defining a " << toString(keyword)
          << " " << *name);
      }

      trace("env") << "changing " << ct->keywordAndName()
                   << " to a " << toString(keyword) << endl;
      ct->keyword = (CompoundType::Keyword)keyword;
    }

    // check that the previous was a forward declaration
    if (!ct->forward) {
      return env.error(stringc
        << ct->keywordAndName() << " has already been defined");
    }

    // now it is no longer a forward declaration
    ct->forward = false;
    prevWasForward = true;

    verifyCompatibleTemplates(env, ct);

    this->ctype = ct;           // annotation
    ret = env.makeType(loc, ct);
  }

  else if (ct && templateArgs) {
    CompoundType *primary = ct;
    TemplateInfo *primaryTI = primary->templateInfo();

    // this is supposed to be a specialization
    if (!primaryTI) {
      return env.error("attempt to specialize a non-template");
    }

    // make a new type, since a specialization is a distinct template
    // [cppstd 14.5.4 and 14.7]; but don't add it to any scopes
    ret = env.makeNewCompound(ct, NULL /*scope*/, stringName, loc, keyword,
                              false /*forward*/);

    // dsw: need to register it at least, even if it isn't added to
    // the scope, otherwise I can't print out the name of the type
    // because at the top scope I don't know the scopeKind
    env.typeAcceptingScope()->registerVariable(ct->typedefVar);

    // similarly, the parentScope should be set properly
    env.setParentScope(ct);

    this->ctype = ct;           // annotation

    // 'makeNewCompound' will already have put the template *parameters*
    // into 'specialTI', but not the template arguments
    // TODO: this is copied from Env::instantiateTemplate(); collapse it
    {
      FOREACH_ASTLIST_NC(TemplateArgument, *templateArgs, _iter) {
        TemplateArgument *iter = _iter.data();
        xassert(iter->sarg.hasValue());
        ct->templateInfo()->arguments.append(new STemplateArgument(iter->sarg));
      }
    }

    // add this type to the primary's list of specializations; we are not
    // going to add 'ct' to the environment, so the only way to find the
    // specialization is to go through the primary template
    primaryTI->addInstantiation(env.tfac, ct->getTypedefVar());
    
    if (tracingSys("template")) {
      cout << "TS_classSpec::itcheck: "
           << "template typechecked, appending to instantiations of primary"
           << endl;
      primaryTI->debugPrint();
    }
  }

  else {      // !ct
    xassert(!ct);
    if (templateArgs) {
      env.error("cannot specialize a template that hasn't been declared");
    }

    // no existing compound; make a new one
    Scope *destScope = env.typeAcceptingScope();
    ret = env.makeNewCompound(ct, destScope, stringName,
                              loc, keyword, false /*forward*/);
    this->ctype = ct;              // annotation
  }

  tcheckIntoCompound(env, dflags, ct, inTemplate,
                     true /*reallyTcheckFunctionBodies*/,
                     containingClass);
//    env.deMutantify(ct);             // we now just avoid them during specialization resolution
  
  if (prevWasForward && ct->isTemplate()) {
    // we might have had forward declarations of template
    // instances that now can be made non-forward by tchecking
    // this syntax
    env.instantiateForwardClasses(env.acceptingScope(), ct->getTypedefVar());
  }

  return ret;
}

                                                          
// type check once we know what 'ct' is; this is also called
// to check newly-cloned AST fragments for template instantiation
void TS_classSpec::tcheckIntoCompound(
  Env &env, DeclFlags dflags,    // as in tcheck
  CompoundType *ct,              // compound into which we're putting declarations
  bool inTemplate,               // true if this is a template class (uninstantiated)
  bool reallyTcheckFunctionBodies, // in second pass really tcheck or just save the context
  CompoundType *containingClass) // if non-NULL, ct is an inner class
{
  // should have set the annotation by now
  xassert(ctype);

  // FIX: how can this fail?
  //    xassert(inTemplate == !!ct->templateInfo());
  //
  // update: this can fail because in Env::instantiateTemplate() Scott
  // decided to call this function with inTemplate == false; not sure
  // why, but I'll bet I shouldn't change it, so I'll weaken the
  // assertion.
  //
  // sm: 'inTemplate' means this is an uninstantated template; it is set
  // to false when processing an instantiation, because then there are no
  // holes, hence it is not a "template"
  if (inTemplate) xassert(ct->templateInfo());

  // let me map from compounds to their AST definition nodes
  ct->syntax = this;

  // only report serious errors while checking the class,
  // in the absence of actual template arguments
  DisambiguateOnlyTemp disOnly(env, inTemplate /*disOnly*/);

  // we should not be in an ambiguous context, because that would screw
  // up the environment modifications; if this fails, it could be that
  // you need to do context isolation using 'DisambiguateNestingTemp'
  xassert(env.disambiguationNestingLevel == 0);

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

      // cppstd 10, para 1: must be a class type (another
      // unfortunate const cast..)
      CompoundType *base 
        = const_cast<CompoundType*>(baseVar->type->ifCompoundType());
      if (!base) {
        env.error(stringc
          << "`" << *(iter->name) << "' is not a class or "
          << "struct or union, so it cannot be used as a base class");
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

  // open a scope, and install 'ct' as the compound which is
  // being built; in fact, 'ct' itself is a scope, so we use
  // that directly
  env.extendScope(ct);

  // look at members: first pass is to enter them into the environment
  FOREACH_ASTLIST_NC(Member, members->list, iter) {
    Member *member = iter.data();
    member->tcheck(env);
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
  }

  // default ctor, copy ctor, operator=; only do this for C++.
  if (env.lang.isCplusplus) {
    addCompilerSuppliedDecls(env, loc, ct);
  }

  // let the CompoundType build additional indexes if it wants
  ct->finishedClassDefinition(env.conversionOperatorName);

  // second pass: check function bodies
  bool innerClass = !!containingClass;
  if (!innerClass) {
    tcheckFunctionBodies(env, reallyTcheckFunctionBodies);
  }

  // now retract the class scope from the stack of scopes; do
  // *not* destroy it!
  env.retractScope(ct);

  if (innerClass) {
    // set the constructed scope's 'parentScope' pointer now that
    // we've removed 'ct' from the Environment scope stack; future
    // (unqualified) lookups in 'ct' will thus be able to see
    // into the containing class [cppstd 3.4.1 para 8]
    ct->parentScope = containingClass;
  }
  
  env.addedNewCompound(ct);
}


// this is pass 2 of tchecking a class
void TS_classSpec::tcheckFunctionBodies
  (Env &env, bool reallyTcheckFunctionBodies)
{
  CompoundType *ct = env.scope()->curCompound;
  xassert(ct);

  // check function bodies and elaborate ctors and dtors of member
  // declarations
  FOREACH_ASTLIST_NC(Member, members->list, iter) {
    if (iter.data()->isMR_func()) {
      Function *f = iter.data()->asMR_func()->f;

      // ordinarily we'd complain about seeing two declarations
      // of the same class member, so to tell D_name::itcheck not
      // to complain, this flag says we're in the second pass
      // tcheck of an inline member function
      f->dflags = (DeclFlags)(f->dflags | DF_INLINE_DEFN);

      if (reallyTcheckFunctionBodies) {
        // check it now
        f->tcheck(env, true /*checkBody*/);

        // remove DF_INLINE_DEFN so if I clone this later I can play the
        // same trick again (TODO: what if we decide to clone while down
        // in 'f->tcheck'?)
        f->dflags = (DeclFlags)(f->dflags & ~DF_INLINE_DEFN);
      }
      else {
        // check it later
        Variable *fvar = f->nameAndParams->var;
        xassert(fvar);
        if (fvar->funcDefn) {
          xassert(fvar->funcDefn == f);
        } else {
          // this would have been done in Function::tcheck() had it
          // been called.  I need it so that later I can find the
          // function body from the variable and instantiate it
          fvar->funcDefn = f;
        }
        fvar->setFlag(DF_DELAYED_INST);
      }
    }
    else if (iter.data()->isMR_decl()) {
      Declaration *d0 = iter.data()->asMR_decl()->d;
      FAKELIST_FOREACH_NC(Declarator, d0->decllist, decliter) {
        // initializers should have been tchecked by now
        if (decliter->init) {
          xassert(decliter->type);
        }
      }
    }
  }

  // a gcc-2.95.3 compiler bug is making this code segfault..
  // will disable it for now and try again when I have more time
  //
  // update: I got this to work by fixing Scope::getCompoundIter(),
  // which was returning an entire StrSObjDict instead of an iter..
  // but that still should have worked (though it wasn't what I
  // intended), so something still needs to be investigated

  // check function bodies of any inner classes, too, since only
  // a non-inner class will call tcheckFunctionBodies directly
  StringSObjDict<CompoundType>::IterC innerIter(ct->getCompoundIter());

  // if these print different answers, gcc has a bug
  TRACE("sm", "compound top: " << ct->private_compoundTop() << "\n" <<
              "iter current: " << innerIter.private_getCurrent());

  for (; !innerIter.isDone(); innerIter.next()) {
    CompoundType *inner = innerIter.value();
    if (!inner->syntax) {
      // this happens when all we have is a forward decl
      continue;
    }

    TRACE("inner", "checking bodies of " << inner->name);

    // open the inner scope
    env.extendScope(inner);

    // check its function bodies (it's somewhat of a hack to
    // resort to inner's 'syntax' poiner)
    inner->syntax->tcheckFunctionBodies(env, reallyTcheckFunctionBodies);

    // retract the inner scope
    env.retractScope(inner);
  }
}


Type *TS_enumSpec::itcheck(Env &env, DeclFlags dflags)
{
  env.setLoc(loc);

  #warning this lossage should only be allowed in C mode

  EnumType *et = NULL;
  Type *ret = NULL;
  if (name) {
    et = env.lookupPQEnum(new PQ_name(env.loc(), name));
  }
  if (et) {
    ret = env.makeType(loc, et);
  } else {
    et = new EnumType(name);
    ret = env.makeType(loc, et);
    if (name) {
      env.addEnum(et);

      // make the implicit typedef
      Variable *tv = env.makeVariable(loc, name, ret, DF_TYPEDEF | DF_IMPLICIT);
      et->typedefVar = tv;
      if (!env.addVariable(tv)) {
        // this isn't really an error, because in C it would have
        // been allowed, so C++ does too [ref?]
        //return env.error(stringc
        //  << "implicit typedef associated with enum " << et->name
        //  << " conflicts with an existing typedef or variable",
        //  true /*disambiguating*/);
      }
    }
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
  f->tcheck(env, false /*checkBody*/);

  checkMemberFlags(env, f->dflags);
}

void MR_access::tcheck(Env &env)
{
  env.setLoc(loc);

  env.scope()->curAccess = k;
}

void MR_publish::tcheck(Env &env)
{
  env.setLoc(loc);

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
  return tgt_type;
}

// provide a well-defined size for the array from the size of the
// initializer, such as in this case:
//   char sName[] = "SOAPPropertyBag";
Type *computeArraySizeFromLiteral(Env &env, Type *tgt_type, Initializer *init)
{
  if (tgt_type->isArrayType() &&
      !tgt_type->asArrayType()->hasSize() &&
      init->isIN_expr() &&
      init->asIN_expr()->e->type->isArrayType() &&
      init->asIN_expr()->e->type->asArrayType()->hasSize()
      ) {
    tgt_type = env.tfac.cloneType(tgt_type);
    tgt_type->asArrayType()->size = init->asIN_expr()->e->type->asArrayType()->size;
    xassert(tgt_type->asArrayType()->hasSize());
  }
  return tgt_type;
}

// true if the declarator corresponds to a local/global variable declaration
bool isVariableDC(DeclaratorContext dc)
{
  return dc==DC_TF_DECL ||    // global
         dc==DC_S_DECL ||     // local
         dc==DC_CN_DECL;      // local in a Condition
}

void Declarator::mid_tcheck(Env &env, Tcheck &dt)
{
  // true if we're immediately in a class body
  bool inClassBody = !!env.scope()->curCompound;

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
    name->tcheck(env);
  }
  env.getQualifierScopes(qualifierScopes, name, LF_DECLARATOR);
  env.extendScopeSeq(qualifierScopes);

  if (init) dt.dflags |= DF_INITIALIZED;

  // get the variable from the IDeclarator
  decl->tcheck(env, dt, false /*inGrouping*/);
  if (!dt.var) {
    return;      // an error was found and reported; bail rather than die later
  }
  var = env.storeVar(dt.var);
  type = env.tfac.cloneType(dt.type);
  context = dt.context;

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
  // UPDATE: dsw: why wait until pass 2?  I need to change it to pass
  // 1 to get in/d0088.cc to work and all the other elsa and oink
  // tests also work
  #warning investigate this
  if (init) {
    // TODO: check the initializer for compatibility with
    // the declared type

    // TODO: check compatibility with dflags; e.g. we can't allow
    // an initializer for a global variable declared with 'extern'

    tcheck_init(env);
  }

  // not sure what's the best place to test this, nor what the
  // exact rule is; let's try this...
  //
  // UPDATE: dsw: the problem is that gcc allows it
  #warning this should be a CCLang flag, not an ifdef
  #ifndef GNU_EXTENSION
  if (isVariableDC(dt.context) &&
      !dt.hasFlag(DF_EXTERN) &&
      type->isArrayType() &&
      type->asArrayType()->size == ArrayType::NO_SIZE) {
    env.error("array must have a size in variable declaration");
  }
  #endif // GNU_EXTENSION

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
                var2->getOverloadSet(), var->type->asFunctionType())) {
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

  // TODO: in the case of class data members, delay checking the
  // initializer until the entire class body has been scanned
      
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
  if (type->isArrayType()) {
    type->asArrayType()->size = var->type->asArrayType()->size;
  }
}


// ------------------ IDeclarator ------------------
// Check some restrictions regarding the use of 'operator'; might
// add some errors to the environment, but otherwise doesn't
// change anything.  Parameters are same as D_name_tcheck, plus
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

  // cppstd doesn't say this explicitly, but every place that mentions
  // what an operator can be allows only non-static members, if it
  // allows members at all
  // UPDATE: dsw: operator new() has to be static
  #warning so make it specific to new
//    if (scope->curCompound && (dt.dflags & DF_STATIC)) {
//      env.error(loc, "operator member functions cannot be static");
//    }

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
        NONMEMBER | TWOPARAMS,                      // OP_BITAND
        NONMEMBER | TWOPARAMS,                      // OP_BITXOR
        NONMEMBER | TWOPARAMS,                      // OP_BITOR

        // 13.5.3
        TWOPARAMS,                                  // OP_ASSIGN
        TWOPARAMS,                                  // OP_PLUSEQ
        TWOPARAMS,                                  // OP_MINUSEQ
        TWOPARAMS,                                  // OP_MULTEQ
        TWOPARAMS,                                  // OP_DIVEQ
        TWOPARAMS,                                  // OP_MODEQ
        TWOPARAMS,                                  // OP_LSHIFTEQ
        TWOPARAMS,                                  // OP_RSHIFTEQ
        TWOPARAMS,                                  // OP_BITANDEQ
        TWOPARAMS,                                  // OP_BITXOREQ
        TWOPARAMS,                                  // OP_BITOREQ

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


// This function is called whenever a constructed type is passed to a
// lower-down IDeclarator which *cannot* accept member function types.
// (sm 7/10/03: I'm now not sure exactly what that means...)
void possiblyConsumeFunctionType(Env &env, Declarator::Tcheck &dt)
{
  if (dt.funcSyntax) {
    if (dt.funcSyntax->cv != CV_NONE) {
      env.error("cannot have const/volatile on nonmember functions");
    }
    dt.funcSyntax = NULL;

    // close the parameter list
    dt.type->asFunctionType()->doneParams();
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
  ft->doneParams();
}


// This function is perhaps the most complicated in this entire
// module.  It has the responsibility of adding a variable called
// 'name' to the environment.  But to do this it has to implement the
// various rules for when declarations conflict, overloading,
// qualified name lookup, etc.
//
// Update: I've now broken some of this mechanism apart and implemented
// the pieces in Env, so it's perhaps a bit less complicated now.
static void D_name_tcheck(
  // environment in which to do general lookups
  Env &env,

  // contains various information about 'name', notably it's type
  Declarator::Tcheck &dt,
  
  // true if we're a D_grouping is innermost to a D_pointer
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
  // put something reasonable into the 'dt.var' field
  makeDummyVar:
  {
    if (!consumedFunction) {
      possiblyConsumeFunctionType(env, dt);
    }

    // the purpose of this is to allow the caller to have a workable
    // object, so we can continue making progress diagnosing errors
    // in the program; this won't be entered in the environment, even
    // though the 'name' is not NULL
    dt.var = env.makeVariable(loc, unqualifiedName, dt.type, dt.dflags);

    // set up the variable's 'scope' field as if it were properly
    // entered into the scope; this is for error recovery, in particular
    // for going on to check the bodies of methods
    scope->registerVariable(dt.var);

    return;
  }

realStart:
  if (!name) {
    // no name, nothing to enter into environment
    possiblyConsumeFunctionType(env, dt);
    dt.var = env.makeVariable(loc, NULL, dt.type, dt.dflags);
    return;
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
      possiblyConsumeFunctionType(env, dt);     // TODO: can't befriend cv members..
      dt.var = env.makeVariable(loc, unqualifiedName, dt.type, dt.dflags);
      return;
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
      scope->curCompound->name == NULL &&
      !scope->curCompound->syntax->tcheckAsIfNamed
     ) {
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
  // class member lists..
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
        env.error(dt.type, stringc
          << "the name `" << *name << "' is overloaded, but the type `"
          << dt.type->toString() << "' doesn't match any of the "
          << howMany << " declared overloaded instances");
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

  // is this a nonstatic member function?
  if (dt.type->isFunctionType()) {
    if (scope->curCompound &&
        !isFriend &&
        !isConstructor &&               // ctors don't have a 'this' param
        !(dt.dflags & DF_STATIC) &&
        (!name->hasQualifiers() ||
         prior->type->asFunctionTypeC()->isMethod())) {
      TRACE("memberFunc", "member function: " << *name);

      // add the implicit 'this' parameter
      makeMemberFunctionType(env, dt, scope->curCompound, loc);
    }
    else {
      TRACE("memberFunc", "non-member function: " << *name);
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

  // For function templates at least we need to intercept template
  // specializations (at least complete specializiations, since this
  // only really matters for the instantiations and there is never all
  // call to instantiate template for a complete specialization) so
  // that they do not make another variable for their definition of
  // there is already one for their declaration; What will happen
  // otherwise for function templates is that they will appear to be
  // in the overload set of their primary, which is what the lookup
  // for 'prior' found above.
  //
  // sm: 7/30/04: For in/t0230.cc, the call to
  // 'findTemplPrimaryForSignature' yields a bogus error about
  // ambiguity.  Since the comment above suggests this is only
  // important for specializations, and functions cannot be partially
  // specialized, I'm adding a check that we only enter the following
  // block of the type is concrete (no variables).  I don't think this
  // is the right fix, in fact I suspect this entire block should
  // simply be deleted and some fix made elsewhere, but it will do for
  // now.
  if (overloadSet &&
      dt.type->isFunctionType() &&
      !dt.type->containsVariables()) {
    // FIX: other places I used MM_WILD, but I think MM_BIND is
    // probably right; UPDATE: MM_BIND doesn't work because the
    // signature might contain type variables, which is not allowed
    Variable *prim = env.findTemplPrimaryForSignature
      (overloadSet, dt.type->asFunctionType(), MatchTypes::MM_WILD);
    if (prim) {
      xassert(prim->templateInfo());
      xassert(prim->templateInfo()->isPrimary());

      // get a list of STemplateArguments for the following query
      SObjList<STemplateArgument> sargs;
      if (name->isPQ_template()) {
        env.templArgsASTtoSTA(name->asPQ_templateC()->args, sargs);
      }

      Variable *prevInst = env.getInstThatMatchesArgs
        (prim->templateInfo(), sargs, dt.type);
      if (prevInst) {
        prior = prevInst;
      }
    }
  }

  // make a new variable; see implementation for details
  dt.var = env.createDeclaration(loc, unqualifiedName, dt.type, dt.dflags,
                                 scope, enclosingClass, prior, overloadSet);
}

void D_name::tcheck(Env &env, Declarator::Tcheck &dt, bool inGrouping)
{
  env.setLoc(loc);

  // 7/27/04: This has been disabled because Declarator::mid_tcheck
  // takes care of tchecking the name in advance.
  //if (name) {
  //  name->tcheck(env);
  //}

  // do *not* call 'possiblyConsumeFunctionType', since D_name_tcheck
  // will do so if necessary, and in a different way

  this->type = dt.type;     // annotation
  D_name_tcheck(env, dt, inGrouping, loc, name);
}


// cppstd, 8.3.2 para 4:
//   "There shall be no references to references, no arrays of
//    references, and no pointers to references."

void D_pointer::tcheck(Env &env, Declarator::Tcheck &dt, bool inGrouping)
{
  env.setLoc(loc);
  possiblyConsumeFunctionType(env, dt);

  if (dt.type->isReference()) {
    env.error(stringc
      << "cannot create a "
      << (isPtr? "pointer" : "reference")
      << " to a reference");
  }
  else {
    // apply the pointer type constructor
    if (!dt.type->isError()) {
      dt.type = env.tfac.syntaxPointerType(loc, isPtr, cv, dt.type, this);
    }

    // annotation
    this->type = dt.type;
  }
  
  // recurse
  base->tcheck(env, dt, false /*inGrouping*/);
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


void D_func::tcheck(Env &env, Declarator::Tcheck &dt, bool inGrouping)
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

      // conversion operator (grammar ensures must be ON_conversion)
      if (name->isPQ_operator()) {
        ON_conversion *conv = name->asPQ_operator()->o->asON_conversion();

        if (params->isNotEmpty()) {
          env.error("conversion operator cannot accept arguments");
        }

        // compute the named type; this becomes the return type
        ASTTypeId::Tcheck tc(DF_NONE, DC_ON_CONVERSION);
        conv->type = conv->type->tcheck(env, tc);
        dt.type = conv->type->getType();
        specialFunc = FF_CONVERSION;
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
          // I'm not sure if either of the following two error conditions
          // can occur, because I don't parse something as a ctor unless
          // some very similar conditions hold
          if (!inClass) {
            env.error("constructors must be class members");
            return;    // otherwise would segfault below..
          }
          else if (nameString != inClass->name) {
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
    
    else {     // C
      if (env.lang.allowImplicitIntRetType) {
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

  // grab the template parameters before entering the parameter scope
  //
  // dsw: FIX: I think this is seriously broken in the case of
  // typechecking a function template instantiation and just by
  // accident doesn't show up in any of our tests; maybe it is not
  // possible for it to go wrong, but it sure is weird.
  //
  // sm: I agree this isn't right in all cases, though I don't quite
  // get the point you're making.
  TemplateInfo *templateInfo = env.takeFTemplateInfo();

  // make a new scope for the parameter list
  Scope *paramScope = env.enterScope(SK_PARAMETER, "D_func parameter list scope");
//    cout << "D_func::tcheck env.locStr() " << env.locStr() << endl;
//    if (templateInfo) templateInfo->debugPrint();
//    env.debugPrintScopes();

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
  if (env.lang.emptyParamsMeansPureVarargFunc && params->isEmpty()) {
    xassert(ct==0);
    ft->flags |= FF_VARARGS;
  }

  // the verifier will type-check the pre/post at this point
  env.checkFuncAnnotations(ft, this);

  env.exitScope(paramScope);

  if (exnSpec) {
    ft->exnSpec = exnSpec->tcheck(env);
  }

  // call this after attaching the exception spec, if any
  //ft->doneParams();
  // update: doneParams() is done by 'possiblyConsumeFunctionType'
  // or 'D_name_tcheck', depending on what declarator is next in
  // the chain

  // now that we've constructed this function type, pass it as
  // the 'base' on to the next-lower declarator
  dt.type = ft;
  this->type = dt.type;       // annotation

  // if this variable is a specialization, then mark it as such so
  // that in particular it doesn't get added to the overload set of
  // its primary during typechecking below
  if (templateInfo) {
    if (templateInfo->isMutant()) {
      // strange case that happens when one function template body
      // instantiates another; make sure that this concern it
      // irrelevant since nothing will get added to the scope
    } else if (templateInfo->isCompleteSpecOrInstantiation() ||
               templateInfo->isPartialSpec()) {
      dt.dflags |= DF_TEMPL_SPEC;
    }
  }

  base->tcheck(env, dt, inGrouping);
  xassert(dt.var);

  // don't stomp on an existing template info
  if (!dt.var->templateInfo()) {
//        cout << "before: dt.var->setTemplateInfo(templateInfo)" << endl;
//        cout << "templateInfo->debugPrint()" << endl;
//        if (templateInfo) templateInfo->debugPrint();
//        if (params->first()) {
//          cout << "params->first()->gdb()" << endl;
//          params->first()->gdb();
//        }

    dt.var->setTemplateInfo(templateInfo);
    if (getDeclaratorId() &&
        getDeclaratorId()->isPQ_template()) {
      // some disambiguation situations don't end up attaching a
      // template info: elsa/in/d0005.cc
      if (!dt.var->templateInfo()) {
        // FIX: this can happen if we have gone down the wrong
        // branch of a disambiguation.  It can also happen if we are
        // in a function template instantiation where we are
        // typechecking the cloned AST.  Since it isn't easy to
        // check for this second case, I just avoid it.  All I
        // really need the arguments for is when we are creating a
        // declaration so that when we find the definition later we
        // can match them up.
//            xassert(env.hasDisambErrors());
      } else {
        env.initArgumentsFromASTTemplArgs(dt.var->templateInfo(),
                                          getDeclaratorId()->asPQ_templateC()->args);
      }
    }
  }
}


void D_array::tcheck(Env &env, Declarator::Tcheck &dt, bool inGrouping)
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
        this->type = dt.type;       // annotation
        base->tcheck(env, dt, inGrouping);
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
        if (dt.context == DC_S_DECL &&
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
          if (env.scope()->scopeKind == SK_CLASS) {
            if (sz < 0) {
              env.error(loc, "member array size must be nonnegative");
            }
          }
          else {
            if (sz <= 0) {
              env.error(loc, "array size must be positive");
            }
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
  this->type = dt.type;       // annotation
  base->tcheck(env, dt, inGrouping);
}


void D_bitfield::tcheck(Env &env, Declarator::Tcheck &dt, bool inGrouping)
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

  this->type = dt.type;       // annotation
  D_name_tcheck(env, dt, inGrouping, loc, name);
}


// this function is very similar to D_pointer::tcheck
void D_ptrToMember::tcheck(Env &env, Declarator::Tcheck &dt, bool inGrouping)
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
    this->type = dt.type = env.getSimpleType(SL_UNKNOWN, ST_DEPENDENT);
    base->tcheck(env, dt, false /*inGrouping*/);
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

  // annotation
  this->type = dt.type;

  // recurse
  base->tcheck(env, dt, false /*inGrouping*/);
}


void D_grouping::tcheck(Env &env, Declarator::Tcheck &dt, bool inGrouping)
{
  env.setLoc(loc);

  // don't call 'possiblyConsumeFunctionType', since the
  // D_grouping is supposed to be transparent

  this->type = dt.type;       // annotation

  // the whole purpose of this AST node is to communicate
  // this one piece of context: inGrouping = true
  base->tcheck(env, dt, true /*inGrouping*/);
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

    const_cast<Statement*&>(expr->ambiguity) = NULL;
    const_cast<Statement*&>(decl->ambiguity) = expr;

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
    call->inner1_itcheck(env);
    if (noDisambErrors(env.errors)) {
      // ok, finish up; it's safe to assume that the E_constructor
      // interpretation would fail if we tried it
      TRACE("disamb", toString(loc) << ": selected E_funCall");
      env.errors.prependMessages(existing);
      call->type = env.tfac.cloneType(call->inner2_itcheck(env));
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

Type *E_intLit::itcheck_x(Env &env, Expression *&replacement)
{
  // TODO: this is wrong; see cppstd 2.13.1 para 2
  i = strtoul(text, NULL /*endp*/, 0 /*radix*/);
  return env.getSimpleType(SL_UNKNOWN, ST_INT);
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
    if (id==ST_WCHAR_T) len--;    // don't count 'L' if present
    p = p->continuation;
  }

  Type *charConst = env.getSimpleType(SL_UNKNOWN, id, CV_CONST);
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
  // TODO: wrong; cppstd 2.13.2 paras 1 and 2

  int tempLen;
  string temp;

  char const *srcText = text;
  if (*srcText == 'L') srcText++;

  quotedUnescape(temp, tempLen, srcText, '\'',
                 false /*allowNewlines*/);
  if (tempLen == 0) {
    return env.error("character literal with no characters");
  }
  else if (tempLen > 1) {
    // below I only store the first byte
    env.warning("wide character literals not properly implemented");
  }

  c = (unsigned int)temp[0];
  
  return env.getSimpleType(SL_UNKNOWN, ST_CHAR);
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

  // compute type: *pointer* to the thing 'receiverVar' is
  // a *reference* to
  //
  // (sm: it seems to me that type cloning should be unnecessary since
  // this is intended to be the *same* object as __receiver)
  return env.makePointerType(env.loc(), CV_NONE,
                             receiver->type->asRval());
}


Type *E_variable::itcheck_x(Env &env, Expression *&replacement)
{
  return itcheck_var(env, LF_NONE);
}

Type *E_variable::itcheck_var(Env &env, LookupFlags flags)
{
  name->tcheck(env);
  Variable *v = env.lookupPQVariable(name, flags);
  if (!v) {
    // dsw: In K and R C it seems to be legal to call a function
    // variable that has never been declareed.  At this point we
    // notice this fact and if we are in K and R C we insert a
    // variable with signature "int (...)" which is what I recall as
    // the correct signature for such an implicit variable.
    if (env.lang.allowCallToUndeclFunc && (flags & LF_IMPL_DECL_FUNC)) {
      // this should happen in C mode only so name must be a PQ_name
      v = env.makeUndeclFuncVar(name->asPQ_name()->name);
    } else {
      #warning this should be a CCLang flag, not an ifdef
      #ifdef GNU_EXTENSION
      // dsw: I need a way of handling builtins; there are too many to
      // add manually
      // FIX: these should be somewhere else
      static char const * const builtin = "__builtin";
      static int const builtin_len = strlen(builtin);
      if (name->isPQ_name() &&
          strncmp(name->asPQ_name()->name, builtin, builtin_len)==0) {
        v = env.makeUndeclFuncVar(name->asPQ_name()->name);
      } else {
      #endif // GNU_EXTENSION

      // 10/23/02: I've now changed this to non-disambiguating,
      // prompted by the need to allow template bodies to call
      // undeclared functions in a "dependent" context [cppstd 14.6
      // para 8].  See the note in TS_name::itcheck.
      return env.error(name->loc, stringc
                       << "there is no variable called `" << *name << "'",
                       EF_NONE);

      #ifdef GNU_EXTENSION
      } // goes with the 'if (name->isPQ_name() && ...' above
      #endif // GNU_EXTENSION

    }
  }
  xassert(v);

  if (v->hasFlag(DF_TYPEDEF)) {
    return env.error(name->loc, stringc
      << "`" << *name << "' used as a variable, but it's actually a typedef",
      EF_DISAMBIGUATES);
  }
       
  // TODO: access control check

  var = env.storeVarIfNotOvl(v);

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
static Variable *outerResolveOverload(Env &env,
                                      PQName * /*nullable*/ finalName,
                                      SourceLoc loc, Variable *var,
                                      Type *receiverType, FakeList<ArgExpression> *args)
{
  // are we even in a situation where we can do overloading?
  if (!var->overload) return NULL;

  // special case: does 'finalName' directly name a particular
  // conversion operator?  e.g. in/t0226.cc
  if (finalName &&
      finalName->isPQ_operator() &&
      finalName->asPQ_operator()->o->isON_conversion()) {
    ON_conversion *conv = finalName->asPQ_operator()->o->asON_conversion();
    Type *namedType = conv->type->getType();

    // find the operator in the overload set
    SFOREACH_OBJLIST_NC(Variable, var->overload->set, iter) {
      Type *iterRet = iter.data()->type->asFunctionType()->retType;
      if (iterRet->equals(namedType)) {
        return iter.data();
      }
    }

    env.error(stringc << "cannot find conversion operator yielding `" 
                      << namedType->toString() << "'");
    return NULL;
  }

  // temporarily avoid dealing with considerations of mixed static and
  // non-static members in overload set
  bool allMethod0 = allMethods(var->overload->set);
  bool allNonMethod0 = allNonMethods(var->overload->set);
  if (! (allMethod0 || allNonMethod0) ) return NULL;

  xassert( (((int)allMethod0) + ((int)allNonMethod0)) == 1 );
  OVERLOADINDTRACE(::toString(loc)
        << ": overloaded(" << var->overload->set.count()
        << ") call to " << var->name);

  // fill an array with information about the arguments
  GrowArray<ArgumentInfo> argInfo(args->count() + (allMethod0?1:0) );
  {
    int index = 0;

    if (allMethod0) {
      if (!receiverType) {
        // dsw: error message parallels that of g++; feel free to change it
        // sm: I suspect we'll actually want to remove it, since the tcheck
        // that follows overload resolution will catch this error also.
        env.error("Cannot call member function without object");
        return NULL;
      }
      // TODO: take into account whether the receiver is an rvalue
      // or an lvalue
      argInfo[index] = ArgumentInfo(SE_NONE, makeLvalType(env, receiverType));
      index++;
    }

    FAKELIST_FOREACH_NC(ArgExpression, args, iter) {
      argInfo[index] = ArgumentInfo(iter->getSpecial(), iter->getType());
      index++;
    }
  }

  // resolve overloading
  bool wasAmbig;     // ignored, since error will be reported
  return resolveOverload(env, loc, &env.errors,
                         OF_NONE, var->overload->set, finalName, argInfo, wasAmbig);
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


// dsw: this function should eventually be the constant function
// "true" except for any references to env.doOverload (and possibly
// env.lang.allowOverloading, though that's handled elsewhere for now)
static bool reallyDoOverload(Env &env, FakeList<ArgExpression> *args) {
  Env::TemplTcheckMode mode = env.getTemplTcheckMode();
  // note: there is a situation in which mode can actually be
  // TTM_2TEMPL_FUNC_DECL: if you are in the parameter list of the
  // definition of a templatized function and it refers to the
  // instantiation of a templatized class and in the constructor for
  // that class there is a member initializer that calls a constructor
  // function; this comes up in in/big/nsCLiveconnectFactory.i:22245
  // UPDATE: other changes should be fixing this
  xassert(mode == Env::TTM_1NORMAL || mode == Env::TTM_3TEMPL_DEF);
  return env.doOverload         // user wants overloading
    && mode == Env::TTM_1NORMAL; // we are in real code
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
  inner1_itcheck(env);

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

  return inner2_itcheck(env);
}

void E_funCall::inner1_itcheck(Env &env)
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

  Expression *f = func->skipGroups();
  if (f->isE_variable()) {
    // tell the E_variable *not* to do instantiation of
    // templates, because that will need to wait until 
    // we see the argument types
    //
    // what follows is essentially Expression::tcheck
    // specialized to E_variable, but with a special lookup
    // flag passed
    E_variable *evar = f->asE_variable();
    xassert(!evar->ambiguity);
    func->type = evar->type = env.tfac.cloneType(
      evar->itcheck_var(env, LF_TEMPL_PRIMARY | LF_IMPL_DECL_FUNC));
  }
  // TODO: need similar handling for member function templates
  else {
    // do the general thing
    func->tcheck(env, func);
  }
}

Type *E_funCall::inner2_itcheck(Env &env)
{
  if (func->isE_variable() &&
      func->asE_variable()->name->getName() == env.special_testOverload) {
    // if I'm trying to test it, I want it performed; do this up here
    // so I turn it on before resolving the arguments
    env.doOverload = true;
    env.doOperatorOverload = true;     // this too, when test wants overloading
  }

  // check the argument list
  args = tcheckArgExprList(args, env);

  // argument-dependent re-lookup and function template instantiation
  if (func->skipGroups()->isE_variable()) {
    E_variable *evar = func->skipGroups()->asE_variable();
    evar->var = env.lookupPQVariable_function_with_args(
      evar->name, LF_NONE, args /*funcArgs*/);
    if (evar->var) {
      evar->type = env.tfac.cloneType(evar->var->type);
    }
    else {
      // error already reported, but don't proceed below since
      // the NULL evar->var will cause problems
      return evar->type = env.getSimpleType(SL_UNKNOWN, ST_ERROR);
    }
  }

  // the type of the function that is being invoked
  Type *t = func->type->asRval();

  // automatically coerce function pointers into functions
  if (t->isPointerType()) {
    t = t->asPointerTypeC()->atType;
    // if it is an E_variable then its overload set will be NULL so we
    // won't be doing overload resolution in this case
  }

  // check for operator()
  CompoundType *ct = t->ifCompoundType();
  if (ct) {
    Variable const *funcVar = ct->getNamedFieldC(env.functionOperatorName, env);
    if (funcVar) {
      t = funcVar->type;
      
      // TODO: There are (at least) two problems here:
      //   - 'operator()' may be overloaded, but we only pick the first
      //   - the AST should be rewritten to explicitly refer to
      //     'operator()' so that elaboration works correctly
    }
    else {
      // fall through, error case below handles it
    }
  }

  if (!t->isFunctionType()) {
    return env.error(func->type->asRval(), stringc
      << "you can't use an expression of type `" << func->type->toString()
      << "' as a function");
  }
  
  // skip grouping parens (cppstd 13.3.1.1 para 1)
  Expression *func = this->func->skipGroups();

  // for internal testing
  if (func->isE_variable()) {
    Type *ret = internalTestingHooks(env, 
      func->asE_variable()->name->getName(), args);
    if (ret) {
      return ret;
    }
  }

  // check for function calls that need overload resolution
  if (reallyDoOverload(env, args)) {
    // simple E_funCall to a named function
    if (func->isE_variable()) {
      E_variable *evar = func->asE_variable();
      
      Variable *chosen = 
        outerResolveOverload(env, evar->name, evar->name->loc, evar->var, 
                             env.implicitReceiverType(), args);
      if (chosen) {
        // rewrite AST to reflect choice
        //
        // the stored type will be the dealiased type in hopes this
        // achieves 7.3.3 para 13, "This has no effect on the type of
        // the function, and in all other respects the function
        // remains a member of the base class."
        chosen = env.storeVar(chosen);
        evar->var = chosen;
        evar->type = env.tfac.cloneType(chosen->type);
        t = chosen->type;    // for eventual return value
      }
      else {
        // dealias anyway
        evar->var = env.storeVar(evar->var);
      }
    }

    // method call to a named function
    if (func->isE_fieldAcc() &&
        // in the case of a call to a compiler-synthesized
        // destructor, the 'field' is currently NULL (that might
        // change); but in that case overloading is not possible,
        // so this code can safely ignore it (e.g. in/t0091.cc)
        func->asE_fieldAcc()->field) {
      E_fieldAcc *efld = func->asE_fieldAcc();

      Variable *chosen =
        outerResolveOverload(env, efld->fieldName, efld->fieldName->loc, efld->field,
                             efld->obj->type, args);
      if (chosen) {
        // rewrite AST
        chosen = env.storeVar(chosen);
        efld->field = chosen;
        efld->type = env.tfac.cloneType(chosen->type);
        t = chosen->type;
      }
      else {
        efld->field = env.storeVar(efld->field);
      }
    }
  }

  // make sure this function has been typechecked; FIX: when we
  // explicitly elaborate in the implicit "this->" then this code
  // should become redundant
  if (func->isE_variable()) {
    // if it is a pointer to a function that function should get
    // instantiated when its address is taken
    if (func->asE_variable()->var->type->isFunctionType()) {
      env.ensureFuncMemBodyTChecked(func->asE_variable()->var);
    }
  }

  // TODO: make sure the argument types are compatible
  // with the function parameters

  // dsw: doesn't overloading succeeding guarantee this?

  // sm: Actually, it doesn't.  First, if there's no overload set,
  // then we won't have done any such resolution.  Moreover, there are
  // instances where overload resolution will choose a candidate that
  // later yields an error (e.g. access control, binding a non-const
  // reference to a temporary, etc.).  So, the strategy is to do
  // overload resolution when necessary to pick the best type 't', but
  // then tcheck the arguments against 't' as if we had no reason to
  // suspect 't' was a good function to call.

  FunctionType *ft = t->asFunctionType();

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
         args->nth(0)->getSpecial(),     // is it special?
         args->nth(0)->getType(),        // source type
         args->nth(1)->getType(),        // dest type
         expect);                        // expected result
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
         args->nth(0)->getSpecial(),     // is it special?
         args->nth(0)->getType(),        // source type
         args->nth(1)->getType(),        // dest type
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

  // dsw: I will assume for now that if overloading succeeds, that the
  // checking that it implies subsumes the below concern:
  // TODO: make sure the argument types are compatible
  // with the constructor parameters

  if (type->isCompoundType() &&
      !type->asCompoundType()->isComplete()) {
    return env.error(stringc
      << "attempt to construct incomplete type `" << type->toString() << "'");
  }

  Variable *ctor = outerResolveOverload_ctor(env, env.loc(), type, args,
                                             reallyDoOverload(env, args));
  if (ctor) {
    env.ensureFuncMemBodyTChecked(ctor);
    ctorVar = env.storeVar(ctor);
  }

  return type;
}


// cppstd sections: 5.2.5 and 3.4.5
Type *E_fieldAcc::itcheck_x(Env &env, Expression *&replacement)
{
  obj->tcheck(env, obj);
  fieldName->tcheck(env);   // shouldn't have template arguments, but won't hurt

  // get the type of 'obj', and make sure it's a compound
  Type *rt = obj->type->asRval();
  CompoundType *ct = rt->ifCompoundType();
  if (!ct) {
    // maybe it's a type variable because we're accessing a field
    // of a template parameter?
    if (rt->isTypeVariable()) {
      // call it a reference to the 'dependent' variable instead of
      // leaving it NULL; this helps the Cqual++ type checker a little
      field = env.dependentTypeVar;
      return field->type;
    }

    if (fieldName->getName()[0] == '~') {
      // invoking destructor explicitly, which is allowed for all
      // types; most of the time, the rewrite in E_funCall::itcheck
      // will replace this, but in the case of a type which is an
      // array of objects, this will leave the E_fieldAcc's 'field'
      // member NULL ...
      return env.makeDestructorFunctionType(SL_UNKNOWN, NULL /*ct*/);
    }

    return env.error(rt, stringc
      << "non-compound `" << rt->toString()
      << "' doesn't have fields to access");
  }
  
  // make sure the type has been completed
  if (!ct->isComplete()) {
    return env.error(rt, stringc
      << "attempt to access a member of incomplete type "
      << ct->keywordAndName());
  }

  // look for the named field
  Variable *f = ct->lookupPQVariable(fieldName, env);
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
  
  if (f->getType()->isFunctionType()) {
    env.ensureFuncMemBodyTChecked(f);
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
  // TODO: do overloading here; for now, just replace ourselves with a
  // '*' and a '.'
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


inline ArgumentInfo argInfo(Expression *e)
{
  return ArgumentInfo(e->getSpecial(), e->type);
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
  // consider the possibility of operator overloading
  if (env.doOperatorOverload &&
      (expr->type->asRval()->isCompoundType() ||
       expr->type->asRval()->isEnumType())) {
    OVERLOADINDTRACE("found overloadable unary " << toString(op) <<
                     " near " << env.locStr());
    StringRef opName = env.operatorName[op];

    // argument information
    GrowArray<ArgumentInfo> args(1);
    args[0] = argInfo(expr);

    // prepare resolver
    OverloadResolver resolver(env, env.loc(), &env.errors,
                              OF_NONE,
                              NULL, // I assume operators can't have explicit template arguments
                              args);

    // user-defined candidates
    resolver.addUserOperatorCandidates(expr->type, opName);

    // built-in candidates
    resolver.addBuiltinUnaryCandidates(op);

    // pick the best candidate
    Variable *winner = resolver.resolve();
    if (winner && !winner->hasFlag(DF_BUILTIN)) {
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
        // replace '~a' with '::operator~(a)'
        // TODO: that is wrong if namespaces exist
        replacement = new E_funCall(
          // function to invoke
          new E_variable(new PQ_qualifier(SL_UNKNOWN, NULL /*qualifier*/,
                                          NULL /*targs*/, pqo)),
          // arguments
          makeExprList1(expr)
        );
      }

      // for now, just re-check the whole thing
      replacement->tcheck(env, replacement);
      return replacement->type;
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
  if (!env.doOperatorOverload) {
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
    args[0] = argInfo(e1);
    if (e2) {
      args[1] = argInfo(e2);
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
    resolver.addUserOperatorCandidates(e1->type, opName);

    // built-in candidates
    resolver.addBuiltinBinaryCandidates(op, args[0].type, args[1].type);

    // pick one
    Variable *winner = resolver.resolve();
    if (winner) {
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
          // replace 'a+b' with '::operator+(a,b)'
          // (TODO: that's wrong in the presence of namespaces)
          replacement = new E_funCall(
            // function to invoke
            new E_variable(new PQ_qualifier(SL_UNKNOWN, NULL /*qualifier*/,
                                            NULL /*targs*/, pqo)),
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
      }
    }
  }
 
  // not replaced
  return NULL;
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

  // TODO: make sure 'expr' is compatible with given operator

  return env.getSimpleType(SL_UNKNOWN, ST_INT);
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

  return expr->type->asRval();
}


Type *E_binary::itcheck_x(Env &env, Expression *&replacement)
{
  e1->tcheck(env, e1);
  e2->tcheck(env, e2);

  // help disambiguate t0182.cc
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

  if (op == BIN_BRACKETS) {
    // built-in a[b] is equivalent to *(a+b)
    replacement = new E_deref(new E_binary(e1, BIN_PLUS, e2));
    replacement->tcheck(env, replacement);
    return replacement->type;
  }

  // get types of arguments, converted to rval
  Type *lhsType = e1->type->asRval();
  Type *rhsType = e2->type->asRval();

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
  expr->tcheck(env, expr);

  if (expr->type->isError()) {
    // skip further checking because the tree is not necessarily
    // as the later stages expect; e.g., an E_variable might have
    // a NULL 'var' field
    return expr->type;
  }

  // NOTE: do *not* unwrap any layers of parens:
  // cppstd 5.3.1 para 3: "A pointer to member is only formed when
  // an explicit & is used and its operand is a qualified-id not
  // enclosed in parentheses."
  if (expr->isE_variable()) {
    E_variable *e_var = expr->asE_variable();
    xassert(e_var->var);
    // make sure we instantiate any functions that have their address
    // taken
    if (e_var->var->type->isFunctionType()) {
      env.ensureFuncMemBodyTChecked(e_var->var);
    }
    if (e_var->var->hasFlag(DF_MEMBER) &&
        (!e_var->var->hasFlag(DF_STATIC)) &&
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
        e_var->name->hasQualifiers() ) {
      return makePTMType(env, e_var);
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
    if (ct->lookupVariableC(env.operatorName[OP_STAR], env)) {
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
    if (ct->lookupVariableC(env.operatorName[OP_BRACKETS], env)) {
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
  
  return ctype->getType();
}


Type *E_cond::itcheck_x(Env &env, Expression *&replacement)
{
  cond->tcheck(env, cond);
  // In gcc it is legal to omit the 'then' part;
  // http://gcc.gnu.org/onlinedocs/gcc-3.4.1/gcc/Conditionals.html#Conditionals
  if (th) {
    th->tcheck(env, th);
  }
  el->tcheck(env, el);
  
  // TODO: verify 'cond' makes sense in a boolean context
  // TODO: verify 'th' and 'el' return the same type

  // dsw: shouldn't the type of the expression should be the least
  // upper bound (lub) of the types?
  // sm: sort of.. the rules are spelled out in cppstd 5.16.  there's
  // no provision for computing the least common ancestor in the class
  // hierarchy, but the rules *are* nonetheless complex

  Type *thType = NULL;
  if (th) {
    thType = th->type;
  } else {
    thType = cond->type;
  }

  // dsw: if one of them is NULL, use the other one; see in/d0095.cc;
  // actually gcc only allows this if it is not just void* but also 0,
  // so this is too lenient
  #warning this should be a CCLang flag, not an ifdef
  #ifdef GNU_EXTENSION
  if (thType->isPointerType() &&
      thType->asPointerType()->atType->isVoid()) {
    return el->type;
  }
  #endif // GNU_EXTENSION
  return thType;
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

  // dsw: I think under some gnu extensions perhaps sizeof's aren't
  // const (like with local arrays that use a variable to determine
  // their size at runtime).  Therefore, not making const.
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
  if (t->isCompoundType() &&
      !t->asCompoundType()->isComplete()) {
    env.error(stringc << "cannot create an object of incomplete type `"
                      << t->toString() << "'");
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
                                reallyDoOverload(env, ctorArgs->list));
    // ctor0 can be null when the type is a simple type, such as an
    // int; I assume that ctor0 being NULL is the correct behavior in
    // that case
    if (ctor0) {
      env.ensureFuncMemBodyTChecked(ctor0);
    }
    ctorVar = env.storeVar(ctor0);
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
  return env.type_info_const_ref;
}


Type *E_typeidType::itcheck_x(Env &env, Expression *&replacement)
{
  ASTTypeId::Tcheck tc(DF_NONE, DC_E_TYPEIDTYPE);
  ttype = ttype->tcheck(env, tc);
  return env.type_info_const_ref;
}


Type *E_grouping::itcheck_x(Env &env, Expression *&replacement)
{
  expr->tcheck(env, expr);
  return expr->type;
}


// --------------------- Expression constEval ------------------
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
    ASTCASEC(E_boolLit, b)
      result = b->b? 1 : 0;
      return true;

    ASTNEXTC(E_intLit, i)
      result = i->i;
      return true;

    ASTNEXTC(E_charLit, c)
      result = c->c;
      return true;

    ASTNEXTC(E_variable, v)
      if (v->var->hasFlag(DF_ENUMERATOR)) {
        // this is an enumerator; find the corresponding
        // enum type, and look up the name to find the value
        EnumType *et = v->var->type->asCVAtomicType()->atomic->asEnumType();
        EnumType::Value const *val = et->getValue(v->var->name);
        xassert(val);    // otherwise the type information is wrong..
        result = val->value;
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
        // In gcc it is legal to omit the 'then' part;
        // http://gcc.gnu.org/onlinedocs/gcc-3.4.1/gcc/Conditionals.html#Conditionals
        if (c->th) {
          return c->th->constEval(msg, result);
        } else {
          return result;
        }
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
      msg = stringc << kindName() << " is not constEval'able";
      return false;

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
      // In gcc it is legal to omit the 'then' part;
      // http://gcc.gnu.org/onlinedocs/gcc-3.4.1/gcc/Conditionals.html#Conditionals
             (c->th ? c->th->hasUnparenthesizedGT() : false) ||
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
      return false;

    ASTENDCASEC
  }
}


SpecialExpr Expression::getSpecial() const
{
  ASTSWITCHC(Expression, this) {
    ASTCASEC(E_intLit, i)
      return i->i==0? SE_ZERO : SE_NONE;
     
    ASTNEXTC(E_stringLit, s)
      PRETEND_USED(s);
      return SE_STRINGLIT;
      
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
    outerResolveOverload_ctor(env, loc, type, args, reallyDoOverload(env, args)));
}


// -------------------- TemplateDeclaration ---------------
void TemplateDeclaration::tcheck(Env &env)
{
  // Note: This code has been partially copied to TD_tmember::itcheck (below).

  // if this is a complete specialization put nothing on the stack as
  // we are still in normal code
  bool inCompleteSpec = params->isEmpty();
  // Second star to the right, and straight on till morning
  Restorer<Env::TemplTcheckMode> changeMode(env.tcheckMode,
    inCompleteSpec? env.tcheckMode : Env::TTM_3TEMPL_DEF);

  // make a new scope to hold the template parameters
  Scope *paramScope = env.enterScope(SK_TEMPLATE, "template declaration parameters");

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
  f->tcheck(env, true /*checkBody*/);

  // dsw: Template function specializations (both complete and
  // partial) have to get registered into the namespace somewhere.
  // For compound types it is done in TS_classSpec::itcheck() (from
  // which the code below is imitated), but I wonder why it isn't in
  // TD_class.  This seems to be the proper place for it to be done
  // for Functions.  Note that we can't do it down inside Function, as
  // that is called to typecheck cloned templates by
  // instantiateTemplate().

  PQName const *name = f->nameAndParams->decl->getDeclaratorIdC();
  FunctionType *funcType = f->nameAndParams->type->asFunctionType();
  Variable *fVar = f->nameAndParams->var;

  // do we have template arguments? that is, are we a specialization?
  // If so, hang us off of the appropriate primary.  If we are a
  // primary, skip the whole thing because we got entered into the
  // namespace during the tcheck() above.  Remember to look to see if
  // we have already been declared and if so, to re-use the
  // declaration and not duplicate it.
  if (!fVar->templateInfo()) {
    // we should be a function member of a class template primary
    //
    // FIX: do this
    xassert(!fVar->templateInfo());
  } else if (fVar->templateInfo()->isCompleteSpecOrInstantiation() ||
             fVar->templateInfo()->isPartialSpec()) {
    // find the primary
    Variable *primary = env.lookupPQVariable_primary_resolve
      (name, LF_TEMPL_PRIMARY, funcType, MatchTypes::MM_WILD);
    xassert(primary);
    xassert(primary->templateInfo());
    xassert(primary->templateInfo()->isPrimary());
    if (!primary) {
      env.error("cannot specialize a template that hasn't been declared");
      return;
    }
    TemplateInfo *primaryTI = primary->templateInfo();
    if (!primaryTI) {
      env.error("attempt to specialize a non-template");
      return;
    }
    xassert(primaryTI->isPrimary());

    // this is I suppose a completely gratuitous check that I thought
    // was important at some point
    PQName const *unqual = name->getUnqualifiedNameC();
    if (unqual->isPQ_template()) {
      PQ_template const *t = unqual->asPQ_templateC();
      ASTList<TemplateArgument> const &templateArgs = t->args;
      xassert(env.checkIsoToASTTemplArgs(fVar->templateInfo()->arguments, templateArgs));
    } else {
      xassert(fVar->templateInfo()->arguments.count() == 0);
    }

    // add this type to the primary's list of specializations; the
    // only way to find the specialization is to go through the
    // primary template
    //
    // is there already a forward declaration for it?
    Variable *forward = primaryTI->getInstantiationOfVar(env.tfac, fVar);
    if (forward) {
      xassert(forward->templateInfo()->getMyPrimaryIdem() == primaryTI);
      xassert(forward->funcDefn == f);
      // FIX: It would be nice if we could still detect this, but not
      // this way any more.
//        if (forward->funcDefn) {
//          env.error(stringc << "duplicate definition of specialization for " << fVar->toString());
//          return;
//        } else {
      env.provideDefForFuncTemplDecl(forward, primaryTI, f);
      env.instantiateForwardFunctions(forward, primary);
//        }
    } else {
      primaryTI->addInstantiation(env.tfac, fVar);
      if (tracingSys("template")) {
          cout << "TS_classSpec::itcheck: "
               << "definition of " << fVar->toString()
               << " appended to instantiation of primary" << endl;
        primaryTI->debugPrint();
      }
    }
    xassert(fVar->templateInfo()->getMyPrimaryIdem() == primaryTI);
  } else if (fVar->templateInfo()->isPrimary()) {
    // we should be the definition for a declared primary
    
    #if 0      // old
      Variable *primary = env.lookupPQVariable_primary_resolve
        (name, LF_TEMPL_PRIMARY, funcType, MatchTypes::MM_ISO);
    #else
      // sm: The goal appears to be to obtain the primary's Variable,
      // but that is already in 'fVar'.  I added an assertion that
      // the primary obtained by lookup is the same as 'fVar', and that
      // assertion passes in our current (7/23/04) suite.  However,
      // the lookup fails for t0218.cc because it should be using
      // 'constructor-special'.  Since 'fVar' already has been through
      // the processing that introduces 'constructor-special', I am
      // fixing t0218.cc by simply using 'fVar' instead of doing
      // another lookup.  
      //
      // Note that in general lookup is a complicated beast, and 
      // should be avoided when possible.  That is, if an identifier
      // appears in the source code, it should be looked up; but for
      // internally finding an object related to some other object
      // in hand, lookup should not be used.
      Variable *primary = fVar;
    #endif
    xassert(primary);

    TemplateInfo *primaryTI = primary->templateInfo();
    xassert(primaryTI);
    xassert(primaryTI->isPrimary());
    if (fVar->funcDefn) {
      if (fVar->funcDefn != f) {
        env.error(stringc << "Duplicate definition of function template primary " << fVar->name);
        return;
      }
      // nothing to do; we must have been the original definition
    } else {
      xfailure("Should never happen as function template primary def'n's reuse the decl");
      env.provideDefForFuncTemplDecl(fVar, primaryTI, f);
    }
    env.instantiateForwardFunctions(fVar, primary);
  }
}


void TD_proto::itcheck(Env &env)
{
  // check the declaration; works like TD_func because D_func is the
  // place we grab template parameters, and that's shared by both
  // definitions and prototypes
  DisambiguateOnlyTemp disOnly(env, true /*disOnly*/);
  d->tcheck(env, DC_TD_PROTO);
}


void TD_class::itcheck(Env &env)
{
  // check the class definition; it knows what to do about
  // the template parameters (just like for functions)
  type = spec->tcheck(env, DF_NONE);
}


void TD_tmember::itcheck(Env &env)
{
  // The params from 'this' object have already been set up; but I need
  // to dig down into 'd' and get its params too, since otherwise its
  // default 'tcheck' would obliterate the ones from 'this'.
  //
  // Note that the effect of this code is to make
  //   template <class S>
  //   template <class T>
  // indistinguishable from
  //   template <class S, class T>
  // but the spec clearly regards them as different.  One possible solution
  // is to have separate slots in Scope for class-params and member-params.
  // Another might be to change the way declarations gather extant parameters,
  // and have them look at the enclosing *two* scopes for template params
  // instead of just the innermost enclosing scope.  For now I merely note
  // these possibilities and move on.
  //
  // What follows is a specialized version of TemplateDeclaration::tcheck (above).

  // get the scope made by my caller
  Scope *paramScope = env.scope();
  xassert(paramScope->isTemplateScope() &&
          !paramScope->canAcceptNames);

  // temporarily allow it to accept more names
  paramScope->canAcceptNames = true;

  // check each of d's parameters
  FAKELIST_FOREACH_NC(TemplateParameter, d->params, iter) {
    iter->tcheck(env, paramScope->templateParams);
  }

  // revert acceptance
  paramScope->canAcceptNames = false;

  // check what is inside 'd', past its template parameters
  d->itcheck(env);
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
  Variable *var = env.makeVariable(loc, name, fullType, DF_TYPEDEF);
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
  ASTTypeId::Tcheck tc(DF_PARAMETER, DC_TP_NONTYPE);

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
    if (expr->constEval(env, i)) {
      sarg.setInt(i);
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


// -------------------------- NamespaceDecl -------------------------
void ND_alias::tcheck(Env &env)
{
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

    EnumType *origEnum = origScope->lookupEnum(origVar->name);
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
