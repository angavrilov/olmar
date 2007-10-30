//  Copyright 2006 Hendrik Tews, All rights reserved.                  *
//  See file license.txt for terms of use                              *
//**********************************************************************

// astgen specific ocaml reflection stuff

#ifndef OCAML_REFLECT
#define OCAML_REFLECT

#include "ast_reflect_ocaml_reflect.h"


void reflect_into_ocaml(char ** argv, string base, ASTSpecFile * ast);



// ------ variant types, will be treated in gen_reflection, eventually ------

value ocaml_reflect_AccessCtl(AccessCtl const * x);

value ocaml_reflect_FieldFlags(FieldFlags const * x);


#endif // OCAML_REFLECT
