// vcgen.cc
// vcgen methods on the C abstract syntax tree

#include "c.ast.gen.h"          // C ast
#include "absval.ast.gen.h"     // abstract domain values
#include "aenv.h"               // AEnv
#include "cc_type.h"            // FunctionType, etc.
#include "sobjlist.h"           // SObjList
#include "trace.h"              // tracingSys
#include "strutil.h"            // quoted, plural
#include "paths.h"              // countExprPaths
#include "predicate.ast.gen.h"  // Predicate ast, incl. P_equal, etc.

#include <stdlib.h>             // getenv

#define IN_PREDICATE(env) Restorer<bool> restorer(env.inPredicate, true)


// use for places that aren't implemented yet
AbsValue *avTodo()
{
  return new AVint(12345678);   // hopefully stands out in debug output
}


// ------------------- TranslationUnit ---------------------
void TranslationUnit::vcgen(AEnv &env) const
{
  FOREACH_ASTLIST(TopForm, topForms, iter) {
    iter.data()->vcgen(env);
  }
}


// --------------------- TopForm ----------------------
void TF_decl::vcgen(AEnv &env) const
{
  decl->vcgen(env);
}


bool shouldSkipFunc(char const *name, char const *stage)
{
  static char const *selectedFunc = getenv("SELFUNC");
  if (selectedFunc && 0!=strcmp(name, selectedFunc)) {
    traceProgress() << "skipping " << stage << " of "
                    << name << " due to SELFUNC" << endl;
    return true;
  }
  else {
    return false;
  }
}

  
// reference every variable in 'vars'
void instantiateVariables(AEnv &env, SObjList<Variable> const &vars)
{                                    
  SFOREACH_OBJLIST(Variable, vars, iter) {
    // avoid instantiating logic variables.. I think there may
    // be unsoundness lurking if any path visits the same
    // quantifier more than once.. might facts from the previous
    // quantification survive to the next one?
    if (!iter.data()->hasFlag(DF_LOGIC)) {
      env.get(iter.data());
    }
  }
}


void TF_func::vcgen(AEnv &env) const
{
  if (shouldSkipFunc(name(), "vcgen")) {
    return;
  }

  env.newFunction(this);
  FunctionType const &ft = *(ftype());

  // for every local variable, and every global variable referenced,
  // make up logic variables and introduce type-based facts; the
  // purpose of this is to make sure these things get introduced now,
  // not in the middle of some quantified expression where the
  // quantifier will want to capture new facts..
  env.pushPathFactsFrame();
  instantiateVariables(env, params);
  instantiateVariables(env, locals);
  instantiateVariables(env, globalRefs);
  env.setFuncFacts(env.popPathFactsFrame());

  int numRoots = roots.count();
  traceProgress() << "analyzing " << name()
                  << " (" << numPaths << plural(numPaths, " path")
                  << " total from " << numRoots << plural(numRoots, " root")
                  << ") ...\n";

  // synthesized logic variable for return value
  env.result = nameParams->decl->asD_func()->result;

  // for each root...
  int rootIndex=0;
  SFOREACH_OBJLIST(Statement, roots, rootIter) {
    Statement const *root = rootIter.data();
    rootIndex++;

    traceProgress() << "  root " << rootIndex << "/" << numRoots
                    << ", " << root->numPaths
                    << plural(root->numPaths, " path") << "\n";

    // for each path from that root...
    for (int path=0; path < root->numPaths; path++) {
      traceProgress() << "    path " << path << "\n";

      if (tracingSys("printAnalysisPath")) {
        SObjList<Statement> nodeList;
        printPathFrom(nodeList, path /*index*/, root, false /*isContinue*/);
      }

      // ----------- build the abstract environment ----------
      // clear anything left over in env
      env.newPath();

      // ------------- establish set of known facts ---------------
      // add the path's start predicate as an assumption
      if (root == body) {
        // root is start of function; precondition is start predicate
        if (ft.precondition) {
          IN_PREDICATE(env);

          // add any precondition bindings
          FOREACH_ASTLIST(Declaration, ft.precondition->decls, iter) {
            iter.data()->vcgen(env);
          }

          env.addFact(ft.precondition->expr->vcgenPred(env, 0 /*path*/),
                      "precondition");
        }
      }
      else {
        // the root should be an invariant statement, and that is
        // the start predicate
        S_invariant const *inv = root->asS_invariantC();

        //env.addFact(inv->expr->vcgenPred(env, 0 /*path*/), "invariant");
        env.addFact(inv->vcgenPredicate(env), "invariant");
      }

      // -------------- interpret the code ------------
      // now interpret the function body
      SObjList<Statement /*const*/> stmtList;
      root->vcgenPath(env, stmtList, path, false /*cont*/);
      if (env.inconsistent) {
        traceProgress() << "      (infeasible)\n";
      }

      // NOTE: the path's termination predicate will be proven
      // inside the vcgenPath call above

      // print the results
      if (tracingSys("absInterp")) {
        cout << "interpretation path:\n";
        env.print();
      }
    }
  }

  env.result = NULL;
  env.currentFunc = NULL;
}


// --------------------- Declaration --------------------
// NOTE: this is only used for globals; local declarations are
// taken apart by S_decl
void Declaration::vcgen(AEnv &env) const
{
  FOREACH_ASTLIST(Declarator, decllist, iter) {
    Declarator const *d = iter.data();
    if (d->init) {
      d->vcgen(env, d->init->vcgen(env, d->var->type, 0));
    }
    else {
      // declarator's vcgen just acts like an assignment, so if
      // there's no initializing expression, we don't do anything here
    }
  }
}

AVvar *fieldRep(CompoundType::Field const *field)
{
  return new AVvar(field->name, "field value representative");
}

void Declarator::vcgen(AEnv &env, AbsValue *value) const
{
  // treat the declaration as an assignment; the variable was added
  // to the abstract environment in TF_func::vcgen
  env.updateVar(var, value);
  
  // add facts about the variable and its value which are a
  // consequence of being declared at this point
  env.addDeclarationFacts(var, value);
}


// ----------------------- Statement ----------------------
// eval 'expr' in a predicate (theorem-proving) context
Predicate *vcgenPredicate(AEnv &env, Expression *expr, int path)
{
  if (path != 0) {
    // used to be just an assertion, but then I needed more info...
    cout << "(path=" << path
         << ") predicate expr problem: " << expr->toString() << endl;
    xfailure("nonzero path count in vcgenPredicate");
  }

  IN_PREDICATE(env);
  return expr->vcgenPred(env, path);
}

void proveNonOwning(AEnv &env, AbsValue *ptrVal, char const *why)
{
  ptrVal = env.rval(ptrVal);
  env.prove(P_notEqual(env.avSel(ptrVal, env.avOwnerField_state()),
                       env.avOwnerState_owning()),
            why);
}


