// cc_elaborate.cc            see license.txt for copyright and terms of use
// code for cc_elaborate.h

// Convention w.r.t. 'env.doElaborate':
//   - The caller (in cc_tcheck.cc) checks this flag, and only calls 
//     into cc_elaborate.cc when it is true.
//   - The functions in cc_elaborate.cc xassert that it is true, but do
//     not otherwise check it.
// This ensures that the caller respects the flag, and also minimizes
// what will have to change later when (if) elaboration becomes a separate
// pass, rather than integrated into tcheck.

#include "cc_elaborate.h"      // this module
#include "cc_ast.h"            // Declaration
#include "cc_env.h"            // Scope, Env
#include "ast_build.h"         // makeExprList1, etc.
#include "trace.h"             // TRACE


// ---------------------- FullExpressionAnnot ---------------------
FullExpressionAnnot::FullExpressionAnnot()
{}

FullExpressionAnnot::~FullExpressionAnnot()
{}


FullExpressionAnnot::StackBracket::StackBracket
  (Env &env0, FullExpressionAnnot &fea0)
  : env(env0)
  , fea(fea0)
  , s(fea.tcheck_preorder(env))
{}

FullExpressionAnnot::StackBracket::~StackBracket()
{
  fea.tcheck_postorder(env, s);
}


// ------------------------ misc ----------------------
// (what follows could be better organized)

E_constructor *makeCtorExpr
  (Env &env, Variable *var, CompoundType *cpdType, FakeList<ArgExpression> *args)
{
  xassert(env.doElaboration);
  xassert(var);                 // this is never for a temporary
  xassert(!var->hasFlag(DF_TYPEDEF));
  PQName *name0 = env.make_PQ_fullyQualifiedName(cpdType);
  E_constructor *ector0 =
    new E_constructor(new TS_name(env.loc(), name0, false),
                      args);
  ector0->artificial = true;
  ector0->var = var;            // The variable being ctored
  return ector0;
}

Statement *makeCtorStatement
  (Env &env, Variable *var, CompoundType *cpdType, FakeList<ArgExpression> *args)
{
  xassert(env.doElaboration);
  E_constructor *ector0 = makeCtorExpr(env, var, cpdType, args);
  Statement *ctorStmt0 = new S_expr(env.loc(), new FullExpression(ector0));
  ctorStmt0->tcheck(env);
  return ctorStmt0;
}

Statement *makeDtorStatement(Env &env, Type *type)
{
  // hmm, can't say this because I don't have a var to say it about.
//    xassert(!var->hasFlag(DF_TYPEDEF));
  xassert(env.doElaboration);
  E_funCall *efc0 =
    new E_funCall(new E_variable(env.make_PQ_fullyQualifiedDtorName(type->asCompoundType())),
                  FakeList<ArgExpression>::emptyList());
  Statement *dtorStmt0 = new S_expr(env.loc(), new FullExpression(efc0));
  dtorStmt0->tcheck(env);
  return dtorStmt0;
}


//  void D_func::elaborateParameterCDtors(Env &env)
//  {
//    FAKELIST_FOREACH_NC(ASTTypeId, params, iter) {
//      iter->decl->elaborateCDtors
//        (env,
//         // Only Function and Declaration have any DeclFlags; the only
//         // one that seems appropriate is this one
//         DF_PARAMETER
//         );
//    }
//  }


void Declarator::elaborateCDtors(Env &env)
{
  xassert(env.doElaboration);

  // get this context from the 'var', don't make a mess passing
  // it down from above
  bool isMember = var->hasFlag(DF_MEMBER);
  bool isTemporary = var->hasFlag(DF_TEMPORARY);
  bool isParameter = var->hasFlag(DF_PARAMETER);

  // the code originally got this from syntactic context, but that's
  // not always correct, and this is much easier
  DeclFlags dflags = var->flags;

  bool isStatic = (dflags & DF_STATIC)!=0;
                       
  // open the scope indicated by the qualifiers, if any
  Scope *qualifierScope = openQualifierScope(env);

  if (!isParameter) {
    if (init) {
      if (isMember && !isStatic) {
        // special case: virtual methods can have an initializer that is
        // the int constant "0", which means they are abstract.  I could
        // check for the value "0", but I don't bother.
        if (! (var->type->isFunctionType()
               && var->type->asFunctionType()->isMethod()
               && var->hasFlag(DF_VIRTUAL))) {
          env.error(env.loc(), "initializer not allowed for a non-static member declaration");
        }
        // but we still shouldn't make a ctor for this function from the
        // "0".
        goto ifInitEnd;
      }
      
      #if 0    // sm: not quite right b/c of my change to 'isMember' (fails in/t0012.cc), and not important
      if (isMember && isStatic && !type->isConst()) {
        env.error(env.loc(), "initializer not allowed for a non-const static member declaration");
        goto ifInitEnd;
      }
      #endif // 0

      // TODO: check the initializer for compatibility with
      // the declared type

      // TODO: check compatibility with dflags; e.g. we can't allow
      // an initializer for a global variable declared with 'extern'

      // dsw: or a typedef
      if (var->hasFlag(DF_TYPEDEF)) {
        // dsw: FIX: should the return value get stored somewhere?
        // FIX: the loc is probably wrong
        env.error(env.loc(), "initializer not allowed for a typedef");
        goto ifInitEnd;
      }

      // dsw: Arg, do I have to deal with this?
      // TODO: in the case of class data members, delay checking the
      // initializer until the entire class body has been scanned

      xassert(!isTemporary);
      // make the call to the ctor; FIX: Are there other things to check
      // in the if clause, like whether this is a typedef?
      if (init->isIN_ctor()) {
        xassert(!(decl->isD_name() && !decl->asD_name()->name)); // that is, not an abstract decl
        // FIX: What should we do for non-CompoundTypes?
        if (type->isCompoundType()) {
          ctorStatement = makeCtorStatement(env, var, type->asCompoundType(),
                                            init->asIN_ctor()->args);
        }
      } else if (type->isCompoundType()) {
        if (init->isIN_expr()) {
          // assert that it is not an abstract decl
          xassert(!(decl->isD_name() && !decl->asD_name()->name));
          // just call the one-arg ctor; FIX: this is questionable; we
          // haven't decided what should really happen for an IN_expr;
          // update: dsw: I'm sure that is right
          ctorStatement =
            makeCtorStatement(env, var, type->asCompoundType(),
                              makeExprList1(init->asIN_expr()->e));
        } else if (init->isIN_compound()) {
          xassert(!(decl->isD_name() && !decl->asD_name()->name)); // that is, not an abstract decl
          // just call the no-arg ctor; FIX: this is questionable; it
          // is undefined what should happen for an IN_compound since
          // it is a C99-ism.
          ctorStatement = makeCtorStatement(env, var, type->asCompoundType(),
                                            FakeList<ArgExpression>::emptyList());
        }
      }

      // jump here if we find an error in the init code above and report
      // it but want to keep going
    ifInitEnd: ;                  // must have a statement here
    }
    else /* init is NULL */ {
      if (type->isCompoundType() &&
          !var->hasFlag(DF_TYPEDEF) &&
          !(decl->isD_name() && !decl->asD_name()->name) && // that is, not an abstract decl
          !isTemporary &&
          !isMember &&
          !( (dflags & DF_EXTERN)!=0 ) // not extern
          ) {
        // call the no-arg ctor; for temporaries do nothing since this is
        // a temporary, it will be initialized later
        ctorStatement = makeCtorStatement(env, var, type->asCompoundType(),
                                          FakeList<ArgExpression>::emptyList());
      }
    }

    // if isTemporary we don't want to make a ctor since by definition
    // the temporary will be initialized later
    if (isTemporary ||
        (isMember && !(isStatic && type->isConst())) ||
        ( (dflags & DF_EXTERN)!=0 ) // extern
        ) {
      xassert(!ctorStatement);
    } else if (type->isCompoundType() &&
               !var->hasFlag(DF_TYPEDEF) &&
               !(decl->isD_name() && !decl->asD_name()->name) && // that is, not an abstract decl
               (!isMember ||
                (isStatic && type->isConst() && init)) &&
               !( (dflags & DF_EXTERN)!=0 ) // not extern
               ) {
      xassert(ctorStatement);
    }
//    } else {
//      // isParameter
//      // make the copy ctor
//      ctorStatement = makeCtorStatement(env, var, type, FakeList<ArgExpression>::emptyList());

    // make the dtorStatement
    if (type->isCompoundType() &&
        !var->hasFlag(DF_TYPEDEF) &&
        !(decl->isD_name() && !decl->asD_name()->name) && // that is, not an abstract decl
        !( (dflags & DF_EXTERN)!=0 ) // not extern
        ) {
      dtorStatement = makeDtorStatement(env, type);
    } else {
      xassert(!dtorStatement);
    }
  }

  // I think this is wrong.  We only want it for function DEFINITIONS.
//    // recuse on the parameters if we are a D_func
//    if (decl->isD_func()) {
  // NOTE: this is also implemented wrong: use getD_func()
//      decl->asD_func()->elaborateParameterCDtors(env);
//    }

  if (qualifierScope) {
    env.retractScope(qualifierScope);
  }
}


