// cc_lang.h            see license.txt for copyright and terms of use
// language options that the parser (etc.) is sensitive to

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
  // the name of the function; this is a C99 feature
  bool implicitFuncVariable;
                      
  // when true, and we see a class declaration inside something,
  // pretend it was at toplevel scope anyway
  bool noInnerClasses;

  // when true, an uninitialized global data object is typechecked as
  // a common symbol ("C" in the nm(1) manpage) instead of a bss
  // symbol ("B").  This means that the following is not an error:
  //   int a; int a;
  // gcc seems to operate as if this is true, whereas g++ not.
  bool uninitializedGlobalDataIsCommon;

public:
  CCLang() { ANSI_C(); }

  void ANSI_C();            // settings for ANSI C
  void ANSI_Cplusplus();    // settings for ANSI C++
};

#endif // CCLANG_H
