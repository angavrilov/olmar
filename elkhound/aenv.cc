// aenv.cc
// code for aenv.h

#include "aenv.h"               // this module
#include "c.ast.gen.h"          // C ast
#include "absval.ast.gen.h"     // IntValue & children

AEnv::AEnv(StringTable &table)
  : ints(),
    stringTable(table),
    counter(1)
{}

AEnv::~AEnv()
{}


void AEnv::clear()
{
  ints.empty();
}


void AEnv::set(StringRef name, IntValue *value)
{
  if (ints.isMapped(name)) {
    ints.remove(name);
  }
  ints.add(name, value);
}


IntValue *AEnv::get(StringRef name)
{
  IntValue *ret;
  if (ints.query(name, ret)) {
    return ret;
  }
  else {
    return NULL;
  }
}


IntValue *AEnv::freshIntVariable(char const *why)
{
  StringRef name = stringTable.add(stringc << "v" << counter++);
  return new IVvar(name, why);
}


IntValue *AEnv::grab(IntValue *v) { return v; }
void AEnv::discard(IntValue *)    {}
IntValue *AEnv::dup(IntValue *v)  { return v; }

void AEnv::print()
{
  for (StringSObjDict<IntValue>::Iter iter(ints);
       !iter.isDone(); iter.next()) {
    string const &name = iter.key();
    IntValue const *value = iter.value();
    
    cout << "  " << name << ":\n";
    if (value) {
      value->debugPrint(cout, 4);
    }
    else {
      cout << "    null\n";
    }
  }
}




