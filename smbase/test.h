// test.h            see license.txt for copyright and terms of use
// utilities for test code
// Scott McPeak, 1999  This file is public domain.

#ifndef __TEST_H
#define __TEST_H

#include <iostream.h>      // cout
#include <stdio.h>         // printf
#include "exc.h"           // xBase
#include "nonport.h"       // getMilliseconds


// reports uncaught exceptions
//
// 12/30/02: I used to print "uncaught exception: " before
// printing the exception, but this is meaningless to the
// user and the message usually has enough info anyway
#define USUAL_MAIN                              \
void entry();                                   \
int main()                                      \
{                                               \
  try {                                         \
    entry();                                    \
    return 0;                                   \
  }                                             \
  catch (xBase &x) {                            \
    cout << x << endl;                          \
    return 4;                                   \
  }                                             \
}

// same as above, but with command-line args
#define ARGS_MAIN                               \
void entry(int argc, char *argv[]);             \
int main(int argc, char *argv[])                \
{                                               \
  try {                                         \
    entry(argc, argv);                          \
    return 0;                                   \
  }                                             \
  catch (xBase &x) {                            \
    cout << x << endl;                          \
    return 4;                                   \
  }                                             \
}


// convenient for printing the value of a variable or expression
#define PVAL(val) cout << #val << " = " << (val) << endl


// easy way to time a section of code
class TimedSection {
  char const *name;
  long start;

public:
  TimedSection(char const *n) : name(n) {
    start = getMilliseconds();
  }
  ~TimedSection() {
    cout << name << ": " << (getMilliseconds() - start) << " msecs\n";
  }
};


#endif // __TEST_H

