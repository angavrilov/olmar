// matchtype.h

// Attempt to match one type to another using standard recursive
// unification, returning a boolean answering "did they match?".  Note
// however that the notion of matching is asymmetric between left and
// right: we are asking if the left argument is 'at least as specific
// as' the right argument.  Upon encountering unbound variables on the
// right (only) generate bindings such that the unification proceeds;
// For bound variables on the left or right, replace with the binding
// and verify the match.  Prevent infinite loops by caching the pairs
// that have been considered; this works as the problem is monotonic.
//
// This implements the unification for matching template arguments
// with parameters; note that both sides can contain variables since
// template instantiations can occur within the bodies of other
// templates; this fact prevents implementation by a simpler
// algorithm.

#ifndef MATCHTYPE_H
#define MATCHTYPE_H

#include "cc_type.h"            // Type hierarchy
#include "vptrmap.h"            // VoidPtrMap

// register a pair of objects so you can recognize them later; I'm
// thinking if Scott's hashtable class wasn't so hard to figure out I
// could use that instead.
class VoidPairSet {
  // We abuse this guy a bit by feeding him arbirary ints instead of
  // pointers to genuine objects.
  VoidPtrMap setMap;

  class Pair {
    void *a;                    // not owner
    void *b;                    // not owner

    static unsigned int hash0(void *a, void *b) {
      // prime: http://www.utm.edu/research/primes/lists/small/small.html
      static unsigned int const p0 = 1500450271U; // prime
      static unsigned int const p1 = 2860486313U; // prime
      return (reinterpret_cast<unsigned int>(a)*p0 +
              reinterpret_cast<unsigned int>(b))*p1;
    }

    public:
    Pair(void *a0, void *b0) : a(a0), b(b0) {}
    bool equals(Pair const *p2) const { return a==p2->a && b==p2->b; }
    unsigned int hashvalue() const    { return hash0(a, b); }
  };

  // return true if was already there
  bool lookup(void *a, void *b, bool insertIfMissing);

public:
  // is this pair in the set?
  bool in(void *a, void *b)  { return lookup(a, b, false /*insertIfMissing*/); }
  // put this pair into the set
  void put(void *a, void *b) { lookup(a, b, true /*insertIfMissing*/); }
};


template<class T>
class PairSet {
  VoidPairSet vPairSet;
  public:
  bool in(T *a, T *b) {
    return vPairSet.in(a, b);
  }
  void put(T *a, T *b) {
    vPairSet.put(a, b);
  }
};


// this class holds the "global" data used during matching,
// particularly the set of bindings
class MatchTypes {
public:       // types
  enum MFlags {
    MT_NONE      = 0x00000000,
    // this is the top-level call for the traversal of a given type
    MT_TOP       = 0x00000001,
    MT_ALL_FLAGS = 0x00000001
  };

public:       // data
  // handle to use to construct new types when necessary
  TypeFactory &tfac;

  // map from parameter names to arguments
  StringSObjDict<STemplateArgument> bindings;

  // cache of pairs of types seen to prevent infinite loops
  PairSet<Type> typePairSet;

private:      // funcs
  // disallowed
  MatchTypes(MatchTypes&);

  // does listA match listB pairwise?  NOTE: the SYMMETRY in the list
  // serf/ownerness in contrast to the other function for operating on
  // lists of STemplateArgument-s below.
  bool match(ObjList<STemplateArgument> &listA,
             ObjList<STemplateArgument> &listB,
             MFlags mFlags);

  // check 1) if 'a' is a specialization/instantiation of the same
  // primary as 'b', and 2) the instantiation arguments of 'a' matches
  // that of 'b' pairwise
  bool match(TemplateInfo *a, TemplateInfo *b, MFlags mFlags);

  bool match(STemplateArgument *a, STemplateArgument const *b, MFlags mFlags);

  bool match_cva   (CVAtomicType *a,        Type *b, MFlags mFlags);
  bool match_ptr   (PointerType *a,         Type *b, MFlags mFlags);
  bool match_ref   (ReferenceType *a,       Type *b, MFlags mFlags);
  bool match_func  (FunctionType *a,        Type *b, MFlags mFlags);
  bool match_array (ArrayType *a,           Type *b, MFlags mFlags);
  bool match_ptm   (PointerToMemberType *a, Type *b, MFlags mFlags);

  bool unifyIntToVar(int i0, Variable *v1);
  bool unifyToTypeVar(Type *a, Type *b, MFlags mFlags);

public:       // funcs
  // initially, the set of bindings is empty
  MatchTypes(TypeFactory &t) : tfac(t), bindings() {}
  ~MatchTypes();

  // top level entry for checking if Type 'a' matches Type 'b'.
  bool match(Type *a, Type *b, MFlags mFlags);

  // does listA match listB pairwise?  NOTE: asymmetry in the list
  // serf/ownerness of the first and second arguments.
  bool match(SObjList<STemplateArgument> &listA,
             ObjList<STemplateArgument> &listB,
             MFlags mFlags);
};


ENUM_BITWISE_OPS(MatchTypes::MFlags, MatchTypes::MT_ALL_FLAGS);


#endif // MATCHTYPE_H
