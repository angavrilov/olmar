// stdconv.cc                       see license.txt for copyright and terms of use
// code for stdconv.h

#include "stdconv.h"      // this module
#include "cc_type.h"      // Type
#include "cc_env.h"       // Env
#include "trace.h"        // tracingSys


// ----------------------- StandardConversion -------------------
string toString(StandardConversion c)
{
  stringBuilder sb;

  if (c == SC_ERROR) {
    return string("SC_ERROR");
  }

  if (c == SC_IDENTITY) {
    return string("SC_IDENTITY");
  }

  #define CASE(label)                  \
    case label:                        \
      if (sb.length()) { sb << "|"; }  \
      sb << #label;                    \
      break;

  switch (c & SC_GROUP_1_MASK) {
    default: return stringc << "bad code: " << (int)c;
    case SC_IDENTITY: break;
    CASE(SC_LVAL_TO_RVAL)
    CASE(SC_ARRAY_TO_PTR)
    CASE(SC_FUNC_TO_PTR)
  }

  switch (c & SC_GROUP_3_MASK) {
    default: return stringc << "bad code: " << (int)c;
    case SC_IDENTITY: break;
    CASE(SC_QUAL_CONV)
  }

  switch (c & SC_GROUP_2_MASK) {
    default: return stringc << "bad code: " << (int)c;
    case SC_IDENTITY: break;
    CASE(SC_INT_PROM)
    CASE(SC_FLOAT_PROM)
    CASE(SC_INT_CONV)
    CASE(SC_FLOAT_CONV)
    CASE(SC_FLOAT_INT_CONV)
    CASE(SC_PTR_CONV)
    CASE(SC_PTR_MEMB_CONV)
    CASE(SC_BOOL_CONV)
  }

  #undef CASE

  if (c & ~(SC_GROUP_1_MASK | SC_GROUP_2_MASK | SC_GROUP_3_MASK)) {
    return stringc << "bad code: " << (int)c;
  }

  return sb;
}


StandardConversion removeLval(StandardConversion scs)
{
  if ((scs & SC_GROUP_1_MASK) == SC_LVAL_TO_RVAL) {
    // remove this transformation
    return scs & (SC_GROUP_2_MASK | SC_GROUP_3_MASK);
  }
  else {
    return scs;
  }
}


bool isSubsequenceOf(StandardConversion left, StandardConversion right)
{
  StandardConversion L, R;
  
  L = left & SC_GROUP_1_MASK;                                   
  R = right & SC_GROUP_1_MASK;
  if (!( L == SC_IDENTITY || L == R )) {
    return false;
  }

  L = left & SC_GROUP_2_MASK;
  R = right & SC_GROUP_2_MASK;
  if (!( L == SC_IDENTITY || L == R )) {
    return false;
  }

  L = left & SC_GROUP_3_MASK;
  R = right & SC_GROUP_3_MASK;
  if (!( L == SC_IDENTITY || L == R )) {
    return false;
  }

  return true;
}


// ---------------------------- SCRank ----------------------------
// table 9
SCRank getRank(StandardConversion scs)
{
  if ((scs & SC_GROUP_2_MASK) >= SC_INT_CONV) {
    return SCR_CONVERSION;
  }
  
  if (scs & SC_GROUP_2_MASK) {
    return SCR_PROMOTION;
  }
  
  return SCR_EXACT;
}


// --------------------- getStandardConversion --------------------
bool isIntegerPromotion(AtomicType const *src, AtomicType const *dest);

// int (including bitfield), bool, or enum
bool isIntegerNumeric(Type const *t, SimpleType const *tSimple)
{
  if (tSimple) {
    return isIntegerType(tSimple->type) ||
           tSimple->type == ST_BOOL;
  }

  // TODO: bitfields are also a valid integer conversion source,
  // once I have an explicit representation for them

  return t->isEnumType();
}

// any of above, or float
bool isNumeric(Type const *t, SimpleType const *tSimple)
{
  return isIntegerNumeric(t, tSimple) ||
         (tSimple && isFloatType(tSimple->type));
}


// cppstd 13.6 para 2
bool isPromotedArithmetic(SimpleTypeId id)
{
  return ST_INT <= id && id <= ST_UNSIGNED_LONG_INT ||
         ST_FLOAT <= id && id <= ST_LONG_DOUBLE;
}


#if 0   // unused
static char const *atomicName(AtomicType::Tag tag)
{
  switch (tag) {
    default: xfailure("bad tag");
    case AtomicType::T_SIMPLE:   return "simple";
    case AtomicType::T_COMPOUND: return "compound";
    case AtomicType::T_ENUM:     return "enum";
    case AtomicType::T_TYPEVAR:  return "type variable";
  }
}
#endif // 0

