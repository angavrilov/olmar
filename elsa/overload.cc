// overload.cc                       see license.txt for copyright and terms of use
// code for overload.h

// This module intends to implement the overload resolution procedure
// described in cppstd clause 13.  However, there is a large gap
// between the English description there and an implementation in
// code, so it's likely there are omissions and deviations.

#include "overload.h"      // this module
#include "cc_env.h"        // Env
#include "variable.h"      // Variable
#include "cc_type.h"       // Type, etc.
#include "trace.h"         // TRACE


// ------------------- Candidate -------------------------
Candidate::Candidate(Variable *v, int numArgs)
  : var(v),
    conversions(numArgs)
{}

Candidate::~Candidate()
{}


string Candidate::conversionDescriptions(char const *indent) const
{
  stringBuilder sb;

  for (int i=0; i < conversions.size(); i++) {
    sb << "\n%%% overload:   " << indent << toString(conversions[i]);
  }
  
  return sb;
}


// ------------------ resolveOverload --------------------
// prototypes
Candidate * /*owner*/ makeCandidate
  (Env &env, OverloadFlags flags, Variable *var, GrowArray<ArgumentInfo> &args);
Candidate *pickWinner(ObjArrayStack<Candidate> &candidates,
                      GrowArray<ArgumentInfo> &args,
                      int low, int high);
int compareCandidates(Candidate const *left, Candidate const *right,
                      GrowArray<ArgumentInfo> &args);
int compareConversions(ArgumentInfo const &src,
  ImplicitConversion const &left, Type const *leftDest,
  ImplicitConversion const &right, Type const *rightDest);
int compareStandardConversions
  (ArgumentInfo const &leftSrc, StandardConversion left, Type const *leftDest,
   ArgumentInfo const &rightSrc, StandardConversion right, Type const *rightDest);
bool convertsPtrToBool(Type const *src, Type const *dest);
bool isPointerToCompound(Type const *type, CompoundType const *&ct);
bool isReferenceToCompound(Type const *type, CompoundType const *&ct);
bool isPointerToCompoundMember(Type const *type, CompoundType const *&ct);
bool isBelow(CompoundType const *low, CompoundType const *high);
bool isProperSubpath(CompoundType const *LS, CompoundType const *LD,
                     CompoundType const *RS, CompoundType const *RD);




Variable *resolveOverload(
  Env &env,
  OverloadFlags flags,
  SObjList<Variable> &varList,
  GrowArray<ArgumentInfo> &args)
{                   
  // for debug printing
  IFDEBUG( char const *indent = (flags & OF_NO_USER)? "    " : "  "; )

  // generate a sequence of candidates ("viable functions"), in an
  // array
  ObjArrayStack<Candidate> candidates(varList.count());
  SFOREACH_OBJLIST_NC(Variable, varList, iter) {
    Candidate *c = makeCandidate(env, flags, iter.data(), args);
    if (c) {
      TRACE("overload", indent << "candidate: " << c->var->toString() <<
                        " at " << toString(c->var->loc) <<
                        c->conversionDescriptions(indent));
      candidates.push(c);
    }
  }

  if (candidates.isEmpty()) {
    if (!( flags & OF_NO_ERRORS )) {
      // TODO: expand this message greatly: explain which functions
      // are candidates, and why each is not viable
      env.error("no viable candidate for function call");
    }
    return NULL;
  }

  // use a tournament to select a candidate that is not worse
  // than any of those it faced
  Candidate const *winner =
    pickWinner(candidates, args, 0, candidates.length()-1);

  // now verify that the picked winner is in fact better than any
  // of the other candidates (since the order is not necessarily linear)
  for (int i=0; i < candidates.length(); i++) {
    if (candidates[i] == winner) {
      continue;    // skip it, no need to compare to itself
    }

    if (compareCandidates(winner, candidates[i], args) == -1) {
      // ok, it's better
    }
    else {
      // not better, so there is no function that is better than
      // all others
      if (!( flags & OF_NO_ERRORS )) {
        // TODO: expand this message
        env.error("ambiguous overload, no function is better than all others");
      }
      return NULL;
    }
  }

  // 'winner' is indeed the winner
  return winner->var;
}


