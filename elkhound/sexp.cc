// sexp.cc
// pretty-printing for sexp.ast

#include "sexp.ast.gen.h"     // this module


string Sexp::toString() const
{
  stringBuilder sb;
  toStringBld(sb);
  return sb;
}


// ----------------------- S_leaf -------------------
void S_leaf::toStringBld(stringBuilder &sb) const
{
  sb << name;
}

int S_leaf::toStringLen() const
{
  return strlen(name);
}

void S_leaf::pprint(int ind, int &pending, stringBuilder &out, int width) const
{
  toStringBld(out);
}


// ------------------------- S_func -------------------------
void S_func::toStringBld(stringBuilder &sb) const
{
  sb << "(" << name;
  FOREACH_ASTLIST(Sexp, args, iter) {
    sb << " ";
    iter.data()->toStringBld(sb);
  }
  sb << ")";
}

int S_func::toStringLen() const
{
  int ret = 2 + strlen(name);
  FOREACH_ASTLIST(Sexp, args, iter) {
    ret += 1 + iter.data()->toStringLen();
  }
  return ret;
}

static void indent(stringBuilder &sb, int ind)
{
  while (ind--) {
    sb << " ";
  }
}

// see comment in sexp.ast
void S_func::pprint(
  int ind,              // current # of spaces from left edge
  int &pendingParens,   // # of close-parens ready to print
  stringBuilder &out,   // output stream
  int pageWidth) const  // maximum # of chars per line
{
  // if linear print will finish before page width, do it
  if (ind + toStringLen() < pageWidth) {
    out << "(" << name;
    FOREACH_ASTLIST(Sexp, args, iter) {
      out << " " << iter.data()->toString();
    }
    out << ")";
    return;
  }

  // break into several lines like
  // (func
  //   (..arg1..)
  //   (..arg2..)
  //   .
  //   .
  //   (..argN..)
  // )
  out << "(" << name;
  int argsToPrint = args.count();
  if (argsToPrint == 0) {
    // the line won't fit anyway; just finish it and continue
    out << ")";
    return;
  }

  FOREACH_ASTLIST(Sexp, args, iter) {
    // get ready to print it
    out << "\n";
    indent(out, ind+2);
                            
    // see if we're at the last argument
    argsToPrint--;
    if (argsToPrint == 0) {
      // about to print last argument; could fold my close-paren
      // (plus any accumulated) into its close-paren line
      pendingParens++;
      iter.data()->pprint(ind+2, pendingParens, out, pageWidth);
    }                           
    else {
      // not at the last argument; can't print mine, nor any
      // inherited
      int parens = 0;
      iter.data()->pprint(ind+2, parens, out, pageWidth);
    }
  }

  // if we're going to print at least one close-paren on its own
  // line, collapse all close-parens that could also be printed here
  if (pendingParens) {
    out << "\n";
    indent(out, ind);
    while (pendingParens) {
      out << ")";
      pendingParens--;
    }
  }
}


S_func *mkSfunc(char const *name, Sexp *arg, ...)
{
  S_func *ret = new S_func(name, NULL);

  // process arguments until encounter one that's NULL
  va_list arglist;
  va_start(arglist, arg);
  while (arg) {
    ret->args.append(arg);
    arg = va_arg(arglist, Sexp*);
  }

  return ret;
}


// --------------------- test code ---------------------
#ifdef TEST_SEXP

#include "test.h"    // USUAL_MAIN

void printSexp(Sexp *s, int width)
{
  cout << "------ width = " << width << " ---------\n";
  stringBuilder sb;
  int parens = 0;
  s->pprint(0, parens, sb, width);
  cout << sb << endl;
}

S_leaf *leaf(char const *n)
{
  return new S_leaf(n);
}


void entry()
{
  Sexp *s = mkSfunc("foo", leaf("bar"), leaf("baz"), NULL);
  printSexp(s, 20);
  printSexp(s, 5);

  Sexp *s2 = mkSfunc("FORALL", mkSfunc("i", leaf("j"), leaf("k"), NULL), s, NULL);
  printSexp(s2, 70);
  printSexp(s2, 20);
  printSexp(s2, 5);

  Sexp *s3 = mkSfunc("EQ", mkSfunc("select", leaf("mem0"), leaf("field1"), NULL),
                           s2, NULL);
  printSexp(s3, 70);
  printSexp(s3, 20);
  printSexp(s3, 5);



}

USUAL_MAIN

#endif // TEST_SEXP
