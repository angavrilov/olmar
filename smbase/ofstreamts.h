// ofstreamts.h

// quarl 2006-05-25 initial version, factored from 2006-05-16 astgen.cc

#ifndef OFSTREAMTS_H
#define OFSTREAMTS_H

#include <fstream.h>
#include "str.h"

// An ofstream which is timestamp-conscious.  It first writes to a temporary
// file, and on success renames it to the target destination.  However, it
// doesn't do this if the file hasn't changed.  This way we avoid messing with
// the timestamp, which is annoying since 'make' will think we have to rebuild
// everything.
//
// Call 'dontsave()' to avoid saving, e.g. if an error occurred.
class ofstreamTS : public ofstream {
  string destFname;
  stringBuilder tmpFname;
  bool dosave;

  const char *init_fname(string const &destFname0)
  {
    destFname = destFname0;
    tmpFname = destFname0; tmpFname << ".tmp";
    return tmpFname;
  }

  void openTmp()
  {
    open(tmpFname.c_str());
  }

  void save();

public:
  ofstreamTS(string const &destFname0) : dosave(true)
  { init_fname(destFname0); openTmp(); }
  ~ofstreamTS() { if (dosave) save(); }

  void dontsave() { dosave = false; }
};

#endif
