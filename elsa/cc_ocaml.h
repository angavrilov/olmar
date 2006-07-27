// cc_ocaml.h            see license.txt for copyright and terms of use
// ocaml serialization utility functions



#ifndef CC_OCAML_H
#define CC_OCAML_H

#include "cc_flags.h"
#include "ocamlhelp.h"
#include "variable.h"      // Variable
#include "cc_type.h"       // CType, FunctonType, CompoundType


#define DEBUG_CAML_GLOBAL_ROOTS
#ifdef DEBUG_CAML_GLOBAL_ROOTS

#define caml_register_global_root debug_caml_register_global_root
#define caml_remove_global_root debug_caml_remove_global_root
void debug_caml_register_global_root (value *);
void debug_caml_remove_global_root (value *);
void print_caml_root_status();

#endif


enum CircularAstType {
  CA_Empty,
  CA_CType,
  CA_Function
};


class CircularAstPart {
 public:
  CircularAstType ca_type;
  union {
    CType * type; 		/* tagged CA_CType */
    Function * func;		/* tagged CA_Function */
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
void postpone_circular_CType(ToOcamlData * data, value val, 
			    unsigned field, CType * type);
void postpone_circular_Function(ToOcamlData * data, value val, 
				unsigned field, Function * func);
void finish_circular_pointers(ToOcamlData * data);


extern bool caml_start_up_done;

value ocaml_from_SourceLoc(const SourceLoc &, ToOcamlData *);

//********************** value generation ************************************
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


//*********************** ocaml_val cleanup **********************************
// all these functions are empty, 
// however, defining them here empty is better than another hack in astgen
// hand written ocaml serialization cleanup
inline void detach_ocaml_SourceLoc(const SourceLoc &) {}
inline void detach_ocaml_DeclFlags(const DeclFlags &) {}
inline void detach_ocaml_CVFlags(const CVFlags &) {}
inline void detach_ocaml_function_flags(const FunctionFlags &f) {}
inline void detach_ocaml_SimpleTypeId(const SimpleTypeId &) {}
inline void detach_ocaml_TypeIntr(const TypeIntr &) {}
inline void detach_ocaml_AccessKeyword(const AccessKeyword &) {}
inline void detach_ocaml_OverloadableOp(const OverloadableOp &) {}
inline void detach_ocaml_UnaryOp(const UnaryOp &) {}
inline void detach_ocaml_EffectOp(const EffectOp &) {}
inline void detach_ocaml_BinaryOp(const BinaryOp &) {}
inline void detach_ocaml_CastKeyword(const CastKeyword &) {}
inline void detach_ocaml_CompoundType_Keyword(const CompoundType::Keyword &) {}


#endif // CC_OCAML_H
