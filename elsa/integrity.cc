// integrity.cc
// code for integrity.h

#include "integrity.h"         // this module


void IntegrityVisitor::foundAmbiguous(void *obj, void **ambig, char const *kind)
{
  // 2005-06-29: I have so far been unable to provoke this error by
  // doing simple error seeding, because it appears to be masked by a
  // check in the elaboration visitor regarding visiting certain lists
  // more than once.  Among other things, that means if there is a bug
  // along these lines, a user will discover it by seeing the
  // unfriendly list-visit assertion failure insteead of the message
  // here.  But, that stuff is wrapped up in the Daniel's lowered
  // visitor mechanism, which I don't want to mess with now.  Anyway,
  // I'm reasonably confident that this check will work properly.
  xfatal(toString(loc) << ": internal error: found ambiguous " << kind);
}


bool IntegrityVisitor::visitDeclarator(Declarator *obj)
{
  // make sure the type is not a DQT if we are not in a template
  if (!inTemplate) {
    checkNontemplateType(obj->var->type);
    checkNontemplateType(obj->type);
  }

  return true;
}

void IntegrityVisitor::checkNontemplateType(Type *t)
{
  if (t->containsGeneralizedDependent()) {
    xfatal(toString(loc) << ": internal error: found dependent type `"
                         << t->toString() << "' in non-template");
  }
}


// EOF
