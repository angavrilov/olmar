#include "cc_qualifiers_dummy.h"

int cc_qual_flag;

/*static*/ void Qualifiers::insertInstancesIntoGraph() {}
string toString(Qualifiers *q) {return string("");}
Qualifiers *deepClone(Qualifiers *q) {return q;}
Type const *applyQualifierLiteralsToType(Qualifiers *q, Type const *baseType) {return baseType;}
