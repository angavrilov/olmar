#include "qualifiers.h"

// Daniel Wilkerson dsw@cs.berkeley.edu

//  I am sure that checking this global is faster than checking
//  tracingSys("cc_qual")
int cc_qual_flag;


// functions for the opaque type used in cc_types.cc
string toString(Qualifiers *q) {
  if (q) return q->toString();
  else return string("");
}

Qualifiers *deepClone(Qualifiers *q) {
  if (!q) return new Qualifiers();
  return q->deepClone();
}


string Qualifiers::varsToString() {
  if (!qv) return string("");
  stringBuilder s;
#ifdef CC_QUAL
  // dsw: Prototype for Matt.
    QualifierVariable *tainted_qvar = findQConst("$tainted");
    if (leqQVar(tainted_qvar, qv)) {
      char tmp[200];
//        sprintf(tmp, "%x", (unsigned)this);
      s << " /*transitive ";
//        s << "(" << tmp << ")";
      sprintf(tmp, "%x", (unsigned)qv);
      s << "$tainted " << nameQVar(qv) << "(" << tmp << ")";
//        sprintf(tmp, "%x", (unsigned)tainted_qvar);
//        s << " tainted:" << nameQVar(tainted_qvar) << "(" << tmp << ")";
      s << " */";
    }
#else
    return string("");
#endif
  return s;
}


// This is an attempt to imitate applyCVToType() for our general
// qualifiers.  Yes, there is a lot of commented-out code so the
// context of applyCVToType() is present.
Type const *applyQualifierLiteralsToType(Qualifiers *q, Type const *baseType)
{
  if (baseType->isError()) {
    return baseType;
  }

  if (!q || !q->ql) {
    // keep what we've got
    return baseType;
  }

  // first, check for special cases
  switch (baseType->getTag()) {
    case Type::T_ATOMIC: {
      CVAtomicType const &atomic = baseType->asCVAtomicTypeC();
      // dsw: It seems to me that this is an optimization, so I turn
      // it off for now.
//        if ((atomic.cv | cv) == atomic.cv) {
//          // the given type already contains 'cv' as a subset,
//          // so no modification is necessary
//          return baseType;
//        }
//        else {
        // we have to add another CV, so that means creating
        // a new CVAtomicType with the same AtomicType as 'baseType'
        CVAtomicType *ret = new CVAtomicType(atomic);

        // but with the new flags added
//          ret->cv = (CVFlags)(ret->cv | cv);

        // this already done by the constructor
//          if (atomic.ql) ret->ql = atomic.ql->deepClone(NULL);
//          else xassert(!ret->ql);

        // dsw: FIX: this is being applied in a way with typedefs as
        // to doubly-annotate.
//          cout << "OLD: " << (ret->ql?ret->ql->toString():string("(null)")) << endl;
//          cout << "BEING APPLIED: " << ql->toString() << endl;
        if (q) ret->q = q->deepClone(ret->q);
//          // find the end of the list and copy the QualifierLiterals there
//          QualifierLiterals **qlp;
//          for(qlp = &(ret->ql); *qlp; qlp = &((*qlp)->next)) {}
//          *qlp = ql;

        return ret;
//        }
      break;
    }

    case Type::T_POINTER: {
      // logic here is nearly identical to the T_ATOMIC case
      PointerType const &ptr = baseType->asPointerTypeC();
      if (ptr.op == PO_REFERENCE) {
        return NULL;     // can't apply CV to references
      }
      // dsw: again, I think this is an optimization
//        if ((ptr.cv | cv) == ptr.cv) {
//          return baseType;
//        }
//        else {
        PointerType *ret = new PointerType(ptr);

//          ret->cv = (CVFlags)(ret->cv | cv);

        // this already done by the constructor
//          if (ptr.ql) ret->ql = ptr.ql->deepClone(NULL);
//          else xassert(!ret->ql);
        if (q) ret->q = q->deepClone(ret->q);

//          // find the end of the list and copy the QualifierLiterals there
//          QualifierLiterals **qlp;
//          for(qlp = &(ret->ql); *qlp; qlp = &((*qlp)->next)) {}
//          *qlp = ql;

        return ret;
//        }
      break;
    }

    default:    // silence warning
    case Type::T_FUNCTION:
    case Type::T_ARRAY:
      // can't apply CV to either of these (function has CV, but
      // can't get it after the fact)
      return NULL;
  }
}
