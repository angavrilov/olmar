// iptree.cc
// code for iptree.h

#include "iptree.h"       // this module
#include "autofile.h"     // AutoFILE
#include "syserr.h"       // xsyserror
#include "trace.h"        // TRACE


// --------------------- Relevance ----------------------
char const *toString(Relevance r)
{
  char const * const map[] = {
    "R_IRRELEVANT",
    "R_UNKNOWN",
    "R_RELEVANT"
  };

  STATIC_ASSERT(TABLESIZE(map) == NUM_RELEVANCES);
  xassert((unsigned)r < (unsigned)NUM_RELEVANCES);
  return map[r];
}


// ------------------- VariantResults -------------------
char const *toString(VariantResult r)
{
  char const * const map[] = {
    "VR_UNKNOWN",
    "VR_PASSED",
    "VR_FAILED"
  };

  STATIC_ASSERT(TABLESIZE(map) == NUM_VRESULTS);
  xassert((unsigned)r < (unsigned)NUM_VRESULTS);
  return map[r];
}


void printBits(ArrayStack<char> const &bits)
{
  for (int i=0; i < bits.length(); i++) {
    cout << (bits[i]? '1' : '0');
  }
}


void iprintResults(VariantCursor p, ArrayStack<char> &bits)
{
  if (p->data) {
    // print the result
    cout << toString(p->data) << "  ";

    // and the bitstring
    printBits(bits);
    cout << "\n";
  }

  if (p->zero) {
    bits.push(0);
    iprintResults(p->zero, bits);
    bits.pop();
  }

  if (p->one) {
    bits.push(1);
    iprintResults(p->one, bits);
    bits.pop();
  }
}


void printResults(VariantResults &results)
{
  ArrayStack<char> bits;
  VariantCursor p = results.getTop();

  iprintResults(p, bits);
}


// ------------------------ Node ------------------------
Node::Node(int lo_, int hi_)
  : lo(lo_),
    hi(hi_),
    children(),
    rel(R_UNKNOWN)
{
  xassert(lo <= hi);
}

Node::~Node()
{}


bool Node::contains(Node const *n) const
{
  return lo <= n->lo && n->hi <= hi;
}


bool Node::contains(int n) const
{
  return lo <= n && n <= hi;
}


void Node::insert(Node *n)
{
  //TRACE("insert", "attempting to insert " << n->rangeString() <<
  //                " into " << this->rangeString());

  // 'n' must nest properly inside 'this'
  if (!contains(n)) {
    xfatal("attempting to insert " << n->rangeString() <<
           " into " << this->rangeString());
  }
  
  // search for a child that contains 'n'
  ObjListMutator<Node> mut(children);
  while (!mut.isDone()) {
    if (n->hi < mut.data()->lo) {
      // 'n' goes in 'children' before 'mut'
      mut.insertBefore(n);
      return;
    }
    
    if (mut.data()->contains(n)) {
      // 'n' gets inserted inside 'mut'
      mut.data()->insert(n);
      return;
    }

    if (n->contains(mut.data())) {
      // stick 'mut' into 'n'
      n->insert(mut.remove());
      continue;
    }

    // must be that 'n' goes after 'mut'
    if (!( n->lo > mut.data()->hi )) {
      xfatal(n->rangeString() << " doesn't come after " << mut.data()->rangeString());
    }
    mut.adv();
  }
  
  // put 'n' at the end of the child list
  mut.insertBefore(n);
}


Node const *Node::queryC(int n) const
{
  FOREACH_OBJLIST(Node, children, iter) {
    if (iter.data()->contains(n)) {
      return iter.data()->queryC(n);
    }
  }

  xassert(contains(n));  
  return this;
}


int writeSegment(FILE *fp, GrowArray<char> const &source,
                 int start, int len)
{
  xassert(len >= 0);
  xassert(start >= 0);
  xassert(start+len <= source.size());

  fwrite(source.getArray()+start, 1, len, fp);
  
  return len;
}


