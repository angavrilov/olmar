// cc_err.h            see license.txt for copyright and terms of use
// objects for representing errors in C++ code

#ifndef CC_ERR_H
#define CC_ERR_H

#include "macros.h"    // ENUM_BITWISE_OR
#include "str.h"       // string
#include "srcloc.h"    // SourceLoc

#include <ostream.h>   // ostream


// flags on errors
enum ErrorFlags {
  // ordinary error
  EF_NONE          = 0x00,

  // informative, but not an error; typically, such messages should
  // have a switch to disable them
  EF_WARNING       = 0x01,

  // This flag means the error will not be supressed in template code.
  // Most errors inside templates are suppressed since they may be due
  // to having incomplete information; but an EF_STRONG error is one
  // where we are sure that the code really is invalid.  In theory, as
  // cppstd does not require diagnosis of errors in uninstantiated
  // template code, changing an EF_STRONG to EF_NONE should not cause
  // Elsa to become nonconformant, merely less convenient.  Nevertheless,
  // some of our regression tests contain (intentional) errors that are 
  // diagnosed only because of EF_STRONG.
  EF_STRONG        = 0x02,

  // when this is true, the error message should be considered
  // when disambiguation; when it's false, it's not a sufficiently
  // severe error to warrant discarding an ambiguous alternative;
  // for the most part, only environment lookup failures are
  // considered to disambiguate
  EF_DISAMBIGUATES = 0x04,

  // this is a warning that will not be suppressed in template code
  EF_STRONG_WARNING= 0x08,

  EF_ALL           = 0x0F
};
ENUM_BITWISE_OR(ErrorFlags)


// an error message from the typechecker; I plan to expand
// this to contain lots of information about the error, but
// for now it's just a string like every other typechecker
// produces
class ErrorMsg {
public:
  SourceLoc loc;          // where the error happened
  string msg;             // english explanation
  ErrorFlags flags;       // various
  
  // string of instantiation locations leading to the error; if
  // no instantiations are involved, this should be "", which does
  // not require any allocation to store (you can also pass NULL
  // to mean "")
  string instLoc;

public:
  ErrorMsg(SourceLoc L, rostring m, ErrorFlags f)
    : loc(L), msg(m), flags(f), instLoc() {}
  ErrorMsg(SourceLoc L, rostring m, ErrorFlags f, rostring i)
    : loc(L), msg(m), flags(f), instLoc(i) {}
  ~ErrorMsg();

  bool isWarning() const
    { return !!(flags & (EF_WARNING | EF_STRONG_WARNING)); }
  bool disambiguates() const
    { return !!(flags & EF_DISAMBIGUATES); }

  string toString() const;
};


// list of errors; a reference to this will be passed to functions
// that want to report errors to the user
class ErrorList {
private:           
  // the error objects are actually stored in reverse order, such
  // that (effectively) appending is fast and prepending is slow;
  // this field is private because I want to isolate knowledge of
  // this reverse ordering
  ObjList<ErrorMsg> list;

public:
  ErrorList();                      // empty list initially
  virtual/*...*/ ~ErrorList();      // deallocates error objects

  // add an error to the end of list (actually the beginning of
  // 'list', but clients don't know that, nor should they care)
  virtual void addError(ErrorMsg * /*owner*/ obj);

  // add an error to the beginning; this is unusual, and generally
  // should only be done when it is known that there aren't very
  // many error objects in the list
  void prependError(ErrorMsg * /*owner*/ obj);

  // take all of the error messages in 'src' and (semantically)
  // append them after messages in 'this'; this operation leaves
  // 'src' empty, and takes time no more than proportional to the
  // length of the 'src' list; it's O(1) if 'this' is empty
  void takeMessages(ErrorList &src);         // append
  
  // this one takes time proportional to 'this' list
  void prependMessages(ErrorList &src);      // prepend 

  // mark all of the messages with EF_WARNING
  void markAllAsWarnings();

  // delete all of the existing messages
  void deleteAll() { list.deleteAll(); }

  // keep only messages meeting a specific criteria
  void filter(bool (*pred)(ErrorMsg *msg));

  // various counts of error objects
  int count() const;            // total
  int numErrors() const;        // # that are not EF_WARNING
  int numWarnings() const;      // # that *are* EF_WARNING

  // true if any are EF_DISAMBIGUATES
  bool hasDisambErrors() const;
  bool isEmpty() const { return list.isEmpty(); }

  // print all the errors, one per line, in order
  void print(ostream &os) const;
  string printToString() const;
};


#endif // CC_ERR_H