// for each parameter, determine an ICS, and return the resulting
// Candidate; return NULL if the function isn't viable; this
// implements cppstd 13.3.2
Candidate * /*owner*/ makeCandidate
  (Env &env, OverloadFlags flags, Variable *var, GrowArray<ArgumentInfo> &args)
{
  Owner<Candidate> c(new Candidate(var, args.size()));

  FunctionType *ft = var->type->asFunctionType();

  // simultaneously iterate over parameters and arguments
  SObjListIter<Variable> paramIter(ft->params);
  int argIndex = 0;
  while (!paramIter.isDone() && argIndex < args.size()) {
    if (flags & OF_NO_USER) {  
      // only consider standard conversions
      StandardConversion scs =
        getStandardConversion(NULL /*env*/, args[argIndex].special, args[argIndex].type,
                              paramIter.data()->type);
      if (scs != SC_ERROR) {
        ImplicitConversion ics;
        ics.addStdConv(scs);
        c->conversions[argIndex] = ics;
      }
      else {
        return NULL;
      }
    }
    else {
      // consider both standard and user-defined
      ImplicitConversion ics =
        getImplicitConversion(env, args[argIndex].special, args[argIndex].type,
                              paramIter.data()->type);
      if (ics) {
        c->conversions[argIndex] = ics;
      }
      else {
        return NULL;           // no conversion sequence possible
      }
    }
    
    paramIter.adv();
    argIndex++;
  }

  // extra arguments?
  if (argIndex < args.size()) {
    if (ft->acceptsVarargs()) {
      // fill remaining with IC_ELLIPSIS
      ImplicitConversion ellipsis;
      ellipsis.addEllipsisConv();
      while (argIndex < args.size()) {
        c->conversions[argIndex] = ellipsis;
        argIndex++;
      }
    }
    else {
      // too few arguments, cannot form a conversion
      return NULL;
    }
  }
  
  // extra parameters?
  if (!paramIter.isDone()) {
    if (paramIter.data()->value) {
      // the next parameter has a default value, which implies all
      // subsequent parameters have default values as well; for
      // purposes of overload resolution, we simply ignore the extra
      // parameters [cppstd 13.3.2 para 2, third bullet]
    }
    else {
      // no default value, argument must be supplied but is not,
      // so cannot form a conversion
      return NULL;
    }
  }

  return c.xfr();
}


// this is a simple tournament, as suggested in footnote 123,
// cppstd 13.3.3 para 2
Candidate *pickWinner(ObjArrayStack<Candidate> &candidates,
                      GrowArray<ArgumentInfo> &args,
                      int low, int high)
{
  if (low == high) {
    // only one candidate
    return candidates[low];
  }

  // divide the pool in half and select a winner from each half
  int mid = (low+high+1)/2;
    // 1,3 -> 2
    // 1,2 -> 2
    // 2,3 -> 3
  Candidate *left = pickWinner(candidates, args, low, mid-1);
  Candidate *right = pickWinner(candidates, args, mid, high);

  // compare the candidates to get one that is not worse than the other
  int choice = compareCandidates(left, right, args);
  if (choice <= 0) {
    return left;    // left is better, or neither is better or worse
  }
  else {
    return right;   // right is better
  }
}


// compare overload candidates, returning:
//   -1 if left is better
//    0 if they are indistinguishable
//   +1 if right is better
// this is cppstd 13.3.3 para 1, second group of bullets
int compareCandidates(Candidate const *left, Candidate const *right,
                      GrowArray<ArgumentInfo> &args)
{
  // decision so far
  int ret = 0;

  // iterate over parameters too, since we need to know the
  // destination type in some cases
  FunctionType const *leftFunc = left->var->type->asFunctionTypeC();
  FunctionType const *rightFunc = right->var->type->asFunctionTypeC();
  SObjListIter<Variable> leftParam(leftFunc->params);
  SObjListIter<Variable> rightParam(rightFunc->params);

  // walk through list of arguments, comparing the conversions
  for (int i=0; i < args.size(); i++) {
    // get parameter types; they can be NULL if we walk off into the ellipsis
    // of a variable-argument function
    Type const *leftDest = leftParam.isDone()? NULL : leftParam.data()->type;
    Type const *rightDest = rightParam.isDone()? NULL : rightParam.data()->type;

    int choice = compareConversions(args[i], left->conversions[i], leftDest,
                                             right->conversions[i], rightDest);
    if (ret == 0) {
      // no decision so far, fold in this comparison
      ret = choice;
    }
    else if (choice == 0) {
      // this comparison offers no information
    }
    else if (choice != ret) {
      // this comparison disagrees with a previous comparison, which
      // makes two candidates indistinguishable
      return 0;
    }
  }

  if (ret != 0) {
    return ret;     // at least one of the comparisons is decisive
  }

  // the next comparison says that non-templates are better than
  // template function *specializations*.. I'm not entirely sure
  // what "specialization" means, whether it's something declared using
  // the specialization syntax or just an instance of a template..
  // I'm going to use the latter interpretation since I think it
  // makes more sense
  if (!leftFunc->templateParams && rightFunc->templateParams) {
    return -1;     // left is non-template
  }
  else if (leftFunc->templateParams && !rightFunc->templateParams) {
    return +1;     // right is non-template
  }
  
  // TODO: next rule talks about comparing templates to find out
  // which is more specialized
  
  // TODO: last rule talks about selecting from among several possible
  // user-defined conversion functions, but I haven't implemented that
  // part of conversion selection yet so I don't know how that context
  // will be identified
  
  // no more rules remain, candidates are indistinguishable
  return 0;
}


