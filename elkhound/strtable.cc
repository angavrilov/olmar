// strtable.cc
// code for strtable.h

#include "strtable.h"    // this module
#include "xassert.h"     // xassert
                          

STATICDEF char const *StringTable::identity(void *data)
{
  return (char const*)data;
}


StringTable::StringTable()
  : hash(identity),
    longStrings(NULL)
{
  racks = new Rack(NULL);
}


StringTable::~StringTable()
{
  while (racks != NULL) {
    Rack *temp = racks;
    racks = racks->next;
    delete temp;
  }

  while (longStrings != NULL) {
    LongString *temp = longStrings;
    longStrings = longStrings->next;
    delete temp->data;
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

  int len = strlen(src)+1;     // include null terminator

  // is it a long string?
  if (len >= longThreshold) {
    char *d = new char[len];
    ret = d;
    memcpy(d, src, len);

    // prepend a new long-string entry
    longStrings = new LongString(longStrings, d);
  }

  else {
    // see if we need a new rack
    if (len > racks->availBytes()) {
      // need a new rack
      xassert(len <= rackSize);
      racks = new Rack(racks);     // prepend new rack
    }

    // add the string to the last rack
    ret = racks->nextByte();
    memcpy(racks->nextByte(), src, len);
    racks->usedBytes += len;
  }

  // enter the intended location into the indexing structures
  hash.add(ret, (void*)ret);

  return ret;
}


StringRef StringTable::get(char const *src) const
{
  return (StringRef)hash.get(src);
}
