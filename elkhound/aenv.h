// aenv.h
// abstract environment

#ifndef AENV_H
#define AENV_H

#include "strsobjdict.h"   // StringSObjDict
#include "strtable.h"      // StringRef
#include "sobjlist.h"      // SObjList
#include "stringset.h"     // StringSet
#include "ohashtbl.h"      // OwnerHashTable

class AbsValue;            // absval.ast
class P_and;               // predicate.ast
class Predicate;           // predicate.ast
class VariablePrinter;     // aenv.cc
class Variable;            // variable.h
class TF_func;             // c.ast


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
  // environment maps program variable declarators to abstract domain values
  OwnerHashTable<AbsVariable> bindings;

  // (owner) set of known facts, as a big conjunction
  P_and *facts;

  // list of objects addresses known to be distinct, in addition to
  // those for which 'memvar' is true; the two lists are concatenated
  // into one list of things all assumed to be mutually distinct
  SObjList<AbsValue> distinct;

  // monotonic integer for making new names
  int counter;

  // accessor functions for types
  P_and *typeFacts;

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

  // need access to the string table to make new names
  StringTable &stringTable;

  // # of failed proofs
  int failedProofs;

private:     // funcs
  bool innerProve(Predicate * /*serf*/ pred,
                  char const *printFalse,
                  char const *printTrue,
                  char const *context);
  void printFact(VariablePrinter &vp, Predicate const *fact);

public:      // funcs
  AEnv(StringTable &table, Variable const *mem);
  ~AEnv();

  // forget everything
  void clear();

  // set/get variable values
  void set(Variable const *var, AbsValue *value);
  AbsValue *get(Variable const *var);

  // make and return a fresh variable reference; the string
  // is attached to indicate what this variable stands for,
  // or why it was created; the prefix becomes part of the variable name
  AbsValue *freshVariable(char const *prefix, char const *why);

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

  // set/get the current abstract value of memory
  AbsValue *getMem() { return get(mem); }
  void setMem(AbsValue *newMem) { set(mem, newMem); }

  // proof assumption
  void addFact(AbsValue *expr);
  void addBoolFact(AbsValue *expr, bool istrue);
  void addFalseFact(AbsValue *falseExpr) { addBoolFact(falseExpr, false); }

  // proof obligation
  void prove(AbsValue const *expr, char const *context);

  // pseudo-memory-management; semantics not very precise at the moment
  AbsValue *grab(AbsValue *v);
  void discard(AbsValue *v);
  AbsValue *dup(AbsValue *v);

  // misc
  StringRef str(char const *s) const { return stringTable.add(s); }
  
  // syntactic sugar for absvals
  AbsValue *avSelect(AbsValue *mem, AbsValue *obj, AbsValue *offset)
    { return avFunc3("select", mem, obj, offset); }
  AbsValue *avUpdate(AbsValue *mem, AbsValue *obj, AbsValue *offset, AbsValue *newValue)
    { return avFunc4("update", mem, obj, offset, newValue); }
  AbsValue *avPointer(AbsValue *obj, AbsValue *offset)
    { return avFunc2("pointer", obj, offset); }
  AbsValue *avObject(AbsValue *ptr)
    { return avFunc1("object", ptr); }
  AbsValue *avOffset(AbsValue *ptr)
    { return avFunc1("offset", ptr); }
  AbsValue *avLength(AbsValue *obj)
    { return avFunc1("length", obj); }
  AbsValue *avFirstZero(AbsValue *mem, AbsValue *obj)
    { return avFunc2("firstZero", mem, obj); }

  AbsValue *avFunc1(char const *func, AbsValue *v1);
  AbsValue *avFunc2(char const *func, AbsValue *v1, AbsValue *v2);
  AbsValue *avFunc3(char const *func, AbsValue *v1, AbsValue *v2, AbsValue *v3);
  AbsValue *avFunc4(char const *func, AbsValue *v1, AbsValue *v2, AbsValue *v3, AbsValue *v4);
  AbsValue *avFunc5(char const *func, AbsValue *v1, AbsValue *v2, AbsValue *v3, AbsValue *v4, AbsValue *v5);


  // debugging
  void print();
};

#endif // AENV_H
