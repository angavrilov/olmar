// ccparse.cc            see license.txt for copyright and terms of use
// code for ccparse.h

#include "ccparse.h"      // this module
#include "cc_ast.h"       // ASTVisitor
#include "trace.h"        // TRACE

#include <iostream.h>     // cout


// ----------------------- ParseEnv -----------------------
char const *maybeNull(StringRef n)
{
  if (n) {
    return n;
  }
  else {
    return "(null)";
  }
}

void ParseEnv::pushClassName(StringRef n)
{
  TRACE("className", "pushing " << maybeNull(n));
  classNameStack.push(n);
}

void ParseEnv::popClassName()
{
  StringRef n = classNameStack.pop();
  TRACE("className", "popping " << maybeNull(n));
  PRETEND_USED(n);
}


SimpleTypeId ParseEnv::uberSimpleType(SourceLoc loc, UberModifiers m)
{
  m = (UberModifiers)(m & UM_TYPEKEYS);

  // implement cppstd Table 7, p.109
  switch (m) {
    case UM_CHAR:                         return ST_CHAR;
    case UM_UNSIGNED | UM_CHAR:           return ST_UNSIGNED_CHAR;
    case UM_SIGNED | UM_CHAR:             return ST_SIGNED_CHAR;
    case UM_BOOL:                         return ST_BOOL;
    case UM_UNSIGNED:                     return ST_UNSIGNED_INT;
    case UM_UNSIGNED | UM_INT:            return ST_UNSIGNED_INT;
    case UM_SIGNED:                       return ST_INT;
    case UM_SIGNED | UM_INT:              return ST_INT;
    case UM_INT:                          return ST_INT;
    case UM_UNSIGNED | UM_SHORT | UM_INT: return ST_UNSIGNED_SHORT_INT;
    case UM_UNSIGNED | UM_SHORT:          return ST_UNSIGNED_SHORT_INT;
    case UM_UNSIGNED | UM_LONG | UM_INT:  return ST_UNSIGNED_LONG_INT;
    case UM_UNSIGNED | UM_LONG:           return ST_UNSIGNED_LONG_INT;
    case UM_SIGNED | UM_LONG | UM_INT:    return ST_LONG_INT;
    case UM_SIGNED | UM_LONG:             return ST_LONG_INT;
    case UM_LONG | UM_INT:                return ST_LONG_INT;
    case UM_LONG:                         return ST_LONG_INT;
    case UM_SIGNED | UM_SHORT | UM_INT:   return ST_SHORT_INT;
    case UM_SIGNED | UM_SHORT:            return ST_SHORT_INT;
    case UM_SHORT | UM_INT:               return ST_SHORT_INT;
    case UM_SHORT:                        return ST_SHORT_INT;
    case UM_WCHAR_T:                      return ST_WCHAR_T;
    case UM_FLOAT:                        return ST_FLOAT;
    case UM_DOUBLE:                       return ST_DOUBLE;
    case UM_LONG | UM_DOUBLE:             return ST_LONG_DOUBLE;
    case UM_VOID:                         return ST_VOID;

    // GNU extensions
    case UM_UNSIGNED | UM_LONG_LONG | UM_INT:  return ST_UNSIGNED_LONG_LONG;
    case UM_UNSIGNED | UM_LONG_LONG:           return ST_UNSIGNED_LONG_LONG;
    case UM_SIGNED | UM_LONG_LONG | UM_INT:    return ST_LONG_LONG;
    case UM_SIGNED | UM_LONG_LONG:             return ST_LONG_LONG;
    case UM_LONG_LONG | UM_INT:                return ST_LONG_LONG;
    case UM_LONG_LONG:                         return ST_LONG_LONG;
    
    // C99/GNU complex types
    case UM_FLOAT | UM_COMPLEX:                return ST_FLOAT_COMPLEX;
    case UM_DOUBLE | UM_COMPLEX:               return ST_DOUBLE_COMPLEX;
    case UM_LONG | UM_DOUBLE | UM_COMPLEX:     return ST_LONG_DOUBLE_COMPLEX;

    // C99 imaginary types
    case UM_FLOAT | UM_IMAGINARY:              return ST_FLOAT_IMAGINARY;
    case UM_DOUBLE | UM_IMAGINARY:             return ST_DOUBLE_IMAGINARY;
    case UM_LONG | UM_DOUBLE | UM_IMAGINARY:   return ST_LONG_DOUBLE_IMAGINARY;

    default:
      error(loc, stringc << "malformed type: " << toString(m));
      return ST_ERROR;
  }
}


UberModifiers ParseEnv
  ::uberCombine(SourceLoc loc, UberModifiers m1, UberModifiers m2)
{
  // check for long long (GNU extension)
  if (m1 & m2 & UM_LONG) {
    // were there already two 'long's?
    if ((m1 | m2) & UM_LONG_LONG) {
      error(loc, "too many `long's");
    }

    // make it look like only m1 had 'long long' and neither had 'long'
    m1 = (UberModifiers)((m1 & ~UM_LONG) | UM_LONG_LONG);
    m2 = (UberModifiers)(m2 & ~(UM_LONG | UM_LONG_LONG));
  }

  // any duplicate flags?
  UberModifiers dups = (UberModifiers)(m1 & m2);
  if (!lang.isCplusplus) {
    // C99 6.7.3p4: const/volatile/restrict can be redundantly combined
    dups = (UberModifiers)(dups & ~UM_CVFLAGS);
  }
  if (dups) {
    // C++ 7.1.5p1
    error(loc, stringc << "duplicate modifier: " << toString(dups));
  }

  return (UberModifiers)(m1 | m2);
}


