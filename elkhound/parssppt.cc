// parssppt.cc
// code for parssppt.h

#include "parssppt.h"     // this module
#include "glr.h"          // toplevelParse
#include "trace.h"        // traceProcessArg


// ---------------------- ParseTree --------------------
ParseTreeAndTokens::ParseTreeAndTokens(SemanticValue &top)
  : treeTop(top),
    lexer2()
{}

ParseTreeAndTokens::~ParseTreeAndTokens()
{}


// ---------------------- other support funcs ------------------
bool toplevelParse(ParseTreeAndTokens &ptree, char const *grammarFname,
                   char const *inputFname, char const *symOfInterestName)
{
  // parse
  GLR glr;
  return glr.glrParseFrontEnd(ptree.lexer2, ptree.treeTop,
                              grammarFname, inputFname, 
                              symOfInterestName);
}


// useful for simple treewalkers
bool treeMain(ParseTreeAndTokens &ptree, int argc, char **argv)
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
            "  useful tracing flags:\n"
            "    parse           print shift/reduce steps of parsing algorithm\n"
            "    grammar         echo the grammar\n"
            "    ambiguities     print ambiguities encountered during parsing\n"
            "    conflict        SLR(1) shift/reduce conflicts (fork points)\n"
            "    ... the complete list is in parsgen.txt ...\n"
            ;
    exit(0);
  }

  return toplevelParse(ptree, argv[1], argv[2], symOfInterestName);
}
