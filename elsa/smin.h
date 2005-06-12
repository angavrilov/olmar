// smin.h
// structural minimizer

#ifndef SMIN_H
#define SMIN_H

#include "iptparse.h"       // parseFile
#include "iptree.h"         // IPTree

class Minimizer {
public:      // data
  // filenames
  string sourceFname;       // source file to minimize
  string testProg;          // test program to run

  // original contents of 'sourceFname;
  GrowArray<char> source;
  
  // structural info about 'source'
  IPTree tree;
  
  // record of which variants have been tried
  VariantResults results;
  
  // count of tests run
  int passingTests;
  int failingTests;
  int cachedTests;
  
  // sum of the lengths of all inputs tested
  long long testInputSum;

private:     // funcs    
  bool minimize(Node *n);
  bool minimizeChildren(ObjListIterNC<Node> &iter);

public:      // funcs
  Minimizer();
  ~Minimizer();

  // run the test with the current contents of the source file
  VariantResult runTest();

  // write the variant corresponding to the current state of the tree
  // nodes' relevance, and return a cursor denoting the bitstring for
  // this variant; returns with 'size' set to size of written file
  VariantCursor write(int &size);
  
  // kick off the minimizer
  void minimize();
};

#endif // SMIN_H