int Node::write(FILE *fp, GrowArray<char> const &source,
                VariantCursor &cursor) const
{
  int curOffset = lo;
  int ret = 0;

  FOREACH_OBJLIST(Node, children, iter) {
    Node const *c = iter.data();

    // write data preceding 'c'
    ret += writeSegment(fp, source, curOffset, c->lo - curOffset);

    // possibly write 'c'
    if (c->rel) {
      cursor = cursor->getOne();
      ret += c->write(fp, source, cursor);
    }
    else {
      cursor = cursor->getZero();
    }
    curOffset = c->hi+1;
  }
  
  // upper bound
  int hi = this->hi;
  if (hi >= source.size()) {
    hi = source.size()-1;
  }

  // write data after final child
  ret += writeSegment(fp, source, curOffset, hi+1-curOffset);
  
  return ret;
}


string Node::rangeString() const
{
  if (hi < MAXINT) {
    return stringc << "[" << lo << ", " << hi << "]";
  }
  else {
    return stringc << "[" << lo << ", +inf)";
  }
}

static void indent(ostream &os, int ind)
{
  for (int i=0; i<ind; i++) {
    os << ' ';
  }
}

void Node::debugPrint(ostream &os, int ind) const
{
  indent(os, ind);
  cout << rangeString() << " \t" << toString(rel) << "\n";
  
  FOREACH_OBJLIST(Node, children, iter) {
    iter.data()->debugPrint(os, ind+2);
  }
}


// ------------------------ IPTree -----------------------
IPTree::IPTree()
  : top(NULL)
{
  top = new Node(0, MAXINT);
}

IPTree::~IPTree()
{
  delete top;
}


Node *IPTree::insert(int lo, int hi)
{ 
  Node *n = new Node(lo, hi);
  top->insert(n);
  return n;
}


Node const *IPTree::queryC(int n) const
{
  if (!top || !top->contains(n)) {
    return NULL;
  }

  return top->queryC(n);
}


int IPTree::write(rostring fname, GrowArray<char> const &source,
                  VariantCursor &cursor) const
{
  AutoFILE fp(toCStr(fname), "w");
  int ret = 0;

  if (top->rel) {
    cursor = cursor->getOne();
    ret += top->write(fp, source, cursor);
  }
  else {
    cursor = cursor->getZero();
  }

  return ret;
}


int IPTree::getLargestFiniteEndpoint()
{
  if (top->hi < MAXINT) {
    return top->hi;
  }
  
  if (top->children.isEmpty()) {
    return top->lo;
  }
  
  return top->children.last()->hi;
}


void IPTree::gdb() const
{
  if (!top) {
    cout << "(empty tree)\n";
  }
  else {
    top->debugPrint(cout, 0);
  }
}


// ---------------------- readFile -----------------------
void readFile(rostring fname, GrowArray<char> &dest)
{
  AutoFILE fp(toCStr(fname), "r");

  enum { SZ=4096 };
  int curOffset = 0;

  for (;;) {
    dest.ensureIndexDoubler(curOffset+SZ);
    int len = fread(dest.getArrayNC()+curOffset, 1, SZ, fp);
    if (len < 0) {
      xsyserror("read");
    }
    if (len == 0) {
      break;
    }

    curOffset += len;
  }
  
  // trim the array
  dest.setSize(curOffset);
}


// ---------------------- test code ----------------------
#ifdef TEST_IPTREE
#include "test.h"        // USUAL_MAIN

void entry()
{
  IPTree t;

  t.insert(0,115);       // whole file

  t.insert(0,9);         // prelude
  t.insert(10,16);       // int g;
  t.insert(17,23);       // int h;
  t.insert(24,41);       // int bar(int,int);
  t.insert(42,42);       // whitespace

  t.insert(43,113);      // all of foo
  t.insert(51,62);       // parameter list
  t.insert(56,62);       // ", int y"

  t.insert(69,75);       // "g += x;"
  t.insert(71,74);       // "+= x"
  t.insert(71,71);       // "+"

  t.insert(79,92);       // "h = g + x + y";
  t.insert(81,91);       // "= g + x + y"
  t.insert(85,87);       // "+ x"
  t.insert(89,91);       // "+ y"

  t.insert(96,111);      // "return bar(2,3);"
  t.insert(103,110);     // "bar(2,3)"
  t.insert(107,109);     // "2,3"
  t.insert(108,109);     // ",3"

  t.gdb();
}

USUAL_MAIN

#endif // TEST_IPTREE

