// matchtype.h

// Attempt to match types or STemplateArguments to another using
// standard recursive unification, returning a boolean answering "did
// they match?"; more specifically the question is asymmetric: "is the
// left argument at least as specific as the right" and if they match
// "what are the (least restrictive) bindings to variables in the
// right argument necessary to make the match".  Note that there are
// three modes of matching which are detailed below in the comments to
// the elements of enum MatchMode and that one mode doesn't generate
// bindings.
//
// It is a non-trivial question whether a call into this code will
// halt.  To ensure that it does 1) all non-recursive loops are over a
// finite datastructure, so they will halt, 2) all call graph loops
// contain a call to match0(), and 3) match0() counts its recursion
// depth, halting if it exeeds a constant threshold.
//
// The property that all call graph loops go throgh match0() can be
// verified by checking that the only methods called recursively other
// than match0() together with the methods they call other than
// match0() are listed exhaustively below.  The names of the methods
// in this class are more verbose than usual to help ensure that this
// fact can be verified by inspection (by simply seraching for
// "match_"), so please don't change them.
//
//   match_TInfo()
//     calls match_Lists()
//   match_Lists()
//     calls match_STA()
//   match_STA()
//     class match_Type()
//   match_Type() and match_rightTypeVar() and match_Atomic()
//     call none

// FIX: I match too liberally: A pointer to a const pointer will match
// a pointer to a non-const pointer, even though they shouldn't since
// the const-ness differs and not at the top.

#ifndef MATCHTYPE_H
#define MATCHTYPE_H

#include "cc_type.h"            // Type hierarchy
#include "ptrmap.h"             // PtrMap
  

// sm: 8/05/04: changed to using StringRef indices instead of
// Variables, because the latter is difficult to use in the
// presence of member templates, as the template parameter lists
// get cloned and re-tchecked, creating new Variables
typedef PtrMap<char const /*StringRef*/, STemplateArgument> STemplateArgumentMap;

// version for a map that hold const ptrs to the values
typedef PtrMap<char const /*StringRef*/, STemplateArgument const> STemplateArgumentCMap;


class MatchBindings {
private:     // data
  // primary map from vars to their bindings; the STemplateArguments
  // are owned by this map
  STemplateArgumentMap map;

  // count the entries; they can't be deleted or overwritten
  int entryCount;

private:     // funcs
  void put0(Variable *key, STemplateArgument *val);
  STemplateArgument const *get0(Variable const *key);

public:      // funcs
  MatchBindings();
  ~MatchBindings();

  // if key is bindable, map it to val; otherwise fail
  void putObjVar(Variable *key, STemplateArgument *val);
  void putTypeVar(TypeVariable *key, STemplateArgument *val);

  // get the value of a key; FIX: hmm, a non-bound key looks the same
  // as a non-bindable key
  //
  // these return serf pointers which will become invalid once
  // this MatchBindings object goes away
  STemplateArgument const *getObjVar(Variable const *key);
  STemplateArgument const *getTypeVar(TypeVariable *key);
  STemplateArgument const *getVar(StringRef name);

  // are the number of bound (not just bindable) entries greater than
  // zero?
  bool isEmpty();

  // dump out the map
  void gdb();
};


void bindingsGdb(STemplateArgumentMap &bindings);


// this class holds the "global" data used during matching,
// particularly the set of bindings
class MatchTypes {
public:                         // types
//    // these flags effect the matching at the node granularity: they are
//    // not stored in the MatchTypes object but are passed down with each
//    // method call; this enum is intended as a collection of orthogonal flags
//    enum MFlags {
//      MT_NONE      = 0x00000000,
//      // this is the top-level call for the traversal of a given type
//      MT_TOP       = 0x00000001,
//      MT_ALL_FLAGS = 0x00000001
//    };

  // NOTE: now we use a matchDepth; It works like this.
  // depth            | pointers match arrays | const and volatile matter
  // -----------------+-----------------------+--------------------------
  // 0, top level     | yes                   | no
  // 1, below top ref | yes                   | yes
  // 2, below top     | no                    | yes

  // Scott has other funky flags that we have to ignore, so this mask
  // is handy
  static CVFlags const normalCvFlagMask;

  // this mode effects the behavior of the entire matching process;
  // there are two matching modes that govern how variables (not only,
  // but usually type variables) are treated
  enum MatchMode {
    MM_NONE,

    // Create Bindings; Vars on the right: if unbound, bind to the
    // left, and if bound are replaced with their binding and matching
    // continues.  Vars on the left: result in an assertion failure.
    MM_BIND,

