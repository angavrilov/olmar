// sexp.cc
// pretty-printing for sexp.ast

#include "sexp.ast.gen.h"     // this module


string Sexp::toString() const
{
  stringBuilder sb;
  toStringBld(sb);
  return sb;
}


void Sexp::pprint(int ind, stringBuilder &out, int pageWidth) const
{
  int parens = 0;
  ipprint(ind, parens, out, pageWidth);
  xassert(parens == 0);
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

void S_leaf::ipprint(int ind, int &pending, stringBuilder &out, int width) const
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
void S_func::ipprint(
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
    goto eatPending;
  }

  // special handling of EQ/NEQ
  if ((0==strcmp(name, "EQ") || 0==strcmp(name, "NEQ")) &&
      argsToPrint==2) {
    // start first argument on same line, then start
    // second argument so it lines up with first, finally
    // putting close-paren on same line as end of 2nd arg
    out << " ";

    // first arg
    args.nthC(0)->pprint(ind+4, out, pageWidth);

    // second arg
    out << "\n";
    indent(out, ind+4);
    args.nthC(1)->ipprint(ind+4, pendingParens, out, pageWidth);

    // finish this form, and we're done
    out << ")";
    goto eatPending;
  }
  
  // scope to hide 'argCt' from the goto label...
  {
    int argCt=0;
    FOREACH_ASTLIST(Sexp, args, iter) {
      argsToPrint--;
      if (argCt++ == 0) {
        // special handling of quantifiers
        if (0==strcmp(name, "FORALL") ||
            0==strcmp(name, "EXISTS")) {
          // always print the first argument to forall/exists on the
          // same line as the quantifier itself
          out << " " << iter.data()->toString();
          continue;
        }
      }

      // get ready to print it
      out << "\n";
      indent(out, ind+2);

      // see if we're at the last argument
      if (argsToPrint == 0) {
        // about to print last argument; could fold my close-paren
        // (plus any accumulated) into its close-paren line
        pendingParens++;
        iter.data()->ipprint(ind+2, pendingParens, out, pageWidth);
      }
      else {
        // not at the last argument; can't print mine, nor any
        // inherited
        iter.data()->pprint(ind+2, out, pageWidth);
      }
    }
  }

  // if we're going to print at least one close-paren on its own
  // line, collapse all close-parens that could also be printed here
eatPending:
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
  s->pprint(0, sb, width);
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
