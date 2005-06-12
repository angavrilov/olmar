// smin.cc
// structural minimizer

#include "test.h"           // ARGS_MAIN
#include "smin.h"           // this module
#include "trace.h"          // TRACE_ARGS

#include <stdlib.h>         // system


Minimizer::Minimizer()
  : sourceFname(),
    testProg(),
    source(10 /*hint*/),
    tree(),
    results(),
    passingTests(0),
    failingTests(0),
    cachedTests(0),
    testInputSum(0)
{}

Minimizer::~Minimizer()
{}


void setIrrelevant(IPTree &t, int offset)
{
  Node *n = t.query(offset);
  if (n) {
    n->rel = R_IRRELEVANT;
  }
}


int system(rostring s)
{
  return system(toCStr(s));
}


VariantResult Minimizer::runTest()
{
  if (0==system(stringc << testProg << " \'" << sourceFname << "\'")) {
    cout << "p";
    passingTests++;
    return VR_PASSED;
  }
  else {
    cout << ".";
    failingTests++;
    return VR_FAILED;
  }
}


VariantCursor Minimizer::write(int &size)
{
  VariantCursor cursor = results.getTop();
  size = tree.write(sourceFname, source, cursor);
  return cursor;
}


void Minimizer::minimize()
{ 
  if (!tree.getTop()) {
    return;
  }

  while (minimize(tree.getTop())) {
    // keep going as long as progress is being made
  }
}


bool Minimizer::minimize(Node *n)
{
  if (!n->rel) {
    // this node is already known to be irrelevant
    return false;
  }
  
  Relevance origRel = n->rel;
  
  // see if we can drop this node
  n->rel = R_IRRELEVANT;       
  int size;
  VariantCursor cursor = write(size);
  if (!cursor->data) {
    // this variant has not already been tried, so test it
    testInputSum += size;
    cursor->data = runTest();
    flush(cout);
  }
  else {
    cachedTests++;
  }
  
  if (cursor->data == VR_PASSED) {
    // the test passed despite dropping this node, so it is irrelevant;
    // leave 'n->rel' as it is (currently set to R_IRRELEVANT)

    // we have made progress
    cout << "\nred:  " << stringf("%10d", size) << " ";
    flush(cout);
    return true;
  }

  else {
    // when we drop this node, the test fails, so it is probably relevant
    n->rel = R_RELEVANT;
  }

  // for performance testing, would disable large-sections-first optimization
  //origRel = R_RELEVANT;

  if (origRel == R_UNKNOWN) {
    // this is the first time this node has been tested, so do not
    // test its children yet; instead wait for the next pass
    if (n->children.isEmpty()) {
      // nothing more to try here, even on the next pass
      return false;
    }
    else {
      // return 'true' to mean that we made progress in the sense of
      // turning this node's 'rel' from R_UNKNOWN to R_RELEVANT, and
      // consequently on the next pass we will investigate the
      // children
      return true;
    }
  }
  
  // this node was tested at least once before, so now try dropping
  // some of its children
  xassert(origRel == R_RELEVANT);
  
  ObjListIterNC<Node> iter(n->children);
  return minimizeChildren(iter);
}


bool Minimizer::minimizeChildren(ObjListIterNC<Node> &iter)
{
  if (iter.isDone()) {
    // no (more) children to minimize, no progress made
    return false;
  }
  
  // remember this child
  Node *n = iter.data();
  
  // try minimizing later children first, with the idea that the
  // static semantic dependencies are likely to go from later to
  // earlier, hence we'd like to start dropping things at the
  // end of the file first
  iter.adv();
  bool progress = minimizeChildren(iter);
  
  // now try minimizing this child
  return minimize(n) || progress;
}


void entry(int argc, char *argv[])
{
  xBase::logExceptions = false;
  string progName = argv[0];
  
  TRACE_ARGS();

  bool checktree = tracingSys("checktree");

  if (checktree) {
    if (argc != 3) {
      cout << "usage: " << progName << " [options] foo.c foo.c.str\n";
      return;
    }
  }
  else {
    if (argc != 4) {
      cout << "usage: " << progName << " [options] foo.c foo.c.str ./test-program\n";
      return;
    }
  }

  Minimizer m;
  m.sourceFname = argv[1];
  string backupFname = stringc << m.sourceFname << ".bak";
  string treeFname = argv[2];
  if (!checktree) {
    m.testProg = argv[3];
  }

  // back up the input
  if (!checktree &&
      0!=system(stringc << "cp \'" << m.sourceFname << "\' \'"
                        << backupFname << "\'")) {
    xfatal("failed to create " << backupFname);
  }

  // read the file into memory
  readFile(m.sourceFname, m.source);

  // build the interval partition tree
  parseFile(m.tree, treeFname);

  // check that its size does not exceed the source file
  {
    int endpt = m.tree.getLargestFiniteEndpoint();
    if (endpt >= m.source.size()) {
      xfatal("the interval tree ends at " << endpt <<
             ", but the file size is only " << m.source.size());
    }
  }

  if (checktree) {
    cout << "tree seems ok\n";
    return;     // done
  }

  // generate the original input
  int size;
  VariantCursor cursor = m.write(size);
  cout << "orig: " << stringf("%10d", size) << " ";
  flush(cout);

  // confirm it is the same as the given original
  if (0!=system(stringc << "cmp -s \'" << m.sourceFname << "\' \'"
                        << backupFname << "\'")) {
    xfatal("failed to accurately re-create original source file");
  }

  // confirm it passes the test
  m.testInputSum += size;
  cursor->data = m.runTest();
  if (cursor->data == VR_PASSED) {
    // yes
  }
  else {
    // no
    xfatal("original source file does not pass the test");
  }

  // begin trying lots of variants
  m.minimize();

  // write the final minimized version
  cout << "\ndone!  writing minimized version to " << m.sourceFname << "\n";
  cursor = m.write(size);
  xassert(cursor->data == VR_PASSED);
  
  cout << "passing=" << m.passingTests
       << ", failing=" << m.failingTests
       << ", total=" << (m.passingTests+m.failingTests)
       << ", inputSum=" << m.testInputSum
       << " (cached=" << m.cachedTests << ")"
       << "\n";
    
  // debugging stuff (interesting, but noisy)   
  if (tracingSys("dumpfinal")) {
    m.tree.gdb();
    printResults(m.results);
  }
}

ARGS_MAIN