void variableLeavingScope(AEnv &env, Variable const *var)
{
  if (var->type->isOwnerPtr()) {
    // OWNER: must be nonowning to leave scope
    proveNonOwning(env, env.get(var), "owner must be nonowning to leave scope");
  }
}


// uber-vcgen: generic, as it uses the embedded CFG; but it
// calls the specific 'vcgen' for the particular statement kind
void Statement::vcgenPath(AEnv &env, SObjList<Statement /*const*/> &path,
                          int index, bool isContinue) const
{
  // validate 'index'
  int exprPaths = countExprPaths(this, isContinue);
  xassert(exprPaths >= 1);
  xassert(0 <= index && index < (numPaths * exprPaths));

  // debugging check
  if (path.contains(this)) {
    cout << "  CIRCULAR path\n";
    return;
  }
  path.prepend(const_cast<Statement*>(this));

  // pick which expression path we will follow
  int exprPath = index % exprPaths;
  index = index / exprPaths;

  // retrieve all successors of this node
  VoidList successors;
  getSuccessors(successors, isContinue);

  if (successors.isEmpty()) {
    // this is a return statement (or otherwise end of function)
    xassert(index == 0);

    // vcgen this statement, telling it no continuation path
    vcgen(env, isContinue, exprPath, NULL);
    if (env.inconsistent) {
      return;
    }

    // prove the function postcondition
    FA_postcondition const *post = env.currentFunc->ftype()->postcondition;
    if (post) {
      IN_PREDICATE(env);
      env.prove(post->expr->vcgenPred(env, 0 /*path*/), "postcondition");
    }
    
    // implicit postconditions: do all scope-exit checks
    SFOREACH_OBJLIST(Variable, env.currentFunc->params, iter1) {
      variableLeavingScope(env, iter1.data());
    }
    SFOREACH_OBJLIST(Variable, env.currentFunc->locals, iter2) {
      variableLeavingScope(env, iter2.data());
    }
  }
  else {
    // consider each choice
    // largely DUPLICATED from paths.cc:printPathFrom
    for (VoidListIter iter(successors); !iter.isDone(); iter.adv()) {
      void *np = iter.data();
      Statement const *s = nextPtrStmt(np);
      int pathsFromS = numPathsThrough(s);

      // are we going to follow 's'?
      if (index < pathsFromS) {
        // yes; vcgen 'node', telling it 's' as the continuation
        vcgen(env, isContinue, exprPath, s);
        if (env.inconsistent) {
          return;
        }

        // is 's' an invariant?
        if (s->isS_invariant()) {
          // terminate the path & prove the invariant
          xassert(exprPath == 0);              // bugfix: had "index" where meant "exprPath" (it's like vcgen call above)
          //env.prove(vcgenPredicate(env, s->asS_invariantC()->expr, exprPath),   // bugfix: had "index" where meant "exprPath" (it's like vcgen call above)
          env.prove(s->asS_invariantC()->vcgenPredicate(env),
                    stringc << "invariant at " << s->loc.toString());
        }
        else {
          // continue the path
          s->vcgenPath(env, path, index, nextPtrContinue(np));
        }
        index = 0;       // remember that we found a path to follow
        break;
      }

      else {
        // no; factor out s's contribution to the path index
        index -= pathsFromS;
      }
    }

    // make sure we followed *some* path
    xassert(index == 0);
  }

  path.removeFirst();
}


void S_skip::vcgen(STMT_VCGEN_PARAMS) const { xassert(path == 0); }

// control-flow targets add nothing; implied predicates are
// added by the preceeding statement
void S_label::vcgen(STMT_VCGEN_PARAMS) const { xassert(path == 0); }
void S_case::vcgen(STMT_VCGEN_PARAMS) const { xassert(path == 0); } 
void S_caseRange::vcgen(STMT_VCGEN_PARAMS) const { xassert(path == 0); }
void S_default::vcgen(STMT_VCGEN_PARAMS) const { xassert(path == 0); }


void S_expr::vcgen(STMT_VCGEN_PARAMS) const
{
  // evaluate and discard
  env.discard(expr->vcgen(env, path));
}


void S_compound::vcgen(STMT_VCGEN_PARAMS) const
{
  // this statement does nothing, because successor info
  // is already part of the CFG
  xassert(path == 0);
}


// canonical statement vcgen: hit the expression language in
// the guard, and then add a predicate based on outgoing edge
void S_if::vcgen(STMT_VCGEN_PARAMS) const
{
  // evaluate the guard
  Predicate *pred = cond->vcgenPred(env, path);

  if (next == thenBranch) {
    // remember that the guard was true
    env.addFact(pred, stringc << "true guard of 'if' at " << loc.toString());
  }
  else {
    // guard is false
    xassert(next == elseBranch);
    env.addFalseFact(pred, stringc << "false guard of 'if' at " << loc.toString());
  }
}


void S_switch::vcgen(STMT_VCGEN_PARAMS) const
{
  // evaluate switch-expression
  expr->vcgen(env, path);

  // for now, learn nothing from the outgoing edge
  cout << "TODO: ignoring implication of followed switch edge\n";
}


void S_while::vcgen(STMT_VCGEN_PARAMS) const
{
  Predicate *pred = cond->vcgenPred(env, path);

  if (next == body) {
    env.addFact(pred, stringc << "true guard of 'while' at " << loc.toString());
  }
  else {
    // 'next' is whatever follows the loop
    env.addFalseFact(pred, stringc << "false guard of 'while' at " << loc.toString());
  }
}


void S_doWhile::vcgen(STMT_VCGEN_PARAMS) const
{
  if (!isContinue) {
    // drop straight into body, learn nothing
    xassert(path == 0);
  }
  else {
    Predicate *pred = cond->vcgenPred(env, path);
    if (next == body) {
      env.addFact(pred, stringc << "true guard of 'do' at " << loc.toString());
    }
    else {
      env.addFalseFact(pred, stringc << "false guard of 'do' at " << loc.toString());
    }
  }
}


void S_for::vcgen(STMT_VCGEN_PARAMS) const 
{
  int modulus = cond->numPaths1();

  if (!isContinue) {
    // init
    // UPDATE: don't do this, because it should get done when 'init'
    // is encountered in the CFG directly
    //init->vcgen(env, false, path / modulus, NULL);
    xassert(path < modulus);
  }
  else {
    // inc
    after->vcgen(env, path / modulus);
  }

  // guard
  Predicate *pred = cond->vcgenPred(env, path % modulus);

  // body?
  if (next == body) {
    env.addFact(pred, stringc << "true guard of 'for' at " << loc.toString());
  }
  else {
    xassert(next == this->next);
    env.addFalseFact(pred, stringc << "false guard of 'for' at " << loc.toString());
  }
}


void S_break::vcgen(STMT_VCGEN_PARAMS) const { xassert(path==0); }
void S_continue::vcgen(STMT_VCGEN_PARAMS) const { xassert(path==0); }

