// cc_elaborate.h            see license.txt for copyright and terms of use
// info for an elaboration pass (see also cc_elaborate.ast)

#ifndef CC_ELABORATE_H
#define CC_ELABORATE_H

// NOTE: can't just #include cc.ast, b/c cc.ast #includes this

#include "astlist.h"      // ASTList
#include "fakelist.h"     // FakeList

class Scope;              // cc_scope.h
class Env;                // cc_env.h
class Type;               // cc_type.h
class FunctionType;       // cc_type.h
class CompoundType;       // cc_type.h
class Variable;           // variable.h

// cc.ast classes
class Declaration;
class E_constructor;
class E_variable;
class Statement;
class ArgExpression;
class Expression;
class MR_func;
class Function;


// Objects with a FullExpressionAnnot have their own scope containing
// both these declarations and their sub-expressions.  The
// declarations come semantically before the sub-expression with
// which this object is associated.
class FullExpressionAnnot {
public:      // types
  // Why is there not a "finally" in C++!?!
  class StackBracket {
    Env &env;
    FullExpressionAnnot &fea;
    Scope *s;
  public:
    explicit StackBracket(Env &env0, FullExpressionAnnot &fea0);
    ~StackBracket();
  };

public:      // data
  ASTList<Declaration> declarations;

public:      // funcs
  FullExpressionAnnot();
  ~FullExpressionAnnot();

  // defined in cc_tcheck.cc (for now)
  Scope *tcheck_preorder(Env &env);
  void tcheck_postorder(Env &env, Scope *scope);
};


E_constructor *makeCtorExpr(Env &env,
                            Variable *target,
                            CompoundType *cpdType,
                            FakeList<ArgExpression> *args);
Statement *makeCtorStatement(Env &env,
                             Variable *target,
                             CompoundType *cpdType,
                             FakeList<ArgExpression> *args);
Statement *makeDtorStatement(Env &env, Type *type);

E_variable *wrapVarWithE_variable(Env &env, Variable *var);
Expression *elaborateCallSite(Env &env, FunctionType *ft, FakeList<ArgExpression> *args);
void elaborateFunctionStart(Env &env, FunctionType *ft);

void completeNoArgMemberInits(Env &env, Function *ctor, CompoundType *ct);
MR_func *makeNoArgCtorBody(Env &env, CompoundType *ct);
MR_func *makeCopyCtorBody(Env &env, CompoundType *ct);
MR_func *makeCopyAssignBody(Env &env, CompoundType *ct);
void completeDtorCalls(Env &env, Function *func, CompoundType *ct);
MR_func *makeDtorBody(Env &env, CompoundType *ct);

#endif // CC_ELABORATE_H
