#ifndef CC_QUALIFIERS_DUMMY_H
#define CC_QUALIFIERS_DUMMY_H

#include "cc_type.h"
#include "str.h"

extern int cc_qual_flag;

class QualifierLiterals {
  public:
  explicit QualifierLiterals(StringRef name, QualifierLiterals *next = NULL) {}
};

class Qualifiers {
  public:
  void *ql;
  SourceLocation loc;

  private:
  Type *t;                      // our type if we know it

  public:
  explicit Qualifiers(const char *, const SourceLocation &, int, Type *,
                      QualifierLiterals *ql0 = NULL) : ql(ql0) {}

  void prependLiteralString(StringRef s) {}
  void appendLiteralString(StringRef s) {}
  void prependQualifierLiterals(QualifierLiterals *ql0) {}
  void appendQualifierLiterals(QualifierLiterals *ql0) {}
  string toString() {return string("");}
  string literalsToString() {return string("");}
  static void insert_instances_into_graph();
};

string toString(Qualifiers *q);
string toString (QualifierLiterals *const &);
Qualifiers *deepClone(Qualifiers *q);
Qualifiers *deepCloneLiterals(Qualifiers *q);
Type *applyQualifierLiteralsToType(Qualifiers *q, Type *baseType);

class Variable_Q;
void nameSubtypeQualifiers(Variable_Q *v);

#endif // CC_QUALIFIERS_DUMMY_H