// If the return type is a CompoundType, then make a temporary and
// point the retObj at it.  The intended semantics of this is to
// override the usual way of analyzing the return value just from the
// return type of the function.  Some analyses may want us to do this
// for all return values, not just those of CompoundType.
//
// Make a Declaration for a temporary.
static Declaration *makeTempDeclaration(Env &env, Type *retType)
{
  // while a user may attempt this, we should catch it earlier and not
  // end up down here.
  xassert(retType->isCompoundType());
  TypeSpecifier *typeSpecifier =
    new TS_name(env.loc(),
                env.make_PQ_fullyQualifiedName(retType->asCompoundType()),
                false);
  xassert(typeSpecifier);
  Declarator *declarator0 =
    new Declarator(new D_name(env.loc(),
                              env.makeTempName() // give it a new unique name
                              ),
                   NULL         // important: no Initializer
                   );
  Declaration *declaration0 =
    new Declaration(DF_TEMPORARY,
                    // should get DF_AUTO since they are auto, but I
                    // seem to recall that Scott just wants to use
                    // that for variables that are explicitly marked
                    // auto; will get DF_DEFINITION as soon as
                    // typechecked, I hope
                    typeSpecifier,
                    FakeList<Declarator>::makeList(declarator0)
                    );

  // typecheck it

  // don't do this for now:
//    int numErrors = env.numErrors();
  declaration0->tcheck(env);

  // FIX: what the heck am I doing here?  There should only be one.
  xassert(declaration0->decllist->count() == 1);
  // leave it for now
  FAKELIST_FOREACH_NC(Declarator, declaration0->decllist, decliter) {
    decliter->elaborateCDtors(env);
  }
  // don't do this for now:
//    xassert(numErrors == env.numErrors()); // shouldn't have added to the errors

  return declaration0;
}

static Declaration *insertTempDeclaration(Env &env, Type *retType)
{
  // make a temporary in the innermost FullExpressionAnnot
  FullExpressionAnnot *fea0 = env.fullExpressionAnnotStack.top();

  // dsw: FIX: I would like to assert here that env.scope() (the
  // current scope) == (fictional) fea0->scope, except that
  // FullExpressionAnnot doesn't have a scope field.

  Declaration *declaration0 = makeTempDeclaration(env, retType);
  // maybe the loc should be: fea0->expr->loc

  // check that it got entered into the current scope
  if (!env.disambErrorsSuppressChanges()) {
    Declarator *declarator0 = declaration0->decllist->first();
    xassert(env.scope()->rawLookupVariable
            (declarator0->decl->asD_name()->name->getName()) ==
            declarator0->var);
  }

  // put it into fea0
  fea0->declarations.append(declaration0);

  return declaration0;
}

static E_variable *wrapVarWithE_variable(Env &env, Variable *var)
{
  PQName *name0 = NULL;
  switch(var->scopeKind) {
    default:
    case SK_UNKNOWN:
      xfailure("wrapVarWithE_variable: var->scopeKind == SK_UNKNOWN; shouldn't get here");
      break;
    case SK_TEMPLATE:
      xfailure("don't handle template scopes yet");
      break;
    case SK_PARAMETER:
      xfailure("shouldn't be getting a parameter scope here");
      break;
    case SK_GLOBAL:
      xassert(!var->scope);
      name0 = new PQ_qualifier(env.loc(), NULL,
                               FakeList<TemplateArgument>::emptyList(),
                               new PQ_name(env.loc(), var->name));
      break;
    case SK_CLASS:
      xassert(var->scope->curCompound);
      name0 = env.make_PQ_fullyQualifiedName
        (var->scope->curCompound, new PQ_name(env.loc(), var->name));
      break;
    case SK_FUNCTION:
      name0 = new PQ_name(env.loc(), var->name);
      break;
  }
  xassert(name0);

  // wrap an E_variable around the var so it is an expression
  E_variable *evar0 = new E_variable(name0);
  {
    Expression *evar1 = evar0;
    evar0->tcheck(env, evar1);
    xassert(evar0 == evar1->asE_variable());
  }
  // it had better find the same variable
  if (!env.disambErrorsSuppressChanges()) {
    xassert(evar0->var == var);
  }

  // dsw: I would like to assert here that retType equals
  // evar0->var->type but I'm not sure how to do that because I don't
  // know how to check equality of types.

  return evar0;
}