// compare two conversion sequences, returning the same choice code
// as above; we need to know the source type and the destination types,
// because some of the comparison criteria use them; this implements
// cppstd 13.3.3.2
int compareConversions(ArgumentInfo const &src,
  ImplicitConversion const &left, Type const *leftDest,
  ImplicitConversion const &right, Type const *rightDest)
{
  // para 2: choose based on what kind of conversion:
  //   standard < user-defined/ambiguous < ellipsis
  {
    static int const map[ImplicitConversion::NUM_KINDS] = {
      0,    // none
      1,    // standard
      2,    // user-defined
      3,    // ellipsis
      2     // ambiguous
    };

    int leftGroup = map[left.kind];
    int rightGroup = map[right.kind];
    xassert(leftGroup && rightGroup);   // make sure neither is IC_NONE

    if (leftGroup < rightGroup) return -1;
    if (rightGroup < leftGroup) return +1;
  }

  if (left.kind == ImplicitConversion::IC_AMBIGUOUS || 
      right.kind == ImplicitConversion::IC_AMBIGUOUS) {
    return 0;    // indistinguishable
  }

  // para 3: compare among same-kind conversions
  xassert(left.kind == right.kind);

  // para 3, bullet 1
  if (left.kind == ImplicitConversion::IC_STANDARD) {
    return compareStandardConversions(src, left.scs, leftDest,
                                      src, right.scs, rightDest);
  }

  // para 3, bullet 2
  if (left.kind == ImplicitConversion::IC_USER_DEFINED) {
    if (left.user != right.user) {
      // different conversion functions, incomparable
      return 0;
    }

    // compare their second conversion sequences
    ArgumentInfo src(SE_NONE, left.user->type->asFunctionTypeC()->retType);
    return compareStandardConversions(src, left.scs2, leftDest,
                                      src, right.scs2, rightDest);
  }
  
  // if ellipsis, no comparison
  xassert(left.kind == ImplicitConversion::IC_ELLIPSIS);
  return 0;
}


inline void swap(CompoundType const *&t1, CompoundType const *&t2)
{
  CompoundType const *temp = t1;
  t1 = t2;
  t2 = temp;
}


// this is a helper for 'compareStandardConversions'; it incorporates
// the knowledge of having found two more cv flag pairs in a
// simultaneous deconstruction of two types that are being compared;
// ultimately it wants to set 'ret' to -1 if all the 'lcv's are
// subsets of all the 'rcv's, and +1 if the subset goes the other way;
// it returns 'true' if the subset relation does not hold
static bool foldNextCVs(int &ret, CVFlags lcv, CVFlags rcv)
{
  if (lcv != rcv) {
    if ((lcv & rcv) == lcv) {    // left is subset, => better
      if (ret > 0) return true;  // but right was better previously, => no decision
      ret = -1;
    }
    if ((lcv & rcv) == rcv) {    // right is subset, => better
      if (ret < 0) return true;  // but left was better previously, => no decision
      ret = +1;
    }
  }
  return false;                  // no problem found
}


