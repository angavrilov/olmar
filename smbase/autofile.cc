// autofile.cc            see license.txt for copyright and terms of use
// code for autofile.h

#include "autofile.h"     // this module
#include "exc.h"          // throw_XOpen


FILE *xfopen(char const *fname, char const *mode)
{
  FILE *ret = fopen(fname, mode);
  if (!ret) {
    throw_XOpen(fname);
  }
  
  return ret;
}


AutoFILE::AutoFILE(char const *fname, char const *mode)
  : AutoFclose(xfopen(fname, mode))
{}

AutoFILE::~AutoFILE()
{
  // ~AutoFclose closes the file
}


// EOF