void S_return::vcgen(STMT_VCGEN_PARAMS) const
{
  xassert(next == NULL);
  if (expr) {
    // model as an assignment to 'result'
    env.set(env.result, expr->vcgen(env, path));
  }
  else {
    xassert(path == 0);
  }
}


void S_goto::vcgen(STMT_VCGEN_PARAMS) const { xassert(path==0); }


void S_decl::vcgen(STMT_VCGEN_PARAMS) const
{
  int subexpPaths = 1;
  FOREACH_ASTLIST(Declarator, decl->decllist, dcltr) {
    Declarator const *d = dcltr.data();

    AbsValue *initVal;
    if (d->init) {
      subexpPaths = countExprPaths(d->init);

      // evaluate 'init' to an abstract value; the initializer
      // will need to know which type it's constructing
      initVal = d->init->vcgen(env, d->var->type, path % subexpPaths);

      path = path / subexpPaths;
    }
    else {
      // make up a new name for the uninitialized value
      // (if it's global we get to assume it's 0... not implemented..)
      // UPDATE: now that TF_func::vcgen does a pre-instantiation of
      // all locals, this should not be needed
      // UPDATE2: putting it back as I try to sort this out..
      initVal = env.freshVariable(d->var->name,
        stringc << "UNINITialized value of var " << d->var->name);
        
      if (d->var->type->asRval()->isOwnerPtr()) {
        // OWNER: uninitialized pointers are implicitly dead
        trace("owner") << "initing state to dead for " << d->var->name << endl;
        initVal = env.avUpd(initVal,                     // start object
                            env.avOwnerField_state(),    // field to set
                            env.avOwnerState_dead());    // field value
      }
    }

    // add a binding for the variable
    d->vcgen(env, initVal);
  }

  // if we entered the loop at all, this ensures the last iteration
  // consumed all of 'path'; if we never entered the loop, this
  // checks that path was 0
  xassert(path < subexpPaths);
}


void S_assert::vcgen(STMT_VCGEN_PARAMS) const
{
  // map the expression to my abstract domain
  Predicate *p = vcgenPredicate(env, expr, path);
  xassert(p);

  // try to prove it (is not equal to 0)
  env.prove(p, stringc << "assertion at " << loc.toString());
}

void S_assume::vcgen(STMT_VCGEN_PARAMS) const
{
  // evaluate
  Predicate *p = vcgenPredicate(env, expr, path);
  xassert(p);

  // remember it as a known fact
  env.addFact(p, stringc << "thmprv_assume at " << loc.toString());
}

void S_invariant::vcgen(STMT_VCGEN_PARAMS) const
{
  // we only call the 'vcgen' of an invariant at the *start* of a path
  //env.addFact(vcgenPredicate(env, expr, path));

  // update: I'll let the path driver take care of both adding premises,
  // and proving obligations; so s_invariant itself just holds onto the
  // expression
}

Predicate *S_invariant::vcgenPredicate(AEnv &env) const
{
  IN_PREDICATE(env);

  Predicate *exprPred = expr->vcgenPred(env, 0 /*path*/);
  if (inferFacts.isEmpty()) {
    return exprPred;
  }
  else {
    // construct a conjunction for expr and inferFacts
    P_and *conj = new P_and(NULL);
    conj->conjuncts.append(exprPred);
    SFOREACH_OBJLIST(Expression, inferFacts, iter) {
      conj->conjuncts.append(iter.data()->vcgenPred(env, 0 /*path*/));
    }
    return conj;
  }
}

void S_thmprv::vcgen(STMT_VCGEN_PARAMS) const
{
  xassert(path==0);
  IN_PREDICATE(env);
  s->vcgen(env, false, path, NULL);
}


// ---------------------- Expression -----------------
AbsValue *E_intLit::vcgen(AEnv &env, int path) const
{
  xassert(path==0);
  return env.grab(new AVint(i));
}

AbsValue *E_floatLit::vcgen(AEnv &env, int path) const { return avTodo(); }
AbsValue *E_stringLit::vcgen(AEnv &env, int path) const
{
  xassert(path==0);

  // make a new variable to stand for this string's address
  AbsValue *addr = env.freshVariable("str", stringc << "address of " << quoted(s));
  env.addDistinct(addr);

  // expression which stands for the object as a whole
  AbsValue *object = env.avSel(env.getMem(), addr);

  // assert the length of this object is the length of the string (w/ null)
  env.addFact(P_equal(env.avLength(object),
                      env.avInt(strlen(s)+1)), "string literal length");

  // assert that the offset of the first zero is the length - 1
  env.addFact(P_equal(env.avFirstZero(object),
                      env.avInt(strlen(s))), "string literal zero");

  // the result of this expression is a pointer to the string's start
  return env.avSub(addr, env.avSub(new AVint(0), env.avWhole()));
}


AbsValue *E_charLit::vcgen(AEnv &env, int path) const
{
  xassert(path==0);
  return env.grab(new AVint(c));
}

//AbsValue *E_structLit::vcgen(AEnv &env, int path) const { return avTodo(); }

AbsValue *E_variable::vcgen(AEnv &env, int path) const
{
  xassert(path==0);

  if (var->type->isFunctionType()) {
    // this is the name of a function.. let's say for each global
    // function we have a logic variable of the same name which
    // represents its address
    return env.grab(new AVvar(var->name, "address of global function"));
  }

  if (!env.isMemVar(var)) {
    // ordinary variable: lval of that name
    return env.avLval(var, env.avWhole());
  }
  else {
    // memory variable: lval into 'mem' variable
    return env.avLval(env.mem, env.avSub(env.getMemVarAddr(var), env.avWhole()));
  }
  
//    if (var->type->isArrayType()) {
//      // is name of an array, in which case we return the name given
//      // to the array's location (it's like an immutable pointer)
//      return env.avPointer(env.getMemVarAddr(var), new AVint(0));
//    }
//    else {
//      // memory variable: return an expression which will read it out of memory
//      return env.avSelect(env.getMem(), env.getMemVarAddr(var), new AVint(0));
//    }
}


void deadenOwnerState(AEnv &env, AbsValue *value, char const *why)
{
  if (!value->isAVlval()) {
    // not an lval; probably either NULL, or the return value
    // from an owner-returning function; it will have to be up
    // to the type system to prevent me from losing a reference
    // this way
    trace("owner") << "couldn't deaden rval: " << why << endl;
    return;
  }

  AVlval *lval = value->asAVlval();

  // modify source's 'state' field
  trace("owner") << "deadening state: " << why << endl;
  env.setLval(lval->progVar,
              env.avAppendIndex(lval->offset, env.avOwnerField_state()),
              env.avOwnerState_dead());
}