Expression *elaborateCallSite(Env &env, FunctionType *ft,
                              FakeList<ArgExpression> *args)
{
  Expression *retObj = NULL;

  // If the return type is a CompoundType, then make a temporary and
  // point the retObj at it.  NOTE: This can never accidentally create
  // a temporary for a dtor for a non-temporary object because the
  // retType for a dtor is void.  However, we do need to guard against
  // this possibility for ctors.
  if (ft->retType->isCompoundType()) {
    Declaration *declaration0 = insertTempDeclaration(env, ft->retType);
    xassert(declaration0->decllist->count() == 1);
    Variable *var0 = declaration0->decllist->first()->var;
    retObj = wrapVarWithE_variable(env, var0);
  }

  // Elaborate cdtors for CompoundType arguments being passed by
  // value.
  //
  // For each parameter, if it is a CompoundType (pass by value) then 1)
  // make a temporary variable here for it that has a dtor but not a
  // ctor 2) for the corresponding argument, replace it with a comma
  // expression where a) the first part is an E_constructor for the
  // temporary that has one argument that is what was the arugment in
  // this slot and the ctor is the copy ctor, and b) E_variable for the
  // temporary

  SObjListIterNC<Variable> paramsIter(ft->params);

  FAKELIST_FOREACH_NC(ArgExpression, args, arg) {
    if (paramsIter.isDone()) {
      // FIX: I suppose we could still have arguments here if there is a
      // ellipsis at the end of the parameter list.  Can those be passed
      // by value?
      break;
    }

    Variable *param = paramsIter.data();
    Type *paramType = param->getType();
    if (paramType->isCompoundType()) {
      // E_variable that points to the temporary
      Declaration *declaration0 = insertTempDeclaration(env, paramType);
      xassert(declaration0->decllist->count() == 1);
      Variable *tempVar = declaration0->decllist->first()->var;
      E_variable *byValueArg = wrapVarWithE_variable(env, tempVar);

      // E_constructor for the temporary that calls the copy ctor for
      // the temporary taking the real argument as the copy ctor
      // argument.
      E_constructor *ector = makeCtorExpr(env, tempVar, paramType->asCompoundType(),
                                          makeExprList1(arg->expr));

      // combine into a comma expression so we do both but return the
      // value of the second
      arg->expr = new E_binary(ector, BIN_COMMA, byValueArg);
      arg->tcheck(env);
    }

    paramsIter.adv();
  }

  if (!paramsIter.isDone()) {
    // FIX: if (!paramsIter.isDone()) then have to deal with default
    // arguments that are pass by value if such a thing is even
    // possible.
  }

  return retObj;
}


void elaborateFunctionStart(Env &env, FunctionType *ft)
{
  if (ft->retType->isCompoundType()) {
    // We simulate return-by-value for class-valued objects by
    // passing a hidden additional parameter of type C& for a
    // return value of type C.  For static semantics, that means
    // adding an environment entry for a special name, "<retVal>".
    Variable *v = env.makeVariable(env.loc(), env.str("<retVal>"),
                                   env.tfac.cloneType(ft->retType), DF_PARAMETER);
    env.addVariable(v);
  }
}


// add no-arg MemberInits to existing ctor body ****************

// Does this Variable want a no-arg MemberInitializer?
static bool wantsMemberInit(Variable *var) {
  // function members should be skipped
  if (var->type->isFunctionType()) return false;
  // skip arrays for now; FIX: do something correct here
  if (var->type->isArrayType()) return false;
  if (var->isStatic()) return false;
  if (var->hasFlag(DF_TYPEDEF)) return false;
  // FIX: do all this with one test
  xassert(!var->hasFlag(DF_AUTO));
  xassert(!var->hasFlag(DF_REGISTER));
  xassert(!var->hasFlag(DF_EXTERN));
  xassert(!var->hasFlag(DF_VIRTUAL)); // should be functions only
  xassert(!var->hasFlag(DF_EXPLICIT)); // should be ctors only
  xassert(!var->hasFlag(DF_FRIEND));
  xassert(!var->hasFlag(DF_NAMESPACE));
  xassert(var->isMember());
  return true;
}

// find the MemberInitializer that initializes data member memberVar;
// return NULL if none
static MemberInit *findMemberInitDataMember
  (FakeList<MemberInit> *inits, // the inits to search
   Variable *memberVar)         // data member to search for the MemberInit for
{
  MemberInit *ret = NULL;
  FAKELIST_FOREACH_NC(MemberInit, inits, mi) {
    xassert(!mi->base || !mi->member); // MemberInit should do only one thing
    if (mi->member == memberVar) {
      xassert(!ret);            // >1 MemberInit; FIX: this is a user error, not our error
      ret = mi;
    }
  }
  return ret;
}

// find the MemberInitializer that initializes data member memberVar;
// return NULL if none
static MemberInit *findMemberInitSuperclass
  (FakeList<MemberInit> *inits, // the inits to search
   CompoundType *superclass)    // superclass to search for the MemberInit for
{
  MemberInit *ret = NULL;
  FAKELIST_FOREACH_NC(MemberInit, inits, mi) {
    xassert(!mi->base || !mi->member); // MemberInit should do only one thing
    if (mi->base == superclass) {
      xassert(!ret);            // >1 MemberInit; FIX: this is a user error, not our error
      ret = mi;
    }
  }
  return ret;
}


// Finish supplying to a ctor the no-arg MemberInits for unmentioned
// superclasses and members.  NOTE: to be correct, the input
// ctor->inits had better have already been typechecked.  Any new ones
// created are typchecked here before being added.  FIX: maybe this
// should be a method on Function.
void completeNoArgMemberInits(Env &env, Function *ctor, CompoundType *ct)
{
  // can't do any of this since Function *ctor can't have been
  // typechecked yet
//    Type *funcType = ctor->funcType;
//    xassert(funcType->isFunctionType());
//    FunctionType *ctorFunc = funcType->asFunctionType();
//    xassert(ctorFunc->isConstructor());
//    // FIX: assert here that _ctor_ is a ctor for _ct_; don't know how
//    // to do it.  Scott says it is not easy.

  SourceLoc loc = env.loc();

  // Iterate through the members in the declaration order (what is in
  // the CompoundType).  For each one, check to see if we have a
  // MemberInit for it.  If so, append that (prepend and revese
  // later); otherwise, make one.  This has the effect of
  // canonicalizing the MemberInit call order even if none needed to
  // be added, which I think is in the spec; at least g++ does it (and
  // gives a warning, which I won't do.)
  FakeList<MemberInit> *oldInits = ctor->inits;
  // NOTE: you can't make a new list of inits that is a FakeList
  // because we are still traversing over the old one.  Linked lists
  // are a premature optimization!
  VoidList newInits;            // I don't care that this isn't typechecked.
  // NOTE: don't do this!
//    FakeList<MemberInit> *newInits = FakeList<MemberInit>::emptyList();
  
  FOREACH_OBJLIST(BaseClass, ct->bases, iter) {
    BaseClass *base = const_cast<BaseClass*>(iter.data());
    // omit initialization of virtual base classes, whether direct
    // virtual or indirect virtual.  See cppstd 12.6.2 and the
    // implementation of Function::tcheck_memberInits()
    //
    // FIX: We really should be initializing the direct virtual bases,
    // but the logic is so complex I'm just going to omit it for now
    // and err on the side of not calling enough initializers;
    // UPDATE: the spec says we can do it multiple times for copy
    // assign operator, so I wonder if that holds for ctors also.
    if (!ct->hasVirtualBase(base->ct)) {
      FakeList<TemplateArgument> *targs = env.make_PQ_templateArgs(base->ct);
      PQName *name = env.make_PQ_possiblyTemplatizedName(loc, base->ct->name, targs);
      MemberInit *mi = findMemberInitSuperclass(oldInits, base->ct);
      if (!mi) {
        mi = new MemberInit(name, FakeList<ArgExpression>::emptyList());
        mi->tcheck(env, ctor->ctorThisLocalVar, ct);
      }
      newInits.prepend(mi);
    } else {
//        cerr << "Omitting a direct base that is also a virtual base" << endl;
    }
  }
  // FIX: virtual bases omitted for now

  SFOREACH_OBJLIST_NC(Variable, ct->dataMembers, iter) {
    Variable *var = iter.data();
    if (!wantsMemberInit(var)) continue;
    MemberInit *mi = findMemberInitDataMember(oldInits, var);
    // It seems that ints etc. shouldn't have explicit no-arg ctors.
    // Actually, it seems that even some classes are not default
    // initialized!  However, if they are of POD type and I default
    // initialize them anyway, all I'm doing is calling their
    // implicitly-defined no-arg ctor, which (eventually) will simply do
    // nothing for the scalar data members, which is equivalent.
    // 12.6.2 para 4: If a given nonstatic data member or base class is
    // not named by a mem-initializer-id, then
    // -- If the entity is a nonstatic data member of ... class type (or
    // array thereof) or a base class, and the entity class is a non-POD
    // class, the entity is default-initialized (8.5). ....
    // -- Otherwise, the entity is not initialized. ....
    if (!mi && var->type->isCompoundType()) {
      mi = new MemberInit(new PQ_name(loc, var->name), FakeList<ArgExpression>::emptyList());
      mi->tcheck(env, ctor->ctorThisLocalVar, ct);
    }
    if (mi) newInits.prepend(mi);
  }

  // *Now* we can destroy the linked list structure and rebuild it
  // again while also reversing the list.
  ctor->inits = FakeList<MemberInit>::emptyList();
  for (VoidListIter iter(newInits); !iter.isDone(); iter.adv()) {
    MemberInit *mi = static_cast<MemberInit*>(iter.data());
    mi->next = NULL;
    ctor->inits = ctor->inits->prepend(mi);
  }
}


