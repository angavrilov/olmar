// cc_ocaml.h            see license.txt for copyright and terms of use
// ocaml serialization utility functions



#ifndef CC_OCAML_H
#define CC_OCAML_H

#include "cc_flags.h"
#include "ocamlhelp.h"
#include "variable.h"      // Variable
#include "cc_type.h"       // CType, FunctonType, CompoundType


enum CircularAstType {
  CA_Empty,
  CA_Type
};


class CircularAstPart {
 public:
  CircularAstType ca_type;
  union {
    CType * type; 		/* tagged CA_Type */
  } ast;
  value val;
  unsigned field;
  CircularAstPart * next;

  CircularAstPart();
  ~CircularAstPart();
};                                          


class ToOcamlData {
public:
  SObjSet<const void*> stack;		// used to detect cycles in the ast
  value source_loc_hash;
  unsigned postponed_count;
  CircularAstPart * postponed_circles;

  ToOcamlData();
  ~ToOcamlData();
};


value ref_None_constr(ToOcamlData * data);
void postpone_circular_type(ToOcamlData * data, value val, 
			    unsigned field, CType * type);
void finish_circular_pointers(ToOcamlData * data);


extern bool caml_start_up_done;

value ocaml_from_SourceLoc(const SourceLoc &, ToOcamlData *);

// for flag sets
value ocaml_from_DeclFlags(const DeclFlags &, ToOcamlData *);
value ocaml_from_CVFlags(const CVFlags &, ToOcamlData *);
value ocaml_from_function_flags(const FunctionFlags &f, ToOcamlData *d);

// for real enums
value ocaml_from_SimpleTypeId(const SimpleTypeId &, ToOcamlData *);
value ocaml_from_TypeIntr(const TypeIntr &, ToOcamlData *);
value ocaml_from_AccessKeyword(const AccessKeyword &, ToOcamlData *);
value ocaml_from_OverloadableOp(const OverloadableOp &, ToOcamlData *);
value ocaml_from_UnaryOp(const UnaryOp &, ToOcamlData *);
value ocaml_from_EffectOp(const EffectOp &, ToOcamlData *);
value ocaml_from_BinaryOp(const BinaryOp &, ToOcamlData *);
value ocaml_from_CastKeyword(const CastKeyword &, ToOcamlData *);
value ocaml_from_CompoundType_Keyword(const CompoundType::Keyword &, 
				      ToOcamlData *);


// value ocaml_from_(const  &, ToOcamlData *);

#endif // CC_OCAML_H
