// variable_annot.h
// skeleton definition

// This header defines the VariableAnnot class, an instance of which
// is embedded into every Variable.  The idea is client analysis can
// have annotation objects (say, Variable_Q) on top of Variable, and
// by putting a Variable_Q pointer into VariableAnnot that analysis
// can easily traverse to its annotation.
//
// The choice to make this a class is motivated by the desire to
// allow several different client analyses in the same process.  The
// programmer arranges for each analysis' annotation pointer to get
// put in VariableAnnot by replacing this skeleton file with one
// that has useful fields and compiling such that this skeleton is
// hidden.
                             
// we've replaced this with a simpler inheritance scheme
#error This file is obsolete.

#ifndef VARIABLE_ANNOT_H
#define VARIABLE_ANNOT_H

#include <stddef.h>     // NULL

// for now there is one client analysis and it uses Variable_Q
// annotation objects, so I'll make the skeleton use that
class Variable_Q;

class VariableAnnot {
public:
  Variable_Q *q;

public:
  VariableAnnot() : q(NULL) {}
};

#endif // VARIABLE_ANNOT_H
