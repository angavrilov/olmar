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


void TF_func::vcgen(AEnv &env) const
{
  env.currentFunc = this;
  FunctionType const &ft = *(ftype());

  int numRoots = roots.count();
  traceProgress() << "analyzing " << name()
                  << " (" << numPaths << plural(numPaths, " path")
                  << " total from " << numRoots << plural(numRoots, " root")
                  << ") ...\n";

  // synthesized logic variable for return value
  env.result = nameParams->decl->asD_func()->result;

  // for each path...
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

      // ----------- build the abstract environment ----------
      // clear anything left over in env
      env.clear();

      // add the function parameters to the environment
      {SFOREACH_OBJLIST(Variable, params, iter) {
        Variable const *v = iter.data();

        if (v->name) {
          // the function parameter's initial value is represented
          // by a logic variable of the same name
          env.set(v, new AVvar(v->name,
            stringc << "starting value of parameter " << v->name));
        }
      }}

      // add local variables to the environment
      {SFOREACH_OBJLIST(Variable, locals, iter) {
        Variable const *var = iter.data();
        xassert(var->name);

        // convenience
        StringRef name = var->name;
        Type const *type = var->type;

        // locals start uninitialized; we make up a new logic variable
        // for each one's initial value
        AbsValue *value = new AVvar(name,
          stringc << "starting value of variable " << name);

        if (var->hasAddrTaken() || type->isArrayType()) {
          // model this variable as a location in memory; make up a name
          // for its address
          AbsValue *addr = env.addMemVar(var);

          if (!type->isArrayType()) {
            // state that, right now, that memory location contains 'value'
            env.addFact(new AVbinary(value, BIN_EQUAL,
              env.avSelect(env.getMem(), addr, new AVint(0))));
          }
          else {
            // will model array contents as elements in memory, rather
            // than as symbolic whole-array values...
            delete value;
          }

          int size = 1;         // default for non-arrays
          if (type->isArrayType() && type->asArrayTypeC().hasSize) {
            // state that the length is whatever the array length is
            // (if the size isn't specified I have to get it from the
            // initializer, but that will be TODO for now)
            size = type->asArrayTypeC().size;
          }

          // remember the length, as a predicate in the set of known facts
          env.addFact(new AVbinary(env.avLength(addr), BIN_EQUAL,
                                   new AVint(size)));
        }

        else {
          // model the variable as a simple, named, unaliasable variable
          env.set(var, value);
        }
      }}

      // add 'result' to the environment so we can record what value
      // is actually returned; initially it has no defined value
      if (!ft.retType->isVoid()) {
        env.set(env.result, env.freshVariable("result", "UNDEFINED return value"));
      }

      // let pre_mem be the name of memory now
      // update: the user can bind this herself
      //env.set(env.str("pre_mem"), env.getMem());

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

          env.addFact(ft.precondition->expr->vcgen(env, 0 /*path*/));
        }
      }
      else {
        // the root should be an invariant statement, and that is
        // the start predicate
        S_invariant const *inv = root->asS_invariantC();

        IN_PREDICATE(env);
        env.addFact(inv->expr->vcgen(env, 0 /*path*/));
      }

      // -------------- interpret the code ------------
      // now interpret the function body
      SObjList<Statement /*const*/> stmtList;
      root->vcgenPath(env, stmtList, path, false /*cont*/);

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
// this is only used for globals; local declarations are
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
  // TODO: finish structure-valued objects
  #if 0
  if (type->isCVAtomicType() &&
      type->asCVAtomicTypeC().atomic->isCompoundType()) {
    CompoundType const &ct = type->asCVAtomicTypeC().atomic->asCompoundTypeC();

    if (ct.keyword != CompoundType::K_UNION &&
        !env.seenStructs.contains(ct.name)) {
      env.seenStructs.add(ct.name);

      // construct an example object
      AVfunc *obj = new AVfunc(stringc << "struct_" << ct.name, NULL);
      FOREACH_OBJLIST(CompoundType::Field, ct.fields, iter) {
        obj->args.append(fieldRep(iter.data()));
      }

      // create accessor functions for each field
      FOREACH_OBJLIST(CompoundType::Field, ct.fields, field) {
        P_forall *fa = new P_forall(NULL,
          new P_binary(avFunc1(stringc << "acc_" << field.data()->name, obj),
                       RE_EQUAL,
                       fieldRep(field.data())));

        // add quantifiers
        FOREACH_OBJLIST(CompoundType::Field, ct.fields, iter) {
          fa->variables.append(fieldRep(iter.data()));
        }

        // remember this rule
        env.typeFacts.append(fa);
      }
    }
  }
  #endif // 0

  #if 0     // moved into TF_func
  // convenience
  StringRef name = var->name;
  Type const *type = var->type;

  if (name) {
    xassert(value != NULL);

    if (var->hasAddrTaken() || type->isArrayType()) {
      // model this variable as a location in memory; make up a name
      // for its address
      AbsValue *addr = env.addMemVar(var);

      if (!type->isArrayType()) {
        // state that, right now, that memory location contains 'value'
        env.addFact(new AVbinary(value, BIN_EQUAL,
          env.avSelect(env.getMem(), addr, new AVint(0))));
      }
      else {
        // will model array contents as elements in memory, rather
        // than as symbolic whole-array values...
      }

      // variables have length 1
      int size = 1;             

      if (type->isArrayType() && type->asArrayTypeC().hasSize) {
        // state that the length is whatever the array length is
        // (if the size isn't specified I have to get it from the
        // initializer, but that will be TODO for now)
        size = type->asArrayTypeC().size;
      }

      // remember the length
      env.addFact(new AVbinary(env.avLength(addr), BIN_EQUAL,
                               new AVint(size)));
    }

    else {
      // model the variable as a simple, named, unaliasable variable
      env.set(var, value);
    }
  }
  #endif // 0
  
  // treat the declaration as an assignment; the variable was added
  // to the abstract environment in TF_func::vcgen
  env.updateVar(var, value);
}


