// prover.cc
// code to run Simplify
                                   
#include "prover.h"    // this module
#include "str.h"       // stringc
#include "mypopen.h"   // popen_execvp
#include "unixutil.h"  // readString, writeAll
#include "trace.h"     // trace

#include <stdio.h>     // perror, FILE stuff
#include <string.h>    // strstr

bool runProver(char const *str)
{
  static bool opened = false;             // true once we've opened the process
  static int toSimplify, fromSimplify;    // pipe file descriptors

  trace("prover") << str << endl;

  if (!opened) {
    // open Simplify as a child process, with pipes to communicate
    char *argv[] = { "Simplify", "-nosc", NULL };
    popen_execvp(&toSimplify, &fromSimplify, NULL, argv[0], argv);

    // open background predicate
    FILE *bgpred = fopen("bgpred.sx", "r");
    if (!bgpred) {
      perror("open bgpred.sx");
      exit(2);
    }

    // read the background predicate and feed it to Simplify
    enum { BUFSIZE = 4096 };
    char buf[BUFSIZE];
    int len;
    while ((len = fread(buf, 1, BUFSIZE, bgpred)) > 0) {
      if (!writeAll(toSimplify, buf, len)) {
        perror("write bgpred to Simplify");
        exit(2);
      }
    }
    fclose(bgpred);

    // read Simplify's response
    if (!readString(fromSimplify, buf, BUFSIZE)) {
      perror("read from Simplify");
      exit(2);
    }
    if (0!=strcmp(buf, ">\t")) {
      fprintf(stderr, "unexpected response from Simplify after bgpred:\n");
      fprintf(stderr, "%s\n", buf);
      exit(2);
    }

    opened = true;
  }

  // hand the current predicate to Simplify
  if (!writeAll(toSimplify, str, strlen(str))) {
    perror("write to Simplify");
    exit(2);
  }

  // read Simplify's response
  for (;;) {
    char response[80];
    if (!readString(fromSimplify, response, 80)) {
      perror("read from Simplify");
      exit(2);
    }

    if (strstr(response, "Invalid")) {
      return false;          // formula is not proven
    }
    if (strstr(response, "Valid")) {
      return true;           // formula is proven
    }

    //printf("unexpected response from Simplify:\n");
    //printf("%s\n", response);

    // go back and read some more; this "triggerless quantifier body"
    // thing is coming up..
  }
}
