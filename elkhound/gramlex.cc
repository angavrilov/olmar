// gramlex.cc
// code for gramlex.h

#include "gramlex.h"     // this module
#include "trace.h"       // debugging trace()
#include "ccsstr.h"      // CCSubstrate

#include <fstream.h>     // cout, ifstream


// ------------------- EmbeddedLang -------------------
EmbeddedLang::EmbeddedLang()
  : text(),
    exprOnly(false),
    isDeclaration(false)
{}

EmbeddedLang::~EmbeddedLang()
{}

                                  
// ----------------- GrammarLexer::FileState --------------------
GrammarLexer::FileState::FileState(SourceFile *file, istream *src)
  : SourceLocation(file),
    source(src),
    bufstate(NULL)
{}


GrammarLexer::FileState::~FileState()
{
  // we let ~GrammarLexer take care of deletions here since we
  // have to know what ~yyFlexLexer is going to do, and we
  // don't have enough context here to know that
}


GrammarLexer::FileState::FileState(FileState const &obj)
{
  *this = obj;
}


GrammarLexer::FileState &GrammarLexer::FileState::
  operator= (FileState const &obj)
{
  if (this != &obj) {
    SourceLocation::operator=(obj);
    source = obj.source;
    bufstate = obj.bufstate;
  }
  return *this;
}


// ---------------------- GrammarLexer --------------------------
GrammarLexer::GrammarLexer(char const *fname, istream *source)
  : yyFlexLexer(source),
    fileState(sourceFileList.open(fname), source),
    fileStack(),
    tokenStartLoc(),
    expectingEmbedded(false),
    embedFinish(0),
    embedMode(0),
    embedded(new CCSubstrate),
    commentStartLine(0),
    integerLiteral(0),
    stringLiteral(""),
    includeFileName("")
{
  // grab initial buffer object so we can restore it after
  // processing an include file (turns out this doesn't work
  // because it's NULL now; see recursivelyProcess())
  fileState.bufstate = yy_current_buffer;
}

GrammarLexer::~GrammarLexer()
{
  // ~yyFlexLexer deletes its current buffer, but not any
  // of the istream sources it's been passed.  
  
  // first let's unpop any unpopped input files
  while (hasPendingFiles()) {
    popRecursiveFile();
  }
  
  // now delete the original istream source
  if (fileState.source != cin) {
    delete fileState.source;
  }
  
  delete embedded;
}


int GrammarLexer::yylexInc()
{                    
  // get raw token
  int code = yylex();

  // include processing
  if (code == TOK_INCLUDE) {
    string fname = includeFileName;
                                       
    // 'in' will be deleted in ~GrammarLexer
    ifstream *in = new ifstream(fname);
    if (!*in) {
      err(stringc << "unable to open include file `" << fname << "'");
    }
    else {
      recursivelyProcess(fname, in);
    }

    // go to next token (tail recursive)
    return yylexInc();
  }

  if (code == TOK_EOF  &&  hasPendingFiles()) {
    popRecursiveFile();
    return yylexInc();
  }


  if (code == TOK_FUNDECL_BODY || code == TOK_FUN_BODY || code == TOK_DECL_BODY) {
    trace("lex") << "yielding embedded (" << code << ") at "
                 << curLocStr() << ": "
                 << curFuncBody() << endl;
  }
  else {
    trace("lex") << "yielding token (" << code << ") "
                 << curToken() << " at "
                 << curLocStr() << endl;
  }

  // nothing special
  return code;
}


string GrammarLexer::curToken() const
{
  // hack around bad decl in FlexLexer.h
  GrammarLexer *ths = const_cast<GrammarLexer*>(this);

  return string(ths->YYText(), ths->YYLeng());
}


string GrammarLexer::curFuncBody() const
{
  return embedded->getFuncBody();
}


string GrammarLexer::curDeclName() const
{
  return embedded->getDeclName();
}


string GrammarLexer::curLocStr() const
{
  return curLoc().toString();
}


void GrammarLexer::err(char const *msg)
{
  cerr << "lexer error at " << curLocStr() << ": " << msg << endl;
}


void GrammarLexer::errorUnterminatedComment()
{
  err(stringc << "unterminated comment, beginning on line " << commentStartLine);
}

void GrammarLexer::errorMalformedInclude()
{
  err(stringc << "malformed include");
}

void GrammarLexer::errorIllegalCharacter(char ch)
{
  err(stringc << "illegal character: `" << ch << "'");
}


void GrammarLexer::recursivelyProcess(char const *fname, istream *source)
{
  trace("lex") << "recursively processing " << fname << endl;
                       
  // grab current buffer; this is necessary because when we
  // tried to grab it in the ctor it was NULL
  fileState.bufstate = yy_current_buffer;
  xassert(fileState.bufstate);

  // push current state
  fileStack.prepend(new FileState(fileState));

  // reset current state
  fileState = FileState(sourceFileList.open(fname), source);

  // storing this in 'bufstate' is redundant because of the
  // assignment above, but no big deal
  fileState.bufstate = yy_create_buffer(source, lexBufferSize);

  // switch underlying lexer over to new file
  yy_switch_to_buffer(fileState.bufstate);
}


void GrammarLexer::popRecursiveFile()
{
  trace("lex") << "done processing " << fileState.fname() << endl;

  // among other things, this prevents us from accidentally deleting
  // flex's first buffer (which it presumably takes care of) or
  // deleting 'cin'
  xassert(hasPendingFiles());

  // close down stuff associated with current file
  yy_delete_buffer(fileState.bufstate);
  delete fileState.source;
  
  // pop stack
  FileState *st = fileStack.removeAt(0);
  fileState = *st;
  delete st;
  
  // point flex at the new (old) buffer
  yy_switch_to_buffer(fileState.bufstate);
}


bool GrammarLexer::hasPendingFiles() const
{
  return fileStack.isNotEmpty();
}



#ifdef TEST_GRAMLEX

int main(int argc)
{
  GrammarLexer lexer;
  traceAddSys("lex");

  cout << "go!\n";

  while (1) {
    // any argument disables include processing
    int code = argc==1? lexer.yylexInc() : lexer.yylex();
    if (code == 0) {  // eof
      break;
    }

    switch (code) {
      case TOK_INCLUDE:
        // if I use yylexInc above, this is never reached
        cout << "include at " << lexer.curLocStr()
             << ": filename is `" << lexer.includeFileName.pcharc()
             << "'\n";
        break;

      case TOK_FUNDECL_BODY:
      case TOK_FUN_BODY:
      case TOK_DECL_BODY:
        cout << "embedded code at " << lexer.curLocStr()
             << ": " << lexer.curFuncBody()
             << endl;

      default:
        cout << "token at " << lexer.curLocStr()
             << ": code=" << code
             << ", text: " << lexer.curToken().pcharc()
             << endl;
    }
  }

  return 0;
}

#endif // TEST_GRAMLEX