int compareStandardConversions
  (ArgumentInfo const &leftSrc, StandardConversion left, Type const *leftDest,
   ArgumentInfo const &rightSrc, StandardConversion right, Type const *rightDest)
{
  // if one standard conversion sequence is a proper subsequence of
  // another, excluding SC_LVAL_TO_RVAL, then the smaller one is
  // preferred
  {
    StandardConversion L = removeLval(left);
    StandardConversion R = removeLval(right);
    if (L != R) {
      if (isSubsequenceOf(L, R)) return -1;
      if (isSubsequenceOf(R, L)) return +1;
    }
  }

  // compare ranks of conversions
  SCRank leftRank = getRank(left);
  SCRank rightRank = getRank(right);
  if (leftRank < rightRank) return -1;
  if (rightRank < leftRank) return +1;

  // 13.3.3.2 para 4, bullet 1:
  //   "A conversion that is not a conversion of a pointer, or pointer
  //    to member, to bool is better than another conversion that is
  //    such a conversion."
  {
    bool L = convertsPtrToBool(leftSrc.type, leftDest);
    bool R = convertsPtrToBool(rightSrc.type, rightDest);
    if (!L && R) return -1;
    if (!R && L) return +1;
  }

  // hierarchy
  // -------------
  //   void           treated as a semantic super-root for this analysis
  //     \            .
  //      A           syntactic root, i.e. least-derived class of {A,B,C}
  //       \          .
  //        B         .
  //         \        .
  //          C       most-derived class of {A,B,C}
  //
  // (the analysis does allow for classes to appear in between these three)

  // 13.3.3.2 para 4, bullet 2:
  //   B* -> A*      is better than  B* -> void*
  //   A* -> void*   is better than  B* -> void*
  // 13.3.3.2 para 4, bullet 3:
  //   C* -> B*      is better than  C* -> A*
  //   C& -> B&      is better than  C& -> A&
  //   B::* <- A::*  is better than  C::* <- A::*
  //   C -> B        is better than  C -> A
  //   B* -> A*      is better than  C* -> A*
  //   B& -> A&      is better than  C& -> A&
  //   C::* <- B::*  is better than  C::* <- A::*
  //   B -> A        is better than  C -> A
  //
  // Q: what about cv flags?  for now I ignore them...
  {
    // first pass:  pointers, and references to pointers
    // second pass: objects, and references to objects
    // third pass:  pointers to members
    for (int pass=1; pass <= 3; pass++) {
      bool (*checkFunc)(Type const *type, CompoundType const *&ct) =
        pass==1 ? &isPointerToCompound   :
        pass==2 ? &isReferenceToCompound :
                  &isPointerToCompoundMember;

      // We're comparing conversions among pointers and references.
      // Name the participating compound types according to this scheme:
      //   left:  LS -> LD      (left source, left dest)
      //   right: RS -> RD
      CompoundType const *LS;
      CompoundType const *LD;
      CompoundType const *RS;
      CompoundType const *RD;

      // are the conversions of the right form?
      if (checkFunc(leftSrc.type, LS) &&
          checkFunc(leftDest, LD) &&
          checkFunc(rightSrc.type, RS) &&
          checkFunc(rightDest, RD)) {
        // in pass 3, the paths go the opposite way, so just swap their ends
        if (pass==3) {
          swap(LS, LD);
          swap(RS, RD);
        }

        // each of the "better than" checks above is really saying
        // that if the paths between source and destination are
        // in the "proper subpath" relation, then the shorter one is better
        if (isProperSubpath(LS, LD, RS, RD)) return -1;
        if (isProperSubpath(RS, RD, LS, LD)) return +1;
      }
    }
  }

  // 13.3.3.2 para 3, bullet 1, sub-bullets 3 and 4:
  // if the conversions yield types that differ only in cv-qualification,
  // then prefer the one with fewer such qualifiers (if one has fewer)
  {
    // preference so far
    int ret = 0;

    // will work through the type constructors simultaneously
    Type const *L = leftDest;
    Type const *R = rightDest;

    // if one is a reference and the other is not, I don't see a basis
    // for comparison in cppstd, so I skip the extra reference
    //
    // update: 13.3.3.2b.cc suggests that in fact cppstd intends
    // 'int' and 'int const &' to be indistinguishable, so I *don't*
    // strip extra references
    #if 0   // I think this was wrong   
    if (L->isReference() && !R->isReference()) {
      L = L->asRvalC();
    }
    else if (!L->isReference() && R->isReference()) {
      R = R->asRvalC();
    }
    #endif // 0

    // deconstruction loop
    while (!L->isCVAtomicType() && !R->isCVAtomicType()) {
      if (L->getTag() != R->getTag()) {
        return 0;            // different types, can't compare
      }

      switch (L->getTag()) {
        default: xfailure("bad tag");

        case Type::T_POINTER: {
          PointerType const *lpt = L->asPointerTypeC();
          PointerType const *rpt = R->asPointerTypeC();

          // assured by non-stackability of references
          xassert(lpt->op == rpt->op);

          if (foldNextCVs(ret, lpt->cv, rpt->cv)) {
            return 0;        // not subset, therefore no decision
          }

          L = lpt->atType;
          R = rpt->atType;
          break;
        }

        case Type::T_FUNCTION:
        case Type::T_ARRAY:
          if (L->equals(R)) {
            return ret;      // decision so far is final
          }
          else {
            return 0;        // different types, can't compare
          }

        case Type::T_POINTERTOMEMBER: {
          PointerToMemberType const *lptm = L->asPointerToMemberTypeC();
          PointerToMemberType const *rptm = R->asPointerToMemberTypeC();

          if (foldNextCVs(ret, lptm->cv, rptm->cv)) {
            return 0;        // not subset, therefore no decision
          }

          if (lptm->inClass != rptm->inClass) {
            return 0;        // different types, can't compare
          }

          L = lptm->atType;
          R = rptm->atType;
          break;
        }
      }
    }

    if (!L->isCVAtomicType() || !R->isCVAtomicType()) {
      return 0;              // different types, can't compare
    }

    // finally, inspect the leaves
    CVAtomicType const *lat = L->asCVAtomicTypeC();
    CVAtomicType const *rat = R->asCVAtomicTypeC();

    if (foldNextCVs(ret, lat->cv, rat->cv)) {
      return 0;              // not subset, therefore no decision
    }

    if (!lat->atomic->equals(rat->atomic)) {
      return 0;              // different types, can't compare
    }

    // 'ret' is our final decision
    return ret;
  }
}


