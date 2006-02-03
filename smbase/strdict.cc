// strdict.cc            see license.txt for copyright and terms of use
// code for strdict.h

#include "strdict.h"        // this module
#include <string.h>         // strcmp


#define FOREACH_NODE(itervar) \
  for(Node *itervar = top; itervar != NULL; itervar = itervar->next)

#define FOREACH_ITER(dict, itervar) \
  for(Iter itervar = (dict).getIter(); !itervar.isDone(); itervar.next())

#define FOREACH_ITERC(dict, itervar) \
  for(IterC itervar = (dict).getIterC(); !itervar.isDone(); itervar.next())

#define MUTABLE_SORT(obj) (const_cast<StringDict&>(obj)).sort()


StringDict::StringDict()
  : top(NULL)
{}


StringDict::StringDict(StringDict const &obj)
  : top(NULL)
{
  *this = obj;
}


StringDict::~StringDict()
{
  SELFCHECK();
  empty();
}


StringDict& StringDict::operator= (StringDict const &obj)
{
  if (this == &obj) {
    return *this;
  }

  empty();

  Node *end = top;
  FOREACH_ITERC(obj, src) {
    Node *newnode = new Node(src.key().c_str(), src.value().c_str());
    if (!end) {
      // first element of list
      end = top = newnode;
    }
    else {
      // adding to end of nonempty list
      end = end->next = newnode;
    }
  }

  SELFCHECK();
  return *this;
}


bool StringDict::operator== (StringDict const &obj) const
{
  // sort both lists
  MUTABLE_SORT(*this);
  MUTABLE_SORT(obj);

  IterC ths(*this), other(obj);
  while (!ths.isDone() && !other.isDone()) {
    if (0!=strcmp(ths.key(), other.key()) ||
        0!=strcmp(ths.value(), other.value())) {
      return false;
    }
    ths.next();
    other.next();
  }

  if (!ths.isDone() || !other.isDone()) {
    // one finished first, so they can't be equal
    return false;
  }

  return true;
}


bool StringDict::isEmpty() const
{
  return top == NULL;
}


int StringDict::size() const
{
  int ret=0;
  FOREACH_ITERC(*this, entry) {
    ret++;
  }
  return ret;
}


bool StringDict::query(char const *key, string &value) const
{
  FOREACH_ITERC(*this, entry) {
    if (0==strcmp(entry.key(), key)) {
      value = entry.value();
      return true;
    }
  }

  return false;
}


string StringDict::queryf(char const *key) const
{
  string ret;
  bool ok = query(key, ret);
  xassert(ok);
  return ret;
}


bool StringDict::isMapped(char const *key) const
{
  string dummy;
  return query(key, dummy);
}


void StringDict::add(char const *key, char const *value)
{
  xassert(!isMapped(key));

  // just prepend; we'll sort later (when an iterator is retrieved)
  top = new Node(key, value, top);

  SELFCHECK();
}


void StringDict::modify(char const *key, char const *newValue)
{
  Iter entry = find(key);
  xassert(!entry.isDone());

  entry.value() = newValue;

  SELFCHECK();
}


void StringDict::addOrModify(char const *key, char const *newValue)
{
  if (!isMapped(key)) {
    add(key, newValue);
  }
  else {
    modify(key, newValue);
  }
}


StringDict::Iter StringDict::find(char const *key)
{
  FOREACH_ITER(*this, entry) {
    if (0==strcmp(entry.key(), key)) {
      return entry;
    }
  }
  return Iter(NULL);
}


void StringDict::remove(char const *key)
{
  xassert(top);

  // check for removal of top element
  if (0==strcmp(top->key, key)) {
    Node *temp = top;
    top = top->next;
    delete temp;
  }

  // find node to remove in tail of list
  else {
    Node *p = top;
    while (p->next && 0!=strcmp(p->next->key, key)) {
      p = p->next;
    }

    if (!p->next) {
      // reached the end of the list without finding the key
      xfailure("failed to find key");
    }

    // remove p->next from the list
    Node *temp = p->next;
    p->next = p->next->next;
    delete temp;
  }

  SELFCHECK();
}


void StringDict::empty()
{
  while (top) {
    Node *temp = top;
    top = top->next;
    delete temp;
  }

  SELFCHECK();
}


StringDict::Iter StringDict::getIter()
{
  sort();        // must return items in sorted order
  return Iter(top);
}


StringDict::IterC StringDict::getIterC() const
{
  //sort();
  const_cast<StringDict*>(this)->sort();    // mutable
  return IterC(top);
}


