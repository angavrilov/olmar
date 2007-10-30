//  Copyright 2006 Hendrik Tews, All rights reserved.                  *
//  See file license.txt for terms of use                              *
//**********************************************************************

// had written reflection additions 

#ifndef ELSA_ADDITIONS
#define ELSA_ADDITIONS


#include "ocaml_reflection_base.h"
#include "cc.ast.gen.h"


extern string ocamlAstFname;

void reflect_into_ocaml(char ** argv, const char * inputFname, 
			TranslationUnit *unit);


#endif // ELSA_ADDITIONS