static char const *ctorName(Type::Tag tag)
{
  switch (tag) {
    default: xfailure("bad tag");
    case Type::T_ATOMIC:          return "atomic";
    case Type::T_POINTER:         return "pointer";
    case Type::T_FUNCTION:        return "function";
    case Type::T_ARRAY:           return "array";
    case Type::T_POINTERTOMEMBER: return "ptr-to-member";
  }
}




// implementation class
class Conversion {
public:
  // original parameters to 'getStandardConversion'
  string *errorMsg;
  Type const *src;
  Type const *dest;

  // eventual return value
  StandardConversion ret;

  // when true, every destination pointer type constructor
  // has had 'const' in its cv flags
  bool destConst;

  // count how many pointer or ptr-to-member type ctors we
  // have stripped so far
  int ptrCtorsStripped;

public:
  Conversion(string *e, Type const *s, Type const *d)
    : errorMsg(e), src(s), dest(d),
      ret(SC_IDENTITY),
      destConst(true),
      ptrCtorsStripped(0)
  {}

  StandardConversion error(char const *why);

  bool stripPtrCtor(CVFlags scv, CVFlags dcv, bool isReference=false);
};


StandardConversion Conversion::error(char const *why)
{
  if (errorMsg) {
    *errorMsg = stringc
      << "cannot convert `" << src->toString()
      << "' to `" << dest->toString()
      << "': " << why;
  }
  return ret = SC_ERROR;
}


// strip pointer constructors, update local state; return true
// if we've encountered an error, in which case 'ret' is set
// to the error code to return
bool Conversion::stripPtrCtor(CVFlags scv, CVFlags dcv, bool isReference)
{    
  if (scv != dcv) {
    if (isReference) {
      // Conversion from 'int &' to 'int const &' is equivalent to
      // SC_LVAL_TO_RVAL, or so I'm led to believe by 13.3.3.2 para 3,
      // second example.  13.3.3.1.4 para 5 talks about "reference-
      // compatible with added qualification", but I don't then see
      // a later discussion of what exactly this means.
      //
      // update: that term is defined in 8.5.3, and I now think that
      // binding 'A&' to 'A const &' should be an SC_QUAL_CONV, just
      // like with pointers...; moreover, I suspect that since
      // SC_LVAL_TO_RVAL and SC_QUAL_CONV have the same *rank*, I'll
      // still be able to pass 13.3.3.2 para 3 ex. 2
      xassert(ret == SC_IDENTITY);     // shouldn't have had anything added yet
      //ret |= SC_QUAL_CONV;
      
      // trying again.. 13.3.3.1.4 para 1
      ret |= SC_IDENTITY;
      
      // old code; if ultimately this solution works, I'll drop the
      // 'isReference' parameter to this function entirely...
      //ret |= SC_LVAL_TO_RVAL;
    }
    else {
      ret |= SC_QUAL_CONV;
    }
  }

  if (scv & ~dcv) {
    error("the source has some cv flag that the dest does not");
    return true;
  }

  if (!destConst && (scv != dcv)) {
    error("changed cv flags below non-const pointer");
    return true;
  }

  if (!( dcv & CV_CONST )) {
    destConst = false;
  }

  ptrCtorsStripped++;
  return false;
}