LocString * /*owner*/ ParseEnv::ls(SourceLoc loc, char const *name)
{
  return new LocString(loc, str(name));
}


void ParseEnv::error(SourceLoc loc, char const *msg)
{
  cout << toString(loc) << ": error: " << msg << endl;
  errors++;
}


void ParseEnv::warning(SourceLoc loc, char const *msg)
{
  cout << toString(loc) << ": warning: " << msg << endl;
  warnings++;
}


void ParseEnv::diagnose3(Bool3 b, SourceLoc loc, char const *msg)
{
  if (!b) {
    error(loc, msg);
  }
  else if (b == B3_WARN) {
    warning(loc, msg);
  }
}


// ---------------------- AmbiguityChecker -----------------
// check for ambiguities
// (I don't want this inheriting from LoweredASTVisitor; I just want
// to count ambiguities in the base AST.)
// NOT Intended to be used with LoweredASTVisitor
class AmbiguityChecker : public ASTVisitor {
public:
  int ambiguousNodes;    // count of nodes with non-NULL ambiguity links

public:
  AmbiguityChecker() : ambiguousNodes(0) {}

  virtual void foundAmbiguous(char const *kind, SourceLoc loc);

  // check each of the kinds of nodes that have ambiguity links
  bool visitTopForm(TopForm *obj);
  bool visitASTTypeId(ASTTypeId *obj);
  bool visitDeclarator(Declarator *obj);
  bool visitStatement(Statement *obj);
  bool visitCondition(Condition *obj);
  bool visitExpression(Expression *obj);
  bool visitArgExpression(ArgExpression *obj);
  bool visitTemplateParameter(TemplateParameter *obj);
  bool visitTemplateArgument(TemplateArgument *obj);
  bool visitPQName(PQName *obj);
};


void AmbiguityChecker::foundAmbiguous(char const *kind, SourceLoc loc)
{
  ambiguousNodes++;
}


#define VISIT(type)                                         \
  bool AmbiguityChecker::visit##type(type *obj)             \
  {                                                         \
    if (obj->ambiguity) {                                   \
      foundAmbiguous(obj->kindName(), getASTNodeLoc(obj));  \
    }                                                       \
    return true;                                            \
  }

VISIT(TopForm)
VISIT(ASTTypeId)
VISIT(Declarator)
VISIT(Statement)
VISIT(Condition)
VISIT(Expression)
VISIT(ArgExpression)
VISIT(TemplateParameter)
VISIT(TemplateArgument)

#undef VISIT

bool AmbiguityChecker::visitPQName(PQName *obj)
{
  if (obj->isPQ_qualifier() &&
      obj->asPQ_qualifier()->ambiguity) {
    ambiguousNodes++;
  }
  return true;
}


template <class T>
int numAmbiguousNodes_impl(T *t)
{
  AmbiguityChecker c;
  t->traverse(c);
  return c.ambiguousNodes;
}


int numAmbiguousNodes(TranslationUnit *unit)
{ return numAmbiguousNodes_impl(unit); }

int numAmbiguousNodes(Statement *stmt)
{ return numAmbiguousNodes_impl(stmt); }

int numAmbiguousNodes(Expression *e)
{ return numAmbiguousNodes_impl(e); }

int numAmbiguousNodes(ASTTypeId *t)
{ return numAmbiguousNodes_impl(t); }


// ---------------- AmbiguityRejecter --------------------
class AmbiguityRejecter : public AmbiguityChecker {
public:
  virtual void foundAmbiguous(char const *kind, SourceLoc loc);
};

void AmbiguityRejecter::foundAmbiguous(char const *kind, SourceLoc loc)
{
  xfatal(toString(loc) << ": internal error: found ambiguous " << kind);
}

void rejectAmbiguousNodes(TranslationUnit *unit)
{
  AmbiguityRejecter r;
  unit->traverse(r);
}


// --------------------- LocationSearcher ------------------
LocationSearcher::LocationSearcher()
  : loc(SL_UNKNOWN)
{}


#define DEFN(type)                                \
  bool LocationSearcher::visit##type(type *obj)   \
  {                                               \
    if (loc == SL_UNKNOWN) {                      \
      loc = obj->loc;                             \
    }                                             \
                                                  \
    /* examine children if location not found */  \
    return loc == SL_UNKNOWN;                     \
  }

DEFN(TopForm)
DEFN(PQName)
DEFN(TypeSpecifier)
DEFN(Enumerator)
DEFN(Member)
DEFN(IDeclarator)
DEFN(Statement)
DEFN(Initializer)
DEFN(TemplateParameter)

#undef DEFN


// EOF
