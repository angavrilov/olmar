// attrexpr.cc
// code for attrexpr.h

#include "attrexpr.h"     // this module
#include "strtokp.h"      // StrtokParse
#include "glrtree.h"      // Reduction, AttrContext
#include "grammar.h"      // Production
#include "attr.h"         // Attributes
#include "gramast.h"      // ast type constants
#include "exc.h"          // xBase
#include "flatutil.h"     // Flatten, xfer helpers

#include <ctype.h>        // isspace
#include <stdlib.h>       // atoi


// --------------- AttrLvalue ---------------------
AttrLvalue::~AttrLvalue()
{}


AttrLvalue::AttrLvalue(Flatten &flat)
  : attrName(flat)
{}

void AttrLvalue::xfer(Flatten &flat)
{
  flat.xferInt(whichTree);
  flat.xferInt(symbolIndex);
  attrName.xfer(flat);
}


STATICDEF AttrLvalue AttrLvalue::
  parseRef(Production const *prod, char const *refString)
{
  // parse refString into tag  and field
  StrtokParse tok(refString, ".");
  if (tok != 2) {
    xfailure("attribute reference has too many or too few fields");
  }

  string tag = tok[0];
  string field = tok[1];

  // search in the production for this name and tag
  int index = prod->findTag(tag);
  if (index == -1) {
    // didn't find it
    xfailure(stringb("couldn't find symbol for " << refString));
  }

  // make an AttrLvalue
  return AttrLvalue(0 /*which*/, index, field);
}


string AttrLvalue::toString(Production const *prod) const
{
  return stringc << prod->symbolTag(symbolIndex)
                 << "." << attrName;
}


void AttrLvalue::check(Production const *ctx) const
{
  // these exception messages are most meaningful if they can
  // be attached to the context of the action or condition in
  // which they occur; so it is assumed that context will be
  // supplied by an exception handler higher up on the call chain

  Symbol const *sym = ctx->symbolByIndexC(symbolIndex);
  if (!sym->isNonterminal()) {
    THROW(xBase(stringc << "symbol index " << symbolIndex
                        << " refers to a terminal!"));
  }

  if (!sym->asNonterminalC().hasAttribute(attrName)) {
    THROW(xBase(stringc << "`" << attrName << "' is not an attribute "
                           "of symbol " << sym->name));
  }
}


int AttrLvalue::getFrom(AttrContext const &actx) const
{
  if (symbolIndex == 0) {
    return actx.reductionC(whichTree).getAttrValue(attrName);
  }
  else {
    return actx.reductionC(whichTree).children.nthC(symbolIndex-1)->
             getAttrValue(attrName);
  }
}


void AttrLvalue::storeInto(AttrContext &actx, int newValue) const
{
  if (symbolIndex == 0) {
    actx.reduction(whichTree).setAttrValue(attrName, newValue);
  }
  else {
    actx.reduction(whichTree).children.nth(symbolIndex-1)->
      setAttrValue(attrName, newValue);
  }
}


// ---------------------- AExprNode --------------------------
AExprNode::~AExprNode()
{}


void AExprNode::xfer(Flatten &flat)
{
  if (flat.writing()) {
    // write the tag
    flat.writeInt((int)getTag());
  }
  else {
    // the tag gets read by 'readObj', below
  }
}


STATICDEF AExprNode *AExprNode::readObj(Flatten &flat)
{
  // read the tag
  Tag tag = (Tag)flat.readInt();
  formatAssert(0 <= tag && tag < NUM_TAGS);

  // create the object
  AExprNode *ret;
  switch (tag) {
    default: xfailure("bad tag");
    case T_LITERAL:   ret = new AExprLiteral(flat);    break;
    case T_ATTRREF:   ret = new AExprAttrRef(flat);    break;
    case T_FUNC:      ret = new AExprFunc(flat);       break;
  }

  ret->xfer(flat);
  return ret;
}


void AExprNode::check(Production const *) const
{}


// --------------------- AExprLiteral ------------------------
AExprLiteral::AExprLiteral(Flatten &flat)
  : AExprNode(flat)
{}

void AExprLiteral::xfer(Flatten &flat)
{
  AExprNode::xfer(flat);
  flat.xferInt(value);
}


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


AExprAttrRef::AExprAttrRef(Flatten &flat)
  : AExprNode(flat),
    ref(flat)
{}

void AExprAttrRef::xfer(Flatten &flat)
{
  AExprNode::xfer(flat);
  ref.xfer(flat);
}


int AExprAttrRef::eval(AttrContext const &actx) const
{
  return ref.getFrom(actx);
}

void AExprAttrRef::check(Production const *ctx) const
{
  ref.check(ctx);
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


AExprFunc::AExprFunc(Flatten &flat)
  : AExprNode(flat),
    func(NULL)
{}

void AExprFunc::xfer(Flatten &flat)
{
  AExprNode::xfer(flat);
  
  // xfer the function index
  if (flat.writing()) {
    flat.writeInt(func - funcEntries);
  }
  else {
    func = funcEntries + flat.readInt();
    formatAssert(0 <= func - funcEntries &&
                      func - funcEntries < numFuncEntries);
  }

  // xfer the args list
  xferObjList_readObj(flat, args);
}


#define MAKE_OP_FUNC(name, op) \
  static int name(int a, int b) { return a op b; }
MAKE_OP_FUNC(plus, +)
MAKE_OP_FUNC(minus, -)
MAKE_OP_FUNC(times, *)
MAKE_OP_FUNC(dividedBy, /)
MAKE_OP_FUNC(equals, ==)
MAKE_OP_FUNC(nequals, !=)
MAKE_OP_FUNC(less, <)
MAKE_OP_FUNC(lesseq, <=)
MAKE_OP_FUNC(oror, ||)

int ifFunc(int cond, int thenExp, int elseExp)
{
  return cond? thenExp : elseExp;
}

/*
  struct FuncEntry {   	       	   // one per predefined func
    char const *name;                // name of fn (e.g. "+")
    int astTypeCode;                 // ast node type that represents this fn (see gramast.h)
    int numArgs;                     // # of arguments it requires
    Func eval;                       // code to evaluate
  };
*/
AExprFunc::FuncEntry const AExprFunc::funcEntries[] = {
  { "+",  EXP_PLUS,  2, (AExprFunc::Func)plus },
  { "-",  EXP_MINUS, 2, (AExprFunc::Func)minus },
  { "*",  EXP_MULT,  2, (AExprFunc::Func)times },
  { "/",  EXP_DIV,   2, (AExprFunc::Func)dividedBy },
  { "==", EXP_EQ,    2, (AExprFunc::Func)equals },
  { "!=", EXP_NEQ,   2, (AExprFunc::Func)nequals },
  { "<",  EXP_LT,    2, (AExprFunc::Func)less },
  { "<=", EXP_LTE,   2, (AExprFunc::Func)lesseq },
  { "||", EXP_OR,    2, (AExprFunc::Func)oror },
  { "if", EXP_COND,  3, (AExprFunc::Func)ifFunc },
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


void AExprFunc::check(Production const *ctx) const
{
  FOREACH_OBJLIST(AExprNode, args, iter) {
    iter.data()->check(ctx);
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