    // same as MM_BIND except vars on left are regarded as naming a
    // unique type (with no relation to same-named vars on the right);
    // this is for 14.5.5.2
    MM_BIND_UNIQUE,

    // Wildcard Typevars; Vars on the right: are just wildcards,
    // matching anything, but making no record of that match.  Vars on
    // the left: cause the matching process to fail unless the match a
    // wildcard on the right.
    MM_WILD,

    // Isomorphic Types; Vars on the right: if unbound, bind to the
    // left if it is a typevar, otherwise fail; if bound, replace with
    // that typevar and check equal to the left, otherwise fail.  Vars
    // on the left: if not matched with a var on the right, fail.
    MM_ISO,
    
    NUM_MATCH_MODES
  };

private:                        // data
  TypeFactory &tfac;

  MatchMode const mode;

  // sm: Respect the same kinds of comparison modifiers as
  // Type::equals.
  //
  // TODO: Type::equals passes the eflags down manually, because only
  // some of the flags are propagated down to lower levels.  For now,
  // in this module, I just propagate them all, which is probably
  // wrong, but it's not clear how best to fold masking propagation
  // into the existing recursion structure.
  //
  // TODO: Not all the flags are respected yet.
  Type::EqFlags const eflags;

  // recursion depth of calls to match0(); used to detect infinite
  // loops
  int recursionDepth;

  // the maximum depth match0() may recurse to; FIX: this should be
  // const, but I need to be able to reset it from a tracing flag to
  // something smaller so that the tests for this don't take so long;
  // perhaps a configuration time flag should enable that at compile
  // time
  static int recursionDepthLimit;

public:
  // map from TypeVariable or a Variable key to STemplateArgument
  // value
  MatchBindings bindings;

private:                        // funcs
  // disallowed
  MatchTypes(MatchTypes&);

  // compute the set subtraction acv minus bcv as a set of flags;
  // return true if the answer is positive, that is acv is a superset
  // of bcv; false otherwise
  bool subtractFlags(CVFlags acv, CVFlags bcv, CVFlags &finalFlags);
  bool bindValToVar(Type *a, Type *b, int matchDepth);
  bool match_rightTypeVar(Type *a, Type *b, int matchDepth);

  bool match_cva   (CVAtomicType *a,        Type *b, int matchDepth);
  bool match_ptr   (PointerType *a,         Type *b, int matchDepth);
  bool match_ref   (ReferenceType *a,       Type *b, int matchDepth);
  bool match_func  (FunctionType *a,        Type *b, int matchDepth);
  bool match_array (ArrayType *a,           Type *b, int matchDepth);
  bool match_ptm   (PointerToMemberType *a, Type *b, int matchDepth);

  // check 1) if 'a' is a specialization/instantiation of the same
  // primary as 'b', and 2) the instantiation arguments of 'a' matches
  // that of 'b' pairwise
  bool match_TInfo(TemplateInfo *a, TemplateInfo *b, int matchDepth);

  // similar for PseudoInstantiations
  bool match_TInfo_with_PI(TemplateInfo *a, PseudoInstantiation *b,
                           int matchDepth);
  bool match_PI(PseudoInstantiation *a, PseudoInstantiation *b,
                int matchDepth);

  bool match_variables(Type *a, TypeVariable *b, int matchDepth);

  bool unifyIntToVar(int i0, Variable *v1);

  // internal method for checking if Type 'a' matches Type 'b'.
  bool match0(Type *a, Type *b, int matchDepth);

public:
  MatchTypes(TypeFactory &tfac0, MatchMode mode0, Type::EqFlags eflags0 = Type::EF_EXACT);
  ~MatchTypes();

  MatchMode getMode() const { return mode; }
  bool hasEFlag(Type::EqFlags f) const { return !!(eflags & f); }

  // top level entry for checking if Type 'a' matches Type 'b'.
  bool match_Type(Type *a, Type *b, int matchDepth = 0);
  bool match_Atomic(AtomicType *a, AtomicType *b, int matchDepth);

  bool match_STA(STemplateArgument *a, STemplateArgument const *b, int matchDepth = 0);

  // does listA match listB pairwise?
  bool match_Lists(ObjList<STemplateArgument> &listA,
                   ObjList<STemplateArgument> &listB,
                   int matchDepth = 0);

  // variants for when one of them is an SObjList ...
  bool match_Lists(SObjList<STemplateArgument> &listA,
                   ObjList<STemplateArgument> &listB,
                   int matchDepth = 0);
  bool match_Lists(ObjList<STemplateArgument> &listA,
                   SObjList<STemplateArgument> &listB,
                   int matchDepth = 0);
};


char const *toString(MatchTypes::MatchMode m);


#endif // MATCHTYPE_H