AbsValue *E_funCall::vcgen(AEnv &env, int path) const
{
  // get type of called function
  FunctionType const &ft = func->type->asRval()->asFunctionTypeC();

  // evaluate the argument expressions, taking into consideration
  // which path to follow
  SObjList<AbsValue> argExps;
  {
    // 'func'
    int subexpPaths = func->numPaths1();
    func->vcgen(env, path % subexpPaths);   // ignore value; my analysis only uses type info
    path = path / subexpPaths;

    // args
    ObjListIter<FunctionType::Param> paramIter(ft.params);
    FOREACH_ASTLIST(Expression, args, iter) {
      Expression const *arg = iter.data();

      // evaluate
      subexpPaths = arg->numPaths1();
      AbsValue *val = arg->vcgen(env, path % subexpPaths);
      
      // if 'val' is an lval, grab the current value, so that the
      // owner modification below will not affect what the function
      // precondition sees
      AbsValue *valToPass = env.rval(val);

      if (arg->type->asRval()->isOwnerPtr() &&
          paramIter.data()->type->isOwnerPtr()) {
        // OWNER: when an owner pointer is passed to an owner param,
        // the argument becomes dead
        deadenOwnerState(env, val,
                         stringc << "passed to owner param: " << arg->toString());
      }
      // TODO: add typechecking rules to prevent passing a serf pointer
      // into an owner param

      argExps.prepend(valToPass);
      path = path / subexpPaths;

    }
    xassert(path < subexpPaths);

    argExps.reverse();      // when it's easy to stay linear..
  }

  // --------------- predicate: fn symbol application ----------------
  if (env.inPredicate) {
    StringRef funcName = func->asE_variable()->name;
    AVfunc *f = new AVfunc(funcName, NULL);
    while (argExps.isNotEmpty()) {
      f->args.append(argExps.removeAt(0));
    }
    return f;
  }

  // -------------- modify environment for pre/post ----------------
  // the pre- and postconditions get evaluated to an abstract value in
  // an environment augmented to include parameters (and in the case
  // of the postcondition, 'result'); I do *not* create another
  // environment, because I can't possibly get confused by variable
  // names, since every variable in the program has a unique name
  SObjListMutator<AbsValue> argExpIter(argExps);
  FOREACH_OBJLIST(FunctionType::Param, ft.params, paramIter) {
    // bind the names to the expressions that are passed
    env.set(paramIter.data()->decl,
            argExpIter.data());

    argExpIter.adv();
  }

  // let pre_mem = mem
  //newEnv.set(env.str("pre_mem"), newEnv.getMem());

  // ----------------- prove precondition ---------------
  IN_PREDICATE(env);

  if (ft.precondition) {
    // include precondition bindings in the environment
    FOREACH_ASTLIST(Declaration, ft.precondition->decls, iter) {
      iter.data()->vcgen(env);
    }

    // finally, interpret the precondition to arrive at a predicate to
    // prove
    Predicate *predicate = ft.precondition->expr->vcgenPred(env, 0);
    xassert(predicate);      // tcheck should have verified we can represent it

    // prove this predicate
    env.prove(predicate, stringc << "precondition of call: " << toString());
  }

  // -------------- conservatively destroy state ------------
  // NOTE: by keeping the environment from before, we are interpreting
  // all references to parameters as their values in the *pre* state
  env.forgetAcrossCall(this);

  // ----------------- assume postcondition ---------------
  // make a new variable to stand for the value returned
  AbsValue *resultVal = NULL;
  if (!ft.retType->isVoid()) {
    resultVal = env.freshVariable("result",
                  stringc << "function call result: " << toString());

    // add this to the environment so if the programmer talks about
    // the result, it will state something about the result variable
    env.set(ft.result, resultVal);
  }

  if (ft.postcondition) {
    // evaluate it
    Predicate *predicate = ft.postcondition->expr->vcgenPred(env, 0);
    xassert(predicate);

    // assume it
    env.addFact(predicate, stringc << "postcondition from call: " << toString());
  }

  // result of calling this function is the result variable
  return resultVal;
}


AbsValue *E_fieldAcc::vcgen(AEnv &env, int path) const
{
  // get val for the object
  AbsValue *objVal = obj->vcgen(env, path);
    
  // and field index for the field
  AbsValue *fieldIndex = env.avInt(field->index);

  // is it an lvalue?  (it almost always is)
  if (objVal->isAVlval()) {
    AVlval *objLval = objVal->asAVlval();

    // append this field's index as an offset
    objLval->offset = env.avAppendIndex(objLval->offset, fieldIndex);
    return objLval;
  }

  else {
    // unusual, but not impossible; just select within the rvalue
    return env.avSel(objVal, fieldIndex);
  }

  #if 0   // old
    if (obj->isE_variable()) {
      Variable *var = obj->asE_variable()->var;
      if (!var->hasAddrTaken()) {
        // modeled as an unaliased tuple
        // retrieve named piece of the (whole) structure value
        return env.avGetElt(env.avInt(field->index), env.get(var));
      }
    }

    if (obj->isE_deref()) {
      E_deref const *deref = obj->asE_derefC();

      // partially DUPLICATED from E_assign::vcgen; reflective of general
      // problem with lvalues

      // get an abstract value for the address of the object being modified
      AbsValue *addr = deref->ptr->vcgen(env, path);

      if (0==strcmp(field->compound->name, "OwnerPtrMeta")) {
        // OWNER: model the 'OwnerPtrMeta' struct as a tuple (obviously
        // I need to generalize the decision of what to model as a tuple)

        // get the tuple value
        AbsValue *tuple =
           env.avSelect(
             env.getMem(),
             env.avObject(addr),
             env.avOffset(addr));

        // select the field I want
        return env.avGetElt(env.avInt(field->index), tuple);
      }

      #if 0    // old: from before I used + to form offset
      if (!env.inPredicate) {
        // the address should have offset 0
        env.prove(P_equal(env.avOffset(addr), env.avInt(0)),
                  "object field access at offset 0");

        // TODO: assign type tags to allocated regions, and prove here
        // that the type tag is correct
      }

      // since we've proved the offset is 0, assume it (this really for
      // the predicate case, where we *don't* prove it, so it acts like
      // an unstated assumption about a pointer to which a field
      // accessor is applied)
      env.addFact(P_equal(env.avOffset(addr), env.avInt(0)),
                  "implicit offset=0 assumption for field access pointer");
      #endif // 0

      // read from memory
      return env.avSelect(env.getMem(),
        env.avObject(addr),                // object being accessed
        env.avSum(env.avOffset(addr),      // offset is pointer's offset (usually 0)
                  env.get(field->decl)));  //  + symbolic offset of the field
    }

    cout << "TODO: unhandled structure field access: " << toString() << endl;
    return env.freshVariable("field",
             stringc << "structure field access: " << toString());
  #endif // 0           
}


AbsValue *E_sizeof::vcgen(AEnv &env, int path) const
{
  // whether 'expr' has side effects or not is irrelevant; they
  // won't be executed at run time; we only are about the type
  return env.grab(new AVint(size));
}

AbsValue *E_unary::vcgen(AEnv &env, int path) const
{
  return env.grab(new AVunary(op, expr->vcgen(env, path)));
}