// one of the goals of this function is to *not* construct any
// intermediate Type objects; I should be able to do this computation
// without allocating, and if I can then that avoids interaction
// problems with Type annotation systems
StandardConversion getStandardConversion
  (string *errorMsg, SpecialExpr srcSpecial, Type const *src, Type const *dest)
{
  Conversion conv(errorMsg, src, dest);

  // --------------- group 1 ----------------
  if (src->isReference() &&
      !src->asRvalC()->isFunctionType() &&
      !src->asRvalC()->isArrayType() &&
      !dest->isReference()) {
    conv.ret |= SC_LVAL_TO_RVAL;

    src = src->asPointerTypeC()->atType;

    // the src type must be complete for this conversion
    if (src->isCompoundType() &&
        src->asCompoundTypeC()->forward) {
      return conv.error("type must be complete to strip '&'");
    }
    
    // am I supposed to check cv flags?
  }
  else if (!src->isReference() && dest->isReference()) {
    // binding an (rvalue) object to a reference

    // are we trying to bind to a non-const reference?  if so,
    // then we can't do it (cppstd 13.3.3.1.4 para 3); I haven't
    // implemented the exception for the 'this' argument yet
    PointerType const *destPT = dest->asPointerTypeC();
    if (!destPT->atType->isConst()) {
      // can't form the conversion
      return conv.error("attempt to bind an rvalue to a non-const reference");
    }

    // strip off the destination reference
    dest = dest->asPointerTypeC()->atType;
    
    // now, one final exception: ordinarily, there's no standard
    // conversion from C to P (where C inherits from P); but it *is*
    // legal to bind an rvalue of type C to a const reference to P
    // (cppstd 13.3.3.1.4 para 1)
    if (dest->isCompoundType() && 
        src->isCompoundType() &&
        src->asCompoundTypeC()->hasStrictBaseClass(dest->asCompoundTypeC())) {
      // TODO: ambiguous? inaccessible?
      return SC_PTR_CONV;     // "derived-to-base Conversion"
    }
  }
  else if (src->asRvalC()->isArrayType() && dest->isPointer()) {
    // 7/19/03: fix: 'src' can be an lvalue (cppstd 4.2 para 1)

    conv.ret |= SC_ARRAY_TO_PTR;

    src = src->asRvalC()->asArrayTypeC()->eltType;
    dest = dest->asPointerTypeC()->atType;

    // do one level of qualification conversion checking
    CVFlags scv = src->getCVFlags();
    CVFlags dcv = dest->getCVFlags();

    if (srcSpecial == SE_STRINGLIT &&
        scv == CV_CONST &&
        dcv == CV_NONE) {
      // special exception of 4.2 para 2: string literals can be
      // converted to 'char*' (w/o 'const'); we'll already have
      // converted 'char const []' to 'char const *', so this just
      // adds the qualification conversion
      //
      // TODO: it might be nice to have a CCLang option to disable
      // this, so that we could get soundness at the expense of
      // compatibility with legacy code
      conv.ret |= SC_QUAL_CONV;
      scv = CV_NONE;   // avoid error in stripPtrCtor, below
    }

    if (conv.stripPtrCtor(scv, dcv))
      { return conv.ret; }
  }
  else if (src->isFunctionType() && dest->isPointerType()) {
    conv.ret |= SC_FUNC_TO_PTR;

    dest = dest->asPointerTypeC()->atType;

    if (conv.stripPtrCtor(src->getCVFlags(), dest->getCVFlags()))
      { return conv.ret; }
  }

  // At this point, if the types are to be convertible, their
  // constructed type structure must be isomorphic, possibly with the
  // exception of cv flags and/or the containing class for member
  // pointers.  The next phase checks the isomorphism and verifies
  // that any difference in the cv flags is within the legal
  // variations.

  // ---------------- group 3 --------------------
  // deconstruct the type constructors until at least one of them
  // hits the leaf
  while (!src->isCVAtomicType() &&
         !dest->isCVAtomicType()) {
    if (src->getTag() != dest->getTag()) {
      return conv.error("different type constructors");
    }

    switch (src->getTag()) {
      default: xfailure("bad type tag");

      case Type::T_POINTER: {
        PointerType const *s = src->asPointerTypeC();
        PointerType const *d = dest->asPointerTypeC();

        if (s->op != d->op) {                                  
          // the source could not be a reference, because we
          // already took care of that above, and references
          // cannot be stacked
          xassert(s->op != PO_REFERENCE);
          xassert(d->op == PO_REFERENCE);

          // thus, we're trying to convert to an lvalue
          return conv.error("cannot convert rvalue to lvalue");
        }

        src = s->atType;
        dest = d->atType;

        bool isReference = (s->op == PO_REFERENCE);

        // we look at the cv flags one level down because all of the
        // rules in cppstd talk about things like "pointer to cv T",
        // i.e. pairing the * with the cv one level down in their
        // descriptive patterns
        if (conv.stripPtrCtor(src->getCVFlags(), dest->getCVFlags(), isReference))
          { return conv.ret; }

        break;
      }

      case Type::T_FUNCTION: {
        // no variance is allowed whatsoever once we reach a function
        // type, which is a little odd since I'd think it would be
        // ok to pass
        //   int (*)(Base*)
        // where
        //   int (*)(Derived*)
        // is expected, but I don't see such a provision in cppstd
        if (src->equals(dest)) {
          return conv.ret;
        }
        else {
          return conv.error("unequal function types");
        }
      }

      case Type::T_ARRAY: {
        // like functions, no conversions are possible on array types,
        // including (as far as I can see) converting
        //   int[3]
        // to
        //   int[]
        if (src->equals(dest)) {
          return conv.ret;
        }
        else {
          return conv.error("unequal array types");
        }
      }

      case Type::T_POINTERTOMEMBER: {
        PointerToMemberType const *s = src->asPointerToMemberTypeC();
        PointerToMemberType const *d = dest->asPointerToMemberTypeC();

        if (s->inClass != d->inClass) {
          if (conv.ptrCtorsStripped == 0) {
            // opposite to first ptr ctor, we allow Base -> Derived
            if (!d->inClass->hasUnambiguousBaseClass(s->inClass)) {
              return conv.error("src member's class is not an unambiguous "
                                "base of dest member's class");
            }
            else if (d->inClass->hasVirtualBase(s->inClass)) {
              return conv.error("src member's class is a virtual base of "
                                "dest member's class");
            }
            else {
              // TODO: check accessibility.. this depends on the access privileges
              // of the code we're in now..

              // this is actually a group 2 conversion
              conv.ret |= SC_PTR_MEMB_CONV;
            }
          }
          else {
            // after the first ctor, variance is not allowed
            return conv.error("unequal member classes in ptr-to-member that "
                              "is not the topmost type");
          }
        }

        src = s->atType;
        dest = d->atType;

        if (conv.stripPtrCtor(src->getCVFlags(), dest->getCVFlags()))
          { return conv.ret; }

        break;
      }
    }
  }

  // ---------------- group 2 --------------

  // if both types have not arrived at CVAtomic, then they
  // are not convertible
  if (!src->isCVAtomicType() ||
      !dest->isCVAtomicType()) {
    // exception: pointer -> bool
    if (dest->isSimple(ST_BOOL) &&
        (src->isPointerType() || src->isPointerToMemberType())) {
      return conv.ret | SC_BOOL_CONV;
    }

    // exception: 0 -> (null) pointer
    if (srcSpecial == SE_ZERO) {
      if (dest->isPointerType()) {
        return conv.ret | SC_PTR_CONV;
      }
      if (dest->isPointerToMemberType()) {
        return conv.ret | SC_PTR_MEMB_CONV;
      }
    }

    if (errorMsg) {
      // if reporting, I go out of my way a bit here since I expect
      // this to be a relatively common error and I'd like to provide
      // as much information as will be useful
      if (dest->isReference()) {
        return conv.error("cannot convert rvalue to lvalue");
      }

      return conv.error(stringc
        << "different type constructors, "
        << ctorName(src->getTag()) << " vs. "
        << ctorName(dest->getTag()));
    }
    else {
      return SC_ERROR;     // for performance, don't make the string at all
    }
  }

  // now we're down to atomics; we expect equality, but ignoring cv
  // flags because they've already been handled

  CVAtomicType const *s = src->asCVAtomicTypeC();
  CVAtomicType const *d = dest->asCVAtomicTypeC();

  if (conv.ptrCtorsStripped > 0) {
    if (conv.ptrCtorsStripped == 1) {
      if (dest->isSimple(ST_VOID)) {
        return conv.ret | SC_PTR_CONV;      // converting T* to void*
      }

      if (src->isCompoundType() &&
          dest->isCompoundType() &&
          src->asCompoundTypeC()->hasStrictBaseClass(dest->asCompoundTypeC())) {
        if (!src->asCompoundTypeC()->
              hasUnambiguousBaseClass(dest->asCompoundTypeC())) {
          return conv.error("base class is ambiguous");
        }
        // TODO: check accessibility.. this depends on the access privileges
        // of the code we're in now..
        return conv.ret | SC_PTR_CONV;      // converting Derived* to Base*
      }
    }

    // since we stripped ptrs, final type must be equal
    if (s->atomic->equals(d->atomic)) {
      return conv.ret;
    }
    else {
      return conv.error("incompatible atomic types");
    }
  }
  else {
    // further info on this: 13.3.3.1 para 6, excerpt:
    //   "Any difference in top-level cv-qualification is
    //    subsumed by the initialization and does not
    //    constitute a conversion."

    #if 0    // am I supposed to do any checking?
    // I'm not perfectly clear on the checking I should do for
    // the cv flags here.  lval-to-rval says that 'int const &'
    // becomes 'int' whereas 'Foo const &' becomes 'Foo const'
    if ((conv.ret & SC_LVAL_TO_RVAL) &&     // did we do lval-to-rval?
        s->atomic->isSimpleType()) {        // were we a ref to simple?
      // clear any 'const' on the source
      scv &= ~CV_CONST;
    }

    if (scv != dcv) {
      return conv.error("different cv flags (is this right?)");
    }                         
    #endif // 0
  }

  if (s->atomic->equals(d->atomic)) {
    return conv.ret;    // identical now
  }

  SimpleType const *srcSimple = src->isSimpleType() ? src->asSimpleTypeC() : NULL;
  SimpleType const *destSimple = dest->isSimpleType() ? dest->asSimpleTypeC() : NULL;

  if (destSimple && destSimple->type == ST_PROMOTED_ARITHMETIC &&
      srcSimple && isPromotedArithmetic(srcSimple->type)) {
    // polymorphic match
    return conv.ret;
  }

  if (isIntegerPromotion(s->atomic, d->atomic)) {
    return conv.ret | SC_INT_PROM;
  }

  if (srcSimple && srcSimple->type == ST_FLOAT &&
      destSimple && destSimple->type == ST_DOUBLE) {
    return conv.ret | SC_FLOAT_PROM;
  }

  if (isIntegerNumeric(src, srcSimple) &&
      destSimple && isIntegerType(destSimple->type)) {
    return conv.ret | SC_INT_CONV;
  }

  if (isNumeric(src, srcSimple) &&
      destSimple && destSimple->type == ST_BOOL) {
    return conv.ret | SC_BOOL_CONV;
  }

  bool srcFloat = srcSimple && isFloatType(srcSimple->type);
  bool destFloat = destSimple && isFloatType(destSimple->type);
  if (srcFloat && destFloat) {
    return conv.ret | SC_FLOAT_CONV;
  }

  if (isNumeric(src, srcSimple) &&
      isNumeric(dest, destSimple) &&
      (srcFloat || destFloat)) {     // last test required if both are enums
    return conv.ret | SC_FLOAT_INT_CONV;
  }

  // no more conversion possibilities remain; I don't print the
  // atomic kinds, because the error is based on more than just
  // the kinds; moreover, since I already know I didn't strip
  // any ptr ctors, the full types should be easy to read
  return conv.error("incompatible atomic types");
}


