// attrexpr.cc
// code for attrexpr.h

#include "attrexpr.h"     // this module
#include "strtokp.h"      // StrtokParse
#include "glrtree.h"      // Reduction, AttrContext
#include "grammar.h"      // Production
#include "attr.h"         // Attributes

#include <ctype.h>        // isspace
#include <stdlib.h>       // atoi


// --------------- AttrLvalue ---------------------
AttrLvalue::~AttrLvalue()
{}

STATICDEF AttrLvalue AttrLvalue::
  parseRef(Production const *prod, char const *refString)
{
  // parse refString into name, tag (optional), and field
  StrtokParse tok(refString, ".");
  if (tok < 2 || tok > 3) {
    xfailure("attribute reference has too many or too few fields");
  }
  
  string name = tok[0];	       // first
  string field = tok[tok-1];   // last
  string tag(NULL);
  if (tok == 3) {
    tag = tok[1];	       // middle
  }

  // search in the production for this name and tag
  int index = prod->findTaggedSymbol(name, tag);
  if (index == -1) {
    // didn't find it
    xfailure(stringb("couldn't find symbol for " << refString));
  }

  // make an AttrLvalue
  return AttrLvalue(index, field);
}


string AttrLvalue::toString(Production const *prod) const
{
  return stringc << prod->taggedSymbolName(symbolIndex)
                 << "." << attrName;
}


Attributes const &AttrLvalue::getNamedAttrC(AttrContext const &actx) const
{
  if (symbolIndex == 0) {
    return actx.parentAttrC();
  }

  else {
    return actx.reductionC().children.nthC(symbolIndex-1)->attr;
  }
}


int AttrLvalue::getFrom(AttrContext const &actx) const
{
  return getNamedAttrC(actx).get(attrName);
}


void AttrLvalue::storeInto(AttrContext &actx, int newValue) const
{
  getNamedAttr(actx).set(attrName, newValue);
}



// ---------------------- AExprNode --------------------------
AExprNode::~AExprNode()
{}


// --------------------- AExprLiteral ------------------------
int AExprLiteral::eval(AttrContext const &) const
{					  
  return value;
}


string AExprLiteral::toString(Production const *) const
{
  return stringb(value);
}


// ------------------- AExprAttrRef --------------------------
AExprAttrRef::~AExprAttrRef()
{}


int AExprAttrRef::eval(AttrContext const &actx) const
{
  return ref.getFrom(actx);
}

string AExprAttrRef::toString(Production const *prod) const
{
  return ref.toString(prod);
}


// --------------------- AExprFunc ---------------------------
AExprFunc::AExprFunc(char const *name)
  : func(NULL),     // set below
    args()
{
  // map the name into a function
  INTLOOP(i, 0,	numFuncEntries) {
    if (0==strcmp(name, funcEntries[i].name)) {
      func = &funcEntries[i];
      break;
    }
  }
  
  if (func == NULL) {
    xfailure(stringc << "nonexistent function: " << name);
  }
}


AExprFunc::~AExprFunc()
{}


#define MAKE_OP_FUNC(name, op) \
  static int name(int a, int b) { return a op b; }
MAKE_OP_FUNC(plus, +)
MAKE_OP_FUNC(equals, ==)
MAKE_OP_FUNC(nequals, !=)
MAKE_OP_FUNC(less, <)
MAKE_OP_FUNC(lesseq, <=)

int ifFunc(int cond, int thenExp, int elseExp)
{
  return cond? thenExp : elseExp;
}

/*
  struct FuncEntry {   	       	   // one per predefined func
    char const *name;                // name of fn (e.g. "+")
    int numArgs;                     // # of arguments it requires
    Func eval;                       // code to evaluate
  };
*/
AExprFunc::FuncEntry const AExprFunc::funcEntries[] = {
  { "+", 2, (AExprFunc::Func)plus },
  { "==", 2, (AExprFunc::Func)equals },
  { "!=", 2, (AExprFunc::Func)nequals },
  { "<", 2, (AExprFunc::Func)less },
  { "<=", 2, (AExprFunc::Func)lesseq },
  { "if", 3, (AExprFunc::Func)ifFunc },
};


int const AExprFunc::numFuncEntries =
  TABLESIZE(AExprFunc::funcEntries);


int AExprFunc::eval(AttrContext const &actx) const
{
  if (args.count() != func->numArgs) {
    xfailure(stringb("called " << func->name << " with " <<
                     args.count() << " arguments"));
  }

  // can't dynamically construct an argument list because C doesn't
  // provide a way
  switch (func->numArgs) {
    case 0:
      return func->eval();

    case 1:
      return func->eval(args.nthC(0)->eval(actx));

    case 2:
      return func->eval(args.nthC(0)->eval(actx), args.nthC(1)->eval(actx));

    case 3:
      return func->eval(args.nthC(0)->eval(actx),
                        args.nthC(1)->eval(actx),
                        args.nthC(2)->eval(actx));

    default:
      xfailure("too many arguments; can only handle 3 right now");
      return 0;     // silence warning
  }
}


string AExprFunc::toString(Production const *prod) const
{
  stringBuilder sb;
  sb << "(" << func->name;

  FOREACH_OBJLIST(AExprNode, args, iter) {
    sb << " " << iter.data()->toString(prod);
  }

  sb << ")";
  return sb;
}


// ---------------------- other funcs -------------------------
static void skipws(char const *&text)
{
  while (isspace(*text)) {
    text++;
  }
}

// special in the sense of having meaning to the parser
static bool isspecial(char c)
{
  return c=='(' || c==')' || c==0 || isspace(c);
}

static string collectName(char const *&text)
{
  stringBuilder sb;
  while (!isspecial(*text)) {
    sb << *text;
    text++;
  }
  return sb;
}


static int collectInt(char const *&text)
{
  string num = collectName(text);
  return atoi(num);
}


// parse thing at 'text', moving it to first char beyond thing parsed
AExprNode *recursiveParse(Production const *prod, char const *&text)
{
  // examine first character
  skipws(text);

  if (*text == 0) {
    xfailure("can't parse an empty string as an expression");
    return NULL;     // silence warning
  }

  else if (*text == ')') {
    xfailure("unexpected `)'");
    return NULL;     // silence warning
  }

  else if (*text == '(') {
    // parse this as a function expression; first, get name of function
    text++;
    skipws(text);

    if (isspecial(*text)) {
      xfailure("expected a name after `('");
    }

    // create the exprnode to hold it
    AExprFunc *func = new AExprFunc(collectName(text));   // (owner) (leaked if exception thrown)

    // parse arguments
    while (1) {
      skipws(text);
      if (*text == ')') {
        text++;
        break;
      }

      func->args.append(recursiveParse(prod, text));
    }

    return func;
  }

  else if (isdigit(*text)) {
    // literal
    return new AExprLiteral(collectInt(text));
  }

  else {
    // attribute reference
    return new AExprAttrRef(AttrLvalue::parseRef(prod, collectName(text)));
  }
}


AExprNode *parseAExpr(Production const *prod, char const *text)
{
  return recursiveParse(prod, text);
}
