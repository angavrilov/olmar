// asthelp.cc
// code for what's declared in asthelp.h

#include "asthelp.h"       // this module
#include "strutil.h"       // quoted

// ----------- debugPrint helpers -----------------------
ostream &ind(ostream &os, int indent)
{
  while (indent--) {
    os << " ";
  }
  return os;
}


void debugPrintStr(string const &s, char const *name,
                   ostream &os, int indent)
{
  ind(os, indent) << name << " = " << quoted(s) << "\n";
}


void debugPrintList(ASTList<string> const &list, char const *name,
                    ostream &os, int indent)
{
  ind(os, indent) << name << ": ";
  {                                 
    int ct=0;
    FOREACH_ASTLIST(string, list, iter) {
      if (ct++ > 0) {
        os << ", ";
      }
      os << quoted(*( iter.data() ));
    }
  }
}
