// cc_lang.h            see license.txt for copyright and terms of use
// language options that the parser (etc.) is sensitive to

// a useful reference:
//   Incompatibilities Between ISO C and ISO C++
//   David R. Tribble
//   http://david.tribble.com/text/cdiffs.htm

#ifndef CCLANG_H
#define CCLANG_H

class CCLang {
public:
  // when this is true, and the parser sees "struct Foo { ... }",
  // it will pretend it also saw "typedef struct Foo Foo;" -- i.e.,
  // the structure (or class) tag name is treated as a type name
  // by itself
  //
  // NOTE: right now I ignore this flag, and actually insert the
  // implicit typedef in all cases
  bool tagsAreTypes;

  // when true, recognize C++ keywords in input stream
  bool recognizeCppKeywords;

  // when true, every function body gets an implicit
  //   static char const __func__[] = "function-name";
  // declaration just inside the opening brace, where function-name is
  // the name of the function; this is a C99 feature (section 6.4.2.2)
  bool implicitFuncVariable;

  // behavior of gcc __FUNCTION__ and __PRETTY_FUNCTION__
  // see also
  //   http://gcc.gnu.org/onlinedocs/gcc-3.4.1/gcc/Function-Names.html
  //   http://gcc.gnu.org/onlinedocs/gcc-2.95.3/gcc_4.html#SEC101
  enum GCCFuncBehavior {
    GFB_none,              // ordinary symbols
    GFB_string,            // string literal (they concatenate!)
    GFB_variable,          // variables, like __func__
  } gccFuncBehavior;

  // when true, and we see a class declaration inside something,
  // pretend it was at toplevel scope anyway; this also applies to
  // enums, enumerators and typedefs
  //
  // dsw: I find that having boolean variables that are in the
  // negative sense is usually a mistake.  I would reverse the sense
  // of this one.
  //
  // sm: The 'no' is a little misleading.  In the 'false' case,
  // syntax reflects semantics naturally; only in the 'true' case
  // is something unusual going on.  A positive-sense name might be
  // the unwieldy 'turnApparentlyInnerClassesIntoOuterClasses'.
  bool noInnerClasses;

  // when true, an uninitialized global data object is typechecked as
  // a common symbol ("C" in the nm(1) manpage) instead of a bss
  // symbol ("B").  This means that the following is not an error:
  //   int a; int a;
  // gcc seems to operate as if this is true, whereas g++ not.
  bool uninitializedGlobalDataIsCommon;

  // when true, if a function has an empty parameter list then it is
  // treated as supplying no parameter information (C99 6.7.5.3 para 14)
  bool emptyParamsMeansNoInfo;

  // when false, we do not complain if someone tries to dereference a
  // non-pointer.. this is done to overcome the lack of full support
  // for overloading, which causes me to compute the wrong types
  // sometimes (defaults to true, but is false in C++ mode for now)
  bool complainUponBadDeref;

  // when true, require all array sizes to be positive; when false,
  // 0-length arrays are allowed as class/struct fields
  bool strictArraySizeRequirements;

  // when true, we allow overloaded function declarations (same name,
  // different signature)
  bool allowOverloading;

  // when true, to every compound type add the name of the type itself
  bool compoundSelfName;

  // when true, allow a function call to a function that has never
  // been declared, implicitly declaring the function in the global
  // scope; this is for C89 (and earlier) support
  bool allowImplicitFunctionDecls;

  // when true, allow function definitions that omit any return type
  // to implicitly return 'int'.
  bool allowImplicitInt;
  
  // GNU extension: when true, allow local variable arrays to have
  // sizes that are not constant
  bool allowDynamicallySizedArrays;

  // GCC extension: when true, you can say things like 'enum Foo;' and
  // it declares that an enum called Foo will be defined later
  bool allowIncompleteEnums;

  // C language, and GNU extension for C++: allow a class to have a
  // member (other than the constructor) that has the same name as the
  // class
  bool allowMemberWithClassName;

  // every C++ compiler I have does overload resolution of operator=
  // incorrectly; this flag causes Elsa to do the same
  bool nonstandardAssignmentOperator;

  // when true, "_Bool" is a built-in type keyword (C99)
  bool predefined_Bool;

  // declare the various GNU __builtin functions; see
  // Env::addGNUBuiltins in gnu.cc
  bool declareGNUBuiltins;

  // catch-call for behaviors that are unique to C++ but aren't
  // enumerated above; these behaviors are candidates for being split
  // out as separate flags, but there currently is no need
  bool isCplusplus;

public:
  CCLang() { ANSI_C89(); }

  // The predefined settings below are something of a best-effort at
  // reasonable starting configurations.  Every function below sets
  // *all* of the flags; they are not incremental.  Users are
  // encouraged to explicitly set fields after activating a predefined
  // setting to get a specific setting.

  void KandR_C();           // settings for K&R C
  void ANSI_C89();          // settings for ANSI C89
  void ANSI_C99();          // settings for ANSI C99
  void GNU_C();             // settings for GNU C
  void GNU_KandR_C();       // GNU 3.xx C + K&R compatibility
  void GNU2_KandR_C();      // GNU 2.xx C + K&R compatibility

  void ANSI_Cplusplus();    // settings for ANSI C++ 98
  void GNU_Cplusplus();     // settings for GNU C++
};

#endif // CCLANG_H