AbsValue *E_effect::vcgen(AEnv &env, int path) const
{
  cout << "TODO: unhandled side effect: " << toString() << endl;
  return expr->vcgen(env, path);
}


AbsValue *coerceArrayToPointer(AEnv &env, AbsValue *v)
{
  xassert(v->isAVlval());     // must be a reference to the array
  AVlval *lval = v->asAVlval();
  return env.avAppendIndex(lval->offset, env.avInt(0));
}


// usually this is op(val1,val2), but it might be something else
// depending on the type of 'e1' (the expression which yielded 'val1');
// both expression values may be rvals initially
AbsValue *arithmeticCombinator(AEnv &env, AbsValue *val1, Expression *e1,
                               BinaryOp op, AbsValue *val2)
{
  #if 0
  if (e1->type->asRval()->isArrayType()) {
    // coerce to pointer
    val1 = coerceArrayToPointer(env, val1);
  }    
  // make them rvals now, after the array coercion opportunity
  #endif // 0

  val1 = env.rval(val1);
  val2 = env.rval(val2);

  if (e1->type->asRval()->isPointerType()) {
    if (op==BIN_PLUS) {
      // use the addOffset function
      return env.avAddOffset(val1, val2);
    }
    if (op==BIN_MINUS) {
      // similar but with negation
      return env.avAddOffset(val1, env.grab(new AVunary(UNY_MINUS, val2)));
    }
    xfailure("bad pointer operation");
  }

  return env.grab(new AVbinary(val1, op, val2));
}


AbsValue *E_binary::vcgen(AEnv &env, int path) const
{
  if (e2->numPaths == 0) {
    // simple side-effect-free binary op
    return arithmeticCombinator(env, e1->vcgen(env, path), e1,
                                op, e2->vcgen(env, 0));
  }

  // consider operator
  if (op==BIN_AND || op==BIN_OR) {
    // evaluate LHS
    int modulus = e1->numPaths1();
    Predicate *lhs = e1->vcgenPred(env, path % modulus);
    path = path / modulus;

    // eval RHS *if* it's followed
    if (path > 0) {
      // we follow rhs; this implies something about the value of lhs:
      // if it's &&, lhs had to be true; false for ||
      env.addBoolFact(lhs, op==BIN_AND,
        stringc << "LHS guard given RHS is eval'd " << toString());

      // the true/false value is now entirely determined by rhs
      return e2->vcgen(env, path-1);
    }
    else {
      // we do *not* follow rhs; this also implies something about lhs
      env.addBoolFact(lhs, op==BIN_OR,
        stringc << "LHS guard given RHS is *not* eval'd " << toString());

      // it's a C boolean, yielding 1 or 0 (and we know which)
      return env.grab(new AVint(op==BIN_OR? 1 : 0));
    }
  }

  else {
    // evaluate LHS
    int modulus = e1->numPaths1();
    AbsValue *lhs = e1->vcgen(env, path % modulus);
    path = path / modulus;

    // non-short-circuit: always evaluate RHS
    return arithmeticCombinator(env, lhs, e1, op, e2->vcgen(env, path));
  }
}


AbsValue *E_addrOf::vcgen(AEnv &env, int path) const
{
  xassert(path==0);

  AbsValue *exprVal = expr->vcgen(env, path);
  if (exprVal->isAVlval()) {
    AVlval *lval = exprVal->asAVlval();
    xassert(lval->progVar == env.mem);
    return lval->offset;       // pointer value relative to 'mem'
  }

//    if (expr->isE_variable()) {
//      return env.avPointer(env.getMemVarAddr(expr->asE_variable()->var), // obj
//                           new AVint(0));                                // offset
//    }
  else {
    cout << "TODO: unhandled addrof: " << toString() << endl;
    return avTodo();
  }
}


void verifyPointerAccess(AEnv &env, Expression const *expr, AbsValue *addr)
{
  // for a while I was doing ok checking these even inside predicates,
  // but it induces an ordering constraint on the conjuncts in invariants,
  // and that is hard to respect during automatic strengthening
  if (!env.inPredicate) {                         
    env.prove(P_named2(env.str("okSelOffset"),
                       env.getMem(), addr),
              stringc << "pointer access check: " << expr->toString());
                                                         
    #if 0     // old
    env.prove(new P_relation(env.avOffset(addr), RE_GREATEREQ,
                             env.avInt(0)),
                             stringc << "pointer lower bound: " << expr->toString());
    env.prove(new P_relation(env.avOffset(addr), RE_LESS,
                             env.avLength(env.avObject(addr))),
                             stringc << "pointer upper bound: " << expr->toString());
    #endif // 0
  }
}

AbsValue *E_deref::vcgen(AEnv &env, int path) const
{
  AbsValue *addr = env.rval(ptr->vcgen(env, path));

  if (ptr->type->asRval()->isOwnerPtr()) {
    // OWNER: pointer must be in owning state to use
    trace("owner") << "owner pointer being dereferenced for read\n";
    env.prove(P_equal(env.avSel(addr, env.avOwnerField_state()),
                      env.avOwnerState_owning()),
              "owner must be in owner state to dereference");

    // dereferencing an owner needs to actually dereference the 'ptr' field
    addr = env.avSel(addr, env.avOwnerField_ptr());
  }

  // emit a proof obligation for safe access through 'addr'
  verifyPointerAccess(env, this, addr);

  return env.avLval(env.mem, addr);

  //return env.avSelect(env.getMem(), env.avObject(addr), env.avOffset(addr));
}


AbsValue *E_cast::vcgen(AEnv &env, int path) const
{
  AbsValue *v = expr->vcgen(env, path);

  // if the target of the assignment has pointer type and
  // the source has integer type, encode it as a pointer with
  // object 0; this is copied from E_assign, and should replace
  // that code eventually
  if (ctype->type->asRval()->isPointerType() &&
      expr->type->isSimple(ST_INT)) {
    // encode as pointer: should be null..
    env.proveIsZero(env.rval(v), "only zeroes can be cast to pointers");
    v = env.nullPointer();

    // OWNER: if the target is an owner pointer, construct
    // a tuple for it
    if (ctype->type->asRval()->isOwnerPtr()) {
      // first of all, we can only assign the integer zero to owners
      // (redundant given above check, but won't hurt)
      env.proveIsNullPointer(v, "can only assign zero to owners");

      // construct the tuple
      trace("owner") << "yielding a null-owner tuple\n";
      v = env.avUpd(
            env.avInt(0),                // start with no-information tuple
            env.avOwnerField_ptr(),      // field: ptr
            v);                          // value: pointer(0,0)
      v = env.avUpd(
            v,                           // value to update
            env.avOwnerField_state(),    // field: state
            env.avOwnerState_null());    // value: nullowner
    }
  }

  if (!ctype->type->asRval()->isOwnerPtr() &&
      expr->type->asRval()->isOwnerPtr()) {
    // OWNER: if casting from owner to nonowner, select 'ptr' field
    v = env.avSel(env.rval(v), env.avOwnerField_ptr());
  }

  // if casting from array to pointer, get pointer to element 0
  if (ctype->type->asRval()->isPointerType() &&
      expr->type->asRval()->isArrayType()) {
    v = coerceArrayToPointer(env, v);
  }

  return v;
}