// This function implements Section 4.5, which contains
// implementation-determined details.  Promotions are distinguished
// from conversions in that they are preferred over conversions during
// overload resolution.  Since this distinction is implementation-
// dependent, I envision that users might replace this function with
// an implementation that better suits them.
//
// NOTE:  One way to get a conservative analysis that never silently
// chooses among potentially-ambiguous choices is to make this always
// return false.
//
// Another idea:  It would be nice to have a set of tests such that
// by running the tests one could determine what choices a given compiler
// makes, so that this function could be modified accordingly to
// imitate that behavior.
bool isIntegerPromotion(AtomicType const *src, AtomicType const *dest)
{
  bool srcSimple = src->isSimpleType();
  bool destSimple = dest->isSimpleType();

  SimpleTypeId sid = srcSimple? src->asSimpleTypeC()->type : ST_ERROR;
  SimpleTypeId did = destSimple? dest->asSimpleTypeC()->type : ST_ERROR;

  if (did == ST_INT ||
      did == ST_PROMOTED_ARITHMETIC) {
    // paragraph 1: char/short -> int
    // implementation choice: I assume char is 8 bits and short
    // is 16 bits and int is 32 bits, so all map to 'int', as
    // opposed to 'unsigned int'
    if (sid == ST_CHAR ||
        sid == ST_UNSIGNED_CHAR ||
        sid == ST_SIGNED_CHAR ||
        sid == ST_SHORT_INT ||
        sid == ST_UNSIGNED_SHORT_INT) {
      return true;
    }

    // paragraph 2: wchar_t/enum -> int
    // implementation choice: I assume wchar_t and all enums fit into ints
    if (sid == ST_WCHAR_T ||
        src->isEnumType()) {
      return true;
    }

    // TODO: paragraph 3: bitfields

    // paragraph 4: bool -> int
    if (sid == ST_BOOL) {
      return true;
    }
  }

  return false;
}


void test_getStandardConversion(
  Env &env, SpecialExpr special, Type const *src, Type const *dest,
  int expected)
{
  // run our function
  string errorMsg;
  StandardConversion actual = getStandardConversion(&errorMsg, special, src, dest);

  // turn any resulting messags into warnings, so I can see their
  // results without causing the final exit status to be nonzero
  if (actual == SC_ERROR) {
    env.warning(errorMsg);
  }

  // did the function do what we expected?
  if (actual != expected) {
    // no, explain the situation
    env.error(stringc
      << "getStandardConversion("
      << toString(special) << ", `"
      << src->toString() << "', `"
      << dest->toString() << "') yielded "
      << toString(actual) << ", but I expected "
      << toString((StandardConversion)expected));
  }
  else if (tracingSys("gSC")) {
    // make a warning to show what happened anyway
    env.warning(stringc
      << "getStandardConversion("
      << toString(special) << ", `"
      << src->toString() << "', `"
      << dest->toString() << "') yielded "
      << toString(actual));
  }
}
