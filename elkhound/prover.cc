// prover.cc
// code to run Simplify
                                   
// needed for mkstemp
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include "prover.h"    // this module
#include "str.h"       // stringc

#include <stdlib.h>    // mkstemp
#include <unistd.h>    // write
#include <stdio.h>     // perror

bool runProver(char const *str)
{
  // make a temp file
  char fname[] = "/tmp/proverXXXXXX";
  int fd = mkstemp(fname);
  if (fd < 0) {
    perror("mkstemp");
    exit(2);
  }

  // stash the contents of 'str' in it
  write(fd, str, strlen(str));
  write(fd, "\n", 1);    // necessary?

  if (close(fd) < 0) {
    perror("close");
    exit(2);
  }
  
  
  // run Simplify on that file
  int code = system(stringc << "Simplify < " << fname << " | grep Valid >/dev/null");
  
  
  // clean up the temp file
  if (unlink(fname) < 0) {
    perror("unlink");      // but continue anyway
  }
  
  
  return code == 0;
}
