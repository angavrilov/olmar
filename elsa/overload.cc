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


bool Candidate::hasAmbigConv() const
{
  for (int i=0; i < conversions.size(); i++) {
    if (conversions[i].isAmbiguous()) {
      return true;
    }
  }
  return false;
}


void Candidate::conversionDescriptions() const
{
  for (int i=0; i < conversions.size(); i++) {
    OVERLOADTRACE(i << ": " << toString(conversions[i]));
  }
}


// ------------------ resolveOverload --------------------
// prototypes
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



// this function can be used anyplace that there's only one list
// of original candidate functions
Variable *resolveOverload(
  Env &env,
  SourceLoc loc,
  ErrorList * /*nullable*/ errors,
  OverloadFlags flags,
  SObjList<Variable> &varList,
  GrowArray<ArgumentInfo> &args,
  bool &wasAmbig)
{
  OverloadResolver r(env, loc, errors, flags, args, varList.count());
  r.processCandidates(varList);
  return r.resolve(wasAmbig);
}


void OverloadResolver::printArgInfo()
{
  IFDEBUG(
    if (tracingSys("overload")) {
      overloadTrace() << "arguments:\n";
      for (int i=0; i < args.size(); i++) {
        overloadTrace() << "  " << i << ": "
             << toString(args[i].special) << ", "
             << args[i].type->toString() << "\n";
      }
    }
  )
}


OverloadResolver::~OverloadResolver()
{
  //overloadNesting--;
}


void OverloadResolver::processCandidates(SObjList<Variable> &varList)
{
  SFOREACH_OBJLIST_NC(Variable, varList, iter) {
    processCandidate(iter.data());
  }
}

void OverloadResolver::processCandidate(Variable *v)
{
  OVERLOADINDTRACE("candidate: " << v->toString() <<
                   " at " << toString(v->loc));

  if ((flags & OF_NO_EXPLICIT) && v->hasFlag(DF_EXPLICIT)) {
    // not a candidate, we're ignoring explicit constructors
    OVERLOADTRACE("(not viable due to 'explicit')");
  }
  else {
    Candidate *c = makeCandidate(v);
    if (c) {
      IFDEBUG( c->conversionDescriptions(); )
      candidates.push(c);
    }
    else {
      OVERLOADTRACE("(not viable)");
    }
  }
}


void OverloadResolver::processPossiblyOverloadedVar(Variable *v)
{
  if (v->overload) {
    processCandidates(v->overload->set);
  }
  else {
    processCandidate(v);
  }
}


void OverloadResolver::addAmbiguousBinaryCandidate(Variable *v)
{
  Candidate *c = new Candidate(v, 2);
  c->conversions[0].addAmbig();
  c->conversions[1].addAmbig();

  OVERLOADINDTRACE("candidate: ambiguous-arguments placeholder");
  IFDEBUG( c->conversionDescriptions(); )
  
  candidates.push(c);
}


void OverloadResolver::
  addUserOperatorCandidates(Type *lhsType, StringRef opName)
{
  lhsType = lhsType->asRval();

  // member candidates
  if (lhsType->isCompoundType()) {
    Variable *member = lhsType->asCompoundType()->lookupVariable(opName, env);
    if (member) {
      processPossiblyOverloadedVar(member);
    }
  }

  // non-member candidates; this lookup ignores member functions
  Variable *nonmember = env.lookupVariable(opName, LF_SKIP_CLASSES);
  if (nonmember) {
    processPossiblyOverloadedVar(nonmember);
  }
}


void OverloadResolver::addBuiltinUnaryCandidates(OverloadableOp op)
{
  ArrayStack<Variable*> &builtins = env.builtinUnaryOperator[op];
  for (int i=0; i < builtins.length(); i++) {
    processCandidate(builtins[i]);
  }
}


void OverloadResolver::addBuiltinBinaryCandidates(OverloadableOp op,
  Type *lhsType, Type *rhsType)
{
  ObjArrayStack<CandidateSet> &builtins = env.builtinBinaryOperator[op];
  for (int i=0; i < builtins.length(); i++) {
    builtins[i]->instantiateBinary(env, *this, op, lhsType, rhsType);
  }
}


