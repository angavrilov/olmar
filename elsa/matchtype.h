// matchtype.h
// Match a type pattern (from a template) with a concrete type,
// generating either "failure" or a set of bindings.  This
// implements the one-sided unification for matching template
// arguments with parameters.

#ifndef MATCHTYPE_H
#define MATCHTYPE_H

#include "cc_type.h"       // Type hierarchy


// this class holds the "global" data used during matching,
// particularly the set of bindings
class MatchTypes {
public:       // types
  enum Flags {
    ASA_NONE      = 0x00000000,
    // this is the top-level call for the traversal of a given type
    ASA_TOP       = 0x00000001,
    ASA_ALL_FLAGS = 0x00000001
  };

public:       // data
  // handle to use to construct new types when necessary
  TypeFactory &tfac;

  // map from parameter names to arguments
  StringSObjDict<STemplateArgument> bindings;

private:      // funcs
  // disallowed
  MatchTypes(MatchTypes&);

  // FIX: EXACT COPY OF THE CODE ABOVE.  NOTE: the SYMMETRY in the
  // list serf/ownerness, in contrast to the above.
  bool atLeastAsSpecificAs_STemplateArgument_list
    (ObjList<STemplateArgument> &list1,
     ObjList<STemplateArgument> &list2,
     Flags asaFlags);

  // check if 'concrete' is a specialization/instantiation of
  // the same primary as the argument 'pat', and that we are at
  // least as specific as
  bool atLeastAsSpecificAs
    (TemplateInfo *concrete, TemplateInfo *pat,
     Flags asaFlags);

  // return true if 'concrete' is at least as specific as 'pat';
  // if successful, any bindings necessary to
  // effect unification with the argument have been added to bindings
  bool atLeastAsSpecificAs(STemplateArgument *concrete,
                           STemplateArgument const *pat,
                           Flags asaFlags);

  bool unifyIntToVar(int i0, Variable *v1);
  bool unifyToTypeVar(Type *t0,
                      Type *t1,
                      Flags asaFlags);

  bool match_cva(CVAtomicType *concrete, Type *t,
                 Flags asaFlags);
  bool match_ptr_ref(Type *concrete, Type *t,
                     Flags asaFlags);
  bool match_func(FunctionType *concrete, Type *t,
                  Flags asaFlags);
  bool match_array
    (ArrayType *concrete, Type *t,
     Flags asaFlags);
  bool match_ptm(PointerToMemberType *concrete, Type *t,
                 Flags asaFlags);


public:       // funcs
  // initially, the set of bindings is empty
  MatchTypes(TypeFactory &t) : tfac(t), bindings() {}
  ~MatchTypes();

  // true if 'concrete' is at least as specific as 'pat', which
  // is a specialization pattern if it contains any type variables.
  // If it succeeds, creates the bindings that allow the unification
  // and returns true; if not returns false
  bool atLeastAsSpecificAs(Type *concrete, Type *pattern,
                           Flags asaFlags);

  // true iff list1 is at least as specific as list2; creates any
  // bindings necessary to effect the unification; NOTE: assymetry in
  // the list serf/ownerness of the first and second arguments.
  bool atLeastAsSpecificAs_STemplateArgument_list
    (SObjList<STemplateArgument> &list1,
     ObjList<STemplateArgument> &list2,
     Flags asaFlags);
};


ENUM_BITWISE_OPS(MatchTypes::Flags, MatchTypes::ASA_ALL_FLAGS);


#endif // MATCHTYPE_H
