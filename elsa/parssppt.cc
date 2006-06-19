// parssppt.cc            see license.txt for copyright and terms of use
// code for parssppt.h

#include "parssppt.h"     // this module
#include "glr.h"          // toplevelParse
#include "trace.h"        // traceProcessArg
#include "syserr.h"       // xsyserror

#include <stdlib.h>       // exit


// ---------------------- ParseTree --------------------
ParseTreeAndTokens::ParseTreeAndTokens(CCLang &L, SemanticValue &top,
                                       StringTable &extTable, char const *fname)
  : treeTop(top),
    lexer(new Lexer(extTable, L, fname)),
    userAct(NULL),
    tables(NULL)
{}

ParseTreeAndTokens::~ParseTreeAndTokens()
{
  delete lexer;
}


// ---------------------- other support funcs ------------------
// process the input file, and yield a parse graph
bool glrParseNamedFile(GLR &glr, LexerInterface &lexer, SemanticValue &treeTop,
                       char const *inputFname)
{
  #if 0    // old
  // do first phase lexer
  traceProgress() << "lexical analysis...\n";
  traceProgress(2) << "lexical analysis stage 1...\n";
  Lexer1 lexer1(inputFname);
  {
    FILE *input = fopen(inputFname, "r");
    if (!input) {
      xsyserror("fopen", inputFname);
    }

    lexer1_lex(lexer1, input);
    fclose(input);

    if (lexer1.errors > 0) {
      printf("L1: %d error(s)\n", lexer1.errors);
      return false;
    }
  }

  // do second phase lexer
  traceProgress(2) << "lexical analysis stage 2...\n";
  lexer2_lex(lexer2, lexer1, inputFname);

  // parsing itself
  lexer2.beginReading();
  return glr.glrParse(lexer2, treeTop);
  #endif // 0

  PRETEND_USED(inputFname);     // stupid module..
  return glr.glrParse(lexer, treeTop);
}


bool toplevelParse(ParseTreeAndTokens &ptree, char const *inputFname)
{
  // parse
  xassert(ptree.userAct != NULL);    // must have been set by now
  xassert(ptree.tables != NULL);

  GLR glr(ptree.userAct, ptree.tables);

  // parse input
  return glrParseNamedFile(glr, *ptree.lexer, ptree.treeTop, inputFname);
}


// hack: need classifier to act like the one for Bison
class SimpleActions : public TrivialUserActions {
public:
  virtual ReclassifyFunc getReclassifier();
  static int reclassifyToken(UserActions *ths,
    int oldTokenType, SemanticValue sval);
};

UserActions::ReclassifyFunc SimpleActions::getReclassifier()
{
  if (tracingSys("identityReclassify")) {
    // don't reclassify anything
    return &TrivialUserActions::reclassifyToken;
  }
  else {
    // reclassify as if typedef's weren't possible
    return &SimpleActions::reclassifyToken;
  }
}

STATICDEF int SimpleActions::reclassifyToken(UserActions *, int type, SemanticValue)
{
  if (type == TOK_NAME) {
    return TOK_VARIABLE_NAME;
  }
  else {
    return type;
  }
}


char *processArgs(int argc, char **argv, char const *additionalInfo) {
  // remember program name
  char const *progName = argv[0];

  // process args
  while (argc >= 2) {
    if (traceProcessArg(argc, argv)) {
      continue;
    }
#if 0
    else if (streq(argv[1], "-sym") && argc >= 3) {
      symOfInterestName = argv[2];
      argc -= 2;
      argv += 2;
    }   
#endif // 0
    else {
      break;     // didn't find any more options
    }
  }

  if (argc != 2) {
    cout << "usage: " << progName << " [options] input-file\n"
//            "  env:\n"
//            "    SYM_OF_INTEREST symbol to watch during analysis\n"
            "  options:\n"
            "    -tr <flags>:       turn on given tracing flags (comma-separated)\n"
            //"    -sym <sym>: name the \"symbol of interest\"\n"
//              "  useful tracing flags:\n"
//              "    parse           print shift/reduce steps of parsing algorithm\n"
//              "    grammar         echo the grammar\n"
//              "    ambiguities     print ambiguities encountered during parsing\n"
//              "    conflict        SLR(1) shift/reduce conflicts (fork points)\n"
//              "    itemsets        print the sets-of-items DFA\n"
//            "    ... the complete list is in parsgen.txt ...\n"
         << (additionalInfo? additionalInfo : "");
    exit(argc==1? 0 : 2);    // error if any args supplied
  }

  return argv[1];
}

void maybeUseTrivialActions(ParseTreeAndTokens &ptree) 
{
  if (tracingSys("trivialActions")) {
    // replace current actions with trivial actions
    //delete ptree.userAct;      // the caller does this
    ptree.userAct = new SimpleActions;
    cout << "using trivial (er, simple..) actions\n";
  }
}

// useful for simple treewalkers
bool treeMain(ParseTreeAndTokens &ptree, int argc, char **argv,
              char const *additionalInfo)
{
  char const *positionalArg = processArgs(argc, argv, additionalInfo);
  maybeUseTrivialActions(ptree);
  return toplevelParse(ptree, positionalArg);
}