AbsValue *E_cond::vcgen(AEnv &env, int path) const
{
  if (th->numPaths == 0 && el->numPaths == 0) {
    int modulus = cond->numPaths1();
    AbsValue *guard = env.rval(cond->vcgen(env, path % modulus));
    path = path / modulus;

    // no side effects in either branch; use ?: operator
    return env.grab(new AVcond(guard, th->vcgen(env,0), el->vcgen(env,0)));
  }

  else {
    int modulus = cond->numPaths1();
    Predicate *guard = cond->vcgenPred(env, path % modulus);
    path = path / modulus;

    int thenPaths = th->numPaths1();
    int elsePaths = el->numPaths1();

    if (path < thenPaths) {
      // taking 'then' branch, so we can assume guard is true
      env.addFact(guard, stringc << "true guard of '?:': " << toString());
      return th->vcgen(env, path);
    }
    else {
      // taking 'else' branch, so guard is false
      env.addFalseFact(guard, stringc << "false guard of '?:': " << toString());
      return el->vcgen(env, path - elsePaths);
    }
  }
}


#if 0
AbsValue *E_gnuCond::vcgen(AEnv &env, int path) const
{
  // again on the assumption there are no side effects in the 'else' branch
  // TODO: make a deep-copier for astgen
  return avTodo();
}
#endif // 0


AbsValue *E_comma::vcgen(AEnv &env, int path) const
{
  int modulus = e1->numPaths1();

  // evaluate and discard
  env.discard(e1->vcgen(env, path % modulus));

  // evaluate and keep
  return e2->vcgen(env, path / modulus);
}


AbsValue *E_sizeofType::vcgen(AEnv &env, int path) const
{
  xassert(path==0);
  return env.grab(new AVint(size));
}


AbsValue *E_new::vcgen(AEnv &env, int path) const
{
  xassert(path==0);

  // to model allocation:
  //   - make up a new logic variable to stand for the address
  //   - state that this address is distinct from any others we know 
  //     about, including NULL
  //   - state that no pointer fields in the program currently point
  //     to the new object    
    
  // new variable
  AbsValue *v = env.freshVariable("newAlloc",
    stringc << "result of " << toString());
    
  // distinct from others
  env.addDistinct(v);

  // we yield a memory offset
  AbsValue *ret = env.avSub(v, env.avWhole());

  // nothing points to this new thing
  env.assumeNoFieldPointsTo(ret);

  return ret;
}


// return true if this variable's value is modeled as a tuple
// which cannot have interior pointers
bool modelAsTuple(Variable *var)
{
  // old condition
  if (var->hasAddrTaken()) { return false; }
  if (var->type->isStructType()) { return true; }

  // OWNER: model owner pointers this way too
  if (var->type->isOwnerPtr()) { return true; }

  return false;
}


AbsValue *E_assign::vcgen(AEnv &env, int path) const
{
  // get source value
  int modulus = src->numPaths1();
  AbsValue *srcValue = src->vcgen(env, path % modulus);
  path = path / modulus;

  // get target lvalue
  AbsValue *targetValue = target->vcgen(env, path);
  xassert(targetValue->isAVlval());     // otherwise can't modify!
  AVlval *targetLval = targetValue->asAVlval();

  // do compound assignment
  if (op != BIN_ASSIGN) {
    srcValue = env.grab(new AVbinary(env.rval(targetLval), op, env.rval(srcValue)));
  }

  if (target->type->asRval()->isOwnerPtr()) {
    // OWNER: verify it is nonowning before reassignment
    proveNonOwning(env, env.rval(targetLval),
                   "verify owner pointer is nonowning before reassignment");
  }

  // do assignment
  env.setLval(targetLval, env.rval(srcValue));

  if (src->type->asRval()->isOwnerPtr()) {
    // OWNER: deaden state of source
    deadenOwnerState(env, srcValue,
                     stringc << "source in assignment: " << toString());
  }

  return env.dup(srcValue);

  #if 0      // I insert explicit casts now
  // HACK: if the target of the assignment has pointer type and
  // the source has integer type, encode it as a pointer with
  // object 0 (the right solution is to, during typechecking,
  // insert explicit coercions; then this code would be a part
  // of the cast-to-pointer coercion)
  if (target->type->asRval()->isPointerType() &&
      src->type->isSimple(ST_INT)) {
    // encode as pointer: some integer offset from the null object
    v = env.avPointer(env.avInt(0), v);
  }
  #endif // 0

  #if 0     // old
  // HACK: if both the target and source are owners, and the source
  // is a dereference of another pointer, then I need to deaden the
  // state of the owner stored in memory (proper solution: do lvals
  // right)
  if (target->type->asRval()->isOwnerPtr() &&
      src->type->asRval()->isOwnerPtr() &&
      src->isE_deref()) {
    E_deref const *srcDeref = src->asE_derefC();
    if (src->numPaths != 0) {    // no side effects
      cout << "WARNING: unhandled ownership transfer: " << toString() << "\n";
    }
    else {
      trace("owner") << "deadening state of owner read through pointer\n";

      // get address of the owner pointer losing ownership
      AbsValue *addr = srcDeref->ptr->vcgen(env, 0 /*path*/);

      // update its 'state' field
      env.setMem(env.avUpdate(env.getMem(),        // starting memory
        env.avObject(addr), env.avOffset(addr),    // object to modify
        env.avSetElt(                              // new value
          env.avOwnerField_state(),                  // modify 'state' field
          v,                                         // prior tuple value
          env.avOwnerState_dead()                    // new state is 'dead'
        )));
    }
  }
  #endif // 0

  #if 0    // old
  if (target->isE_variable()) {
    xassert(path==0);
    Variable const *var = target->asE_variable()->var;

    // extract old value
    AbsValue *old = env.get(var);

    if (op != BIN_ASSIGN) {
      // combine new with old
      v = env.grab(new AVbinary(old, op, v));
    }

    if (var->type->isOwnerPtr()) {
      // OWNER: verify it is nonowning before reassignment
      proveNonOwning(env, old, "verify owner pointer is nonowning before reassignment");
    }

    // properly handles memvars vs regular vars
    env.updateVar(var, v);
  }
  else if (op == BIN_ASSIGN && target->isE_deref()) {
    E_deref *deref = target->asE_deref();
    //string syntax = target->toString();

    // get an abstract value for the address being modified
    AbsValue *addr = deref->ptr->vcgen(env, path);

    // COPIED from E_deref
    if (deref->ptr->type->asRval()->isOwnerPtr()) {
      // OWNER: pointer must be in owning state to use
      trace("owner") << "owner pointer being dereferenced for write\n";
      env.prove(P_equal(env.avGetElt(env.avOwnerField_state(), addr),
                        env.avOwnerState_owning()),
                "owner must be in owner state to dereference");

      // dereferencing an owner needs to actually dereference the 'ptr' field
      addr = env.avGetElt(env.avOwnerField_ptr(), addr);
    }

    // emit a proof obligation for safe access
    verifyPointerAccess(env, this, addr);

    // memory changes
    env.setMem(env.avUpdate(env.getMem(), env.avObject(addr),
                            env.avOffset(addr), v));
  }

  // structure field access
  else if (target->isE_fieldAcc()) {
    E_fieldAcc *fldAcc = target->asE_fieldAcc();

    // accessing a field of a local or global variable
    if (fldAcc->obj->isE_variable()) {
      Variable *var = fldAcc->obj->asE_variable()->var;

      if (modelAsTuple(var)) {
        // modeled as an unaliased tuple

        // get current (whole) structure value
        AbsValue *current = env.get(var);

        // compute updated structure value
        AbsValue *updated =
          env.avSetElt(
            env.avInt(fldAcc->field->index),      // field index
            current,                              // value being updated
            v);                                   // new field value

        if (var->type->isOwnerPtr() &&
            fldAcc->field->index == 0 /*ptr*/) {
          // OWNER: verify it is nonowning before reassignment
          proveNonOwning(env, current,
                         "verify owner pointer is nonowning before reassignment");
        }

        // replace old with new in abstract environment
        env.set(var, updated);
      }
      else {
        goto unhandled;
      }
    }

    // accessing a field through an object pointer
    else if (fldAcc->obj->isE_deref()) {
      // get an abstract value for the address of the object being modified
      AbsValue *addr = fldAcc->obj->asE_deref()->ptr->vcgen(env, path);

      #if 0    // old: before using + for field offsets
      if (!env.inPredicate) {
        // the address should have offset 0
        env.prove(P_equal(env.avOffset(addr), env.avInt(0)),
                  "object field access at offset 0");

        // TODO: assign type tags to allocated regions, and prove here
        // that the type tag is correct
      }                                                
      #endif // 0

      // update memory
      env.setMem(env.avUpdate(env.getMem(),
        env.avObject(addr),                          // object being modified
        env.avSum(env.avOffset(addr),                // offset: pointer offset
                  env.get(fldAcc->field->decl)),     //  + symbolic field offset
        v));                                         // new value
    }

    else {
    unhandled:
      // other forms aren't handled yet
      cout << "TODO: unhandled structure field assignment: " << toString() << endl;
    }
  }

  else {
    cout << "TODO: unhandled assignment: " << toString() << endl;
  }
  #endif // 0
}