// this is a simple tournament, as suggested in footnote 123,
// cppstd 13.3.3 para 2
template <class RESOLVER, class CANDIDATE>
CANDIDATE *tournament(RESOLVER &resolver, int low, int high, CANDIDATE *dummy)
{
  if (low == high) {
    // only one candidate
    return resolver.candidates[low];
  }

  // divide the pool in half and select a winner from each half
  int mid = (low+high+1)/2;
    // 1,3 -> 2
    // 1,2 -> 2
    // 2,3 -> 3
  CANDIDATE *left = tournament(resolver, low, mid-1, dummy);
  CANDIDATE *right = tournament(resolver, mid, high, dummy);

  // compare the candidates to get one that is not worse than the other
  int choice = resolver.compareCandidates(left, right);
  if (choice <= 0) {
    return left;    // left is better, or neither is better or worse
  }
  else {
    return right;   // right is better
  }
}


// tournament, plus final linear scan to ensure it's the best; the
// dummy argument is just to instantiate the 'CANDIDATE' type
template <class RESOLVER, class CANDIDATE>
CANDIDATE *selectBestCandidate(RESOLVER &resolver, CANDIDATE *dummy)
{
  // use a tournament to select a candidate that is not worse
  // than any of those it faced
  CANDIDATE *winner = tournament(resolver, 0, resolver.candidates.length()-1, dummy);

  // now verify that the picked winner is in fact better than any
  // of the other candidates (since the order is not necessarily linear)
  for (int i=0; i < resolver.candidates.length(); i++) {
    if (resolver.candidates[i] == winner) {
      continue;    // skip it, no need to compare to itself
    }

    if (resolver.compareCandidates(winner, resolver.candidates[i]) == -1) {
      // ok, it's better
    }
    else {
      // not better, so there is no function that is better than
      // all others
      return NULL;
    }
  }

  // 'winner' is indeed the winner
  return winner;
}


Variable *OverloadResolver::resolve(bool &wasAmbig)
{
  wasAmbig = false;

  if (candidates.isEmpty()) {
    if (emptyCandidatesIsOk) {
      return NULL;      // caller is prepared to deal with this
    }

    if (errors) {
      // TODO: expand this message greatly: explain which functions
      // are candidates, and why each is not viable
      errors->addError(new ErrorMsg(
        loc, "no viable candidate for function call", EF_NONE));
    }
    OVERLOADTRACE("no viable candidates");
    return NULL;
  }

  if (finalDestType) {
    // include this in the diagnostic output so that I can tell
    // when it will play a role in candidate comparison
    OVERLOADTRACE("finalDestType: " << finalDestType->toString());
  }

  // use a tournament to select a candidate that is not worse
  // than any of those it faced
  Candidate const *winner = selectBestCandidate(*this, (Candidate const*)NULL);
  if (!winner) {
    // TODO: expand this message
    if (errors) {
      errors->addError(new ErrorMsg(
        loc, "ambiguous overload, no function is better than all others", EF_NONE));
    }
    OVERLOADTRACE("ambiguous overload");
    wasAmbig = true;
    return NULL;
  }

  OVERLOADTRACE(toString(loc)
    << ": selected " << winner->var->toString()
    << " at " << toString(winner->var->loc));

  if (winner->hasAmbigConv()) {
    // At least one of the conversions required for the winning candidate
    // is ambiguous.  This might actually mean, had we run the algorithm
    // as specified in the spec, that there's an ambiguity among the
    // candidates, since I fold some of that into the selection of the
    // conversion, for polymorphic built-in operator candidates.  Therefore,
    // this situation should appear to the caller the same as when we
    // definitely do have ambiguity among the candidates.
    if (errors) {
      errors->addError(new ErrorMsg(
        loc, "ambiguous overload or ambiguous conversion", EF_NONE));
    }
    OVERLOADTRACE("ambiguous overload or ambiguous conversion");
    wasAmbig = true;
    return NULL;
  }

  return winner->var;
}


