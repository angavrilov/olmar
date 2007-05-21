//  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *
//  See file license.txt for terms of use                              *
//**********************************************************************

// code for ocsstr.h

#include "ocsstr.h"
#include "xassert.h"
#include "exc.h"


OCSubstrate::OCSubstrate(ReportError *err)
  : EmbeddedLang(err)
{
  reset();
}


void OCSubstrate::reset(int initNest)
{
  state = ST_NORMAL;
  nesting = initNest;
  comment_nesting = 0;
  star = false;
  oparen = false;
  text.setlength(0);
}


OCSubstrate::~OCSubstrate()
{}


void OCSubstrate::handle(char const *str, int len, char finalDelim)
{
# ifdef TEST_OCSSTR
  cout << "oc::handle \"" << substring(str, len) << "\"\n";
# endif
  text.append(str, len);

  for (; len>0; len--,str++) {
    // print_state();
    switch (state) {
    case ST_NORMAL:
      switch (*str) {
      case '(':
	oparen = true;
	star = false;
	break;

      case '*':
	if(oparen) {
	  comment_nesting++;
	  oparen = star = false;
	}
	else {
	  star = true;
	  oparen = false;
	}
	break;

      case ')':
	if(star) {
	  if(comment_nesting == 0) {
#           ifdef TEST_OCSSTR
	    cout << "Error: lonely comment termination ``*)''" << endl;
#           endif
	    err->reportError("lonely comment termination ``*)''");
	  }
	  else
	    comment_nesting--;
	}
	oparen = star = false;
	break;	  
	 
      case '\'':
	if(comment_nesting == 0)
	  state = ST_TICK;
	oparen = star = false;
	break;

      case '{':
	if(comment_nesting == 0)
	  nesting++;
	oparen = star = false;
	break;

      case '}':
	if(comment_nesting == 0) {
	  if(nesting == 0)
	    xfailure("should have left this ocaml verbatim already");
	  nesting--;
	}
	oparen = star = false;
	break;

      case '"':
#       ifdef TEST_OCSSTR
	cout << "Error: ocaml strings are not supported here" << endl;
#       endif
	err->reportError("ocaml strings are not supported here");
	oparen = star = false;
	break;

      default:
	oparen = star = false;
	break;
      }
      break;

    case ST_TICK:
      // looking at the char after a first tick; 
      // look forward now to distinguish the following alternative:
      // - the char following this one is a tick, which then closes a 
      //   char constant
      // - otherwise we saw/see/still expect a type variable and any tick
      //   comming might start again a char constant
      // To be correct we barf on all other character constants
      // (like '\\' or '\"' or '\000' or '\x00' or even '"')

      // Yes: in ocaml the locale determines which characters are allowed 
      // in identifiers! Therefore isalnum is just the right choice!
      if(isalpha(*str) || *str == '_' || 
	 *str == ' ' || *str == '\n' || *str == '\t' || *str == '\r')
	state = ST_TICK_NEXT;
      else {
#       ifdef TEST_OCSSTR
	cout << "Error: ocaml syntax error or unsupported char constant" 
	     << endl;
#       endif
	err->reportError("ocaml syntax error or unsupported char constant");
	state = ST_NORMAL;
      }
      break;

    case ST_TICK_NEXT:
      if(*str == '\'') {
	// character complete, continue
	state = ST_NORMAL;
      }
      // Yes: tick (') might occur in ocaml identifiers
      else if(isalnum(*str) || *str == '_' || 
	      *str == ' ' || *str == '\n' || *str == '\t' || *str == '\r' ||
	      // permit all chars that can follow a typexpr
	      *str == ';' || *str == '*' || *str == ')' || *str == '-' ||
	      *str == '|' || *str == ']' || 
	      *str == '&' || *str == '>' ||
	      *str == ':' || *str == '=' ||
	      *str == ',' 
	      )
	state = ST_NORMAL;
      else if(*str == '}') {
	xassert(comment_nesting == 0);
	if(nesting == 0)
	  xfailure("should have left this ocaml verbatim already");
	nesting--;
	state = ST_NORMAL;
      }
      else {
#       ifdef TEST_OCSSTR
	cout << "Error: ocaml syntax error" << endl;
#       endif
	err->reportError("ocaml syntax error");
	state = ST_NORMAL;
      }
      break;

    default:
      xfailure("unknown state");
    }

#   ifdef TEST_OCSSTR_kk
    cout << " -" << *str << "-> ";
    print_state();
    cout << endl;
#   endif
  }
}


