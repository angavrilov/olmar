// cc_elaborate.cc            see license.txt for copyright and terms of use
// code for cc_elaborate.h

#include "cc_elaborate.h"      // this module
#include "cc_ast.h"            // Declaration
#include "cc_env.h"            // Scope, Env
#include "ast_build.h"         // makeExprList1, etc.
#include "trace.h"             // TRACE

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


E_constructor *makeCtorExpr
  (Env &env, Variable *var, Type *type, FakeList<ArgExpression> *args)
{
  xassert(var);                 // this is never for a temporary
  xassert(!var->hasFlag(DF_TYPEDEF));
  PQName *name0 = env.make_PQ_fullyQualifiedName(type->asCompoundType());
  E_constructor *ector0 =
    new E_constructor(new TS_name(env.loc(), name0, false),
                      args);
  ector0->artificial = true;
  // this way the E_constructor::itcheck() can tell that it is not for
  // a temporary; update: I think the previous sentence is now wrong,
  // and it is due to the artificial flag that it can tell; however I
  // think I need the var set here for a different reason; not
  // investigating this now; just wanted to note that the comment is
  // probably wrong.
  ector0->var = var;
  return ector0;
}

Statement *makeCtorStatement
  (Env &env, Variable *var, Type *type, FakeList<ArgExpression> *args)
{
  E_constructor *ector0 = makeCtorExpr(env, var, type, args);
  Statement *ctorStmt0 = new S_expr(env.loc(), new FullExpression(ector0));
  ctorStmt0->tcheck(env);
  return ctorStmt0;
}

Statement *makeDtorStatement(Env &env, Type *type)
{
  // hmm, can't say this because I don't have a var to say it about.
//    xassert(!var->hasFlag(DF_TYPEDEF));
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
          ctorStatement = makeCtorStatement(env, var, type, init->asIN_ctor()->args);
        }
      } else if (type->isCompoundType()) {
        if (init->isIN_expr()) {
          xassert(!(decl->isD_name() && !decl->asD_name()->name)); // that is, not an abstract decl
          // just call the one-arg ctor; FIX: this is questionable; we
          // haven't decided what should really happen for an IN_expr
          ctorStatement =
            makeCtorStatement(env, var, type,
                              makeExprList1(init->asIN_expr()->e));
        } else if (init->isIN_compound()) {
          xassert(!(decl->isD_name() && !decl->asD_name()->name)); // that is, not an abstract decl
          // just call the no-arg ctor; FIX: this is questionable; it
          // is undefined what should happen for an IN_compound since
          // it is a C99-ism.
          ctorStatement = makeCtorStatement(env, var, type, FakeList<ArgExpression>::emptyList());
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
        ctorStatement = makeCtorStatement(env, var, type, FakeList<ArgExpression>::emptyList());
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
      E_constructor *ector = makeCtorExpr(env, tempVar, paramType,
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


void E_new::elaborate(Env &env, Type *t)
{
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
      ctorStatement = makeCtorStatement(env, var, t, ctorArgs->list);
    }
  }
}


void E_delete::elaborate(Env &env, Type *t)
{
  // TODO: this doesn't work for delete[]

  if (t->isCompoundType()) {
    dtorStatement = makeDtorStatement(env, t);
  }
}


void E_throw::elaborate(Env &env)
{
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

      ctorStatement = makeCtorStatement(env, globalVar, exprType,
                                        makeExprList1(expr));
    }
  }
}


// EOF