Variable *OverloadResolver::resolve()
{
  bool dummy;
  return resolve(dummy);
}


// for each parameter, determine an ICS, and return the resulting
// Candidate; return NULL if the function isn't viable; this
// implements cppstd 13.3.2
Candidate * /*owner*/ OverloadResolver::makeCandidate(Variable *var)
{
  Owner<Candidate> c(new Candidate(var, args.size()));

//    cout << "args.size() " << args.size() << endl;

  FunctionType *ft = var->type->asFunctionType();
//    cout << "ft->params.count() " << ft->params.count() << endl;

  // simultaneously iterate over parameters and arguments
  SObjListIter<Variable> paramIter(ft->params);
  int argIndex = 0;
  while (!paramIter.isDone() && argIndex < args.size()) {
    if (flags & OF_NO_USER) {  
      // only consider standard conversions
      StandardConversion scs =
        getStandardConversion(NULL /*errorMsg*/, 
                              args[argIndex].special, args[argIndex].type,
                              paramIter.data()->type,
                              argIndex==0 && ft->isMethod() /*destIsReceiver*/);
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


// compare overload candidates, returning:
//   -1 if left is better
//    0 if they are indistinguishable
//   +1 if right is better
// this is cppstd 13.3.3 para 1, second group of bullets
int OverloadResolver::compareCandidates(Candidate const *left, Candidate const *right)
{
  // decision so far
  int ret = 0;

  // is exactly one candidate a built-in operator?
  if ((int)left->var->hasFlag(DF_BUILTIN) +
      (int)right->var->hasFlag(DF_BUILTIN) == 1) {
    // 13.6 para 1 explains that if a user-written candidate and a
    // built-in candidate have the same signature, then the built-in
    // candidate is hidden; I implement this by saying that the
    // user-written candidate always wins
    if (left->var->type->equals(right->var->type, 
          Type::EF_IGNORE_RETURN | Type::EF_IGNORE_PARAM_CV)) {
      // same signatures; who wins?
      if (left->var->hasFlag(DF_BUILTIN)) {
        return +1;     // right is user-written, it wins
      }
      else {
        return -1;     // left is user-written, it wins
      }
    }
  }

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

  // if we're doing "initialization by user-defined conversion", then
  // look at the conversion sequences to the final destination type
  if (finalDestType) {
    StandardConversion leftSC = getStandardConversion(
      NULL /*errorMsg*/, SE_NONE, leftFunc->retType, finalDestType);
    StandardConversion rightSC = getStandardConversion(
      NULL /*errorMsg*/, SE_NONE, rightFunc->retType, finalDestType);

    ret = compareStandardConversions(
      ArgumentInfo(SE_NONE, leftFunc->retType), leftSC, finalDestType,
      ArgumentInfo(SE_NONE, rightFunc->retType), rightSC, finalDestType
    );
    if (ret != 0) {
      return ret;
    }
  }

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


// --------------------- ConversionResolver -----------------------
#if 0   // 8/13/03: delete me
void getConversionOperators(SObjList<Variable> &dest, Env &env,
                            CompoundType *ct)
{
  // for now, consider only conversion operators defined in the class
  // itself; later, I'll implement a scheme to collect the list of
  // conversions inherited, plus those in the class itself

  Variable *v = ct->lookupVariable(env.conversionOperatorName, env);
  if (v) {
    if (v->overload) {
      dest.appendAll(v->overload->set);
    }
    else {
      dest.prepend(v);
    }
  }
}
#endif // 0


ImplicitConversion getConversionOperator(
  Env &env,
  SourceLoc loc,
  ErrorList * /*nullable*/ errors,
  Type *srcClassType,
  Type *destType
) {
  CompoundType *srcClass = srcClassType->asRval()->asCompoundType();

  // in all cases, there is effectively one argument, the receiver
  // object of type 'srcClass'
  GrowArray<ArgumentInfo> args(1);
  args[0] = ArgumentInfo(SE_NONE, srcClassType);

  OVERLOADINDTRACE("converting " << srcClassType->toString() <<
                   " to " << destType->toString());

  // set up the resolver; since the only argument is the receiver
  // object, user-defined conversions should never enter the picture,
  // but I'll supply OF_NO_USER just to be sure
  OverloadResolver resolver(env, loc, errors, OF_NO_USER, args);

  // get the conversion operators for the source class
  SObjList<Variable> &ops = srcClass->conversionOperators;

  // 13.3.1.4?
  if (destType->isCompoundType()) {
    CompoundType *destCT = destType->asCompoundType();

    // Where T is the destination class, "... [conversion operators
    // that] yield a type whose cv-unqualified type is the same as T
    // or is a derived class thereof are candidate functions.
    // Conversion functions that return 'reference to T' return
    // lvalues of type T and are therefore considered to yield T for
    // this process of selecting candidate functions."
    SFOREACH_OBJLIST_NC(Variable, ops, iter) {
      Variable *v = iter.data();
      Type *retType = v->type->asFunctionTypeC()->retType->asRval();
      if (retType->isCompoundType() &&
          retType->asCompoundType()->hasBaseClass(destCT)) {
        // it's a candidate
        resolver.processCandidate(v);
      }
    }
  }

  // 13.3.1.5?
  else if (!destType->isReference()) {
    // candidates added in this case are subject to an additional
    // ranking criteria, namely that the ultimate destination type
    // is significant (13.3.3 para 1 final bullet)
    resolver.finalDestType = destType;

    // Where T is the cv-unqualified destination type,
    // "... [conversion operators that] yield type T or a type that
    // can be converted to type T via a standard conversion sequence
    // (13.3.3.1.1) are candidate functions.  Conversion functions
    // that return a cv-qualified type are considered to yield the
    // cv-unqualified version of that type for this process of
    // selecting candidate functions.  Conversion functions that
    // return 'reference to T' return lvalues of type T and are
    // therefore considered to yield T for this process of selecting
    // candidate functions."
    SFOREACH_OBJLIST_NC(Variable, ops, iter) {
      Variable *v = iter.data();
      Type *retType = v->type->asFunctionType()->retType->asRval();
      if (SC_ERROR!=getStandardConversion(NULL /*errorMsg*/,
            SE_NONE, retType, destType)) {
        // it's a candidate
        resolver.processCandidate(v);
      }
    }
  }

  // must be 13.3.1.6
  else {
    // strip the reference
    Type *underDestType = destType->asRval();

    // Where the destination type is 'cv1 T &', "... [conversion
    // operators that] yield type 'cv2 T2 &', where 'cv1 T' is
    // reference-compatible (8.5.3) with 'cv2 T2', are
    // candidate functions."
    SFOREACH_OBJLIST_NC(Variable, ops, iter) {
      Variable *v = iter.data();
      Type *retType = v->type->asFunctionType()->retType;
      if (retType->isReference()) {
        retType = retType->asRval();     // strip the reference
        if (isReferenceCompatibleWith(underDestType, retType)) {
          // it's a candidate
          resolver.processCandidate(v);
        }
      }
    }
  }

  // pick the winner
  bool wasAmbig;
  Variable *winner = resolver.resolve(wasAmbig);
  
  // return an IC with that winner
  ImplicitConversion ic;
  if (!winner) {
    if (!wasAmbig) {
      return ic;        // is IC_NONE
    }
    else {
      ic.addAmbig();
      return ic;
    }
  }

  // compute the standard conversion that obtains the destination
  // type, starting from what the conversion function yields
  StandardConversion sc = getStandardConversion(
    NULL /*errorMsg*/,
    SE_NONE, winner->type->asFunctionType()->retType,   // conversion source
    destType                                            // conversion dest
  );

  ic.addUserConv(SC_IDENTITY, winner, sc);
  return ic;
}

          

// ------------------ LUB --------------------
static CVFlags unionCV(CVFlags cv1, CVFlags cv2, bool &cvdiffers, bool toplevel)
{
  CVFlags cvu = cv1 | cv2;

  // if underlying cv flags have already differed, then at this
  // level we must have a 'const'
  if (cvdiffers) {
    if (toplevel) {
      // once at toplevel, differences are irrelevant
    }
    else {
      cvu |= CV_CONST;
    }
  }

  // did this level witness a cv difference?
  else if (cvu != cv1 || cvu != cv2) {
    cvdiffers = true;
  }

  return cvu;
}

// must have already established similarity
Type *similarLUB(Env &env, Type *t1, Type *t2, bool &cvdiffers, bool toplevel=false)
{
  // this analysis goes bottom-up, because if there are cv differences
  // at a given level, then all levels above it must have CV_CONST in
  // the LUB (otherwise t1 and t2 wouldn't be able to convert to it)

  switch (t1->getTag()) {
    default: xfailure("bad type code");

    case Type::T_ATOMIC: {
      CVAtomicType *at1 = t1->asCVAtomicType();
      CVAtomicType *at2 = t2->asCVAtomicType();
      CVFlags cvu = unionCV(at1->cv, at2->cv, cvdiffers, toplevel);
      return env.tfac.makeCVAtomicType(SL_UNKNOWN, at1->atomic, cvu);
    }

    case Type::T_POINTER: {
      PointerType *pt1 = t1->asPointerType();
      PointerType *pt2 = t2->asPointerType();
      Type *under = similarLUB(env, pt1->atType, pt2->atType, cvdiffers);
      CVFlags cvu = unionCV(pt1->cv, pt2->cv, cvdiffers, toplevel);
      return env.tfac.makePointerType(SL_UNKNOWN, pt1->op, cvu, under);
    }

    case Type::T_FUNCTION:
    case Type::T_ARRAY:
      // similarity implies equality, so LUB is t1==t2
      return t1;

    case Type::T_POINTERTOMEMBER: {
      PointerToMemberType *pmt1 = t1->asPointerToMemberType();
      PointerToMemberType *pmt2 = t2->asPointerToMemberType();
      Type *under = similarLUB(env, pmt1->atType, pmt2->atType, cvdiffers);
      CVFlags cvu = unionCV(pmt1->cv, pmt2->cv, cvdiffers, toplevel);
      return env.tfac.makePointerToMemberType(SL_UNKNOWN, pmt1->inClass, cvu, under);
    }
  }
}

CompoundType *ifPtrToCompound(Type *t)
{
  if (t->isPointer()) {
    PointerType *pt = t->asPointerType();
    if (pt->atType->isCompoundType()) {
      return pt->atType->asCompoundType();
    }
  }
  return NULL;
}

Type *computeLUB(Env &env, Type *t1, Type *t2, bool &wasAmbig)
{
  wasAmbig = false;

  // check for pointers-to-class first
  {
    CompoundType *ct1 = ifPtrToCompound(t1);
    CompoundType *ct2 = ifPtrToCompound(t2);
    CompoundType *lubCt = NULL;
    if (ct1 && ct2) {
      // get CV under the pointers
      CVFlags cv1 = t1->asPointerType()->atType->getCVFlags();
      CVFlags cv2 = t2->asPointerType()->atType->getCVFlags();

      // union them (LUB in cv lattice)
      CVFlags cvu = cv1 | cv2;

      // find the LUB class, if any
      lubCt = CompoundType::lub(ct1, ct2, wasAmbig);
      Type *lubCtType;
      if (!lubCt) {
        if (wasAmbig) {
          // no unique LUB
          return NULL;
        }
        else {
          // no class is the LUB, so use 'void'
          lubCtType = env.tfac.getSimpleType(SL_UNKNOWN, ST_VOID, cvu);
        }
      }
      else {
        // Now I want to make the type 'pointer to <cvu> <lubCt>', but
        // I suspect I may frequently be able to re-use t1 or t2.
        // Given the current fact that I don't deallocate types, that
        // should be advantageous when possible.  Also, don't return
        // a type with cv flags, since that messes up the instantiation
        // of patterns.
        if (ct1==lubCt && cv1==cvu && t1->getCVFlags()==CV_NONE) return t1;
        if (ct2==lubCt && cv2==cvu && t2->getCVFlags()==CV_NONE) return t2;

        // make a type from the class
        lubCtType = env.tfac.makeCVAtomicType(SL_UNKNOWN, lubCt, cvu);
      }

      return env.tfac.makePointerType(SL_UNKNOWN, PO_POINTER, CV_NONE, lubCtType);
    }
  }

  // TODO: I should check for pointer-to-members that are compatible
  // by the existence of a greatest-upper-bound in the class hierarchy,
  // for example:
  //      A   B     .
  //       \ /      .
  //        C       . 
  // LUB(int A::*, int B::*) should be int C::*
  // 
  // However, that's a bit of a pain to do, since it means maintaining
  // the inverse of the 'hasBaseClass' relationship.  Also, I would not
  // be able to follow the simple pattern used for pointer-to-class,
  // since pointer-to-member can have multilevel cv qualification, so I
  // need to flow into the general cv LUB analysis below.
  //
  // Since this situation should be extremely rare, I won't bother for
  // now.

  // ok, inheritance is irrelevant; I need to see if types
  // are "similar" (4.4 para 4)
  if (!t1->equals(t2, Type::EF_SIMILAR)) {
    // not similar to each other, so there's no type that is similar
    // to both (similarity is an equivalence relation)
    return NULL;
  }

  // ok, well, are they equal?  if so, then I don't have to make
  // a new type object; NOTE: this allows identical enum arguments
  // to yield out
  if (t1->equals(t2, Type::EF_IGNORE_TOP_CV)) {
    // use 't1', but make sure we're not returning a cv-qualified type
    return env.tfac.setCVQualifiers(SL_UNKNOWN, CV_NONE, t1, NULL /*syntax*/);
  }
  
  // not equal, but they *are* similar, so construct the lub type; for
  // any pair of similar types, there is always a type to which both
  // can be converted
  bool cvdiffers = false;    // no difference recorded yet
  return similarLUB(env, t1, t2, cvdiffers, true /*toplevel*/);
}


void test_computeLUB(Env &env, Type *t1, Type *t2, Type *answer, int code)
{                          
  // compute the LUB
  bool wasAmbig;
  Type *a = computeLUB(env, t1, t2, wasAmbig);

  // did it do what we expected?
  bool ok = false;
  switch (code) {
    default:
      env.error("bad computeLUB code");
      return;

    case 0:
      if (!a && !wasAmbig) {
        ok = true;
      }
      break;

    case 1:
      if (a && a->equals(answer)) {
        ok = true;
      }
      break;

    case 2:
      if (!a && wasAmbig) {
        ok = true;
      }
      break;
  }

  static bool tracing = tracingSys("computeLUB");
  if (!tracing && ok) {
    return;
  }

  // describe the call
  string call = stringc << "LUB(" << t1->toString()
                        << ", " << t2->toString() << ")";

  // expected result
  string expect;
  switch (code) {
    case 0: expect = "fail"; break;
    case 1: expect = stringc << "yield `" << answer->toString() << "'"; break;
    case 2: expect = "fail with an ambiguity"; break;
  }

  // actual result
  string actual;
  if (a) {
    actual = stringc << "yielded `" << a->toString() << "'";
  }
  else if (!wasAmbig) {
    actual = "failed";
  }
  else {
    actual = "failed with an ambiguity";
  }

  if (tracing) {
    trace("computeLUB") << call << ": " << actual << "\n";
  }

  if (!ok) {
    // synthesize complete message
    env.error(stringc
      << "I expected " << call
      << " to " << expect
      << ", but instead it " << actual);
  }
}


// ----------------- debugging -------------------
int overloadNesting = 0;

ostream &overloadTrace()
{
  ostream &os = trace("overload");
  for (int i=0; i<overloadNesting; i++) {
    os << "  ";
  }
  return os;
}


// EOF
