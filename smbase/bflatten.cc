// bflatten.cc
// code for bflatten.h

#include "bflatten.h"     // this module
#include "exc.h"          // throw_XOpen
#include "syserr.h"       // xsyserror


BFlatten::BFlatten(char const *fname, bool r)
  : readMode(r)
{
  fp = fopen(fname, readMode? "rb" : "wb");
  if (!fp) {
    throw_XOpen(fname);
  }
}

BFlatten::~BFlatten()
{
  fclose(fp);
}


void BFlatten::xferSimple(void *var, unsigned len)
{
  if (writing()) {
    if (fwrite(var, 1, len, fp) < len) {
      xsyserror("fwrite");
    }
  }
  else {
    if (fread(var, 1, len, fp) < len) {
      xsyserror("fread");
    }
  }
}


// ------------------------ test code ---------------------
#ifdef TEST_BFLATTEN

#include "test.h"      // USUAL_MAIN

void entry()
{                    
  // make up some data
  int x = 9;
  string s("foo bar");

  // open a file for writing them
  {
    BFlatten flat("bflat.tmp", false /*reading*/);
    flat.xferInt(x);
    s.xfer(flat);
  }

  // place to put the data we read
  int x2;
  string s2;

  // read them back
  {
    BFlatten flat("bflat.tmp", true /*reading*/);
    flat.xferInt(x2);
    s2.xfer(flat);
  }

  // compare
  xassert(x == x2);
  xassert(s.equals(s2));

  // delete the temp file
  remove("bflat.tmp");
  
  printf("bflatten works\n");
}


USUAL_MAIN


#endif // TEST_BFLATTEN
