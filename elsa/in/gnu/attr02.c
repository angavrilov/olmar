// attr02.c
// these tests focus on the placement of attribute specifier lists,
// largely ignoring the syntax of what's in the (( and ))

// http://gcc.gnu.org/onlinedocs/gcc-3.1/gcc/Attribute-Syntax.html


// an attribute specifier list may appear af the colon following a label
void f()
{
  label: __attribute__((unused))
    goto label;
}


// An attribute specifier list may appear as part of a struct, union
// or enum specifier.

  // It may go either immediately after the struct, union or enum keyword,
  struct __attribute__((blah)) Struct1 {};
  union __attribute__((blah)) Union1 {};
  enum __attribute__((blah)) Enum1 { e1 };

  // or after the closing brace.
  struct Struct2 {} __attribute__((blah));
  union Union2 {} __attribute__((blah));
  enum Enum2 { e2 } __attribute__((blah));

  // subsequent text indicates it is allowed (though ignored) after
  // the keyword in an elaborated type specifier (no "{}")
  struct __attribute__((blah)) Struct1 *s1p;
  union __attribute__((blah)) Union1 *u1p;
  enum __attribute__((blah)) Enum1 *e1p;


// Otherwise, an attribute specifier appears as part of a declaration,
// counting declarations of unnamed parameters and type names
// [sm: so, they regard parameters as being "declarations"]
//
// Any list of specifiers and qualifiers at the start of a declaration
// may contain attribute specifiers, whether or not such a list may in
// that context contain storage class specifiers.
// [sm: what does "contain" main?  arbitrarily intermixed?  seems so...]
// 
// my speculation on the places that a "declaration" can occur:
//   - toplevel
//   - function scope
//   - struct member list
//   - function parameter list
            

// toplevel:
        __attribute__((blah)) int                              x1;
        __attribute__((blah)) int __attribute__((blah))        x2;
static  __attribute__((blah)) int __attribute__((blah))        x3;
        __attribute__((blah)) int __attribute__((blah)) static x4;
typedef __attribute__((blah)) int __attribute__((blah))        x5;

// function scope:
void g()
{
          __attribute__((blah)) int                              x1;
          __attribute__((blah)) int __attribute__((blah))        x2;
  static  __attribute__((blah)) int __attribute__((blah))        x3;
          __attribute__((blah)) int __attribute__((blah)) static x4;
  typedef __attribute__((blah)) int __attribute__((blah))        x5;
}

// struct member list (only in gcc >= 3):
struct Struct3 {
  #if defined(__GNUC__) && __GNUC__ >= 3
          __attribute__((blah)) int                              x1;
          __attribute__((blah)) int __attribute__((blah))        x2;
  short   __attribute__((blah)) int __attribute__((blah))        x3;
  #endif
};

// function parameter list (declaration declarator)
int f1(__attribute((blah)) int x);
int f2(short __attribute((blah)) int x);
int f3(__attribute((blah)) int x, __attribute((blah)) int y);

// and definition declarators
int g1(__attribute((blah)) int x) {}
int g2(short __attribute((blah)) int x) {}
int g3(__attribute((blah)) int x, __attribute((blah)) int y) {}


// In the obsolescent usage where a type of int is implied by the
// absence of type specifiers, such a list of specifiers and
// qualifiers may be an attribute specifier list with no other
// specifiers or qualifiers.
        __attribute__((blah)) /*implicit-int*/                   x6;


// An attribute specifier list may appear immediately before a
// declarator (other than the first) in a comma-separated list of
// declarators in a declaration of more than one identifier using a
// single list of specifiers and qualifiers.
int a1, __attribute__((blah)) a2;
int b1, __attribute__((blah)) *b2;
int c1, * __attribute__((blah)) c2;     // nested declarators?  guess so...

// try this in function scope too
void h()
{
  int a1, __attribute__((blah)) a2;
  int b1, __attribute__((blah)) *b2;
  int c1, * __attribute__((blah)) c2;     // nested declarator
}

// example from the manual; only works in gcc >= 3
#if defined(__GNUC__) && __GNUC__ >= 3
__attribute__((noreturn)) void d0 (void),
         __attribute__((format(printf, 1, 2))) d1 (const char *, ...),
          d2 (void)   ;   // why was the semicolon missing?
#endif


// An attribute specifier list may appear immediately before the
// comma, = or semicolon terminating the declaration of an identifier
// other than a function definition.
int aa1 __attribute__((blah));
int bb1 __attribute__((blah)) = 2;
int cc1 __attribute__((blah)), cc2;
int dd1 __attribute__((blah)), dd2 __attribute__((blah));

int ee1(int) __attribute__((blah));
//int ff1(int) __attribute__((blah)) {}    // this one isn't allowed

// example from the manual
void (****gg1)(void) __attribute__((noreturn));


// a few more nested declarator examples from the manual
void (__attribute__((noreturn)) ****hh1) (void);
char *__attribute__((aligned(8))) *ii1;


// NOTE: I did not test the thing about qualifiers inside [].


// http://gcc.gnu.org/onlinedocs/gcc-3.1/gcc/Asm-Labels.html

// You can specify the name to be used in the assembler code for a C
// function or variable by writing the asm (or __asm__) keyword after
// the declarator as follows:

int foo asm ("myfoo") = 2;

// Where an assembler name for an object or function is specified (see
// Asm Labels), at present the attribute must follow the asm
// specification

int foo2 asm ("myfoo2") __attribute((blah)) = 3;


// EOF
