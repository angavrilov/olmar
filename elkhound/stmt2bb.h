// stmt2bb.h
// translate Cil statement language to Cil basic block language

#ifndef STMT2BB_H
#define STMT2BB_H

#include "cil.h"        // CilStmt, etc.
   

// translate the contained statements, and store the
// result in the bb fields of 'fn'
void translateStmtToBB(CilFnDefn &fn);


// debugging entry 
void doTranslationStuff(CilFnDefn &fn);


#endif // STMT2BB_H
