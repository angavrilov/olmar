// parssppt.cc
// code for parssppt.h

#include "parssppt.h"     // this module
#include "glr.h"          // toplevelParse
#include "trace.h"        // traceProcessArg

// useful for simple treewalkers
ParseTree * /*owner*/ treeMain(int argc, char **argv)
{
  // remember program name
  char const *progName = argv[0];

  // parameters
  char const *symOfInterestName = NULL;

  // process args
  while (argc >= 2) {
    if (traceProcessArg(argc, argv)) {
      continue;
    }
    else if (0==strcmp(argv[1], "-sym") && argc >= 3) {
      symOfInterestName = argv[2];
      argc -= 2;
      argv += 2;
    }
    else {
      break;     // didn't find any more options
    }
  }

  if (argc != 3) {
    cout << "usage: " << progName << " [options] grammar-file input-file\n"
            "  options:\n"
            "    -tr <sys>:  turn on tracing for the named subsystem\n"
            "    -sym <sym>: name the \"symbol of interest\"\n"
            ;
    exit(0);
  }

  return toplevelParse(argv[1], argv[2], symOfInterestName);
}