AbsValue *E_quantifier::vcgen(AEnv &env, int path) const
{
  // this is quite nonideal, but at the moment I don't know exactly
  // what syntactic rule to use, so I just wait until the problem
  // shows up, and then bomb
  xfailure("can't use quantifier(forall/exists) in a non-obviously-boolean context");
  return NULL;   // silence warning
}


// --------------- Expression::vcgenPred -------------------
Predicate *Expression::vcgenPredDefault(AEnv &env, int path) const
{
  // use exprToPred to give this another chance to map into the
  // predicate space; the vcgenPred functions below do a good job
  // when it's obvious something is being used in a predicate context,
  // but if I save a value into a variable and *then* try to treat
  // it as a predicate, I won't learn until later that I need to
  // map it into predicate space
  return exprToPred(env.rval(vcgen(env, path)));
}


Predicate *E_intLit::vcgenPred(AEnv &env, int path) const
{
  xassert(path==0);

  // when a literal integer is interpreted as a predicate,
  // I can map it immediately to true/false
  return new P_lit(i != 0);
}


Predicate *E_floatLit::vcgenPred(AEnv &env, int path) const
  { return vcgenPredDefault(env, path); }

Predicate *E_stringLit::vcgenPred(AEnv &env, int path) const
{
  return new P_lit(true);
}

Predicate *E_charLit::vcgenPred(AEnv &env, int path) const
{
  return new P_lit(c != 0);
}

Predicate *E_variable::vcgenPred(AEnv &env, int path) const
  { return vcgenPredDefault(env, path); }
  

Predicate *E_funCall::vcgenPred(AEnv &env, int path) const
{
  E_variable const *funcVar = func->ifE_variableC();
  if (funcVar) {                        
    // is this function specially declared to be a predicate that
    // Simplify knows about?
    if (funcVar->var->hasFlag(DF_PREDICATE)) {
      // yes, so emit directly
      xassert(path==0);

      // collect the list of argument values
      ASTList<AbsValue> *avArgs = new ASTList<AbsValue>;
      FOREACH_ASTLIST(Expression, args, iter) {
        avArgs->append(env.rval(iter.data()->vcgen(env, path)));
      }
      return new P_named(funcVar->var->name, avArgs);
    }
  }

  // not a simple function name, or not the right kind
  return vcgenPredDefault(env, path); 
}


Predicate *E_fieldAcc::vcgenPred(AEnv &env, int path) const
  { return vcgenPredDefault(env, path); }
Predicate *E_sizeof::vcgenPred(AEnv &env, int path) const
  { return vcgenPredDefault(env, path); }

Predicate *E_unary::vcgenPred(AEnv &env, int path) const
{
  if (op == UNY_NOT) {
    return new P_not(expr->vcgenPred(env, path));
  }
  else {
    return vcgenPredDefault(env, path);
  }
}

Predicate *E_effect::vcgenPred(AEnv &env, int path) const
  { return vcgenPredDefault(env, path); }


