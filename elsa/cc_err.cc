// cc_err.cc            see license.txt for copyright and terms of use
// code for cc_err.h

#include "cc_err.h"      // this module
#include "trace.h"       // tracingSys


// ----------------- ErrorMsg -----------------
ErrorMsg::~ErrorMsg()
{}


string ErrorMsg::toString() const
{
  return stringc << ::toString(loc)
                 << ((flags & EF_WARNING)? ": warning: " :
                     (flags & EF_STRONG_WARNING)? ": warning(error): " :
                     ": error: ")
                 << msg << instLoc;
}


// ---------------- ErrorList -----------------
ErrorList::ErrorList()
  : list()
{}

ErrorList::~ErrorList()
{
  list.deleteAll();
}


void ErrorList::addError(ErrorMsg * /*owner*/ obj)
{
  list.prepend(obj);    // O(1)
}

void ErrorList::prependError(ErrorMsg * /*owner*/ obj)
{
  list.append(obj);     // O(n)
}


void ErrorList::takeMessages(ErrorList &src)
{ 
  if (list.isEmpty()) {
    // this is a common case, and 'concat' is O(1) in this case
    list.concat(src.list);
    return;
  }

  // put all of my messages (which semantically come first) onto the
  // end of the 'src' list; this takes time proportional to the
  // length of 'src.list'
  src.list.concat(list);

  // now put them back onto my list, which will take O(1) since
  // my list is now empty
  list.concat(src.list);
}


void ErrorList::prependMessages(ErrorList &src)
{
  list.concat(src.list);
}


void ErrorList::markAllAsWarnings()
{
  FOREACH_OBJLIST_NC(ErrorMsg, list, iter) {
    iter.data()->flags |= EF_WARNING;
  }
}


int ErrorList::count() const
{
  return list.count();
}

int ErrorList::numErrors() const
{
  return count() - numWarnings();
}

int ErrorList::numWarnings() const
{
  int ct=0;
  FOREACH_OBJLIST(ErrorMsg, list, iter) {
    if (iter.data()->isWarning()) {
      ct++;
    }
  }
  return ct;
}


bool ErrorList::hasDisambErrors() const
{
  FOREACH_OBJLIST(ErrorMsg, list, iter) {
    if (iter.data()->disambiguates()) {
      return true;     // has at least one disambiguating error
    }
  }
  return false;        // no disambiguating errors
}


void ErrorList::print(ostream &os) const
{
  os << printToString();
}


string ErrorList::printToString() const
{
  stringBuilder sb;

  // need to temporarily reverse it, but I promise to restore it
  // when I'm done
  ObjList<ErrorMsg> &nclist = const_cast<ObjList<ErrorMsg>&>(list);

  bool printWarnings = !tracingSys("nowarn");

  nclist.reverse();
  FOREACH_OBJLIST(ErrorMsg, nclist, iter) {
    if (printWarnings || !iter.data()->isWarning()) {
      sb << iter.data()->toString() << "\n";
    }
  }
  nclist.reverse();

  return sb;
}


// EOF