bool OCSubstrate::zeroNesting() const
{
  return nesting == 0 && comment_nesting == 0;
}


// not supported for embedded ocaml
string OCSubstrate::getFuncBody() const
{
  return text;
}


// not supported for embedded ocaml
string OCSubstrate::getDeclName() const
{
  xassert(false);
}


// ------------------ test code -------------------
#ifdef TEST_OCSSTR


void OCSubstrate::print_state() {
  switch(state) {
  case OCSubstrate::ST_NORMAL: 
    cout << "NORMAL";
    break;
  case OCSubstrate::ST_TICK: 
    cout <<"TICK";
    break;
  case OCSubstrate::ST_TICK_NEXT: 
    cout << "TICK_NEXT";
    break;
  default: 
    cout << "??";
  }
  cout << "(nest: " << nesting 
       << ", cnest: " << comment_nesting
       << ", op: " << oparen
       << ", *: " << star << ")";
}


#define OC OCSubstrate
#define Test OCSubstrateTest

// test code is put into a class just so that OCSubstrate
// can grant it access to private fields
class Test {
public:
  void feed(OC &oc, rostring src);
  void test(rostring src, OC::State state, int nesting, int comment_nesting);
  void normal(rostring src, int nesting, int comment_nesting);
  void yes(rostring src);
  void no(rostring src);
  int main();
};


#define min(a,b) ((a)<(b)?(a):(b))

void Test::feed(OC &oc, rostring origSrc)
{
  char const *src = toCStr(origSrc);

  cout << "trying: " << src << endl;
  while (*src) {
    // feed it in 10 char increments, to test split processing too
    int len = min(strlen(src), 10);
    oc.handle(src, len, '}');
    src += len;
  }
}


void Test::test(rostring src, OC::State state, int nesting, int comment_nesting)
{
  OC oc(&silentReportError);
  feed(oc, src);

  if (!( oc.state == state &&
         oc.nesting == nesting &&
	 oc.comment_nesting == comment_nesting &&
	 silentReportError.errors == 0 &&
	 silentReportError.warnings == 0)) {
    xfailure(stringc << "failed on src: " << src);
  }
}


void Test::normal(rostring src, int nesting, int comment_nesting)
{
  test(src, OC::ST_NORMAL, nesting, comment_nesting);
}


void Test::yes(rostring src)
{
  OC oc(&silentReportError);
  feed(oc, src);

  xassert(oc.zeroNesting()&&
	  silentReportError.errors == 0 &&
	  silentReportError.warnings == 0);
}


void Test::no(rostring src)
{
  OC oc(&silentReportError);
  feed(oc, src);

  xassert((!oc.zeroNesting()) &&
	  silentReportError.errors == 0 &&
	  silentReportError.warnings == 0);
}


int Test::main()
{
  // quiet!
  xBase::logExceptions = true;

  normal("'a'{ }", 0, 0);
  normal("'a ", 0, 0);
  normal("(*{ { * '*)", 0 ,0);
  normal("let a'a' = 5", 0, 0);

  yes("a'");
  yes("'a");
  yes("{ (* } *) }");
  yes("type 'a x = { f : 'a; }");
  yes("type 'a x = 'a* int");
  yes("('a)");
  yes("'a->'b");
  yes("[> A of 'a| B of 'b]");
  yes("[< `C|`A of 'a& 'b>`C]");
  yes("(x:'a:>'b)");
  yes("let a : 'a=5;;");
  yes("['a,'b] classpath");
  yes("<a : 'a;..>");
  yes("{f:'a}");

  no("{");

  cout << "\nocsstr: all tests PASSED\n";

  return 0;
}

int main()
{
  Test t;
  return t.main();
}

#endif // TEST_OCSSTR
