// bflatten.h
// binary file flatten implementation

#ifndef BFLATTEN_H
#define BFLATTEN_H

#include "flatten.h"      // Flatten
#include <stdio.h>        // FILE

class BFlatten : public Flatten {
private:     // data
  FILE *fp;
  bool readMode;

public:
  BFlatten(char const *fname, bool reading);
  virtual ~BFlatten();

  // Flatten funcs
  virtual bool reading() const { return readMode; }
  virtual void xferSimple(void *var, unsigned len);
};

                  
// for debugging, write and then read something
template <class T>
T *writeThenRead(T &obj)
{
  char const *fname = "flattest.tmp";

  // write
  {
    BFlatten out(fname, false /*reading*/);
    obj.xfer(out);
  }

  // read
  BFlatten in(fname, true /*reading*/);
  T *ret = new T(in);
  ret->xfer(in);

  remove(fname);

  return ret;
}

#endif // BFLATTEN_H
