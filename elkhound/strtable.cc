// strtable.cc
// code for strtable.h

#include "strtable.h"    // this module
#include "xassert.h"     // xassert
                          

STATICDEF char const *StringTable::identity(void *data)
{
  return (char const*)data;
}


StringTable::StringTable()
  : hash(identity)
{
  firstRack = new Rack(NULL);
  lastRack = firstRack;
}


StringTable::~StringTable()
{
  while (firstRack != NULL) {
    Rack *temp = firstRack;
    firstRack = firstRack->next;
    delete temp;
  }
}


StringRef StringTable::add(char const *src)
{
  // see if it's already here
  StringRef ret = get(src);
  if (ret) {
    return ret;
  }

  // see if we need a new rack
  int len = strlen(src)+1;     // include nul terminator
  if (len > lastRack->availBytes()) {
    // need a new rack
    xassert(len <= rackSize);
    lastRack = new Rack(lastRack);
    
    // note that if we need longer strings, we can accomodate
    // them by allocating bigger racks as a special case (and
    // just casting around the type system)
  }
  
  // add the string to the last rack
  ret = lastRack->nextByte();
  memcpy(lastRack->nextByte(), src, len);
  lastRack->usedBytes += len;

  // enter the intended location into the indexing structures
  hash.add(ret, (void*)ret);

  return ret;
}


StringRef StringTable::get(char const *src) const
{
  return (StringRef)hash.get(src);
}
