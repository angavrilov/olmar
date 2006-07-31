// ocsstr.cc            see license.txt for copyright and terms of use
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
  text.setlength(0);
}


OCSubstrate::~OCSubstrate()
{}


void OCSubstrate::handle(char const *str, int len, char finalDelim)
{
  cout << "oc::handle \"" << substring(str, len) << "\"\n";
  text.append(str, len);

  for (; len>0; len--,str++) {
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
	  if(comment_nesting == 0)
	    err->reportError("lonely comment termination ``*)''");
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
	err->reportError("ocaml strings are not supported here");
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
      if(isalnum(*str) || *str == '_' || 
	 *str == ' ' || *str == '\n' || *str == '\t' || *str == '\r')
	// read or expect a type variable; continue
	state = ST_NORMAL;
      else {
	err->reportError("ocaml syntax error");
	state = ST_NORMAL;
      }
      break;

    default:
      xfailure("unknown state");
    }
  }
}


bool OCSubstrate::zeroNesting() const
{
  return state == ST_NORMAL && nesting == 0;
}


// not supported for embedded ocaml
string OCSubstrate::getFuncBody() const
{
  xassert(false);
}


// not supported for embedded ocaml
string OCSubstrate::getDeclName() const
{
  xassert(false);
}


// ------------------ test code -------------------
#ifdef TEST_OCSSTR

#define OC OCSubstrate
#define Test OCSubstrateTest

// test code is put into a class just so that OCSubstrate
// can grant it access to private fields
class Test {
public:
  void feed(OC &oc, rostring src);
  void test(rostring src, OC::State state, int nesting);
  void normal(rostring src, int nesting);
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


void Test::test(rostring src, OC::State state, int nesting)
{
  OC oc(&silentReportError);
  feed(oc, src);

  if (!( oc.state == state &&
         oc.nesting == nesting)) {
    xfailure(stringc << "failed on src: " << src);
  }
}


void Test::normal(rostring src, int nesting)
{
  test(src, OC::ST_NORMAL, nesting);
}


int Test::main()
{
  // quiet!
  xBase::logExceptions = false;

  cout << "\nocsstr: all tests PASSED\n";

  return 0;
}

int main()
{
  Test t;
  return t.main();
}

#endif // TEST_OCSSTR
