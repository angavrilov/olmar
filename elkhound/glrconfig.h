// glrconfig.h            see license.txt for copyright and terms of use
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


// parser core selection

// original state worklist (SWL) core
#ifndef USE_SWL_CORE
  #define USE_SWL_CORE 0
#endif

// use delayed state heuristic in SWL?
#ifndef USE_DELAYED_STATES
  #define USE_DELAYED_STATES 0
#endif

// new reduction worklist (RWL) core
#ifndef USE_RWL_CORE
  #define USE_RWL_CORE 1
#endif

// ordnary LR core
#ifndef USE_MINI_LR
  #define USE_MINI_LR 1
#endif


// when NO_GLR_SOURCELOC is #defined, we disable all support for
// automatically propagating source location information in the
// parser; user actions can still refer to 'loc', but they just get
// a dummy no-location value
#ifndef GLR_SOURCELOC
  #define GLR_SOURCELOC 1
#endif

#if GLR_SOURCELOC
  #define SOURCELOC(stuff) stuff

  // this one adds a leading comma (I can't put that into the
  // argument <stuff>, because then it looks like the macro is
  // being passed 2 arguments)
  #define SOURCELOCARG(stuff) , stuff

  #define NOSOURCELOC(stuff)
#else
  #define SOURCELOC(stuff)
  #define SOURCELOCARG(stuff)
  #define NOSOURCELOC(stuff) stuff
#endif


// when enabled, NODE_COLUMN tracks in each stack node the
// appropriate column to display it for in debugging dump
#ifndef ENABLE_NODE_COLUMNS
  #define ENABLE_NODE_COLUMNS 1
#endif
#if ENABLE_NODE_COLUMNS
  #define NODE_COLUMN(stuff) stuff
#else
  #define NODE_COLUMN(stuff)
#endif

#if USE_RWL_CORE && !ENABLE_NODE_COLUMNS
  #error node columns are requred for the RWL core
#endif


// when enabled, YIELD_COUNT keeps track of the number of times a
// given semantic value is yielded; this is useful for warning the
// user when a merge is performed but one of the merged values has
// already been yielded to another semantic action, which implies
// that the induced parse forest is incomplete
#ifndef ENABLE_YIELD_COUNT
  #define ENABLE_YIELD_COUNT 1
#endif
#if ENABLE_YIELD_COUNT
  #define YIELD_COUNT(stuff) stuff
#else
  #define YIELD_COUNT(stuff)
#endif


#endif // GLRCONFIG_H
