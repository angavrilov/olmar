// variable.h           
// information about a name
//
// Every binding introduction (e.g. declaration) of a name will own
// one of these to describe the introduced name; every reference to
// that name will be annotated with a pointer to the Variable hanging
// off the introduction.   
//
// The name 'variable' is a slight misnomer; it's used for naming:
//   - local and global variables
//   - logic variables (in the thmprv context)
//   - function names
//   - function parameters
//   - structure fields
//   - enumeration values
//   - typedef'd names (though these are translated-away early on)

#ifndef VARIABLE_H
#define VARIABLE_H

#include "fileloc.h"      // SourceLocation
#include "strtable.h"     // StringRef
#include "cc_flags.h"     // DeclFlags

class Type;               // cc_type.h

class Variable {
public:    // data
  SourceLocation loc;     // location of the name in the source text
  StringRef name;         // name introduced
  Type const *type;       // associated type
  DeclFlags flags;        // various flags

public:    // funcs
  Variable(SourceLocation const &L, StringRef n, 
           Type const *t, DeclFlags f);
  ~Variable();
  
  // some convenient interpretations of 'flags'
  bool hasAddrTaken() const { return flags & DF_ADDRTAKEN; }
  bool isGlobal() const { return flags & DF_GLOBAL; }

  // some ad-hoc thing
  string toString() const;
};

#endif // VARIABLE_H
