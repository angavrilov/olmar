// aenv.h
// abstract environment

#ifndef AENV_H
#define AENV_H

#include "strsobjdict.h"     // StringSObjDict
#include "strtable.h"        // StringRef
#include "sobjlist.h"        // SObjList
#include "stringset.h"       // StringSet
#include "ohashtbl.h"        // OwnerHashTable
#include "absval.ast.gen.h"  // ASTList, AbsValue, AVvar

class P_and;               // predicate.ast
class Predicate;           // predicate.ast
class VariablePrinter;     // aenv.cc
class Variable;            // variable.h
class TF_func;             // c.ast
class E_funCall;           // c.ast


// map terms into predicates.. mostly superceded by Expression::vcgenPred,
// but still used in at least one place..
Predicate *exprToPred(AbsValue const *expr);


// abstract variable: info maintained about program variables
class AbsVariable {
public:
  Variable const *decl;    // (serf) AST node which introduced the name
  AbsValue *value;         // (region?) abstract value bound to the variable

  // when this is false, the variable is modeled as an unaliasable name,
  // and 'value' is its abstract value; when this is true, the variable
  // is considered aliasable, and 'value' is the abstract value of its
  // *address* in memory
  bool memvar;

public:
  AbsVariable(Variable const *d, AbsValue *v, bool m)
    : decl(d), value(v), memvar(m) {}

  // function for storing these in a hash table keyed by Declarator*
  static void const* getAbsVariableKey(AbsVariable *av);
};


// abstract store (environment)
class AEnv {
private:     // data
  // environment maps program variable declarators (Variable const*)
  // to abstract domain values (AbsVariable*); none of the values
  // are ever AVlval's
  OwnerHashTable<AbsVariable> bindings;

  // set of facts that are applicable to all paths in the current
  // function; these are typically facts derived from the types of
  // the variables mentioned in the function
  ObjList<Predicate> funcFacts;
  
  // set of facts known along the current path; it's a pointer so I
  // can push them onto the pathFactsStack; when new facts are
  // introduced, they go into 'pathFacts'
  ObjList<Predicate> *pathFacts;       // (owner)

  // stack of path facts (push=prepend, pop=removeFirst); everything
  // in the stack is considered a known fact for proving purposes;
  // this stack is often empty (therefore only 'pathFacts' has useful
  // information)
  ObjList< ObjList<Predicate> > pathFactsStack;

  // (owner of list of serfs) stack of facts derived from where we are
  // in an expression, e.g. if I see "a ==> b" then I'll put "a" here
  // while I analyze "b"; the facts pointed-to are *not* owned here;
  // whoever pushes the facts owns them
  SObjList<Predicate> exprFacts;

  // list of objects addresses known to be distinct, in addition to
  // those for which 'memvar' is true; the two lists are concatenated
  // into one list of things all assumed to be mutually distinct
  SObjList<AbsValue> distinct;

  // accessor functions for types
  //P_and *typeFacts;

  // small attempt to conserve memory by reuse..
  AVwhole whole;

public:      // data
  // name of the memory variable
  Variable const *mem;

  // name of the result variable
  Variable const *result;

  // function we're currently in
  TF_func const *currentFunc;

  // list of types we've codified with accessor functions
  StringSet seenStructs;

  // true when we're analyzing a predicate; among other things,
  // this changes how function applications are treated
  bool inPredicate;

  // when set to true, calls to 'prove' always return true
  bool disableProver;

  // need access to the string table to make new names
  StringTable &stringTable;

  // # of failed proofs
  int failedProofs;

  // nominally, this is 0; when we discover that our assumptions are
  // inconsistent, this is set to 1 (actually, incremented); a
  // slightly clever scheme allows tracking of inconsistency across
  // push/popFact (i.e. this is sometimes > 1)
  int inconsistent;

private:     // funcs
  bool innerProve(Predicate * /*serf*/ pred,
                  char const *printFalse,
                  char const *printTrue,
                  char const *context);
  void printFact(VariablePrinter &vp, Predicate const *fact);

public:      // funcs
  AEnv(StringTable &table, Variable const *mem);
  ~AEnv();

  StringRef str(char const *s);

  // prepare for a new path; forget everything that was particular
  // to the last path analyzed
  void newPath();

