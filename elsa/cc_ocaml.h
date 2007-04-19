//  Copyright 2006 Hendrik Tews, All rights reserved.                  *
//  See file license.txt for terms of use                              *
//**********************************************************************

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
void check_caml_root_status();

#endif // DEBUG_CAML_GLOBAL_ROOTS


enum CircularAstType {
  CA_Empty,
  CA_CType,
  CA_Function,
  CA_Expression,
  CA_TypeSpecifier,
  CA_Variable,
  CA_CompoundInfo,
  CA_OverloadSet,
  CA_StringRefMapVariable
};


class CircularAstPart {
 public:
  CircularAstType ca_type;
  union {
    CType * type; 		/* tagged CA_CType */
    Function * func;		/* tagged CA_Function */
    Expression * expression;	/* tagged CA_Expression */
    TypeSpecifier * typeSpecifier; /* tagged CA_TypeSpecifier */
    Variable * variable;	/* tagged CA_Variable */
    CompoundType * compound;	/* tagged CA_CompoundInfo */
    OverloadSet * overload;	/* tagged CA_OverloadSet */
    StringRefMap<Variable> * string_var_map; /* tagged CA_StringRefMapVariable*/
  } ast;
  
  // val contains the thing which gets updated later.
  // It is a option reference cell, which must be ref None
  // except for CA_OverloadSet it is list ref, which must be []; and
  // except for CA_StringRefMapVariable it is 
  // a (string, annotated variable) Hashtbl, which must be empty.
  value val;
  CircularAstPart * next;	/* single linked list */

  CircularAstPart();		/* (de-)register val as ocaml root */
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


value ref_constr(value elem, ToOcamlData * data);

/* 
 * The postpone_* functions register an update of some value 
 * through (legal ocaml) side effects
 * to be performed later, in order to avoid circularities. Besides the 
 * ocaml data (in which the deferred circularity is enqued) they also take
 * the value to be updated and the ast node that is to 
 * be serialized later.
 * The first six functions (ie. CType, Function, Expression, TypeSpecifier, 
 * Variable, CompoundType) 
 * update an option ref, for them the value val passed must be ref None.
 * The OverloadSet function updates an list ref. For it val must be ref [].
 * StringRefMapVariable updates a (string, annotated variable) Hashtabl.t,
 * which must be empty.
 */
void postpone_circular_CType(ToOcamlData * data, value val, CType * type);
void postpone_circular_Function(ToOcamlData * data, value val, Function * func);
void postpone_circular_Expression(ToOcamlData *, value, Expression *);
void postpone_circular_TypeSpecifier(ToOcamlData *, value, TypeSpecifier *);
void postpone_circular_Variable(ToOcamlData *, value, Variable *);
void postpone_circular_CompoundInfo(ToOcamlData *, value, CompoundType *);
void postpone_circular_OverloadSet(ToOcamlData *, value, OverloadSet *);
void postpone_circular_StringRefMapVariable(ToOcamlData *, value, 
					    StringRefMap<Variable> *);

// serialize all the deferred circularities enqued in data
void finish_circular_pointers(ToOcamlData * data);


extern bool caml_start_up_done;

value ocaml_from_SourceLoc(const SourceLoc &, ToOcamlData *);
value ocaml_ast_annotation(const void *, ToOcamlData *);
int get_max_annotation(void);

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
value ocaml_from_DeclaratorContext(const DeclaratorContext &, ToOcamlData *);
value ocaml_from_ScopeKind(const ScopeKind &, ToOcamlData *);

// for other types
value ocaml_from_unsigned_long(const unsigned long &, ToOcamlData *);
value ocaml_from_double(const double &, ToOcamlData *);

// hand written ocaml serialization function
inline
value ocaml_from_unsigned_int(const unsigned int &i, ToOcamlData *d) {
  return ocaml_from_unsigned_long(i,d);
}


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
inline void detach_ocaml_ScopeKind(const ScopeKind) {}
inline void detach_ocaml_unsigned_long(const unsigned long &) {}
inline void detach_ocaml_double(const double &) {}
inline void detach_ocaml_unsigned_int(const unsigned int &) {}
inline void detach_ocaml_DeclaratorContext(const DeclaratorContext &) {}

#endif // CC_OCAML_H
