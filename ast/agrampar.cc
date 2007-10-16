// agrampar.cc            see license.txt for copyright and terms of use
// code for agrampar.h

#include "agrampar.h"        // this module
#include "agrampar.tab.h"    // YYSTYPE union
#include "gramlex.h"         // GrammarLexer
#include "exc.h"             // xformat
#include "trace.h"           // tracing debugging functions
#include "owner.h"           // Owner
#include "strutil.h"         // trimWhitespace
#include "strtable.h"        // StringTable

#include <string.h>          // strncmp
#include <ctype.h>           // isalnum
#include <fstream.h>         // ifstream


string unbox(string *s)
{
  string ret = *s;      // later optimization: transfer s.p directly
  delete s;
  return ret;
}

string *box(char const *s)
{
  return new string(s);
}

string *appendStr(string *left, string *right)
{
  string *ret = new string(*left & *right);
  delete left;
  delete right;
  return ret;
}


FieldOrCtorArg *parseCtorArg(rostring origStr)
{
  FieldOrCtorArg *ret = new FieldOrCtorArg(FF_NONE, "", "", "");

  // strip leading and trailing whitespace
  string str = trimWhitespace(origStr);

  // check for owner & nullable flags
  bool maybe_owner = true;
  bool maybe_nullable = true;
  bool maybe_circular = true;
  do {
    if(maybe_owner && prefixEquals(str, "owner")) {
      // if somebody could tell me why I need the static cast in the 
      // next line to get it through g++ 3.4/4.1, I would greatly appreciate 
      // an email at tews@cs.ru.nl
      ret->flags = static_cast<FieldFlags>(ret->flags | FF_IS_OWNER);
      maybe_owner = false;
      str = trimWhitespace(str.substring(5, str.length() - 5)); // skip "owner "
    }
    else if(maybe_nullable && prefixEquals(str, "nullable")) {
      ret->flags = static_cast<FieldFlags>(ret->flags | FF_NULLABLE);
      maybe_nullable = false;
      str = trimWhitespace(str.substring(8, str.length() - 8)); // skip nullable
    }
    else if(maybe_circular && prefixEquals(str, "circular")) {
      ret->flags = static_cast<FieldFlags>(ret->flags | FF_CIRCULAR);
      maybe_circular = false;
      str = trimWhitespace(str.substring(8, str.length() - 8)); // skip circular
    }
    // don't treat "xml" prefixed here, because
    //  - it is more diffcult (need to treat all xml.*)
    //  - nobody needs it (well, elsa doesn't) 
    //  - xml.* is therefore not allowed for constructor arguments
    else {
      // no flags found
      maybe_owner = maybe_nullable = false;
    }
      
  } while(maybe_owner || maybe_nullable);

  // check for an initial value
  char const *equals = strchr(str.c_str(), '=');
  if (equals) {
    ret->defaultValue = equals+1;
    str = trimWhitespace(str.substring(0, equals-str.c_str()));
    trace("defaultValue") << "split into `" << str
                          << "' and `" << ret->defaultValue << "'\n";
  }

  // work from the right adge, collecting alphanumerics into the name;
  // this restricts the kinds of C type syntaxes we allow, but you can use
  // typedefs to express any type within these restrictions
  char const *start = str.c_str();
  char const *p = start + str.length() - 1;
  while ((isalnum(*p) || *p == '_') && p > start) {
    p--;
  }
  if (p == start) {
    xformat(stringc << "missing type specifier in \"" << origStr << "\"");
  }
  p++;

  ret->type = trimWhitespace(substring(start, p-start));
  ret->name = trimWhitespace(string(p));

  return ret;
}


// Bison parser calls this to get a token
int agrampar_yylex(YYSTYPE *lvalp, void *parseParam)
{
  ASTParseParams *par = (ASTParseParams*)parseParam;
  GrammarLexer &lexer = par->lexer;

  int code = lexer.yylexInc();

  // yield semantic values for some things
  switch (code) {
    case TOK_NAME:
    case TOK_INTLIT:
      lvalp->str = box(lexer.curToken());
      break;

    case TOK_EMBEDDED_CODE:
      lvalp->str = box(lexer.curFuncBody());
      // HT: reset embedded pointer to provoke segfaults whenever 
      // it is used without my authorization
      lexer.no_embedded();
      break;

    default:
      lvalp->str = NULL;
  }

  static bool traceIt = tracingSys("tokens");
  if (traceIt) {
    ostream &os = trace("tokens");
    os << lexer.curLocStr() << ": " << code;
    if (lvalp->str) {
      os << ", \"" << *(lvalp->str) << "\"";
    }
    os << "\n";
  }

  return code;
}


void agrampar_yyerror(char const *msg, void *parseParam)
{
  ((ASTParseParams*)parseParam)->lexer.err(msg);
}


// ---------------- external interface -------------------
StringTable stringTable;

ASTSpecFile *readAbstractGrammar(char const *fname)
{
  if (tracingSys("yydebug")) {
    #ifndef NDEBUG
      yydebug = true;
    #else
      cout << "debugging disabled by -DNDEBUG\n";
    #endif
  }

  Owner<GrammarLexer> lexer;
  Owner<ifstream> in;
  if (fname == NULL) {
    // stdin
    lexer = new GrammarLexer(isAGramlexEmbed, stringTable);
  }
  else {
    // file
    in = new ifstream(fname);
    if (!*in) {
      throw_XOpen(fname);
    }
    trace("tmp") << "in is " << in.get() << endl;
    lexer = new GrammarLexer(isAGramlexEmbed, stringTable, fname, in.xfr());
  }

  // astgen uses two embedded languages, 
  // therefore reset lexer.embedded here to force proper switching
  lexer->no_embedded();

  ASTParseParams params(*lexer);

  traceProgress() << "parsing grammar source..\n";
  int retval;
  try {
    retval = agrampar_yyparse(&params);
  }
  catch (xFormat &x) {
    lexer->err(x.cond());     // print with line number info
    throw;
  }

  if (retval == 0) {
    return params.treeTop;
  }
  else {
    xformat("parsing finished with an error");
  }
}



// ----------------------- test code -----------------------
#ifdef TEST_AGRAMPAR

#include "test.h"    // ARGS_MAIN

void entry(int argc, char **argv)
{
  TRACE_ARGS();

  if (argc != 2) {
    cout << "usage: " << argv[0] << " ast-spec-file\n";
    return;
  }

  // parse the grammar spec
  Owner<ASTSpecFile> ast;
  ast = readAbstractGrammar(argv[1]);

  // print it out
  ast->debugPrint(cout, 0);
}

ARGS_MAIN

#endif // TEST_AGRAMPAR