// make no-arg ctor ****************

MR_func *makeNoArgCtorBody(Env &env, CompoundType *ct)
{
  // reversed print AST output; remember to read going up even for the
  // tree leaves

  SourceLoc loc = env.loc();

  //     handlers:
  FakeList<Handler> *handlers = FakeList<Handler>::emptyList();
  //       stmts:
  ASTList<Statement> *stmts = new ASTList<Statement>();
  //       loc = ex/no_arg_ctor1.cc:2:7
  //       succ={ }
  //     S_compound:
  S_compound *body = new S_compound(loc, stmts);
  //     inits:

  // NOTE: The MemberInitializers will be added by
  // completeNoArgMemberInits() during typechecking, so we don't add
  // them now.
  FakeList<MemberInit> *inits = FakeList<MemberInit>::emptyList();

  //       init is null
  Initializer *init = NULL;
  //         exnSpec is null
  ExceptionSpec *exnSpec = NULL;
  //         cv = 
  CVFlags cv = CV_NONE;
  //         params:
  // The no-arg ctor takes no parameters.
  FakeList<ASTTypeId> *params = FakeList<ASTTypeId>::emptyList();
  //             name = A
  //             loc = ex/no_arg_ctor1.cc:2:3
  //           PQ_name:
  // see this point in makeCopyCtorBody() below for a comment on the
  // sufficient generality of this
  PQName *ctorName = new PQ_name(loc, ct->name);
  //           loc = ex/no_arg_ctor1.cc:2:3
  //         D_name:
  IDeclarator *base = new D_name(loc, ctorName);
  //         loc = ex/no_arg_ctor1.cc:2:3
  //       D_func:
  IDeclarator *decl = new D_func(loc,
                                 base,
                                 params,
                                 cv,
                                 exnSpec);
  //     Declarator:
  Declarator *nameAndParams = new Declarator(decl, init);
  //       id = /*cdtor*/
  //       loc = ex/no_arg_ctor1.cc:2:3
  //       cv = 
  //     TS_simple:
  TypeSpecifier *retspec = new TS_simple(loc, ST_CDTOR);
  //     dflags = 
  DeclFlags dflags = DF_MEMBER | DF_INLINE;
  //   Function:
  Function *f =
    new Function(dflags,
                 retspec,
                 nameAndParams,
                 inits,
                 body,
                 handlers);
  f->implicitlyDefined = true;
  //   loc = ex/no_arg_ctor1.cc:2:3
  // MR_func:
  return new MR_func(loc, f);
}


// make copy ctor ****************

static MemberInit *makeCopyCtorMemberInit(PQName *tgtName,
                                          StringRef srcNameS,
                                          StringRef srcMemberNameS,
                                          SourceLoc loc)
{
  //                 name = b
  //                 loc = ../oink/ctor1.cc:12:9
  //               PQ_name:
  PQ_name *srcName = new PQ_name(loc, srcNameS);
  //             E_variable:
  Expression *expr = new E_variable(srcName);
  if (srcMemberNameS) {
    //               name = y
    //               loc = ../oink/ctor1.cc:15:11
    //             PQ_name:
    PQ_name *srcMemberName = new PQ_name(loc, srcMemberNameS);
    //                 name = b
    //                 loc = ../oink/ctor1.cc:15:9
    //               PQ_name:
    //             E_variable:
    //           E_fieldAcc:
    expr = new E_fieldAcc(expr, srcMemberName);
  }
  //           ArgExpression:
  ArgExpression *argExpr = new ArgExpression(expr);
  //         args:
  FakeList<ArgExpression> *args = FakeList<ArgExpression>::makeList(argExpr);
  //           name = A
  //           loc = ../oink/ctor1.cc:12:7
  //         PQ_name:

  // this doesn't work in the case of templates for example
//    PQ_name *tgtName = new PQ_name(loc, tgtNameS);
  xassert(tgtName);

  //       MemberInit:
  MemberInit *mi = new MemberInit(tgtName, args);
  return mi;
}