// use simple insertion sort for now
/*mutable*/ void StringDict::sort()
{
  if (!top) {
    return;
  }

  // invariant: sequence of nodes from 'top' to 'walker', inclusive,
  //            is always sorted
  Node *walker = top;
  while (walker->next != NULL) {
    // see if walker->next is out of order
    if (0 <= strcmp(walker->key, walker->next->key)) {
      // it's in order
      walker = walker->next;
      continue;
    }

    // remove walker->next from where it is (note that this has
    // the effect of advancing walker, so below here we won't
    // have another movement of walker)
    Node *mover = walker->next;
    walker->next = walker->next->next;
    mover->next = NULL;       // (redundant because of (**) lines)

    // insert at head?
    if (0 < strcmp(mover->key, top->key)) {
      mover->next = top;            // (**)
      top = mover;
      continue;
    }

    // must find correct place to insert mover (will find the place
    // where we can insert mover just before searcher->next)
    Node *searcher = top;
    while (0 < strcmp(searcher->next->key, mover->key)) {
      searcher = searcher->next;
      xassert(searcher != walker);
        // otherwise how could mover have been out of order to begin with?
    }

    // insert mover before searcher->next
    mover->next = searcher->next;   // (**)
    searcher->next = mover;
  }

  SELFCHECK();

  #ifndef NDEBUG
    verifySorted();
  #endif
}


void StringDict::verifySorted() const
{
  if (!top) {
    return;
  }

  Node *p = top;
  while (p->next) {
    xassert(0 <= strcmp(p->key, p->next->key));
    p = p->next;
  }
}


// verify that the list is well structured
void StringDict::selfCheck() const
{
  Node *fast = top, *slow = top;
  while (fast && fast->next) {
    fast = fast->next->next;
    slow = slow->next;

    xassert(fast != slow);
      // if these become equal, the list is circular
  }
}


void StringDict::insertOstream(ostream &os) const
{
  FOREACH_ITERC(*this, entry) {
    os << entry.key() << " = " << entry.value() << endl;
  }
}


string StringDict::toString() const
{
  stringBuilder sb;
  sb << "{";
  int count=0;
  FOREACH_ITERC(*this, entry) {
    if (count++ > 0) {
      sb << ",";
    }
    sb << " " << entry.key() << "=\"" << entry.value() << "\"";
  }
  sb << " }";
  return sb;
}


// -------------------- test code ------------------------
#ifdef TEST_STRDICT

#include "test.h"      // USUAL_MAIN
#include <stdlib.h>    // rand

#define myrandom(n) (rand()%(n))

char randChar()
{
  return (char)(myrandom(127-32+1)+32);
}

string randString(int len)
{
  stringBuilder str;
  loopj(len) {
    str << randChar();
  }
  return str;
}

string randStringRandLen(int maxlen)
{
  return randString(myrandom(maxlen)+1);
}

string randKey(StringDict const &dict)
{
  int size = dict.size();
  xassert(size > 0);

  int nth = myrandom(size);
  StringDict::IterC entry(dict);
  for (; nth > 0; entry.next(), nth--)
    {}

  return entry.key();
}


void entry()
{
  StringDict dict;
  int size=0, collisions=0;

  int iters = 1000;
  loopi(iters) {
    switch (myrandom(6)) {
      case 0: {
        // insert a random element
        string key = randStringRandLen(10);
        string value = randStringRandLen(30);

        if (!dict.isMapped(key.c_str())) {
          dict.add(key.c_str(), value.c_str());
          size++;
        }
        else {
          collisions++;
        }
        break;
      }

      case 1: {
        // remove a random element
        if (dict.isEmpty()) {
          break;
        }

        string key = randKey(dict);
        dict.remove(key.c_str());
        size--;
        break;
      }

      case 2: {
        // check a random element that should not be there
        string key = randStringRandLen(10);
        if (dict.isMapped(key.c_str())) {
          collisions++;
        }
        break;
      }

      case 3: {
        // verify that computed length is right
        xassert(size == dict.size());
        break;
      }

      case 4: {
        // test == and =
        StringDict dict2(dict);
        xassert(dict2 == dict);
        xassert(dict2.size() == dict.size());

        // modify it, then verify inequality
        if (!dict2.isEmpty()) {
          string key = randKey(dict2);
          string value = dict2.queryf(key.c_str());

          if (myrandom(2) == 0) {
            dict2.remove(key.c_str());
          }
          else {
            dict2.modify(key.c_str(), stringc << value << "x");
          }
          xassert(dict2 != dict);
        }

        break;
      }

      case 5: {
        // random modification
        if (!dict.isEmpty()) {
          string key = randKey(dict);
          dict.modify(key.c_str(), randStringRandLen(30).c_str());
        }
        break;
      }

      default:
        xfailure("huh?");
        break;
    }
  }

  cout << "final size: " << size
       << "\ncollisions: " << collisions
       << "\n";

  cout << "all tests passed\n";
}

USUAL_MAIN

#endif // TEST_STRDICT
