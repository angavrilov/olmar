// glrconfig.h
// compile-time configuration options which affect the generated
// GLR parser, and the interface to the user actions
//
// the intent is some macros get set externally, and that in turn
// influences the definition of the macros actually used in the
// parser code


#ifndef GLRCONFIG_H
#define GLRCONFIG_H


// disable even xassert-style assertions
//#define NDEBUG_NO_ASSERTIONS


// when NO_GLR_SOURCELOC is #defined, we disable all support for
// automatically propagating source location information in the
// parser; user actions can still refer to 'loc', but they just get
// a dummy no-location value
#define NO_GLR_SOURCELOC
#ifdef NO_GLR_SOURCELOC
  #define SOURCELOC(stuff)
  #define SOURCELOCARG(stuff)
  #define NOSOURCELOC(stuff) stuff
#else
  #define SOURCELOC(stuff) stuff

  // this one adds a leading comma (I can't put that into the
  // argument <stuff>, because then it looks like the macro is
  // being passed 2 arguments)
  #define SOURCELOCARG(stuff) , stuff

  #define NOSOURCELOC(stuff)
#endif


// when enabled, NODE_COLUMN tracks in each stack node the
// appropriate column to display it for in debugging dump
#ifdef ENABLE_NODE_COLUMNS
  #define NODE_COLUMN(stuff) stuff
#else
  #define NODE_COLUMN(stuff)
#endif


// when enabled, YIELD_COUNT keeps track of the number of times a
// given semantic value is yielded; this is useful for warning the
// user when a merge is performed but one of the merged values has
// already been yielded to another semantic action, which implies
// that the induced parse forest is incomplete
#define DISABLE_YIELD_COUNT
#ifndef DISABLE_YIELD_COUNT
  #define YIELD_COUNT(stuff) stuff
#else
  #define YIELD_COUNT(stuff)
#endif



#endif // GLRCONFIG_H