MR_func *makeCopyCtorBody(Env &env, CompoundType *ct)
{
  // reversed print AST output; remember to read going up even for the
  // tree leaves

  SourceLoc loc = env.loc();

  //     handlers:
  FakeList<Handler> *handlers = FakeList<Handler>::emptyList();

  //       stmts:
  ASTList<Statement> *stmts = new ASTList<Statement>();

  //       loc = ../oink/ctor1.cc:16:3
  //       succ={ }
  //     S_compound:
  S_compound *body = new S_compound(loc, stmts);

  // for each member, make a call to its copy ctor; Note that this
  // works for simple types also; NOTE: We build this in reverse and
  // then reverse it.
  FakeList<MemberInit> *inits = FakeList<MemberInit>::emptyList();

  StringRef srcNameS = env.str.add("__other");

  FOREACH_OBJLIST(BaseClass, ct->bases, iter) {
    BaseClass *base = const_cast<BaseClass*>(iter.data());
    // omit initialization of virtual base classes, whether direct
    // virtual or indirect virtual.  See cppstd 12.6.2 and the
    // implementation of Function::tcheck_memberInits()
    //
    // FIX: We really should be initializing the direct virtual bases,
    // but the logic is so complex I'm just going to omit it for now
    // and err on the side of not calling enough initializers
    if (!ct->hasVirtualBase(base->ct)) {
//        env.make_PQ_qualifiedName(base->ct),
      FakeList<TemplateArgument> *targs = env.make_PQ_templateArgs(base->ct);
//        Variable *typedefVar = base->ct->getTypedefName();
//        xassert(typedefVar);
//        PQName *name = env.make_PQ_possiblyTemplatizedName(loc, typedefVar->name, targs);
      PQName *name = env.make_PQ_possiblyTemplatizedName(loc, base->ct->name, targs);
      MemberInit *mi = makeCopyCtorMemberInit(name, srcNameS, NULL, loc);
      inits = inits->prepend(mi);
    } else {
//        cerr << "Omitting a direct base that is also a virtual base" << endl;
    }
  }

  // FIX: What the heck do I do for virtual bases?  This surely isn't
  // right.
  //
  // Also, it seems that the order of interleaving of the virtual and
  // non-virtual bases has been lost.  Have to be careful of this when
  // we pretty print.
  //
  // FIX: This code is broken anyway.
//    FOREACH_OBJLIST_NC(BaseClassSubobj, ct->virtualBases, iter) {
//      BaseClass *base = iter->data();
// // This must not mean what I think.
// //     xassert(base->isVirtual);
//      MemberInit *mi = makeCopyCtorMemberInit(base->ct->name, srcNameS, NULL, loc);
//      inits = inits->prepend(mi);
//    }

  SFOREACH_OBJLIST_NC(Variable, ct->dataMembers, iter) {
    Variable *var = iter.data();
    if (!wantsMemberInit(var)) continue;
    MemberInit *mi = makeCopyCtorMemberInit
      (new PQ_name(loc, var->name), srcNameS, var->name, loc);
    inits = inits->prepend(mi);
  }

  //     inits:
  inits = inits->reverse();

  //       init is null
  Initializer *init = NULL;

  //         exnSpec is null
  ExceptionSpec *exnSpec = NULL;

  //         cv = 
  CVFlags cv = CV_NONE;

  //               init is null
  Initializer *init0 = NULL;

  //                     name = b
  //                     loc = ../oink/ctor1.cc:11:14
  //                   PQ_name:
  PQ_name *name0 = new PQ_name(loc, srcNameS);

  //                   loc = ../oink/ctor1.cc:11:14
  //                 D_name:
  IDeclarator *base0 = new D_name(loc, name0);

  //                 cv = 
  //                 isPtr = false
  //                 loc = ../oink/ctor1.cc:11:13
  //               D_pointer:
  IDeclarator *idecl0 = new D_pointer(loc,
                                      false, // a ref, not a real pointer
                                      CV_NONE,
                                      base0);

  //             Declarator:
  Declarator *decl0 = new Declarator(idecl0, init0);

  //               typenameUsed = false
  bool typenameUsed = false;

  //                 name = B
  //                 loc = ../oink/ctor1.cc:11:5
  //               PQ_name:
  PQName *name = NULL;
  {
    FakeList<TemplateArgument> *targs = env.make_PQ_templateArgs(ct);
    Variable *typedefVar = ct->getTypedefName();
    xassert(typedefVar);
    name = env.make_PQ_possiblyTemplatizedName(loc, typedefVar->name, targs);
  }

  //               loc = ../oink/ctor1.cc:11:5
  //               cv = const
  //             TS_name:
  TypeSpecifier *spec = new TS_name(loc, name, typenameUsed);
  spec->cv = CV_CONST;

  //           ASTTypeId:
  ASTTypeId *param0 = new ASTTypeId(spec, decl0); 
  
  //         params:
  FakeList<ASTTypeId> *params = FakeList<ASTTypeId>::makeList(param0);

  //             name = B
  //             loc = ../oink/ctor1.cc:11:3
  //           PQ_name:
  // FIX: is this a sufficiently general way of getting a name for
  // this class?
  // update: Yes, it is.  We do *not* want to use a fully qualified
  // name for the class here for the name of the ctor.  Just the way
  // naming works in C++ I guess.
  PQName *ctorName = new PQ_name(loc, ct->name);

  //           loc = ../oink/ctor1.cc:11:3
  //         D_name:
  IDeclarator *base = new D_name(loc, ctorName);

  //         loc = ../oink/ctor1.cc:11:3
  //       D_func:
  IDeclarator *decl = new D_func(loc,
                                 base,
                                 params,
                                 cv,
                                 exnSpec);

  //     Declarator:
  Declarator *nameAndParams = new Declarator(decl, init);

  //       id = /*cdtor*/
  //       loc = ../oink/ctor1.cc:11:3
  //       cv = 
  //     TS_simple:
  TypeSpecifier *retspec = new TS_simple(loc, ST_CDTOR);

  //     dflags =
  // FIX: are ctors members?
  // update: old email from Scott says yes.
  DeclFlags dflags = DF_MEMBER | DF_INLINE;

  //   Function:
  Function *f =
    new Function(dflags,
                 retspec,
                 nameAndParams,
                 inits,
                 body,
                 handlers);
  f->implicitlyDefined = true;

  //   loc = ../oink/ctor1.cc:11:3
  // MR_func:
  return new MR_func(loc, f);
}


// make copy assign op ****************

// "return *this;"
static S_return *make_S_return(Env &env)
{
  SourceLoc loc = env.loc();

  //                   name = this
  //                   loc = copy_assign1.cc:12:13
  //                 PQ_name:
  PQ_name *name0 = new PQ_name(loc, env.str.add("this"));
  //               E_variable:
  E_variable *evar0 = new E_variable(name0);
  //             E_deref:
  E_deref *ederef0 = new E_deref(evar0);
  //           FullExpression:
  FullExpression *fullexp0 = new FullExpression(ederef0);
  //           loc = copy_assign1.cc:12:5
  //           succ={ }
  //         S_return:
  return new S_return(loc, fullexp0);
}

// "y = __other.y;"
static S_expr *make_S_expr_memberCopyAssign(Env &env, StringRef memberName)
{
  SourceLoc loc = env.loc();

  //                   name = y
  //                   loc = copy_assign1.cc:11:17
  //                 PQ_name:
  PQ_name *name1 = new PQ_name(loc, memberName);
  //                     name = __other
  //                     loc = copy_assign1.cc:11:9
  //                   PQ_name:
  PQ_name *name2 = new PQ_name(loc, env.str.add("__other"));
  //                 E_variable:
  E_variable *evar2 = new E_variable(name2);
  //               E_fieldAcc:
  E_fieldAcc *efieldacc2 = new E_fieldAcc(evar2, name1);
  //               op = =
  // NOTE: below
  //                   name = y
  //                   loc = copy_assign1.cc:11:5
  //                 PQ_name:
  PQ_name *name3 = new PQ_name(loc, memberName);
  //               E_variable:
  E_variable *evar3 = new E_variable(name3);
  //             E_assign:
  E_assign *eassign0 = new E_assign(evar3, BIN_ASSIGN, efieldacc2);
  //           FullExpression:
  FullExpression *fullexp1 = new FullExpression(eassign0);
  //           loc = copy_assign1.cc:11:5
  //           succ={ }
  //         S_expr:
  return new S_expr(loc, fullexp1);
}

