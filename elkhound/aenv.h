// aenv.h
// abstract environment

#ifndef AENV_H
#define AENV_H

#include "strsobjdict.h"   // StringSObjDict
#include "strtable.h"      // StringRef

class IntValue;            // absval.ast

class AEnv {
private:
  // environment maps program variable names to abstract domain values
  StringSObjDict<IntValue> ints;

  // need access to the string table to make new names
  StringTable &stringTable;

  // monotonic integer for making new names
  int counter;

public:
  AEnv(StringTable &table);
  ~AEnv();

  // forget everything
  void clear();
                                
  // set/get integer values
  void set(StringRef name, IntValue *value);
  IntValue *get(StringRef name);
                                              
  // make and return a fresh variable reference; the string
  // is attached to indicate what this variable stands for,
  // or why it was created
  IntValue *freshIntVariable(char const *why);

  // pseudo-memory-management; semantics not very precise at the moment
  IntValue *grab(IntValue *v);
  void discard(IntValue *v);
  IntValue *dup(IntValue *v);
  
  // debugging
  void print();
};

#endif // AENV_H