Predicate *E_binary::vcgenPred(AEnv &env, int path) const
{
  int modulus = e1->numPaths1();
  int lhsPath = path % modulus;
  int rhsPath = path / modulus;

  if (isPredicateCombinator(op)) {
    if (e2->numPaths == 0) {
      xassert(rhsPath == 0);

      // simple side-effect-free combinator, so there's no path stuff;
      // *but* we do have to assume something about the LHS while
      // analyzing the RHS, e.g. for "if (p && p->foo) ..."
      Predicate *lhs = e1->vcgenPred(env, lhsPath);

      Predicate *rhs;
      if (op != BIN_OR) {
        env.pushExprFact(lhs);
        rhs = e2->vcgenPred(env, 0);
        env.popExprFact();
      }
      else {
        // for "or", assume the negation
        P_not notLHS(lhs);
        try {
          env.pushExprFact(&notLHS);
          rhs = e2->vcgenPred(env, 0);
          env.popExprFact();
        }
        catch (...) {
          notLHS.p = NULL;
          throw;
        }
        notLHS.p = NULL;
      }

      return P_combinator(op, lhs, rhs);
    }

    else {
      // RHS has a side effect, so we need to analyze paths
      xassert(op != BIN_IMPLIES);     // we don't allow ==> in side-effecting exprs

      // evaluate LHS
      Predicate *lhs = e1->vcgenPred(env, lhsPath);

      // eval RHS *if* it's followed
      if (rhsPath > 0) {
        // we follow rhs; this implies something about the value of lhs:
        // if it's &&, lhs had to be true; false for ||
        env.addBoolFact(lhs, op==BIN_AND,
          stringc << "LHS guard given RHS is eval'd " << toString());

        // the true/false value is now entirely determined by rhs
        return e2->vcgenPred(env, rhsPath-1);
      }
      else {
        // we do *not* follow rhs; this also implies something about lhs
        env.addBoolFact(lhs, op==BIN_OR,
          stringc << "LHS guard given RHS is *not* eval'd " << toString());

        // it's a C boolean, yielding 1 or 0 (and we know which)
        return new P_lit(op==BIN_OR? true : false);
      }
    }
  }

  else if (isRelational(op)) {
    // non-short-circuit: always evaluate RHS
    AbsValue *lhs = env.rval(e1->vcgen(env, lhsPath));
    AbsValue *rhs = env.rval(e2->vcgen(env, rhsPath));
    return new P_relation(lhs, binOpToRelation(op), rhs);
  }

  else {
    return vcgenPredDefault(env, path);
  }
}


Predicate *E_addrOf::vcgenPred(AEnv &env, int path) const
{ 
  return new P_lit(true);
}

Predicate *E_deref::vcgenPred(AEnv &env, int path) const
  { return vcgenPredDefault(env, path); }

Predicate *E_cast::vcgenPred(AEnv &env, int path) const
{
  // assume casts preserve truth or falsity (I think that's sound..)
  return expr->vcgenPred(env, path);
}

Predicate *E_cond::vcgenPred(AEnv &env, int path) const
{
  // condition
  int modulus = cond->numPaths1();
  Predicate *guard = cond->vcgenPred(env, path % modulus);
  path = path / modulus;

  if (th->numPaths == 0 && el->numPaths == 0) {
    // no side effects in either branch;
    // map ?: as a pair of implications
    return P_and2(new P_impl(cond->vcgenPred(env, 0),
                             th->vcgenPred(env, 0)),
                  new P_impl(new P_not(cond->vcgenPred(env, 0)),
                             el->vcgenPred(env, 0)));
  }

  else {
    int thenPaths = th->numPaths1();
    int elsePaths = el->numPaths1();

    if (path < thenPaths) {
      // taking 'then' branch, so we can assume guard is true
      env.addFact(guard, stringc << "true guard of '?:': " << toString());
      return th->vcgenPred(env, path);
    }
    else {
      // taking 'else' branch, so guard is false
      env.addFalseFact(guard, stringc << "false guard of '?:': " << toString());
      return el->vcgenPred(env, path - elsePaths);
    }
  }
}

Predicate *E_comma::vcgenPred(AEnv &env, int path) const
{ 
  int modulus = e1->numPaths1();
  
  // evaluate and discard
  env.discard(e1->vcgen(env, path % modulus));

  // evaluate and keep
  return e2->vcgenPred(env, path / modulus);
}


Predicate *E_sizeofType::vcgenPred(AEnv &env, int path) const
{
  xassert(path==0);
  return new P_lit(size != 0);     // should almost always be true
}


Predicate *E_new::vcgenPred(AEnv &env, int path) const
{
  xassert(path==0);
  return new P_lit(true);          // 'new' always returns non-null
}


Predicate *E_assign::vcgenPred(AEnv &env, int path) const
{
  // full analysis in here would be a major pain, and suggests whether
  // my vcgen vs vcgenPred separation is really the right one.. but
  // for now I can punt and just keep using exprToPred to map the
  // term that results from analyzing this thing
  return exprToPred(this->vcgen(env, path));
}


Predicate *E_quantifier::vcgenPred(AEnv &env, int path) const
{
  // gather up the quantifiers
  P_quantifier *ret = new P_quantifier(NULL, NULL, forall);

  FOREACH_ASTLIST(Declaration, decls, outer) {
    FOREACH_ASTLIST(Declarator, outer.data()->decllist, inner) {
      Variable *var = inner.data()->var;

      // this will instantiate the variables
      ret->variables.append(env.get(var)->asAVvar());
    }
  }
  
  // make a new list of facts, into which path-facts will be
  // accumulated when they are generated inside the body
  env.pushPathFactsFrame();

  // analyze the body
  Predicate *body = pred->vcgenPred(env, path);

  // pop off that new frame so we can look at what was adde
  ObjList<Predicate> *localFacts = env.popPathFactsFrame();    // (owner)

  // search the local fact list for anything which refers to the
  // quantified variables; if we find a fact which does *not*
  // refer to them, put that into the outer path-facts
  ObjListMutator<Predicate> factIter(*localFacts);
  while (!factIter.isDone()) {
    Predicate *fact = factIter.data();

    // does this fact refer to any of the variables in 'ret->variables'?
    bool refers = false;
    FOREACH_ASTLIST(AVvar, ret->variables, varIter) {
      AVvar const *var = varIter.data();

      // does it refer to 'var'?
      if (predicateRefersToAV(fact, var)) {
        // yes; fine, will leave it here
        refers = true;
        break;
      }                    
    }
    
    if (!refers) {
      // the fact doesn't refer to any quantified variables; currently
      // this happens when type-related facts happen to get introduced
      // while inside a quantifier; that will be fixed soon, but I'll
      // leave the logic operational; move this fact to the outer
      // path-facts
      env.addFact(factIter.remove(),   // advances factIter
                  "reason lost; was inside a quantifier");
    }
    else {
      // simple advance
      factIter.adv();
    }
  }

  // now, associate any facts remaining in 'localFacts' with the
  // quantified body
  if (localFacts->isNotEmpty()) {
    P_and *newFacts = new P_and(new ASTList<Predicate>);
    while (localFacts->isNotEmpty()) {
      newFacts->conjuncts.append(localFacts->removeFirst());
    }
    
    if (forall) {
      // if the quantifier is a forall, then we construct an implication
      // so the body can assume the facts we've pushed
      body = new P_impl(newFacts, body);
    }
    else {
      // if the quantifier is an exists, we simply conjoin the facts,
      // effectively (again) making them facts which can be assumed while
      // considering the body
      newFacts->conjuncts.append(body);
      body = newFacts;
    }
  }
  delete localFacts;

  // attach the body to the quantifier node itself
  ret->body = body;

  return ret;
}


// --------------------- Initializer --------------------
AbsValue *IN_expr::vcgen(AEnv &env, Type const *, int path) const
{
  return e->vcgen(env, path);
}


AbsValue *IN_compound::vcgen(AEnv &/*env*/, Type const */*type*/, int /*path*/) const
{
  // I have no representation for array and structure values
  return avTodo();
}


