#include "cc_qualifiers_dummy.h"

int cc_qual_flag;

/*static*/ void Qualifiers::insert_instances_into_graph() {}
string toString(Qualifiers *q) {return string("");}
string toString (QualifierLiterals *const &) {return string("");}
Qualifiers *deepClone(Qualifiers *q) {return q;}
Qualifiers *deepCloneLiterals(Qualifiers *q) {return q;}
Type const *applyQualifierLiteralsToType(Qualifiers *q, Type const *baseType) {return baseType;}

void nameSubtypeQualifiers(Variable *v) {} 
