// glrconfig.h
// compile-time configuration options which affect the generated
// GLR parser, and the interface to the user actions
//
// the intent is some macros get set externally, and that in turn
// influences the definition of the macros actually used in the
// parser code


#ifndef GLRCONFIG_H
#define GLRCONFIG_H

// when NO_GLR_SOURCELOC is #defined, we disable all support for
// automatically propagating source location information in the
// parser; user actions can still refer to 'loc', but they just get
// a dummy no-location value
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

#endif // GLRCONFIG_H