// "this->W::operator=(__other);"
static S_expr *make_S_expr_superclassCopyAssign(Env &env, BaseClass *base)
{
  SourceLoc loc = env.loc();

  //                       name = __other
  StringRef other = env.str.add("__other");
  //                       loc = copy_assign2.cc:8:24
  //                     PQ_name:
  PQ_name *name0 = new PQ_name(loc, other);
  //                   E_variable:
  E_variable *evar0 = new E_variable(name0);
  //                 ArgExpression:
  ArgExpression *argexpr0 = new ArgExpression(evar0);
  //               args:
  FakeList<ArgExpression> *args = FakeList<ArgExpression>::makeList(argexpr0);
  //                     fakeName = operator=
  StringRef fakename = env.str.add("operator=");
  //                       op = =
  //                     ON_operator:
  ON_operator *opname = new ON_operator(OP_ASSIGN);
  //                     loc = copy_assign2.cc:8:14
  //                   PQ_operator:
  PQ_operator *assignop = new PQ_operator(loc, opname, fakename);
  //                   targs:
  FakeList<TemplateArgument> *targs = env.make_PQ_templateArgs(base->ct);
  //                   qualifier = W
  //                   loc = copy_assign2.cc:8:11
  //                 PQ_qualifier:
  PQName *superclassName = NULL;
  {
    Variable *typedefVar = base->ct->getTypedefName();
    xassert(typedefVar);
    superclassName = new PQ_qualifier(loc, typedefVar->name, targs, assignop);
  }
  //                     name = this
  //                     loc = copy_assign2.cc:8:5
  //                   PQ_name:
  PQ_name *thisName = new PQ_name(loc, env.str.add("this"));
  //                 E_variable:
  E_variable *thisVar = new E_variable(thisName);
  //               E_arrow:
  E_arrow *earrow = new E_arrow(thisVar, superclassName);
  //             E_funCall:
  E_funCall *efuncall = new E_funCall(earrow, args);
  //           FullExpression:
  FullExpression *fullexpr = new FullExpression(efuncall);
  //           loc = copy_assign2.cc:8:5
  //           succ={ }
  //         S_expr:
  return new S_expr(loc, fullexpr);
}

Declarator *makeCopyAssignDeclarator(Env &env, CompoundType *ct)
{
  SourceLoc loc = env.loc();

  //       init is null
  // NOTE: below inline
  //           exnSpec is null
  // NOTE: below inline
  //           cv = 
  // NOTE: below inline
  //                 init is null
  Initializer *init = NULL;
  //                       name = __other
  StringRef otherName = env.str.add("__other");
  //                       loc = copy_assign1.cc:7:25
  //                     PQ_name:
  PQ_name *otherPQ_name = new PQ_name(loc, otherName);
  //                     loc = copy_assign1.cc:7:25
  //                   D_name:
  D_name *declname = new D_name(loc, otherPQ_name);
  //                   cv = 
  //                   isPtr = false
  //                   loc = copy_assign1.cc:7:24
  //                 D_pointer:
  D_pointer *dptr = new D_pointer(loc, false /*ref, not ptr*/, CV_NONE, declname);
  //               Declarator:
  Declarator *declarator0 = new Declarator(dptr, init);
  //                 typenameUsed = false
  bool typenameUsed = false;
  //                   name = X
  //                   loc = copy_assign1.cc:7:16
  //                 PQ_name:
  PQName *name1 = NULL;
  {
    FakeList<TemplateArgument> *targs = env.make_PQ_templateArgs(ct);
    Variable *typedefVar = ct->getTypedefName();
    xassert(typedefVar);
    name1 = env.make_PQ_possiblyTemplatizedName(loc, typedefVar->name, targs);
  }
  //                 loc = copy_assign1.cc:7:16
  //                 cv = const
  //               TS_name:
  TS_name *tsname1 = new TS_name(loc, name1, typenameUsed);
  tsname1->cv = CV_CONST;
  //             ASTTypeId:
  ASTTypeId *asttypeid = new ASTTypeId(tsname1, declarator0);
  //           params:
  FakeList<ASTTypeId> *params = FakeList<ASTTypeId>::makeList(asttypeid);
  //               fakeName = operator=
  StringRef fakeName = env.str.add("operator=");
  //                 op = =
  //               ON_operator:
  ON_operator *opname = new ON_operator(OP_ASSIGN);
  //               loc = copy_assign1.cc:7:6
  //             PQ_operator:
  PQ_operator *pqop = new PQ_operator(loc, opname, fakeName);
  //             loc = copy_assign1.cc:7:6
  //           D_name:
  D_name *dname = new D_name(loc, pqop);
  //           loc = copy_assign1.cc:7:6
  //         D_func:
  // FIX: this exception spec is wrong: the spec says that the implied
  // copy operator= has an exception spec.
  D_func *dfunc = new D_func(loc, dname, params, CV_NONE, NULL/*exnSpec*/);
  //         cv = 
  //         isPtr = false
  //         loc = copy_assign1.cc:7:4
  //       D_pointer:
  D_pointer *dptr1 = new D_pointer(loc, false /*ref, not ptr*/, CV_NONE, dfunc);
  //     Declarator:
  return new Declarator(dptr1, NULL /*init*/);
}


//  12.8 paragraph 10
//
//  If the lass definition does not explicitly declare a copy assignment
//  operator, one is declared implicitly.  The implicitly-declared copy
//  assignment operator for a class X will have the form
//
//          X& X::operator=(X const &)
//
//  if [lots of complicated conditions here on whether or not the
//  parameter should be a reference to const or not; I'm just going to
//  make it const for now] ...
//
//  The implicitly-declared copy assignment operator for class X has the
//  return type X&; it returns the object for which the assignment
//  operator is invoked, that is, the object assigned to.  An
//  implicitly-declared copy assignment operator is an inline public
//  member of its class. ...
//
//  paragraph 12
//
//  An implicitly-declared copy assignment operator is implicitly defined
//  when an object of its class type is assigned a value of its class type
//  or a value of a class type derived from its class type. ...
//
//  paragraph 13
//
//  The implicitly-defined copy assignment operator for class X performs
//  memberwise assignment of its subobjects.  The direct base classes of X
//  are assigned first, in the order of their declaration in the
//  base-specifier-list, and then the immediate nonstatic data members of
//  X are assigned, in the order in which they were declared in the class
//  definition.  Each subobject is assigned in the manner appropriate to
//  its type:
//
//  -- if the subobject is of class type, the copy assignment operator for
//  the class is used (as if by explicit qualification; that is, ignoring
//  any possible virtual overriding functions in more derived classes);
//
//  -- if the subobject is an array, each element is assigned, in the
//  manner appropriate to the element type.
//
//  -- if the subobject is of scalar type, the built-in assignment
//  operator is used.
//
//  It is unspecified whether subobjects representing virtual base classes
//  are assigned more than once by the implicitly-defined copy assignment
//  operator.
MR_func *makeCopyAssignBody(Env &env, CompoundType *ct)
{
  // reversed print AST output; remember to read going up even for the
  // tree leaves

  // this is a pretty pathetic way to get a loc, since everything in
  // the constructed AST ends up at one point loc
  SourceLoc loc = env.loc();

  //     handlers:
  FakeList<Handler> *handlers = FakeList<Handler>::emptyList();

  //       stmts:
  // NOTE: these are made and appended *in* *order*, not in reverse
  // and then reversed as with the copy ctor
  ASTList<Statement> *stmts = new ASTList<Statement>();

  // For each superclass, make the call to operator =.
  FOREACH_OBJLIST(BaseClass, ct->bases, iter) {
    BaseClass *base = const_cast<BaseClass*>(iter.data());
    // omit initialization of virtual base classes, whether direct
    // virtual or indirect virtual.  See cppstd 12.6.2 and the
    // implementation of Function::tcheck_memberInits()
    //
    // FIX: We really should be initializing the direct virtual bases,
    // but the logic is so complex I'm just going to omit it for now
    // and err on the side of not calling enough initializers
    if (!ct->hasVirtualBase(base->ct)) {
      stmts->append(make_S_expr_superclassCopyAssign(env, base));
    }
  }

  SFOREACH_OBJLIST_NC(Variable, ct->dataMembers, iter) {
    Variable *var = iter.data();
    if (!wantsMemberInit(var)) continue;
    // skip assigning to const or reference members; NOTE: this is an
    // asymmetry with copy ctor, which can INITIALIZE const or ref
    // types, however we cannot ASSIGN to them.
    Type *type = var->type;
    if (type->isReference() || type->isConst()) continue;
    stmts->append(make_S_expr_memberCopyAssign(env, var->name));
  }

  stmts->append(make_S_return(env));

  //       loc = copy_assign1.cc:7:34
  //       succ={ }
  //     S_compound:
  S_compound *body = new S_compound(loc, stmts);

  //     inits:
  FakeList<MemberInit> *inits = FakeList<MemberInit>::emptyList();

  //     Declarator:
  Declarator *nameAndParams = makeCopyAssignDeclarator(env, ct);

  //       typenameUsed = false
  bool typenameUsed = false;
  //         name = X
  //         loc = copy_assign1.cc:7:3
  //       PQ_name:
  PQName *pqnameRetSpec = NULL;
  {
    FakeList<TemplateArgument> *targs = env.make_PQ_templateArgs(ct);
    Variable *typedefVar = ct->getTypedefName();
    xassert(typedefVar);
    pqnameRetSpec = env.make_PQ_possiblyTemplatizedName(loc, typedefVar->name, targs);
  }
  //       loc = copy_assign1.cc:7:3
  //       cv = 
  //     TS_name:
  TypeSpecifier *retspec = new TS_name(loc, pqnameRetSpec, typenameUsed);

  //     dflags = 
  DeclFlags dflags = DF_MEMBER | DF_INLINE;

  //   Function:
  Function *f =
    new Function(dflags,
                 retspec,
                 nameAndParams,
                 inits,
                 body,
                 handlers);
  f->implicitlyDefined = true;

  //   loc = copy_assign1.cc:7:3
  // MR_func:
  return new MR_func(loc, f);
}


