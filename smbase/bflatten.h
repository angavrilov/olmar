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

#endif // BFLATTEN_H