  // similarly for an entirely new function
  void newFunction(TF_func const *newFunc);

  // set/get variable values
  void set(Variable const *var, AbsValue *value);
  AbsValue *get(Variable const *var);    // makes up a variable if no binding
  bool hasBinding(Variable const *var) const;

  // set lvalues
  void setLval(AVlval const *lval, AbsValue *value);
  void setLval(Variable const *var, AbsValue *offset, AbsValue *value);

  // make and return a fresh variable reference; the string
  // is attached to indicate what this variable stands for,
  // or why it was created; the prefix becomes part of the variable name
  // (AVvar is a subclass of AbsValue)
  AVvar *freshVariable(char const *prefix, char const *why);
                                        
  // forget what we knew about memory
  void makeFreshMemory(char const *why);

  // make up a name for the address of the named variable, and add
  // it to the list of known address-taken variables; retuns the
  // AVvar used to represent the address
  AbsValue *addMemVar(Variable const *var);

  // query whether something is a memory variable, and if so
  // retrieve the associated address
  bool isMemVar(Variable const *var) const;
  AbsValue *getMemVarAddr(Variable const *var);

  // change a variable's value; sensitive to whether it is
  // a memvar or not (returns 'newValue')
  AbsValue *updateVar(Variable const *var, AbsValue *newValue);

  // add an address to those considered mutually distinct
  void addDistinct(AbsValue *obj);

  // add facts about the variable and its value which are a
  // consequence of being declared to have 'value'
  void addDeclarationFacts(Variable const *var, AbsValue *value);

  // add an assumption to pathFacts which says that no memory location
  // currently points to 'v'; useful after an allocation, when 'v'
  // stands for the address of the newly allocated object
  void assumeNoFieldPointsTo(AbsValue *v);

  // refresh bindings that aren't known to be constant across a call
  void forgetAcrossCall(E_funCall const *call);

  // set/get the current abstract value of memory
  AbsValue *getMem() { return get(mem); }
  void setMem(AbsValue *newMem); // { set(mem, newMem); }

  // given a value that might be an lvalue, map it to an rvalue;
  // this dereferences AVaddr nodes
  AbsValue *rval(AbsValue *possibleLvalue);

  // proof assumption; these things add to the *path* facts
  void addFact(Predicate * /*owner*/ pred, char const *why);
  void addBoolFact(Predicate *pred, bool istrue, char const *why);
  void addFalseFact(Predicate *falsePred, char const *why)
    { addBoolFact(falsePred, false, why); }

  // fact stack manipulation; this is a bit of a hack for now; better would
  // be to keep a true stack internal to AEnv instead of exporting things
  // like list lengths..
  //int factStackDepth() const;
  //void popRecentFacts(int prevDepth, P_and *dest);

  // push a new frame of path facts
  void pushPathFactsFrame();

  // pop the most recent frame; yields an owner pointer
  ObjList<Predicate> *popPathFactsFrame();

  // move a whole bunch of predicates into funcFacts, and deallocate
  // the list itself too
  void setFuncFacts(ObjList<Predicate> * /*owner*/ newFuncFacts);

  // transfer to 'newFacts' any facts in 'pathFacts' that refer to anything
  // in 'variables'; only look at facts whose index in 'pathFacts' is
  // 'prevDepth' or greater
  //void transferDependentFacts(ASTList<AVvar> const &variables,
  //                            int prevDepth, P_and *newFacts);

  // manipulate the stack of expression-specific facts
  void pushExprFact(Predicate * /*serf*/ pred);
  void popExprFact();      // must pop before corresponding 'pred' is deleted

  // proof obligation; nonzero return means predicate proved, and in particular
  // PR_INCONSISTENT means *any* predicate can be proved because assumptions
  // active are inconsistent
  enum ProofResult { PR_FAIL=0 /*aka false*/, PR_SUCCESS, PR_INCONSISTENT };
  ProofResult prove(Predicate * /*owner*/ pred, char const *context, bool silent=false);

  // pseudo-memory-management; semantics not very precise at the moment
  AbsValue *grab(AbsValue *v);
  void discard(AbsValue *v);
  AbsValue *dup(AbsValue *v);

  // misc
  StringRef str(char const *s) const { return stringTable.add(s); }
  
