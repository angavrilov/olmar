#ifndef QUALIFIERS_H
#define QUALIFIERS_H

// Daniel Wilkerson dsw@cs.berkeley.edu

#include <stdio.h>

#include "strtable.h"
#include "str.h"
#include "xassert.h"
#ifdef CC_QUAL
  #include "cc_qual/cqual_iface.h"
#endif

// boolean: the user passed tr cc_qual on the command line
extern int cc_qual_flag;


// In cqual-land this is actually a C-struct, it has no methods so we
// don't care if it looks like a an opaque void here since we only
// make pointers to this type and never use it directy.
typedef void QualifierVariable;


// linked list of literals: $tainted
class QualifierLiterals {
  public:
  StringRef name;
  QualifierLiterals *next;

  public:
  explicit QualifierLiterals(StringRef name, QualifierLiterals *next = NULL)
    : name(name), next(next) {
  }

  string toString() {
    stringBuilder s;
    s << " " << name;
    if (next) next->buildString(s);
    s << " "; // dsw: FIX: should be able to get rid of this.
    return s;
  }
  QualifierLiterals *deepClone(QualifierLiterals *tail = NULL) {
    return new QualifierLiterals(name, (next? next->deepClone(tail) : tail));
  }

  private: void buildString(stringBuilder &s) {
    s << " " << name;
    if (next) next->buildString(s);
  }
};


// Types may be annotated by both: many (>0) literals and a single
// (=1) variable.
class Qualifiers {
  public:
  QualifierLiterals *ql;
  QualifierVariable *qv;

  explicit Qualifiers(QualifierLiterals *ql0 = NULL)
    : ql(ql0), qv(NULL)
  {}

  // just do a deepClone() on the QualifierLiterals
  Qualifiers *deepClone(Qualifiers *tail = NULL) {
    xassert(!qv);               // not good if a qvar exists already
    QualifierLiterals *tail_ql = NULL;
    if (tail) {
      xassert(!tail->qv);
      if (tail->ql) tail_ql = tail->ql->deepClone();
    }
    if (ql) return new Qualifiers(ql->deepClone(tail_ql));
    else return new Qualifiers(tail_ql);
  }

  void prependLiteralString(StringRef s) {
    prependQualifierLiterals(new QualifierLiterals(s));
  }
  void appendLiteralString(StringRef s) {
    appendQualifierLiterals(new QualifierLiterals(s));
  }

  void prependQualifierLiterals(QualifierLiterals *ql0) {
    if (!ql0) return;           // required for correctness
    QualifierLiterals **qlp;    // find end of ql0
    for(qlp = &ql0; *qlp; qlp = &((*qlp)->next)) {}
    *qlp = ql;
    ql = ql0;
  }
  void appendQualifierLiterals(QualifierLiterals *ql0) {
    if (!ql0) return;           // optimization only
    QualifierLiterals **qlp;    // find end of ql
    for(qlp = &ql; *qlp; qlp = &((*qlp)->next)) {}
    *qlp = ql0;
  }

  string literalsToString() {
    if (ql) return ql->toString();
    else return string("");
  }

  // print as qualifier literals the transitive conclusions of cqual
  string varsToString();

  string toString() {
    stringBuilder s;
    s << literalsToString();
    s << varsToString();
    return s;
  }
};

string toString(Qualifiers *q);
Qualifiers *deepClone(Qualifiers *q);

// like applyCVToType in cc_type.cc but for QualifierLiterals
#include "cc_type.h"
Type const *applyQualifierLiteralsToType(Qualifiers *q, Type const *baseType);

#endif // QUALIFIERS_H
