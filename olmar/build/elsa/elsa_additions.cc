//  Copyright 2006 Hendrik Tews, All rights reserved.                  *
//  See file license.txt for terms of use                              *
//**********************************************************************

// had written reflection additions 

#include "elsa_additions.h"
#include "elsa_reflect_ocaml_reflect.h"


string ocamlAstFname;


void reflect_into_ocaml(char ** argv, const char * inputFname, 
			TranslationUnit *unit){
  CAMLparam0();
  CAMLlocal2(ocaml_unit, of);
  // Put the translation unit into a compilation unit.
  CompilationUnit cu(inputFname);
  cu.unit = unit;

  caml_startup(argv);

  value * register_closure = caml_named_value("register_caml_callbacks");
  xassert(register_closure);
  caml_callback(*register_closure, Val_unit);
  
  ocaml_unit = ocaml_reflect_CompilationUnit(&cu);

  static value * marshal_callback = NULL;
  if(marshal_callback == NULL)
    marshal_callback = caml_named_value("marshal_oast_catch_all");
  xassert(marshal_callback);

  if(ocamlAstFname.length() != 0)
    of = caml_copy_string(toCStr(ocamlAstFname));
  else
    of = caml_copy_string(stringc << inputFname << ".oast");

  // cerr << "call marshal_translation_unit_callback(...., " 
  //      << String_val(of) << ")\n";
  caml_callback2(*marshal_callback, ocaml_unit, of);

  // cout << "wrote " 
  //      << get_max_annotation() 
  //      << " ocaml ast nodes" << endl << flush;
  CAMLreturn0;
}