// ----------------------- Statement ----------------------
// eval 'expr' in a predicate context
AbsValue *vcgenPredicate(AEnv &env, Expression *expr, int path)
{
  xassert(path == 0);
  IN_PREDICATE(env);
  return expr->vcgen(env, path);
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
    
    // prove the function postcondition
    FA_postcondition const *post = env.currentFunc->ftype()->postcondition;
    if (post) {
      IN_PREDICATE(env);
      env.prove(post->expr->vcgen(env, 0 /*path*/), "postcondition");
    }
  }
  else {
    // consider each choice
    for (VoidListIter iter(successors); !iter.isDone(); iter.adv()) {
      void *np = iter.data();
      Statement const *s = nextPtrStmt(np);

      // are we going to follow 's'?
      if (index < s->numPaths) {
        // yes; vcgen 'node', telling it 's' as the continuation
        vcgen(env, isContinue, exprPath, s);

        // is 's' an invariant?
        if (s->isS_invariant()) {
          // terminate the path & prove the invariant
          env.prove(vcgenPredicate(env, s->asS_invariantC()->expr, index),
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
        index -= s->numPaths;
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
  AbsValue *val = cond->vcgen(env, path);

  if (next == thenBranch) {
    // remember that the guard was true
    env.addFact(val);
  }
  else { 
    // guard is false
    xassert(next == elseBranch);
    env.addFact(avNot(val));
  }
}


void S_switch::vcgen(STMT_VCGEN_PARAMS) const
{
  // evaluate switch-expression
  expr->vcgen(env, path);

  // for now, learn nothing from the outgoing edge
  cout << "ignoring implication of followed switch edge\n";
}


void S_while::vcgen(STMT_VCGEN_PARAMS) const
{
  AbsValue *val = cond->vcgen(env, path);
  if (next == body) {
    env.addFact(val);
  }
  else {
    // 'next' is whatever follows the loop
    env.addFact(avNot(val));
  }
}


void S_doWhile::vcgen(STMT_VCGEN_PARAMS) const
{
  if (!isContinue) {
    // drop straight into body, learn nothing
    xassert(path == 0);
  }
  else {
    AbsValue *val = cond->vcgen(env, path);
    if (next == body) {
      env.addFact(val);
    }
    else {
      env.addFact(avNot(val));
    }
  }
}


void S_for::vcgen(STMT_VCGEN_PARAMS) const 
{
  int modulus = cond->numPaths1();

  if (!isContinue) {
    // init
    init->vcgen(env, false, path / modulus, NULL);
  }
  else {
    // inc
    after->vcgen(env, path / modulus);
  }

  // guard
  AbsValue *val = cond->vcgen(env, path % modulus);
  
  // body?
  if (next == body) {
    env.addFact(val);
  }       
  else {
    env.addFact(avNot(val));
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
      initVal = new AVvar(d->var->name, stringc << "UNINITialized value of var "
                                                << d->var->name);
    }

    // add a binding for the variable
    dcltr.data()->vcgen(env, initVal);
  }

  // if we entered the loop at all, this ensures the last iteration
  // consumed all of 'path'; if we never entered the loop, this
  // checks that path was 0
  xassert(path < subexpPaths);
}


void S_assert::vcgen(STMT_VCGEN_PARAMS) const
{
  // map the expression to my abstract domain
  AbsValue *v = vcgenPredicate(env, expr, path);
  xassert(v);     // already checked it was boolean

  // try to prove it (is not equal to 0)
  env.prove(v, stringc << "assertion at " << loc.toString());
}

void S_assume::vcgen(STMT_VCGEN_PARAMS) const
{
  // evaluate
  AbsValue *v = vcgenPredicate(env, expr, path);
  xassert(v);

  // remember it as a known fact
  env.addFact(v);
}

void S_invariant::vcgen(STMT_VCGEN_PARAMS) const
{
  // we only call the 'vcgen' of an invariant at the *start* of a path
  //env.addFact(vcgenPredicate(env, expr, path));
  
  // update: I'll let the path driver take care of both adding premises,
  // and proving obligations; so s_invariant itself just holds onto the
  // expression
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

  // make a new variable to stand for this string's object (address)
  AbsValue *object = env.freshVariable("str", stringc << "address of " << quoted(s));
  env.addDistinct(object);

  // assert the length of this object is the length of the string (w/ null)
  env.addFact(new AVbinary(env.avLength(object), BIN_EQUAL,
              new AVint(strlen(s)+1)));

  // assert that the first zero is (currently?) at the end
  env.addFact(new AVbinary(env.avFirstZero(env.getMem(), object), BIN_EQUAL,
              new AVint(strlen(s)+1)));

  // the result of this expression is a pointer to the string's start
  return env.avPointer(object, new AVint(0));
}


AbsValue *E_charLit::vcgen(AEnv &env, int path) const
{
  xassert(path==0);
  return env.grab(new AVint(c));
}

AbsValue *E_structLit::vcgen(AEnv &env, int path) const { return avTodo(); }

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
    // ordinary variable: look up the current abstract value for this
    // variable, and simply return that
    return env.get(var);
  }
  else if (type->isArrayType()) {
    // is name of an array, in which case we return the name given
    // to the array's location (it's like an immutable pointer)
    return env.avPointer(env.getMemVarAddr(var), new AVint(0));
  }
  else {
    // memory variable: return an expression which will read it out of memory
    return env.avSelect(env.getMem(), env.getMemVarAddr(var), new AVint(0));
  }
}


AbsValue *E_funCall::vcgen(AEnv &env, int path) const
{
  // evaluate the argument expressions, taking into consideration
  // which path to follow
  SObjList<AbsValue> argExps;
  {
    // 'func'
    int subexpPaths = func->numPaths1();
    func->vcgen(env, path % subexpPaths);   // ignore value; my analysis only uses type info
    path = path / subexpPaths;

    // args
    FOREACH_ASTLIST(Expression, args, iter) {
      subexpPaths = iter.data()->numPaths1();
      argExps.prepend(iter.data()->vcgen(env, path % subexpPaths));
      path = path / subexpPaths;
    }
    xassert(path < subexpPaths);

    argExps.reverse();      // when it's easy to stay linear..
  }

  FunctionType const &ft = func->type->asFunctionTypeC();

  // --------------- predicate: fn symbol application ----------------
  if (env.inPredicate) {
    StringRef funcName = func->asE_variable()->name;
    AVfunc *f = new AVfunc(funcName, NULL);
    while (argExps.isNotEmpty()) {
      f->args.append(argExps.removeAt(0));
    }
    return f;
  }

  // -------------- build an environment for pre/post ----------------
  // the pre- and postconditions get evaluated to an abstract value in
  // an environment containing only parameters (and in the case of the
  // postcondition, 'result); eventually it should also contain
  // globals, but right now I don't have globals in the abstract
  // environment..
  AEnv newEnv(env.stringTable, env.mem);
  newEnv.inPredicate = true;       // will only be used to eval pre/post

  // bind the parameters in the new environment
  SObjListMutator<AbsValue> argExpIter(argExps);
  FOREACH_OBJLIST(FunctionType::Param, ft.params, paramIter) {
    // bind the names to the expressions that are passed
    newEnv.set(paramIter.data()->decl,
               argExpIter.data());

    // I claim that putting the arg exp into the new environment
    // counts as "using" it, so discarding it is not necessary
    // (except that it was created in 'env' and passed to 'newEnv'...)

    argExpIter.adv();
  }

  // let mem = current-mem
  newEnv.setMem(env.getMem());

  // let pre_mem = mem
  //newEnv.set(env.str("pre_mem"), newEnv.getMem());

  // ----------------- prove precondition ---------------
  if (ft.precondition) {
    // include precondition bindings in the environment
    FOREACH_ASTLIST(Declaration, ft.precondition->decls, iter) {
      iter.data()->vcgen(newEnv);
    }

    // finally, interpret the precondition in the parameter-only
    // environment, to arrive at a predicate to prove
    AbsValue *predicate = ft.precondition->expr->vcgen(newEnv, 0);
    xassert(predicate);      // tcheck should have verified we can represent it

    // prove this predicate; the assumptions from the *old* environment
    // are relevant
    env.prove(predicate, stringc << "precondition of call: " << toString());

    // no longer needed
    newEnv.discard(predicate);
  }

  // ----------------- assume postcondition ---------------
  // make a new variable to stand for the value returned
  AbsValue *resultVal = NULL;
  if (!ft.retType->isVoid()) {
    resultVal = env.freshVariable("result",
                  stringc << "function call result: " << toString());

    // add this to the mini environment so if the programmer talks
    // about the result, it will state something about the result variable
    newEnv.set(ft.result, resultVal);
  }

  // make up new variable to represent final contents of memory
  env.setMem(env.freshVariable("mem", 
               stringc << "contents of memory after call: " << toString()));
  newEnv.setMem(env.getMem());

  // NOTE: by keeping the environment from before, we are interpreting
  // all references to parameters as their values in the *pre* state

  if (ft.postcondition) {
    // evaluate it
    AbsValue *predicate = ft.postcondition->expr->vcgen(newEnv, 0);
    xassert(predicate);
    
    // assume it
    env.addFact(predicate);
  }

  // result of calling this function is the result variable
  return resultVal;
}


AbsValue *E_fieldAcc::vcgen(AEnv &env, int path) const
{
  // good results here would require abstract values for structures
  return env.freshVariable("field",
           stringc << "structure field access: " << toString());
}


AbsValue *E_unary::vcgen(AEnv &env, int path) const
{ 
  // this comes first because for sizeof we don't want to
  // get any side effects that may be in the expr
  if (op == UNY_SIZEOF) {
    return env.grab(new AVint(expr->type->reprSize()));
  }
  else {
    if (hasSideEffect(op)) {
      cout << "TODO: unhandled side effect: " << toString() << endl;
    }
    return env.grab(new AVunary(op, expr->vcgen(env, path)));
  }
}

AbsValue *E_binary::vcgen(AEnv &env, int path) const
{
  if (e2->numPaths == 0) {
    // simple side-effect-free binary op
    return env.grab(new AVbinary(e1->vcgen(env, path), op, e2->vcgen(env, 0)));
  }

  // evaluate LHS
  int modulus = e1->numPaths1();
  AbsValue *lhs = e1->vcgen(env, path % modulus);
  path = path / modulus;

  // consider operator
  if (op==BIN_AND || op==BIN_OR) {
    // eval RHS *if* it's followed
    if (path > 0) {
      // we follow rhs; this implies something about the value of lhs:
      // if it's &&, lhs had to be true; false for ||
      env.addBoolFact(lhs, op==BIN_AND);

      // the true/false value is now entirely determined by rhs
      return e2->vcgen(env, path-1);
    }
    else {
      // we do *not* follow rhs; this also implies something about lhs
      env.addBoolFact(lhs, op==BIN_OR);
      
      // it's a C boolean, yielding 1 or 0 (and we know which)
      return env.grab(new AVint(op==BIN_OR? 1 : 0));
    }
  }

  else {
    // non-short-circuit: always evaluate RHS
    return env.grab(new AVbinary(lhs, op, e2->vcgen(env, path)));
  }
}


AbsValue *E_addrOf::vcgen(AEnv &env, int path) const
{
  xassert(path==0);

  if (expr->isE_variable()) {
    return env.avPointer(env.getMemVarAddr(expr->asE_variable()->var), // obj
                         new AVint(0));                                // offset
  }
  else {
    cout << "TODO: unhandled addrof: " << toString() << endl;
    return avTodo();
  }
}


AbsValue *E_deref::vcgen(AEnv &env, int path) const
{
  AbsValue *addr = ptr->vcgen(env, path);

  // emit a proof obligation for safe access
  env.prove(new AVbinary(env.avOffset(addr), BIN_GREATEREQ,
                         new AVint(0)),
                         stringc << "pointer lower bound: " << toString());
  env.prove(new AVbinary(env.avOffset(addr), BIN_LESS,
                         env.avLength(env.avObject(addr))),
                         stringc << "pointer upper bound: " << toString());

  return env.avSelect(env.getMem(), env.avObject(addr), env.avOffset(addr));
}


AbsValue *E_cast::vcgen(AEnv &env, int path) const
{
  // I don't know what account I should take of casts..
  return expr->vcgen(env, path);
}


AbsValue *E_cond::vcgen(AEnv &env, int path) const
{
  // condition
  int modulus = cond->numPaths1();
  AbsValue *guard = cond->vcgen(env, path %modulus);
  path = path / modulus;

  if (th->numPaths == 0 && el->numPaths == 0) {
    // no side effects in either branch; use ?: operator
    return env.grab(new AVcond(guard, th->vcgen(env,0), el->vcgen(env,0)));
  }

  else {
    int thenPaths = th->numPaths1();
    int elsePaths = el->numPaths1();

    if (path < thenPaths) {
      // taking 'then' branch, so we can assume guard is true
      env.addFact(guard);
      return th->vcgen(env, path);
    }
    else {
      // taking 'else' branch, so guard is false
      env.addFalseFact(guard);
      return el->vcgen(env, path - elsePaths);
    }
  }
}


AbsValue *E_gnuCond::vcgen(AEnv &env, int path) const
{
  // again on the assumption there are no side effects in the 'else' branch
  // TODO: make a deep-copier for astgen
  return avTodo();
}


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


AbsValue *E_assign::vcgen(AEnv &env, int path) const
{
  int modulus = src->numPaths1();
  AbsValue *v = src->vcgen(env, path % modulus);
  path = path / modulus;

  if (target->isE_variable()) {
    xassert(path==0);
    Variable const *var = target->asE_variable()->var;

    // properly handles memvars vs regular vars
    env.updateVar(var, v);
  }
  else if (target->isE_deref()) {
    // get an abstract value for the address being modified
    AbsValue *addr = target->asE_deref()->ptr->vcgen(env, path);

    // emit a proof obligation for safe access
    env.prove(new AVbinary(env.avOffset(addr), BIN_GREATEREQ,
                           new AVint(0)), "array lower bound");
    env.prove(new AVbinary(env.avOffset(addr), BIN_LESS,
                           env.avLength(env.avObject(addr))), "array upper bound");

    // memory changes
    env.setMem(env.avUpdate(env.getMem(), env.avObject(addr),
                            env.avOffset(addr), v));
  }

  // TODO: finish structure field accesses
  #if 0
  else if (target->isE_fieldAcc()) {
    E_fieldAcc *fldAcc = target->asE_fieldAcc();
    AbsValue *obj = fldAcc->obj->vcgen(env);
  #endif // 0




  else {
    cout << "TODO: unhandled assignment: " << toString() << endl;
  }

  return env.dup(v);
}


AbsValue *E_arithAssign::vcgen(AEnv &env, int path) const
{
  int modulus = src->numPaths1();
  AbsValue *v = src->vcgen(env, path % modulus);
  path = path / modulus;

  // again, only support variables
  if (target->isE_variable()) {
    xassert(path==0);
    Variable *var = target->asE_variable()->var;

    // extract old value
    AbsValue *old = env.get(var);

    // combine it
    v = env.grab(new AVbinary(old, op, v));

    // replace in environment
    env.set(var, v);

    return env.dup(v);
  }
  else {
    cout << "TODO: unhandled assignment: " << toString() << endl;
    return v;
  }
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