  // syntactic sugar for absvals
//    AbsValue *avSelect(AbsValue *mem, AbsValue *obj, AbsValue *offset)
//      { return avFunc3("select", mem, obj, offset); }
//    AbsValue *avUpdate(AbsValue *mem, AbsValue *obj, AbsValue *offset, AbsValue *newValue)
//      { return avFunc4("update", mem, obj, offset, newValue); }
//    AbsValue *avPointer(AbsValue *obj, AbsValue *offset)
//      { return avFunc2("pointer", obj, offset); }
//    AbsValue *avObject(AbsValue *ptr)
//      { return avFunc1("object", ptr); }
//    AbsValue *avOffset(AbsValue *ptr)
//      { return avFunc1("offset", ptr); }

  // new sel/upd symbols
  AbsValue *avSel(AbsValue *obj, AbsValue *index)
    { return avFunc2("sel", obj, index); }
  AbsValue *avUpd(AbsValue *obj, AbsValue *index, AbsValue *value)
    { return avFunc3("upd", obj, index, value); }

  // new lval symbols
  AVlval *avLval(Variable const *var, AbsValue *offset)
    { return new AVlval(var, offset); }
  AbsValue *avSub(AbsValue *index, AbsValue *rest)
    { return new AVsub(index, rest); }
  AVwhole *avWhole()
    { return &whole; }

  // sel/upd indexed with offsets (implementations do a little simplification)
  AbsValue *avSelOffset(AbsValue *obj, AbsValue *offset);
  AbsValue *avUpdOffset(AbsValue *obj, AbsValue *offset, AbsValue *value);
  AbsValue *avAppendIndex(AbsValue *offset, AbsValue *index);

  // pointer arithmetic
  AbsValue *avAddOffset(AbsValue *offset, AbsValue *index)
    { return avFunc2("addOffset", offset, index); }

  // for char* strings
  AbsValue *avLength(AbsValue *obj)
    { return avFunc1("length", obj); }
  AbsValue *avFirstZero(AbsValue *obj)
    { return avFunc1("firstZero", obj); }

//    AbsValue *avSetElt(AbsValue *fieldIndex, AbsValue *obj, AbsValue *newValue)
//      { return avFunc3("setElt", fieldIndex, obj, newValue); }
//    AbsValue *avGetElt(AbsValue *fieldIndex, AbsValue *obj)
//      { return avFunc2("getElt", fieldIndex, obj); }

  AbsValue *avInDegree(AbsValue *mem, AbsValue *ofs, AbsValue *obj)
    { return avFunc3("inDegree", mem, ofs, obj); }

  AbsValue *nullPointer()
    //{ return avPointer(avInt(0), avInt(0)); }
    { return new AVsub(avInt(0), avWhole()); }

  AbsValue *avFunc1(char const *func, AbsValue *v1);
  AbsValue *avFunc2(char const *func, AbsValue *v1, AbsValue *v2);
  AbsValue *avFunc3(char const *func, AbsValue *v1, AbsValue *v2, AbsValue *v3);
  AbsValue *avFunc4(char const *func, AbsValue *v1, AbsValue *v2, AbsValue *v3, AbsValue *v4);
  AbsValue *avFunc5(char const *func, AbsValue *v1, AbsValue *v2, AbsValue *v3, AbsValue *v4, AbsValue *v5);

  AbsValue *avInt(int i);
  AbsValue *avSum(AbsValue *a, AbsValue *b);

  // owner-pointer manipulation constants
  AbsValue *avOwnerField_ptr()            { return avInt(0); }
  AbsValue *avOwnerField_state()          { return avInt(1); }
  
  AbsValue *avOwnerState_null()           { return avInt(0); }
  AbsValue *avOwnerState_dead()           { return avInt(1); }
  AbsValue *avOwnerState_owning()         { return avInt(2); }

  // query..
  bool isNullPointer(AbsValue const *val) const;
  bool isZero(AbsValue const *val) const;

  // prove if not obvious
  void proveIsNullPointer(AbsValue const *val, char const *why);
  void proveIsZero(AbsValue const *val, char const *why);

  // debugging
  void print();
};

// utility
//void addFactsToConjunction(ObjList<Predicate> const &source, P_and &dest);

#endif // AENV_H
