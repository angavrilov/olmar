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

  explicit Qualifiers(const char *name, const SourceLocation &loc, QualifierLiterals *ql0 = NULL)
    : ql(NULL), loc(new SourceFile("UNKNOWN")) {}

  void prependLiteralString(StringRef s) {}
  void appendLiteralString(StringRef s) {}
  void prependQualifierLiterals(QualifierLiterals *ql0) {}
  void appendQualifierLiterals(QualifierLiterals *ql0) {}
  string toString() {return string("");}
  string literalsToString() {return string("");}
  static void insertInstancesIntoGraph();
};

string toString(Qualifiers *q);
string toString (QualifierLiterals *const &);
Qualifiers *deepClone(Qualifiers *q);
Type const *applyQualifierLiteralsToType(Qualifiers *q, Type const *baseType);

#endif // CC_QUALIFIERS_DUMMY_H