// make implicit dtor ****************

// "a.~A();"
static S_expr *make_S_expr_memberDtor(Env &env, StringRef memberName, CompoundType *memberType)
{
  SourceLoc loc = env.loc();

  //       args:
  FakeList<ArgExpression> *args = FakeList<ArgExpression>::emptyList();
  //           name = ~A
  //           loc = ex/dtor2.cc:7:7
  //         PQ_name:
//    PQName *dtorName = env.make_PQ_fullyQualifiedDtorName(memberType);
//    PQ_name *name0 = new PQ_name(loc, env.str(stringc << "~" << memberType->name));
//    PQName *dtorName = name0;
  PQName *dtorName = NULL;
  {
    FakeList<TemplateArgument> *targs = env.make_PQ_templateArgs(memberType);
//      Variable *typedefVar = memberType->getTypedefName();
//      xassert(typedefVar);
//      dtorName = new PQ_qualifier(loc, typedefVar->name, targs, name0);
    dtorName = env.make_PQ_possiblyTemplatizedName
      (loc, env.str(stringc << "~" << memberType->name), targs);
  }

  //             name = a
  //             loc = ex/dtor2.cc:7:5
  //           PQ_name:
  PQ_name *name = new PQ_name(loc, memberName);
  //           var: refers to ex/dtor2.cc:4:5
  //           type: struct A &
  //         E_variable:
  E_variable *evar0 = new E_variable(name);
  //         field: refers to ex/dtor2.cc:1:1
  //         type: ()()
  //       E_fieldAcc:
  E_fieldAcc *efieldacc = new E_fieldAcc(evar0, dtorName);
  //       type: /*cdtor*/
  //     E_funCall:
  E_funCall *efuncall = new E_funCall(efieldacc, args);
  //   FullExpression:
  FullExpression *fullexpr = new FullExpression(efuncall);
  //   loc = ex/dtor2.cc:7:5
  //   succ={ }
  // S_expr:
  return new S_expr(loc, fullexpr);
}

// "this->B::~B();"
static S_expr *make_S_expr_superclassDtor(Env &env, BaseClass *base)
{
  SourceLoc loc = env.loc();

  //       args:
  FakeList<ArgExpression> *args = FakeList<ArgExpression>::emptyList();
  //             name = ~B
  //             loc = ex/dtor2.cc:6:14
  //           PQ_name:

//    PQName *name = env.make_PQ_fullyQualifiedDtorName(base->ct);

  PQ_name *name0 = new PQ_name(loc, env.str(stringc << "~" << base->ct->name));
  //           targs:
  FakeList<TemplateArgument> *targs = env.make_PQ_templateArgs(base->ct);
  //           qualifier = B
  //           loc = ex/dtor2.cc:6:11
  //         PQ_qualifier:
  PQName *name = NULL;
  {
    Variable *typedefVar = base->ct->getTypedefName();
    xassert(typedefVar);
    name = new PQ_qualifier(loc, typedefVar->name, targs, name0);
  }

  //               name = this
  //               loc = ex/dtor2.cc:6:5
  //             PQ_name:
  PQ_name *thisName = new PQ_name(loc, env.str.add("this"));
  //             var: refers to ex/dtor2.cc:5:3
  //             type: struct C *
  //           E_variable:
  E_variable *thisVar = new E_variable(thisName);
  //           type: struct C &
  //         E_deref:
  //         field: refers to ex/dtor2.cc:2:1
  //         type: ()()
  //       E_fieldAcc:
  //       type: /*cdtor*/
  // not sure why it isn't printing as E_arrow
  E_arrow *earrow = new E_arrow(thisVar, name);
  //     E_funCall:
  E_funCall *efuncall = new E_funCall(earrow, args);
  //   FullExpression:
  FullExpression *fullexpr = new FullExpression(efuncall);
  //   loc = ex/dtor2.cc:6:5
  //   succ={ 7:5 }
  // S_expr:
  return new S_expr(loc, fullexpr);
}

void completeDtorCalls(Env &env, Function *func, CompoundType *ct)
{
  xassert(ct);                  // can't be a stand-alone function

  xassert(!func->dtorStatement); // ensure idempotency

  // We add to the statements in *forward* order, unlike when adding
  // to MemberInitializers, but since this is a dtor, not a ctor, we
  // *do* have to do it in reverse.
  SObjStack<S_expr> dtorStmtsReverse;

  FOREACH_OBJLIST(BaseClass, ct->bases, iter) {
    BaseClass *base = const_cast<BaseClass*>(iter.data());
    // omit initialization of virtual base classes, whether direct
    // virtual or indirect virtual.  See cppstd 12.6.2 and the
    // implementation of Function::tcheck_memberInits()
    //
    // FIX: We really should be initializing the direct virtual bases,
    // but the logic is so complex I'm just going to omit it for now
    // and err on the side of not calling enough initializers
    if (!ct->hasVirtualBase(base->ct)) {
      dtorStmtsReverse.push(make_S_expr_superclassDtor(env, base));
    }
  }

  SFOREACH_OBJLIST_NC(Variable, ct->dataMembers, iter) {
    Variable *var = iter.data();
    if (!wantsMemberInit(var)) continue;
    if (!var->type->isCompoundType()) continue;
    dtorStmtsReverse.push(make_S_expr_memberDtor(env, var->name, var->type->asCompoundType()));
  }

  // reverse and append to the statements list
  ASTList<Statement> *dtorStatements = new ASTList<Statement>();
  while (!dtorStmtsReverse.isEmpty()) {
    dtorStatements->append(dtorStmtsReverse.pop());
  }
  // FIX: I can't figure out the bug right now, but in/t0019.cc fails
  // with a seg fault if I put this line *before* the while loop
  // above.  From looking at the data structures, it seems that it
  // shouldn't matter.
  func->dtorStatement = new S_compound(env.loc(), dtorStatements);
}

