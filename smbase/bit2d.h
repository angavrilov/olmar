// bit2d.h
// 2-d array of bits

#ifndef __BIT2D_H
#define __BIT2D_H

#include "typ.h"             // byte, bool
#include "point.h"           // point

class Bit2d {
  byte *data;  	    // bits; [0..stride-1] is first row, etc.
  point size;       // size.x is # of cols, size.y is # of rows
  int stride;       // bytes between starts of adjacent rows;
                    // computable from size.x but stored for quick access

  byte *byteptr(point const &p)               { return data + p.y * stride + (p.x>>3); }
  byte const *byteptrc(point const &p) const  { return data + p.y * stride + (p.x>>3); }
  int datasize() const                        { return size.y * stride; }

public:
  Bit2d(point const &aSize);
  Bit2d(Bit2d const &obj);
  Bit2d& operator= (Bit2d const &obj);     // sizes must be equal already
  ~Bit2d();

  bool okpt(point const &p) const    { return p.gtez() && p < size; }
  point const &Size() const          { return size; }

  bool operator== (Bit2d const &obj) const;     // compare sizes and data

  // bit access (these were inline earlier, but they expand to a huge amount
  // of code (more than 100 bytes), so I've un-inlined them)
  int get(point const &p) const;
  void set(point const &p);     // to 1
  void reset(point const &p);   // to 0
  void setto(point const &p, int val);
  void toggle(point const &p);

  int testAndSet(point const &p);

  // set everything
  void setall(int val);

  // debugging
  void print() const;
};

#endif // __BIT2D_H

