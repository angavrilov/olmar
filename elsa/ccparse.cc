// ccparse.cc            see license.txt for copyright and terms of use
// code for ccparse.h

#include "ccparse.h"      // this module
#include "cc.ast.gen.h"   // ASTVisitor

#include <iostream.h>     // cout


// ----------------------- ParseEnv -----------------------
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

    default:
      cout << toString(loc) << ": error: malformed type: "
           << toString(m) << endl;
      errors++;
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
      cout << toString(loc) << ": error: too many `long's" << endl;
    }

    // make it look like only m1 had 'long long' and neither had 'long'
    m1 = (UberModifiers)((m1 & ~UM_LONG) | UM_LONG_LONG);
    m2 = (UberModifiers)(m2 & ~(UM_LONG | UM_LONG_LONG));
  }

  // any duplicate flags?
  UberModifiers dups = (UberModifiers)(m1 & m2);
  if (dups) {
    cout << toString(loc) << ": error: duplicate modifier: "
         << toString(dups) << endl;
    errors++;
  }
  
  return (UberModifiers)(m1 | m2);
}


// ---------------------- AmbiguityChecker -----------------
// check for ambiguities
class AmbiguityChecker : public ASTVisitor {
public:
  int ambiguousNodes;    // count of nodes with non-NULL ambiguity links
  
public:
  AmbiguityChecker() : ambiguousNodes(0) {}
  
  // check each of the kinds of nodes that have ambiguity links
  bool visitASTTypeId(ASTTypeId *obj);
  bool visitDeclarator(Declarator *obj);
  bool visitStatement(Statement *obj);
  bool visitExpression(Expression *obj);
  bool visitTemplateArgument(TemplateArgument *obj);
};


#define VISIT(type)                             \
  bool AmbiguityChecker::visit##type(type *obj) \
  {                                             \
    if (obj->ambiguity) {                       \
      ambiguousNodes++;                         \
    }                                           \
    return true;                                \
  }

VISIT(ASTTypeId)
VISIT(Declarator)
VISIT(Statement)
VISIT(Expression)
VISIT(TemplateArgument)

#undef VISIT


int numAmbiguousNodes(TranslationUnit *unit)
{
  AmbiguityChecker c;
  unit->traverse(c);
  return c.ambiguousNodes;
}

