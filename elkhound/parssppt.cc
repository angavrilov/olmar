// parssppt.cc
// code for parssppt.h

#include "parssppt.h"     // this module
#include "glr.h"          // toplevelParse
#include "trace.h"        // traceProcessArg
#include "syserr.h"       // xsyserror
#include "grampar.h"      // readGrammarFile


// ---------------------- ParseTree --------------------
ParseTreeAndTokens::ParseTreeAndTokens(CCLang &L, SemanticValue &top)
  : treeTop(top),
    lexer2(L),
    userAct(NULL)
{}

ParseTreeAndTokens::ParseTreeAndTokens(CCLang &L, SemanticValue &top,
                                       StringTable &extTable)
  : treeTop(top),
    lexer2(L, extTable),
    userAct(NULL)
{}

ParseTreeAndTokens::~ParseTreeAndTokens()
{}


// ---------------------- other support funcs ------------------
// process the input file, and yield a parse graph
bool glrParseNamedFile(GLR &glr, Lexer2 &lexer2, SemanticValue &treeTop,
                       char const *inputFname)
{
  // do first phase lexer
  traceProgress() << "lexical analysis...\n";
  traceProgress(2) << "lexical analysis stage 1...\n";
  Lexer1 lexer1;
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
}


bool glrParseFrontEnd(GLR &glr, Lexer2 &lexer2, SemanticValue &treeTop,
                      char const *grammarFname, char const *inputFname)
{
  glr.readBinaryGrammar(grammarFname);

  // parse input
  return glrParseNamedFile(glr, lexer2, treeTop, inputFname);
}


bool toplevelParse(ParseTreeAndTokens &ptree, char const *grammarFname,
                   char const *inputFname)
{
  // parse
  xassert(ptree.userAct != NULL);    // must have been set by now
  GLR glr(ptree.userAct);
  return glrParseFrontEnd(glr, ptree.lexer2, ptree.treeTop,
                          grammarFname, inputFname);
}


// hack: need classifier to act like the one for Bison
class SimpleActions : public TrivialUserActions {
public:
  virtual int reclassifyToken(int oldTokenType, SemanticValue sval);
};

int SimpleActions::reclassifyToken(int type, SemanticValue)
{
  if (type == L2_NAME) {
    return L2_VARIABLE_NAME;
  }
  else {
    return type;
  }
}



// useful for simple treewalkers
bool treeMain(ParseTreeAndTokens &ptree, int argc, char **argv,
              char const *additionalInfo)
{
  // remember program name
  char const *progName = argv[0];

  // process args
  while (argc >= 2) {
    if (traceProcessArg(argc, argv)) {
      continue;
    }
    #if 0
    else if (0==strcmp(argv[1], "-sym") && argc >= 3) {
      symOfInterestName = argv[2];
      argc -= 2;
      argv += 2;
    }   
    #endif // 0
    else {
      break;     // didn't find any more options
    }
  }

  if (argc != 3) {
    cout << "usage: [env] " << progName << " [options] grammar-file input-file\n"
            "  env:\n"
            "    SYM_OF_INTEREST symbol to watch during analysis\n"
            "  options:\n"
            "    -tr <sys>:      turn on tracing for the named subsystem\n"
            //"    -sym <sym>: name the \"symbol of interest\"\n"
            "  useful tracing flags:\n"
            "    parse           print shift/reduce steps of parsing algorithm\n"
            "    grammar         echo the grammar\n"
            "    ambiguities     print ambiguities encountered during parsing\n"
            "    conflict        SLR(1) shift/reduce conflicts (fork points)\n"
            "    itemsets        print the sets-of-items DFA\n"
            "    ... the complete list is in parsgen.txt ...\n"
         << additionalInfo? additionalInfo : "";
    exit(0);
  }

  if (tracingSys("trivialActions")) {
    // replace current actions with trivial actions
    //delete ptree.userAct;      // the caller does this
    ptree.userAct = new SimpleActions;
    cout << "using trivial (er, simple..) actions\n";
  }
  return toplevelParse(ptree, argv[1], argv[2]);
}
