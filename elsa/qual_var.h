// qual_var.h
// Variable_Q, a C++Qual-aware parallel to Variable

#ifndef QUAL_VAR_H
#define QUAL_VAR_H

#include "sobjlist.h"     // SObjList
#include "variable.h"     // Variable

class Type_Q;             // qual_type.h

class Variable_Q : public Variable {
public:
  // global list of all instances
  static SObjList<Variable_Q> instances;

public:
  Variable_Q(SourceLocation const &L, StringRef n,
             Type_Q *t, DeclFlags f,
             bool put_into_instances_list=true
             );
  ~Variable_Q();

  // Q++Qual-aware type of this variable
  Type_Q const *typeC() const;
  Type_Q *type()
    { return const_cast<Type_Q*>(typeC()); }
};


// cast with potential for runtime check in the future
inline Variable_Q *asVariable_Q(Variable *v)
  { return static_cast<Variable_Q*>(v); }


#endif // QUAL_VAR_H