MR_func *makeDtorBody(Env &env, CompoundType *ct)
{
  // reversed print AST output; remember to read going up even for the
  // tree leaves

  SourceLoc loc = env.loc();
  //     handlers:
  FakeList<Handler> *handlers = FakeList<Handler>::emptyList();

  //       stmts:
  ASTList<Statement> *stmts = new ASTList<Statement>();
  //       loc = ex/dtor2.cc:5:8
  //       succ={ 6:5 }
  //     S_compound:
  S_compound *body = new S_compound(loc, stmts);
  //     inits:
  FakeList<MemberInit> *inits = FakeList<MemberInit>::emptyList();
  //       init is null
  Initializer *init = NULL;
  //         exnSpec is null
  ExceptionSpec *exnSpec = NULL;
  //         cv = 
  CVFlags cv = CV_NONE;
  //         params:
  FakeList<ASTTypeId> *params = FakeList<ASTTypeId>::emptyList();
  //             name = ~C
  //             loc = ex/dtor2.cc:5:3
  //           PQ_name:
  // see this point in makeCopyCtorBody() for a comment on the
  // sufficient generality of this
  PQName *dtorName = new PQ_name(loc, env.str(stringc << "~" << ct->name));
  //           loc = ex/dtor2.cc:5:3
  //         D_name:
  IDeclarator *base = new D_name(loc, dtorName);
  //         loc = ex/dtor2.cc:5:3
  //       D_func:
  IDeclarator *decl = new D_func(loc,
                                 base,
                                 params,
                                 cv,
                                 exnSpec);
  //       var: inline <member> <definition> ~C(/*m: struct C & */ )
  //     Declarator:
  Declarator *nameAndParams = new Declarator(decl, init);
  //       id = /*cdtor*/
  //       loc = ex/dtor2.cc:5:3
  //       cv = 
  //     TS_simple:
  TypeSpecifier *retspec = new TS_simple(loc, ST_CDTOR);
  //     dflags = inline
  DeclFlags dflags = DF_MEMBER | DF_INLINE;
  //     funcType: ()(/*m: struct C & */ )
  //   Function:
  Function *f =
    new Function(dflags,
                 retspec,
                 nameAndParams,
                 inits,
                 body,
                 handlers);
  f->implicitlyDefined = true;
  //   loc = ex/dtor2.cc:5:3
  // MR_func:
  return new MR_func(loc, f);
}

//  ****************


void E_new::elaborate(Env &env, Type *t)
{
  xassert(env.doElaboration);

  // TODO: this doesn't work for new[]

  if (t->isCompoundType()) {
    if (env.disambErrorsSuppressChanges()) {
      TRACE("env", "not adding variable or ctorStatement to E_new `" << /*what?*/
            "' because there are disambiguating errors");
    }
    else {
      var = env.makeVariable(env.loc(), env.makeE_newVarName(), t, DF_NONE);
      // even though this is on the heap, Scott says put it into the
      // local scope
      // FIX: this creates extra temporaries if we are typechecked
      // twice
      env.addVariable(var);
      xassert(ctorArgs);
      // FIX: this creates a lot of extra junk if we are typechecked
      // twice
      ctorStatement = makeCtorStatement(env, var, t->asCompoundType(), ctorArgs->list);
    }
  }
}


void E_delete::elaborate(Env &env, Type *t)
{
  xassert(env.doElaboration);

  // TODO: this doesn't work for delete[]

  if (t->isCompoundType()) {
    dtorStatement = makeDtorStatement(env, t);
  }
}


void E_throw::elaborate(Env &env)
{
  xassert(env.doElaboration);

  // sm: I think what follows is wrong:
  //   - 'globalVar' is created, but a declaration is not, so
  //     an analysis might be confused by its sudden appearance
  //   - the same object is used for all types, but makeCtorStatement
  //     is invoked with different types.. it's weird
  //   - the whole thing with throwClauseSerialNumber is bad.. it
  //     *should* be a member of Env, and set from the outside after
  //     construction if a wrapper analysis wants the numbers to not
  //     be re-used

  // If it is a throw by value, it gets copy ctored somewhere, which
  // in an implementation is some anonymous global.  I can't think of
  // where the heck to make these globals or how to organize them, so
  // I just make it in the throw itself.  Some analysis can come
  // through and figure out how to hook these up to their catch
  // clauses.
  Type *exprType = expr->getType()->asRval();
  if (exprType->isCompoundType()) {
    if (!globalVar) {
      globalVar = env.makeVariable(env.loc(), env.makeThrowClauseVarName(), exprType,
                                   DF_STATIC // I think it is a static global
                                   | DF_GLOBAL);
      // These variables persist beyond the stack frame, so I
      // hesitate to do anything but add them to the global scope.
      // FIX: Then again, this argument applies to the E_new
      // variables as well.
      Scope *gscope = env.globalScope();
      gscope->registerVariable(globalVar);
      gscope->addVariable(globalVar);

      ctorStatement = makeCtorStatement(env, globalVar, exprType->asCompoundType(),
                                        makeExprList1(expr));
    }
  }
}


void S_return::elaborate(Env &env)
{
  xassert(env.doElaboration);

  if (expr) {
    FunctionType *ft = env.scope()->curFunction->funcType;
    xassert(ft);

    // FIX: check that ft->retType is non-NULL; I'll put an assert for now
    // sm: FunctionType::retType is never NULL ...
    xassert(ft->retType);

    if (ft->retType->isCompoundType()) {
      // This is an instance of return by value of a compound type.
      // We accomplish this by calling the copy ctor.

      // get the target of the constructor function
      Variable *retVal = env.lookupVariable(env.str("<retVal>"));
      xassert(retVal && retVal->getType()->equals(ft->retType));

      // get the arguments of the constructor function
      FakeList<ArgExpression> *args0 =
        FakeList<ArgExpression>::makeList(new ArgExpression(expr->expr));
      xassert(args0->count() == 1);

      // make the constructor function
      E_constructor *tmpE_ctor = makeCtorExpr(env, retVal, ft->retType->asCompoundType(), args0);
      xassert(tmpE_ctor);       // FIX: what happens if there is no such compatable copy ctor?

      // Recall that expr is a FullExpression, so we re-use it,
      // "floating" it above the ctorExpression made above
      expr->expr = tmpE_ctor;
      ctorExpr = expr;
      expr = NULL;              // prevent two representations of the return value
    }
  }
}


// EOF