bool convertsPtrToBool(Type const *src, Type const *dest)
{
  // I believe this test is meant to transcend any reference bindings
  src = src->asRvalC();
  dest = dest->asRvalC();

  return (src->isPointerType() || src->isPointerToMemberType()) &&
         dest->isBool();
}


// also allows void*, where 'void' is represented with
// a NULL CompoundType
bool isPointerToCompound(Type const *type, CompoundType const *&ct)
{
  type = type->asRvalC();

  if (type->isPointerType()) {
    type = type->asPointerTypeC()->atType;
    if (type->isCompoundType()) {
      ct = type->asCompoundTypeC();
      return true;
    }
    if (type->isVoid()) {
      ct = NULL;
      return true;
    }
  }

  return false;
}


// allows both C& and C, returning C in 'ct'
bool isReferenceToCompound(Type const *type, CompoundType const *&ct)
{ 
  type = type->asRvalC();

  if (type->isCompoundType()) {
    ct = type->asCompoundTypeC();
    return true;
  }

  return false;
}


bool isPointerToCompoundMember(Type const *type, CompoundType const *&ct)
{
  type = type->asRvalC();

  if (type->isPointerToMemberType()) {
    ct = type->asPointerToMemberTypeC()->inClass;
    return true;
  }

  return false;
}


// is 'low' below 'high' in the inheritance hierarchy?
bool isBelow(CompoundType const *low, CompoundType const *high)
{
  return high==NULL ||
         (low!=NULL && low->hasBaseClass(high));
}


// Is the path through the inheritance hierarchy from LS to LD a
// proper sub-path (paths go up) of that from RS to RD?  In fact we
// also require that one of the two endpoints coincide, since that's
// the form of the rules given in 13.3.3.2 para 4.  Note that NULL is
// the representation of 'void', treated as a superclass of
// everything.
bool isProperSubpath(CompoundType const *LS, CompoundType const *LD,
                     CompoundType const *RS, CompoundType const *RD)
{
  if (LS == RS && LD == RD) return false;         // same path

  if (!isBelow(LS, LD)) return false;             // LS -> LD not a path
  if (!isBelow(RS, RD)) return false;             // RS -> RD not a path

  if (LS == RS && isBelow(LD, RD)) {
    // L and R start at the same place, but left ends lower
    return true;
  }

  if (LD == RD && isBelow(RS, LS)) {
    // L and R end at the same place, but right starts lower
    return true;
  }

  return false;
}
