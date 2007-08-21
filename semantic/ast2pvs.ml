(*
 * $Id$
 *
 * Transforms a C++ abstract syntax tree into PVS source code, based on a
 * shallow embedding of C++ expressions and C++ statements as state
 * transformers in PVS.
 *
 * Author: Tjark Weber, (c) 2007 Radboud University
 *)

(* See files Robin/hardware/{expressions,statements}.pvs for the relevant    *)
(* PVS definitions and some explanation of what subset of C++ is available   *)
(* in PVS at the moment.                                                     *)

(* Notes:                                                                    *)
(*                                                                           *)
(* - BIN_ASSIGN denotes simple assignment (=), BIN_PLUS denotes +=, etc.     *)
(* - Variable identifiers are made unique, by appending a number. This       *)
(*   resolves scope issues.  Edit: numbers suppressed for readability.  Use  *)
(*   unique identifiers!                                                     *)
(* - Declarations at file scope are not supported yet.  Easy in principle:   *)
(*   init/dest happens before/after main.  Does not go well with the idea    *)
(*   that we verify functions however.                                       *)
(* - Declarations in for/if/while conditions are not supported yet.  Easy in *)
(*   principle, just needs to be done.                                       *)
(* - Default initialization (§8.5) is not modelled yet.                      *)
(* - For switch statements, a list of case expressions is generated.         *)
(* - C++ functions are turned into PVS definitions.                          *)
(* - There's a software design issue: we need to print similar things        *)
(*   differently, depending on context (e.g. variables with/without their    *)
(*   type).  Should we use different functions, anonymous functions (i.e.    *)
(*   lambda expressions), or functions with additional parameters for that?  *)
(* - Ocaml's pretty-printing module is now used, after I wasted a day        *)
(*   implementing my own pretty-printer, which was loosely based on Derek C. *)
(*   Oppen's article "Prettyprinting" and the Isabelle/SML implementation of *)
(*   that algorithm.                                                         *)
(* - Compound types: The PVS C++ memory model does not support alignment     *)
(*   (yet). The standard specifies that compound types (structs) can be      *)
(*   accessed on a per-member basis, i.e. they need not be initialized       *)
(*   completely.  On the other hand, they come with contructors/destructors  *)
(*   and member functions.  So which "view" do we need -- "the whole thing   *)
(*   is a type" view, or the "composition of members" view?  Note that the   *)
(*   former, due to what could be considered a deficiency in our PVS C++     *)
(*   memory model, will not allow compound types to contain padding (i.e.    *)
(*   bits that are never affected by write operations).  Edit: default       *)
(*   constructors take an address (although in C++/Elsa, they technically    *)
(*   take no argument at all) and return an object.  This can be achieved    *)
(*   only through interpreting the memory contents at the address as an      *)
(*   object, i.e. we need the from_byte function provided by                 *)
(*   interpreted_data_type? .                                                *)
(* - (Non-static, non-constructor) member functions in the AST take an extra *)
(*   "receiver" argument which is a reference to the associated object.  We  *)
(*   treat references as addresses; unlike pointers, they are never stored   *)
(*   in memory directly.                                                     *)
(* - If a function's body does not end with a return statement, we append a  *)
(*   "return_void" statement when the function body is rendered.             *)

(* TODO: most AST constructors are not translated yet.  For some, "assert    *)
(*       false" will raise an exception, but others are just ignored         *)
(*       silently. This is certainly more harmful than good ...              *)

(* TODO: - type casts, conversions (eg. int to bool)?                        *)
(*       - __attribute__                                                     *)
(*       - Pretty-printing currently ignores the layout of the C++ program.  *)
(*         To obtain more readable output, one might want to preserve        *)
(*         whitespace, certain comments etc.                                 *)
(*       - It would be nice if the pre-/postcondition was taken from an      *)
(*         annotation in the C++ program.                                    *)
(*       - arrays                                                            *)

(* flag for debug output *)

let debug = false

let trace s = if debug then Format.print_string s else ()

(* output channel for PVS file *)

let out = ref stdout

(* ------------------------------------------------------------------------- *)
(* AST translation code                                                      *)
(* ------------------------------------------------------------------------- *)

open Cc_ml_types

let string_of_effectOp = function
  | EFF_POSTINC -> "postinc"
  | EFF_POSTDEC	-> "postdec"
  | EFF_PREINC	-> "preinc"
  | EFF_PREDEC	-> "predec"

let string_of_overloadableOp = function
  | OP_NOT         -> "not"
  | OP_BITNOT	   -> "bitnot"
  | OP_PLUSPLUS	   -> "plusplus"
  | OP_MINUSMINUS  -> "minusminus"
  | OP_PLUS	   -> "plus"
  | OP_MINUS	   -> "minus"
  | OP_STAR	   -> "star"
  | OP_AMPERSAND   -> "ampersand"
  | OP_DIV	   -> "div"
  | OP_MOD	   -> "mod"
  | OP_LSHIFT	   -> "lshift"
  | OP_RSHIFT	   -> "rshift"
  | OP_BITXOR	   -> "bitxor"
  | OP_BITOR	   -> "bitor"
  | OP_ASSIGN	   -> "assign"
  | OP_PLUSEQ	   -> "pluseq"
  | OP_MINUSEQ	   -> "minuseq"
  | OP_MULTEQ	   -> "multeq"
  | OP_DIVEQ	   -> "diveq"
  | OP_MODEQ	   -> "modeq"
  | OP_LSHIFTEQ	   -> assert false; "lshifteq"
  | OP_RSHIFTEQ	   -> assert false; "rshifteq"
  | OP_BITANDEQ	   -> assert false; "bitandeq"
  | OP_BITXOREQ	   -> assert false; "bitxoreq"
  | OP_BITOREQ	   -> assert false; "bitoreq"
  | OP_EQUAL	   -> "=="
  | OP_NOTEQUAL	   -> "not_equal"
  | OP_LESS	   -> "<"
  | OP_GREATER	   -> ">"
  | OP_LESSEQ	   -> "<="
  | OP_GREATEREQ   -> ">="
  | OP_AND	   -> "and_exp"
  | OP_OR	   -> "or_exp"
  | OP_ARROW	   -> assert false; "arrow"
  | OP_ARROW_STAR  -> assert false; "arrow_star"
  | OP_BRACKETS	   -> assert false; "brackets"
  | OP_PARENS	   -> assert false; "parens"
  | OP_COMMA	   -> "comma"
  | OP_QUESTION	   -> assert false; "question"
  | OP_MINIMUM	   -> assert false; "minimum"
  | OP_MAXIMUM	   -> assert false; "maximum"

let string_of_unaryOp = function
  | UNY_PLUS    -> "unary_plus"    (*TODO*)
  | UNY_MINUS   -> "unary_minus"   (*TODO*)
  | UNY_NOT     -> "unary_not"     (*TODO*)
  | UNY_BITNOT  -> "unary_bitnot"  (*TODO*)

let string_of_binaryOp = function
  | BIN_EQUAL       -> "=="
  | BIN_NOTEQUAL    -> "not_equal"
  | BIN_LESS	    -> "<"
  | BIN_GREATER	    -> ">"
  | BIN_LESSEQ	    -> "<="
  | BIN_GREATEREQ   -> ">="
  | BIN_MULT	    -> "*"
  | BIN_DIV	    -> "div"
  | BIN_MOD	    -> "rem"
  | BIN_PLUS	    -> "+"
  | BIN_MINUS	    -> "-"
  | BIN_LSHIFT	    -> "<<"
  | BIN_RSHIFT	    -> ">>"
  | BIN_BITAND	    -> "bitand"
  | BIN_BITXOR	    -> "bitxor"
  | BIN_BITOR	    -> "bitor"
  | BIN_AND	    -> "and_exp"
  | BIN_OR	    -> "or_exp"
  | BIN_COMMA	    -> "comma"
  | BIN_MINIMUM	    -> assert false; "minimum"
  | BIN_MAXIMUM	    -> assert false; "maximum"
  | BIN_BRACKETS    -> assert false; "brackets"
  | BIN_ASSIGN	    -> "assign"
  | BIN_DOT_STAR    -> assert false; "dot_star"
  | BIN_ARROW_STAR  -> assert false; "arrow_star"
  | BIN_IMPLIES	    -> assert false; "implies"
  | BIN_EQUIVALENT  -> assert false; "equivalent"

(* binary operator, used in an assignment *)
let string_of_assignOp = function
  | BIN_EQUAL       -> assert false
  | BIN_NOTEQUAL    -> assert false
  | BIN_LESS	    -> assert false
  | BIN_GREATER	    -> assert false
  | BIN_LESSEQ	    -> assert false
  | BIN_GREATEREQ   -> assert false
  | BIN_MULT	    -> "assign_times"
  | BIN_DIV	    -> "assign_div"
  | BIN_MOD	    -> "assign_rem"
  | BIN_PLUS	    -> "assign_plus"
  | BIN_MINUS	    -> "assign_minus"
  | BIN_LSHIFT	    -> "assign_bsl"
  | BIN_RSHIFT	    -> "assign_bsr"
  | BIN_BITAND	    -> "assign_bitand"
  | BIN_BITXOR	    -> "assign_bitxor"
  | BIN_BITOR	    -> "assign_bitor"
  | BIN_AND	    -> assert false
  | BIN_OR	    -> assert false
  | BIN_COMMA	    -> assert false
  | BIN_MINIMUM	    -> assert false
  | BIN_MAXIMUM	    -> assert false
  | BIN_BRACKETS    -> assert false
  | BIN_ASSIGN	    -> "assign"
  | BIN_DOT_STAR    -> assert false
  | BIN_ARROW_STAR  -> assert false
  | BIN_IMPLIES	    -> assert false
  | BIN_EQUIVALENT  -> assert false

let string_of_simpleTypeId = function
  (* actual C++ types *)
  | ST_CHAR                  -> "char"
  | ST_UNSIGNED_CHAR         -> "uchar"
  | ST_SIGNED_CHAR           -> "schar"
  | ST_BOOL                  -> "bool"
  | ST_INT                   -> "int"
  | ST_UNSIGNED_INT          -> "uint"
  | ST_LONG_INT              -> "long"
  | ST_UNSIGNED_LONG_INT     -> "ulong"
  | ST_LONG_LONG             -> "longlong"
  | ST_UNSIGNED_LONG_LONG    -> "ulonglong"
  | ST_SHORT_INT             -> "short"
  | ST_UNSIGNED_SHORT_INT    -> "ushort"
  | ST_WCHAR_T               -> "wchar_t"
  | ST_FLOAT                 -> assert false; "float"
  | ST_DOUBLE                -> assert false; "double"
  | ST_LONG_DOUBLE           -> assert false; "long double"
  | ST_FLOAT_COMPLEX         -> assert false; "float complex"
  | ST_DOUBLE_COMPLEX        -> assert false; "double complex"
  | ST_LONG_DOUBLE_COMPLEX   -> assert false; "long double complex"
  | ST_FLOAT_IMAGINARY       -> assert false; "float imaginary"
  | ST_DOUBLE_IMAGINARY      -> assert false; "double imaginary"
  | ST_LONG_DOUBLE_IMAGINARY -> assert false; "long double imaginary"
  | ST_VOID                  -> "void"
  (* encoding "hacks" *)
  | ST_ELLIPSIS              -> assert false; ">>>>>ellipsis<<<<<"  (*TODO*)
  | ST_CDTOR                 -> assert false; ">>>>>cdtor<<<<<"  (*TODO*)
  | ST_ERROR                 -> assert false; "error"
  | ST_DEPENDENT             -> assert false; "dependent"
  | ST_IMPLINT               -> assert false; "implint"
  | ST_NOTFOUND              -> assert false; "notfound"
  | ST_PROMOTED_INTEGRAL     -> assert false; "promoted integral"
  | ST_PROMOTED_ARITHMETIC   -> assert false; "promoted arithmetic"
  | ST_INTEGRAL              -> assert false; "integral"
  | ST_ARITHMETIC            -> assert false; "arithmetic"
  | ST_ARITHMETIC_NON_BOOL   -> assert false; "arithmetic non bool"
  | ST_ANY_OBJ_TYPE          -> assert false; "any obj type"
  | ST_ANY_NON_VOID          -> assert false; "any non void"
  | ST_ANY_TYPE              -> assert false; "any type"
  | ST_PRET_STRIP_REF        -> assert false; "pret strip ref"
  | ST_PRET_PTM              -> assert false; "pret ptm"
  | ST_PRET_ARITH_CONV       -> assert false; "pret arith conv"
  | ST_PRET_FIRST            -> assert false; "pret first"
  | ST_PRET_FIRST_PTR2REF    -> assert false; "pret first ptr2ref"
  | ST_PRET_SECOND           -> assert false; "pret second"
  | ST_PRET_SECOND_PTR2REF   -> assert false; "pret second ptr2ref"

let string_of_declFlag = function
  | DF_AUTO         		-> assert false; "auto"
  | DF_REGISTER     		-> assert false; "register"
  | DF_STATIC       		-> assert false; "static"
  | DF_EXTERN       		-> assert false; "extern"
  | DF_MUTABLE      		-> assert false; "mutable"
  | DF_INLINE       		-> assert false; "inline"
  | DF_VIRTUAL      		-> assert false; "virtual"
  | DF_EXPLICIT     		-> assert false; "explicit"
  | DF_FRIEND       		-> assert false; "friend"
  | DF_TYPEDEF      		-> assert false; "typedef"
  | DF_NAMESPACE    		-> assert false; "namespace"
  | DF_ENUMERATOR   		-> assert false; "enumerator"
  | DF_GLOBAL       		-> assert false; "global"
  | DF_INITIALIZED  		-> assert false; "initialized"
  | DF_BUILTIN      		-> assert false; "builtin"
  | DF_PARAMETER    		-> assert false; "parameter"
  | DF_MEMBER       		-> assert false; "member"
  | DF_DEFINITION   		-> assert false; "definition"
  | DF_INLINE_DEFN  		-> assert false; "inline_defn"
  | DF_IMPLICIT     		-> assert false; "implicit"
  | DF_FORWARD      		-> assert false; "forward"
  | DF_TEMPORARY    		-> assert false; "temporary"
  | DF_EXTERN_C     		-> assert false; "extern_c"
  | DF_SELFNAME     		-> assert false; "selfname"
  | DF_BOUND_TPARAM 		-> assert false; "bound_tparam"
  | DF_TEMPL_PARAM  		-> assert false; "templ_param"
  | DF_USING_ALIAS  		-> assert false; "using_alias"
  | DF_BITFIELD     		-> assert false; "bitfield"
  | DF_GNU_EXTERN_INLINE 	-> assert false; "gnu extern inline"
  | DF_ADDRTAKEN		-> assert false; "addrtaken"
  | DF_UNIVERSAL		-> assert false; "universal"
  | DF_EXISTENTIAL		-> assert false; "existential"


(* ------------------------------------------------------------------------- *)
(*  Copyright 2006 Hendrik Tews, All rights reserved.                        *)
(*  See file license.txt for terms of use                                    *)
(* ------------------------------------------------------------------------- *)

open Cc_ast_gen_type
open Ml_ctype
open Ast_annotation
open Ast_accessors

(* include Library *)
include Option

(* separate f s [x1; x2]  =  ignore (f x1); ignore (s ()); ignore (f x2) *)
let separate f s =
  let rec sep = function
      x1::x2::xs -> ignore (f x1); ignore (s ()); sep (x2::xs)
    | [x1]       -> ignore (f x1)
    | []         -> ()
  in sep

(* last [x1; ...; xn]  =  xn *)
let rec last = function
    []  -> raise (Failure "last")
  | [x] -> x
  | xs  -> last (List.tl xs)

(**************************************************************************
 *
 * contents of astiter.ml
 *
 **************************************************************************)

let annotation_fun (a : annotated) =
  Format.printf "(%d, %d)" (id_annotation a) (caddr_annotation a)

let bool_fun (b : bool) = Format.print_bool b

let int_fun (i : int) = Format.print_int i

let nativeint_fun (i : nativeint) =
  (*TODO*)
  Format.printf "nativeint_fun: %nd\n" i

let int32_fun (i : int32) =
  Format.printf "%ld" i

let float_fun (x : float) = (assert false; Format.printf "float_fun: %f\n" x)

let string_fun (s : string) =
  (* replace initial '~' by 'destructor_' *)
  let s =
    if String.get s 0 = '~' then
      "destructor_" ^ String.sub s 1 (String.length s - 1)
    else s in
  (* replace final '=' by '_assign' *)
  let s =
    if String.get s (String.length s - 1) = '=' then
      String.sub s 0 (String.length s - 1) ^ "_assign"
    else s in
  (* replace '-' by '_' *)
  let string_map f s =
    for n = 0 to String.length s - 1 do
      String.set s n (f (String.get s n))
    done
  in
  let _ = string_map (function '-' -> '_' | c -> c) s in
  (* remove leading '_'s *)
  let rec string_index p s n =
    if p (String.get s n) then n
    else
      try
	string_index p s (n+1)
      with Invalid_argument _ -> raise Not_found in
  let start = string_index (fun c -> c <> '_') s 0 in
    Format.print_string (String.sub s start (String.length s - start))

let sourceLoc_fun ((file : string), (line : int), (char : int)) =
  trace "sourceLoc_fun(";
  Format.printf "(%s, %d, %d)" file line char;
  trace ")"

let declFlags_fun (dFs : declFlag list) =
  trace "declFlags_fun(";
  trace (string_of_declFlags dFs);
  trace ")"

let simpleTypeId_fun (id : simpleTypeId) =
  trace "simpleTypeId_fun(";
  Format.print_string (string_of_simpleTypeId id);
  trace ")"

let typeIntr_fun (_ : typeIntr) = 
  trace "typeIntr_fun()"  (*TODO*)

let accessKeyword_fun (keyword : accessKeyword) =
  Format.print_string (string_of_accessKeyword keyword)

let cVFlags_fun (fls : cVFlag list) =
  trace "cVFlags_fun(";
  trace (string_of_cVFlags fls);
  trace ")"

let overloadableOp_fun (op : overloadableOp) =
  Format.print_string (string_of_overloadableOp op)

let unaryOp_fun (op : unaryOp) =
  Format.print_string (string_of_unaryOp op)

let effectOp_fun (op : effectOp) =
  Format.print_string (string_of_effectOp op)

let binaryOp_fun (op : binaryOp) =
  Format.print_string (string_of_binaryOp op)

let assignOp_fun (op : binaryOp) =
  Format.print_string (string_of_assignOp op)

let castKeyword_fun (_ : castKeyword) =
  trace "castKeyword_fun()"  (*TODO*)

let function_flags_fun (_ : function_flags) =
  trace "function_flags_fun()"  (*TODO*)

let declaratorContext_fun (dc : declaratorContext) =
  trace "declaratorContext_fun(";
  trace (string_of_declaratorContext dc);
  trace ")"

let scopeKind_fun (_ : scopeKind) = ()

let array_size_fun = function
  | NO_SIZE -> assert false
  | DYN_SIZE -> assert false
  | FIXED_SIZE i -> assert false; int_fun i

let compoundType_Keyword_fun = function
  | K_STRUCT -> Format.print_string "struct"
  | K_CLASS -> Format.print_string "class"
  | K_UNION -> Format.print_string "union"

let templ_kind_fun (kind : templateThingKind) =
  assert false

(***************** variable ***************************)

let rec variable_fun (v : annotated variable) =
  (* unused record copy to provoke compilation errors for new fields *)
  let _dummy = {			
    poly_var = v.poly_var; loc = v.loc; var_name = v.var_name;
    var_type = v.var_type; flags = v.flags; value = v.value;
    defaultParam = v.defaultParam; funcDefn = v.funcDefn;
    overload = v.overload; virtuallyOverride = v.virtuallyOverride;
    scope = v.scope; templ_info = v.templ_info;
  }
  in
    trace "variable_fun(";
    (match v.var_name with
      | Some s -> string_fun s;
	  (*
	    Format.print_string "_";
	    Format.print_int (id_annotation v.poly_var);
	  *)
      | None   -> assert false);
    (*
    (* POSSIBLY CIRCULAR *)
      Option.app cType_fun !(v.var_type);
      declFlags_fun v.flags;
    (* POSSIBLY CIRCULAR *)
      Option.app expression_fun !(v.value);
      Option.app cType_fun v.defaultParam;

    (* POSSIBLY CIRCULAR *)
      Option.app func_fun !(v.funcDefn);
    (* POSSIBLY CIRCULAR *)
      List.iter variable_fun !(v.overload);
      List.iter variable_fun v.virtuallyOverride;

      Option.app scope_fun v.scope;

      Option.app templ_info_fun v.templ_info;
    *)
    trace ")";

(**************** templateInfo ************************)

and templ_info_fun ti =
  (* unused record copy to provoke compilation errors for new fields *)
  let _dummy = {
    poly_templ = ti.poly_templ; templ_kind = ti.templ_kind;
    template_params = ti.template_params;
    template_var = ti.template_var; inherited_params = ti.inherited_params; 
    instantiation_of = ti.instantiation_of; 
    instantiations = ti.instantiations; 
    specialization_of = ti.specialization_of; 
    specializations = ti.specializations; arguments = ti.arguments; 
    inst_loc = ti.inst_loc; 
    partial_instantiation_of = ti.partial_instantiation_of; 
    partial_instantiations = ti.partial_instantiations; 
    arguments_to_primary = ti.arguments_to_primary; 
    defn_scope = ti.defn_scope; 
    definition_template_info = ti.definition_template_info; 
    instantiate_body = ti.instantiate_body; 
    instantiation_disallowed = ti.instantiation_disallowed; 
    uninstantiated_default_args = ti.uninstantiated_default_args; 
    dependent_bases = ti.dependent_bases;
  }
  in
    begin
      assert false;

      annotation_fun ti.poly_templ;
      templ_kind_fun ti.templ_kind;
      List.iter variable_fun ti.template_params;

      (* POSSIBLY CIRCULAR *)
      Option.app variable_fun !(ti.template_var);
      List.iter inherited_templ_params_fun ti.inherited_params;

      (* POSSIBLY CIRCULAR *)
      Option.app variable_fun !(ti.instantiation_of);
      List.iter variable_fun ti.instantiations;

      (* POSSIBLY CIRCULAR *)
      Option.app variable_fun !(ti.specialization_of);
      List.iter variable_fun ti.specializations;
      List.iter sTemplateArgument_fun ti.arguments;
      sourceLoc_fun ti.inst_loc;

      (* POSSIBLY CIRCULAR *)
      Option.app variable_fun !(ti.partial_instantiation_of);
      List.iter variable_fun ti.partial_instantiations;
      List.iter sTemplateArgument_fun ti.arguments_to_primary;
      Option.app scope_fun ti.defn_scope;
      Option.app templ_info_fun ti.definition_template_info;
      bool_fun ti.instantiate_body;
      bool_fun ti.instantiation_disallowed;
      int_fun ti.uninstantiated_default_args;
      List.iter cType_fun ti.dependent_bases;
    end

(************* inheritedTemplateParams ****************)

and inherited_templ_params_fun itp =
  (* unused record copy to provoke compilation errors for new fields *)
  let _dummy = {
    poly_inherited_templ = itp.poly_inherited_templ;
    inherited_template_params = itp.inherited_template_params;
    enclosing = itp.enclosing;
  }
  in
    begin
      assert false;
      assert (!(itp.enclosing) <> None);
      annotation_fun itp.poly_inherited_templ;
      List.iter variable_fun itp.inherited_template_params;

      (* POSSIBLY CIRCULAR *)
      Option.app compound_info_fun !(itp.enclosing);
    end

(***************** cType ******************************)

and baseClass_fun baseClass =
  (* unused record copy to provoke compilation errors for new fields *)
  let _dummy = {
    poly_base = baseClass.poly_base; compound = baseClass.compound;
    bc_access = baseClass.bc_access; is_virtual = baseClass.is_virtual
  } in
    begin
      assert false;
      annotation_fun baseClass.poly_base;
      compound_info_fun baseClass.compound;
      accessKeyword_fun baseClass.bc_access;
      bool_fun baseClass.is_virtual
    end


and compound_info_fun i = 
  (* unused record copy to provoke compilation errors for new fields *)
  let _dummy = {
    compound_info_poly = i.compound_info_poly;
    compound_name = i.compound_name; typedef_var = i.typedef_var;
    ci_access = i.ci_access; compound_scope = i.compound_scope;
    is_forward_decl = i.is_forward_decl;
    is_transparent_union = i.is_transparent_union; keyword = i.keyword;
    data_members = i.data_members; bases = i.bases;
    conversion_operators = i.conversion_operators;
    friends = i.friends; inst_name = i.inst_name; syntax = i.syntax;
    self_type = i.self_type;
  } in
    begin
      trace "compound_info_fun(";
      assert(match !(i.syntax) with
	       | None -> true
	       | Some (TS_classSpec _) -> true
	       | _ -> false);
(*
      annotation_fun i.compound_info_poly;
      Option.app string_fun i.compound_name;
*)
      variable_fun i.typedef_var;
(*
      accessKeyword_fun i.ci_access;
      scope_fun i.compound_scope;
      bool_fun i.is_forward_decl;
      bool_fun i.is_transparent_union;
      compoundType_Keyword_fun i.keyword;
      List.iter variable_fun i.data_members;
      List.iter baseClass_fun i.bases;
      List.iter variable_fun i.conversion_operators;
      List.iter variable_fun i.friends;
      Option.app typeSpecifier_fun !(i.syntax);

      (* POSSIBLY CIRCULAR *)
      Option.app string_fun i.inst_name;

      (* POSSIBLY CIRCULAR *)
      Option.app cType_fun !(i.self_type)
*)
      trace ")";
    end


and enum_value_fun (annot, string, nativeint) =
  begin
    trace "enum_value_fun(";
    (*TODO*)
    Format.printf ">>>>>";
    string_fun string;
    nativeint_fun nativeint;
    Format.printf "<<<<<"; 
    trace ")";
  end


and atomicType_fun x =
  match x with
      SimpleType(annot, simpleTypeId) ->
	trace "SimpleType(";
	simpleTypeId_fun simpleTypeId;
	trace ")";

    | CompoundType(compound_info) ->
	trace "CompoundType(";
        compound_info_fun compound_info;
	trace ")";

    | PseudoInstantiation(annot, str, variable_opt, accessKeyword, 
			 compound_info, sTemplateArgument_list) ->
	assert false;
	string_fun str;
	Option.app variable_fun variable_opt;
	accessKeyword_fun accessKeyword;
	compound_info_fun compound_info;
	List.iter sTemplateArgument_fun sTemplateArgument_list;

    | EnumType(annot, string, variable, accessKeyword, 
	      enum_value_list, has_negatives) ->
	trace "EnumType(";
	assert (Option.isSome string);  (* anonymous enums not supported yet *)
	Option.app string_fun string;  (*identical to variable name?*)
	(*Option.app variable_fun variable;
	accessKeyword_fun accessKeyword;
	List.iter enum_value_fun enum_value_list;
	bool_fun has_negatives;*)
	trace ")";

    | TypeVariable(annot, string, variable, accessKeyword) ->
	assert false;
	string_fun string;
	variable_fun variable;
	accessKeyword_fun accessKeyword;

    | DependentQType(annot, string, variable, 
		    accessKeyword, atomic, pq_name) ->
	assert false;
	string_fun string;
	variable_fun variable;
	accessKeyword_fun accessKeyword;
	atomicType_fun atomic;
	pQName_fun pq_name;


and cType_fun x = 
  match x with
      CVAtomicType(annot, cVFlags, atomicType) ->
	trace "CVAtomicType(";
	cVFlags_fun cVFlags;
	atomicType_fun atomicType;
	trace ")";

    | PointerType(annot, cVFlags, cType) ->
	trace "PointerType(";
	(*TODO*)
	cVFlags_fun cVFlags;
	Format.print_string "pointer[dt_";	
	cType_fun cType;
	Format.print_string "]";
	trace ")";

    | ReferenceType(annot, cType) ->
	trace "ReferenceType(";
	(*TODO*)
	Format.print_string "reference[dt_";
	cType_fun cType;
	Format.print_string "]";
	trace ")";

    | FunctionType(annot, function_flags, cType,
		  variable_list, cType_list_opt) ->
	trace "FunctionType(";
	(*TODO*)
	function_flags_fun function_flags;
	cType_fun cType;
	List.iter variable_fun variable_list;
	Option.app (List.iter cType_fun) cType_list_opt;
	trace ")";

    | ArrayType(annot, cType, array_size) ->
	trace "ArrayType(";
	(*TODO*)
	cType_fun cType;
	array_size_fun array_size;
	trace ")";

    | DependentSizeArrayType (annot, cType, expression) ->
	trace "DependentSizeArrayType(";
	(*TODO*)
	assert false;
	cType_fun cType;
	expression_fun expression;
	trace ")";

    | PointerToMemberType(annot, atomicType (* = NamedAtomicType *), 
			 cVFlags, cType) ->
	assert false;
	assert(match atomicType with 
	  | SimpleType _ -> false
	  | CompoundType _
	  | PseudoInstantiation _
	  | EnumType _
	  | TypeVariable _ 
	  | DependentQType _ -> true);
	atomicType_fun atomicType;
	cVFlags_fun cVFlags;
	cType_fun cType


and derefType = function
    ReferenceType (_, cType) -> cType
  | cType -> cType


and sTemplateArgument_fun ta = 
  let _ = assert false
  in match ta with
    | STA_NONE annot -> 
	annotation_fun annot

    | STA_TYPE(annot, cType) -> 
	annotation_fun annot;
	cType_fun cType

    | STA_INT(annot, int) -> 
	annotation_fun annot;
	int_fun int

    | STA_ENUMERATOR(annot, variable) -> 
	annotation_fun annot;
	variable_fun variable

    | STA_REFERENCE(annot, variable) -> 
	annotation_fun annot;
	variable_fun variable

    | STA_POINTER(annot, variable) -> 
	annotation_fun annot;
	variable_fun variable

    | STA_MEMBER(annot, variable) -> 
	annotation_fun annot;
	variable_fun variable

    | STA_DEPEXPR(annot, expression) -> 
	annotation_fun annot;
	expression_fun expression

    | STA_TEMPLATE annot -> 
	annotation_fun annot

    | STA_ATOMIC(annot, atomicType) -> 
	annotation_fun annot;
	atomicType_fun atomicType


and scope_fun s = 
  (* unused record copy to provoke compilation errors for new fields *)
  let _dummy = {
    poly_scope = s.poly_scope; variables = s.variables; 
    type_tags = s.type_tags; parent_scope = s.parent_scope;
    scope_kind = s.scope_kind; namespace_var = s.namespace_var;
    scope_template_params = s.scope_template_params; 
    parameterized_entity = s.parameterized_entity;
    scope_compound = s.scope_compound
  }
  in
    trace "scope_fun(";
(*
    Hashtbl.iter 
      (fun str var -> string_fun str; variable_fun var)
      s.variables;
    Hashtbl.iter
      (fun str var -> string_fun str; variable_fun var)
      s.type_tags;
    Option.app scope_fun s.parent_scope;
    scopeKind_fun s.scope_kind;
    Option.app variable_fun !(s.namespace_var);
    List.iter variable_fun s.scope_template_params;
    Option.app variable_fun s.parameterized_entity;
*)
    trace ")";


(***************** generated ast nodes ****************)

and translationUnit_fun
  ((annot, topForm_list, scope_opt) : annotated translationUnit_type) =
    trace "translationUnit_fun(";
    List.iter (fun x -> topForm_fun x; Format.printf "@\n@\n") topForm_list;
    trace ")";

and topForm_fun x =
  match x with
    | TF_decl (annot, sourceLoc, declaration) ->
	trace "TF_decl(";
	(* a variable or type declaration at file scope *)
        declaration_fun declaration;
	trace ")";

    | TF_func(annot, sourceLoc, func) ->
	trace "TF_func(";
	func_fun func;
	trace ")";

    | TF_template(annot, sourceLoc, templateDeclaration) ->
	trace "TF_template(";
	(*TODO*)
	Format.printf ">>>>>";
	templateDeclaration_fun templateDeclaration;
	Format.printf "<<<<<";
	trace ")";

    | TF_explicitInst(annot, sourceLoc, declFlags, declaration) ->
	assert false;
	declFlags_fun declFlags;
	declaration_fun declaration

    | TF_linkage(annot, sourceLoc, stringRef, translationUnit) ->
        trace "TF_Linkage(";
	(*TODO*)
	string_fun stringRef;
	translationUnit_fun translationUnit;
        trace ")";

    | TF_one_linkage(annot, sourceLoc, stringRef, topForm) ->
        trace "TF_one_linkage(";
	(*string_fun stringRef;*)  (* "C" for "extern C" -- ignore ?! *)
	topForm_fun topForm;
        trace ")";

    | TF_asm(annot, sourceLoc, e_stringLit) ->
	trace "TF_asm";
        assert false;
	assert(match e_stringLit with E_stringLit _ -> true | _ -> false);
	expression_fun e_stringLit;
	trace ")";

    | TF_namespaceDefn(annot, sourceLoc, stringRef_opt, topForm_list) ->
	trace "TF_namespaceDefn(";
	(*TODO*)
	Option.app string_fun stringRef_opt;
	List.iter topForm_fun topForm_list;
	trace ")";

    | TF_namespaceDecl(annot, sourceLoc, namespaceDecl) ->
	trace "TF_namespaceDecl(";
	namespaceDecl_fun namespaceDecl;
	trace ")";


and func_fun (annot, declFlags, typeSpecifier, declarator, memberInit_list,
	     s_compound_opt, handler_list, func, variable_opt_1,
	     variable_opt_2, statement_opt, bool) =
  begin
    trace "func_fun(";

    assert(match s_compound_opt with
      | None -> true
      | Some (S_compound _) -> true
      | _ -> false);
    assert(match func with
      | FunctionType _ -> true
      | _ -> false);

    (* TODO: static, extern etc.; not supported yet *)
    declFlags_fun declFlags;

    (* type specifier for return value *)
    (* TODO: what's the difference between this and the type in declarator? *)
    (*typeSpecifier_fun typeSpecifier;*)

    (* function definition: f(addresses) = body *)

    Format.printf "@[<2>";

    (* - remainder of return value type *)
    (* - name of function               *)
    (* - names/types of parameters      *)
    declarator_fun declarator;

    (* (for ctors only) member initialization list *)
    List.iter memberInit_fun memberInit_list;

    (* TODO: s_compound_opt may be None, but when can this happen (apparently
             in in/t0524.cc, but in general? - seems obscure) *)
    assert (Option.isSome s_compound_opt);
    Option.app statement_fun s_compound_opt;

    (match declarator with (_, _, _, variable_opt, _, _, _, _) ->
      if (Option.valOf variable_opt).var_name = Some "constructor_special" then
	(* append return statement to the function body if this function is a constructor *)
	begin
	  Format.printf " ##@\nreturn(read_data(pm, dt_";
	  Format.print_string
	    (enclosing_classname_of_member (Option.valOf variable_opt));
	  Format.printf ")(receiver))";
	end
      else
	(* append "return_void" to the function body if necessary *)
	(match s_compound_opt with
	  | Some (S_compound (_, _, statement_list)) ->
	      if List.length statement_list = 0 ||
		(match last statement_list with
		  | S_return _ -> false
		  | _ -> true) then
		Format.printf " ##@\nreturn_void";
	  | _ ->
	      assert false)
    );

    Format.printf "@]@\n@\n";

    (* call function: call_f(expressions) = ... *)

    Format.printf "@[<2>call_";

    (match declarator with
	(annot, iDeclarator, init_opt, variable_opt, ctype_opt,
	declaratorContext, statement_opt_ctor, statement_opt_dtor) ->

      (* name of function *)
      Option.app variable_fun variable_opt;
      (* append the class name in case it's a constructor or assignment
	 operator *)
      if (Option.valOf variable_opt).var_name = Some "constructor_special"
	|| (Option.valOf variable_opt).var_name = Some "operator="
      then
	begin
	  Format.print_string "_";
	  Format.print_string
	    (enclosing_classname_of_member (Option.valOf variable_opt));
	end;
      Format.printf "@[<2>";
      (* arguments of function *)
      (match ctype_opt with
	| Some (FunctionType (_, _, _, variable_list, _)) ->
	    let 
		var_opt_list = List.map (fun x -> Some x) variable_list
	    in let
		var_list = if (Option.valOf variable_opt).var_name =
		  Some "constructor_special" then
		  None :: var_opt_list else var_opt_list
	    in
		 if List.length var_list > 0 then
		   begin
		     Format.printf "@[<2>(";
		     separate (fun v ->
		       (match v with
			 | Some var ->
			     Format.printf "@[<2>";
			     variable_fun var;
			     Format.printf "@ :@ ET[State,@ Semantics_";
			     assert (Option.isSome !(var.var_type));
			     Option.app cType_fun !(var.var_type);
			     Format.printf "]@]";
			 | None     ->
			     Format.printf
			       "@[<2>receiver@ :@ ET[State,@ Semantics_reference[dt_";
			     Format.print_string
			       (enclosing_classname_of_member (Option.valOf variable_opt));
			     Format.printf "]]@]";
		       ))
		       (fun () -> Format.printf ",@ ") var_list;
		     Format.printf ")@]";
		   end;
	| _ ->
	    assert false);
      Format.printf "@ :@ ET[State,@ Semantics_";
      (* remainder of return value type *)
      (* TODO: is this different from the return type? What does
         "remainder" mean? *)
      Option.app (function
	| FunctionType (_, _, cType, _, _) ->
	    cType_fun cType;
	| _ ->
	    assert false) ctype_opt;
      Format.printf "]@ =@]@\n";

      (* arguments of function *)
      (match ctype_opt with
	| Some (FunctionType (_, _, _, variable_list, _)) ->
	    let 
		var_opt_list = List.map (fun x -> Some x) variable_list
	    in let
		var_list = if (Option.valOf variable_opt).var_name =
		  Some "constructor_special" then
		  None :: var_opt_list else var_opt_list
	    in
		 if List.length var_list > 0 then
		   List.iter (fun v ->
		     (match v with
		       | Some var ->
			   (*reference arguments are not copied onto the stack*)
			   let has_reference_type var =
			     match Option.valOf !(var.var_type) with
			       | ReferenceType _ -> true
			       | _ -> false
			   in
			     if has_reference_type var then
			       begin
				 Format.printf "@[<2>";
				 variable_fun var;
				 Format.printf "@ ##@ @[<2>(lambda@ @[<2>(";
				 variable_fun var;
				 Format.printf "_addr@ :@ Address)@]@]:@\n";
			       end
			     else
			       begin
				 Format.printf "@[<2>with_new_stackvar(@[<2>lambda@ @[<2>(";
				 variable_fun var;
				 Format.printf "_addr@ :@ Address)@]@]:@\n";
				 Format.printf "@[<2>e2s(assign(pm, dt_";
				 assert (Option.isSome !(var.var_type));
				 Option.app cType_fun !(var.var_type);
				 Format.printf ")(id(";
				 variable_fun var;
				 Format.printf "_addr),@ ";
				 variable_fun var;
				 Format.printf "))@] ##@\n";
			       end
		       | None ->
			   Format.printf "@[<2>receiver@ ##@ @[<2>(lambda@ @[<2>(receiver_addr@ :@ Address)@]@]:@\n"))
		     (* evaluation order in gcc is right to left *)
		     (List.rev var_list);
	| _ ->
	    assert false);

      (* name of function *)
      Option.app variable_fun variable_opt;
      (* append the class name in case it's a constructor or assignment
	 operator *)
      if (Option.valOf variable_opt).var_name = Some "constructor_special"
	|| (Option.valOf variable_opt).var_name = Some "operator="
      then
	begin
	  Format.print_string "_";
	  Format.print_string
	    (enclosing_classname_of_member (Option.valOf variable_opt));
	end;
      (* arguments of function *)
      (match ctype_opt with 
	| Some (FunctionType (_, _, _, variable_list, _)) ->
	    let 
		var_opt_list = List.map (fun x -> Some x) variable_list
	    in let
		var_list = if (Option.valOf variable_opt).var_name =
		  Some "constructor_special" then
		  None :: var_opt_list else var_opt_list
	    in
		 if List.length var_list > 0 then
		   begin
		     Format.printf "@[<2>(";
		     separate (fun v ->
		       (match v with
			 | Some var ->
			     variable_fun var;
			     Format.print_string "_addr";
			 | None ->
			     Format.print_string "receiver_addr";
		       )) (fun () -> Format.printf ",@ ") var_list;
		     Format.printf ")@]";
		     List.iter (fun _ -> Format.printf ")@]") var_list;
		   end;
	| _ ->
	    assert false);
    );

    Format.printf " ##@\n";
    Format.print_string "catch_return";
    Format.printf "@]";

    (* TODO: handlers for ctor "try" block *)
    assert (List.length handler_list = 0);
    List.iter handler_fun handler_list;

    (* TODO: what is this? contains return type, argument list - how is this
             different from the information in declarator? *)
    (*
      cType_fun func;
    *)

    (* TODO: what is this; a "receiver" variable in case of member functions? *)
    (*
      Option.app variable_fun v variable_opt_1;
    *)

    (* TODO: what is this; a "<retVar>" variable in case of constructors? *)
    (*
      Option.app variable_fun variable_opt_2;
    *)

    (* TODO: what is this; an empty compound statement for trivial destructors? *)
    assert (match statement_opt with
      | None -> true
      | Some (S_compound (_, _, [])) -> true
      | _ -> false);
    (*
      Option.app statement_fun statement_opt;
    *)

    (* TODO: what is this? *)
    (*
      bool_fun bool;
    *)

    trace ")";
  end


and enclosing_classname_of_member (v : annotated variable) : string =
  Option.valOf
    (Option.valOf !((Option.valOf v.scope).scope_compound)).compound_name


and memberInit_fun (annot, pQName, argExpression_list, 
		    variable_opt_1, compound_opt, variable_opt_2, 
		    full_expr_annot, statement_opt) =
  begin
    trace "memberInit_fun(";
    assert (match compound_opt with
      | None
      | Some (CompoundType _) -> true
      | _ -> false);
    Format.printf "e2s(assign(pm, dt_";
    (* type of member *)
    (function
      | PQ_variable (_, _, variable) ->
	  assert (Option.isSome !(variable.var_type));
	  Option.app cType_fun !(variable.var_type);
      | _ ->
	  assert false) pQName;
    Format.printf ")(@[<2>member(@[<2>id(receiver),@ offsets_";
    (* class type *)
    Format.print_string
      (enclosing_classname_of_member (Option.valOf variable_opt_1));
    Format.print_string  "`";
    pQName_fun pQName;  (* name of member *)
    Format.printf "@]),@ ";
    assert (List.length argExpression_list = 1);
    List.iter argExpression_fun argExpression_list;
    Format.printf "@])) ##@\n";

    (* TODO: ? *)
    (* is this different from pQName ? *)
    assert (Option.isSome variable_opt_1);
    assert (not (Option.isSome compound_opt));
    assert (not (Option.isSome variable_opt_2));
    assert (not (Option.isSome statement_opt));
    (*
      Option.app variable_fun variable_opt_1;
      Option.app atomicType_fun compound_opt;
      Option.app variable_fun variable_opt_2;
      fullExpressionAnnot_fun full_expr_annot;
      Option.app statement_fun statement_opt;
    *)
    trace ")";
  end


and declaration_fun (annot, declFlags, typeSpecifier, declarator_list) =
  begin
    trace "declaration_fun(";
    declFlags_fun declFlags;
    if List.mem DF_TYPEDEF declFlags then
      (* typedef declaration *)
      begin
	List.iter (function d ->
	  Format.print_string "% typedef ";
	  typedef_declarator_fun d;
	  Format.printf "@\n";
	  Format.printf "@[<2>Semantics_";
	  typedef_declarator_fun d;
	  Format.printf "@ :@ TYPE@ =@ Semantics_";
	  typeSpecifier_fun typeSpecifier;
	  Format.printf "@]@\n";
	  Format.printf "@[<2>dt_";
	  typedef_declarator_fun d;
	  Format.printf "@ :@ (pod_data_type?[Semantics_";
	  typedef_declarator_fun d;
	  Format.printf "])@ =@ dt_";
	  typeSpecifier_fun typeSpecifier;
          Format.printf "@]") declarator_list;
      end
    else
      (* class/variable/function declaration *)
	(*TODO: declaring a type and at the same time a variable of that type,
	  as in    class C { /* ... */ } c;    is not supported yet.*)
	if List.length declarator_list = 0 then
	  typeSpecifier_fun typeSpecifier
	else
	  List.iter declarator_fun declarator_list;
    trace ")";
  end


and aSTTypeId_fun (annot, typeSpecifier, declarator) =
  begin
    assert false;
    annotation_fun annot;
    typeSpecifier_fun typeSpecifier;
    declarator_fun declarator
  end


and pQName_fun x =
  match x with
      PQ_qualifier(annot, sourceLoc, stringRef_opt, 
		  templateArgument_opt, pQName, 
		  variable_opt, s_template_arg_list) ->
	trace "PQ_qualifier(";
	Option.app string_fun stringRef_opt;
	Option.app templateArgument_fun templateArgument_opt;
	pQName_fun pQName;
	Option.app variable_fun variable_opt;
	List.iter sTemplateArgument_fun s_template_arg_list;
	trace ")";

    | PQ_name(annot, sourceLoc, stringRef) ->
	trace "PQ_name(";
	string_fun stringRef;
	trace ")";

    | PQ_operator(annot, sourceLoc, operatorName, stringRef) ->
	trace "PQ_operator(";
	Format.printf ">>>>>";
	operatorName_fun operatorName;
	string_fun stringRef;
	Format.printf "<<<<<";
	trace ")";

    | PQ_template(annot, sourceLoc, stringRef, templateArgument_opt, 
		 s_template_arg_list) ->
	assert false;
	string_fun stringRef;
	Option.app templateArgument_fun templateArgument_opt;
	List.iter sTemplateArgument_fun s_template_arg_list;

    | PQ_variable(annot, sourceLoc, variable) ->
	trace "PQ_variable(";
	variable_fun variable;
	trace ")";


and typeSpecifier_fun x = 
  match x with
    | TS_name(annot, sourceLoc, cVFlags, pQName, bool, 
	     var_opt_1, var_opt_2) ->
	trace "TS_name(";
	(*TODO*)
(*
	cVFlags_fun cVFlags;
	pQName_fun pQName;
	bool_fun bool;
*)
	assert (Option.isSome var_opt_1);
	assert (not (Option.isSome var_opt_2));
	Option.app variable_fun var_opt_1;
	Option.app variable_fun var_opt_2;
	trace ")";

    | TS_simple(annot, sourceLoc, cVFlags, simpleTypeId) ->
	trace "TS_simple(";
	cVFlags_fun cVFlags;
	simpleTypeId_fun simpleTypeId;
	trace ")";

    | TS_elaborated(annot, sourceLoc, cVFlags, typeIntr, 
		   pQName, namedAtomicType_opt) ->
	trace "TS_elaborated(";
	Format.printf ">>>>>";
	assert(match namedAtomicType_opt with
	  | Some(SimpleType _) -> false
	  | _ -> true);
	annotation_fun annot;
	sourceLoc_fun sourceLoc;
	cVFlags_fun cVFlags;
	typeIntr_fun typeIntr;
	pQName_fun pQName;
	Option.app atomicType_fun namedAtomicType_opt;
	Format.printf "<<<<<";
	trace ")";

    | TS_classSpec(annot, sourceLoc, cVFlags, typeIntr, pQName_opt, 
		  baseClassSpec_list, memberList, compoundType) ->
	trace "TS_classSpec(";
	assert(match compoundType with
	  | CompoundType _ -> true
	  | _ -> false);
	assert (Option.isSome pQName_opt);

	Format.print_string "% ";
	(* keyword: class/struct/union *)
        compoundType_Keyword_fun
	  ((function | CompoundType ct -> ct.keyword
	             | _ -> assert false) compoundType);
	Format.print_string " ";
	Option.app pQName_fun pQName_opt;  (* name of class type *)
	Format.printf "@\n@\n";

	(* collect all field names (with their type) of this class
	   declaration *)
	let fields = List.flatten (List.map (function
		| MR_decl (_, _, declaration) ->
		    (match declaration with
			(_, _, typeSpecifier, declarator_list) ->
			  List.map (fun (_, _, _, variable_opt, _, _, _, _) ->
			    match variable_opt with
			      | Some v -> (typeSpecifier, v)
			      | None -> assert false) declarator_list)
		| _ ->
		    []) (snd memberList)) in

	(* Semantics_<type> : TYPE = ... *)
	Format.printf "@[<2>Semantics_";
	Option.app pQName_fun pQName_opt;  (* name of class type *)
	Format.printf "@ :@ TYPE";
	(* class/struct: cartesian product of members *)
	(* (unions however not!)                      *)
	if (function | CompoundType ct -> ct.keyword <> K_UNION
	  | _ -> assert false) compoundType then
	  begin
	    Format.printf "@ =@ @[<2>[#@ ";
	    separate
	      (fun (t, v) ->
		Format.printf "@[<2>";
		variable_fun v;
		Format.printf ":@ Semantics_";
		typeSpecifier_fun t;
		Format.printf "@]")
	      (fun () -> Format.printf ",@ ") fields;
	    Format.printf "@ #]@]";
	  end;
	Format.printf "@]@\n";

	(* dt_<type> : (interpreted_data_type?) *)
	Format.printf "@[<2>dt_";
	Option.app pQName_fun pQName_opt;  (* name of class type *)
	Format.printf "@ :@ (interpreted_data_type?)@]@\n@\n";

	(* offsets_<type> : [# ... #] *)
	Format.printf "@[<2>offsets_";
	Option.app pQName_fun pQName_opt;  (* name of class type *)
	Format.printf "@ :@ @[<2>[#@ ";
	separate
	  (fun (_, v) ->
	    Format.printf "@[<2>";
	    variable_fun v;
	    Format.printf ":@ nat@]")
	  (fun () -> Format.printf ",@ ") fields;
	Format.printf "@ #]@]@]@\n";

	cVFlags_fun cVFlags;  (*?*)
	typeIntr_fun typeIntr;  (*?*)

	(*TODO: base classes not supported yet*)
	assert (List.length baseClassSpec_list = 0);
	List.iter baseClassSpec_fun baseClassSpec_list;

	(*additional axioms about memory layout etc.*)
	ignore (List.fold_left (fun prev_opt (typeSpecifier, variable) ->
	  begin
	    match prev_opt with
	      | None ->
		  Format.printf "@[<2>";
		  Option.app pQName_fun pQName_opt;  (* name of class type *)
		  Format.print_string "_";
		  variable_fun variable;  (* name of field *)
		  Format.printf "_axiom@ :@ AXIOM@ offsets_";
		  Option.app pQName_fun pQName_opt;  (* name of class type *)
		  Format.print_string "`";
		  variable_fun variable;  (* name of field *)
		  (* the C++ standard only implies >= 0 here, but also *)
		  (* that the amount of padding is the same between    *)
		  (* different classes with identical initial segments *)
		  Format.printf "@ =@ 0@]@\n";
	      | Some (prev_ts, prev_var) ->
		  Format.printf "@[<2>";
		  Option.app pQName_fun pQName_opt;  (* name of class type *)
		  Format.print_string "_";
		  variable_fun variable;  (* name of field *)
		  Format.printf "_axiom@ :@ AXIOM@ offsets_";
		  Option.app pQName_fun pQName_opt;  (* name of class type *)
		  Format.print_string "`";
		  variable_fun variable;  (* name of field *)
		  (* the C++ standard only implies >= here, but also   *)
		  (* that the amount of padding is the same between    *)
		  (* different classes with identical initial segments *)
		  Format.printf "@ =@ ";
		  if (function | CompoundType ct -> ct.keyword = K_UNION
	                       | _ -> assert false) compoundType then
		    (* unions: everything is at the same offset *)
		    Format.print_string "0"
		  else
		    begin
		      Format.print_string "offsets_";
		      Option.app pQName_fun pQName_opt;  (* name of class type *)
		      Format.print_string "`";
		      variable_fun prev_var;  (* name of field *)
		      Format.printf "@ +@ size(uidt(dt_";
		      typeSpecifier_fun prev_ts;
		      Format.print_string "))";
		    end;
		  Format.printf "@]@\n";
	  end;
	  Some (typeSpecifier, variable)) None fields);

	(* total size of the datatype *)
	(* TODO: for unions, we need to take the maximum of the member sizes *)
	if (List.length fields > 0) then
	  let (t, v) = last fields in
	    begin
	      Format.printf "@[<2>dt_";
	      Option.app pQName_fun pQName_opt;  (* name of class type *)
	      Format.printf "_size@ :@ AXIOM@ size(dt_";
	      Option.app pQName_fun pQName_opt;  (* name of class type *)
	      Format.printf ")@ >= offsets_";
	      Option.app pQName_fun pQName_opt;  (* name of class type *)
	      Format.print_string "`";
	      variable_fun v;  (* name of field *)
	      Format.printf "@ +@ size(uidt(dt_";
	      typeSpecifier_fun t;
	      Format.printf "))@]@\n";
	  end;
	Format.printf "@\n";

	(* print member functions *)
	memberList_fun memberList;

	(*atomicType_fun compoundType;*)  (* same as pQName_opt ??? *)
	trace ")";

    | TS_enumSpec(annot, sourceLoc, cVFlags, 
		 stringRef_opt, enumerator_list, enumType) ->
	(* we treat enums as int constants, rather than as a new type *)
	trace "TS_enumSpec(";
	assert(match enumType with 
	  | EnumType _ -> true
	  | _ -> false);
	Format.print_string "% enum ";
	atomicType_fun enumType;
	Format.printf "@\n@\n";
	Format.printf "@[<2>Semantics_";
	atomicType_fun enumType;
	Format.printf "@ :@ TYPE@ =@ Semantics_int@]@\n";
	Format.printf "@[<2>dt_";
	atomicType_fun enumType;
	Format.printf "@ :@ (pod_data_type?[Semantics_";
	atomicType_fun enumType;
	Format.printf "])@ =@ dt_int@]@\n@\n";
	(*cVFlags_fun cVFlags;*)
	(*Option.app string_fun stringRef_opt;*)
	separate enumerator_fun (fun () -> Format.printf "@\n")
	  enumerator_list;
	trace ")";

    | TS_type(annot, sourceLoc, cVFlags, cType) -> 
	trace "TS_type(";
	(*TODO*)
	cVFlags_fun cVFlags;
	cType_fun cType;
	trace ")";

    | TS_typeof(annot, sourceLoc, cVFlags, aSTTypeof) -> 
	assert false;
	annotation_fun annot;
	sourceLoc_fun sourceLoc;
	cVFlags_fun cVFlags;
	aSTTypeof_fun aSTTypeof


and baseClassSpec_fun
    (annot, bool, accessKeyword, pQName, compoundType_opt) =
  begin
    assert(match compoundType_opt with
      | None
      | Some(CompoundType _ ) -> true
      | _ -> false);
    assert false;
    annotation_fun annot;
    bool_fun bool;
    accessKeyword_fun accessKeyword;
    pQName_fun pQName;
    Option.app atomicType_fun compoundType_opt;
  end


and enumerator_fun (annot, sourceLoc, stringRef, 
		    expression_opt, variable, int32) =
  begin
    trace "enumerator_fun(";
    Format.printf "@[<2>";
    string_fun stringRef;  (*same as variable name?*)
    (*Option.app expression_fun expression_opt;*)
    (*variable_fun variable;*)
    Format.printf "@ :@ Semantics_int@ =@ ";
    int32_fun int32;
    Format.printf "@]";
    trace ")";
  end


and memberList_fun (annot, member_list) =
  begin
    trace "memberList_fun(";
    separate member_fun (fun () -> Format.printf "@\n@\n")
      (List.filter (function | MR_func _ -> true | _ -> false) member_list);
    trace ")";
  end


and member_fun x = 
  match x with
    | MR_decl (annot, sourceLoc, declaration) ->
	trace "MR_decl(";
	declaration_fun declaration;
	trace ")";

    | MR_func (annot, sourceLoc, func) -> 
	trace "MR_func(";
	func_fun func;
	trace ")";

    | MR_access (annot, sourceLoc, accessKeyword) ->
	(* TODO: access specifier are just ignored at the moment *)
	trace "MR_access(";
	(*
	annotation_fun annot;
	sourceLoc_fun sourceLoc;
	accessKeyword_fun accessKeyword;
	*)
	trace ")";

    | MR_usingDecl (annot, sourceLoc, nd_usingDecl) -> 
	assert false;
	assert(match nd_usingDecl with ND_usingDecl _ -> true | _ -> false);
	annotation_fun annot;
	sourceLoc_fun sourceLoc;
	namespaceDecl_fun nd_usingDecl;

    | MR_template (annot, sourceLoc, templateDeclaration) -> 
	assert false;
	annotation_fun annot;
	sourceLoc_fun sourceLoc;
	templateDeclaration_fun templateDeclaration


and typedef_declarator_fun (annot, iDeclarator, init_opt,
		   variable_opt, ctype_opt, declaratorContext,
		   statement_opt_ctor, statement_opt_dtor) =
  begin
    trace "typedef_declarator_fun(";

    assert (not (Option.isSome init_opt));
    assert (Option.isSome variable_opt);
    assert (Option.isSome ctype_opt);
    assert (not (Option.isSome statement_opt_ctor));
    assert (not (Option.isSome statement_opt_dtor));

    (* name of new type *)
    Option.app variable_fun variable_opt;

    trace ")";
  end;


and declarator_fun (annot, iDeclarator, init_opt,
		   variable_opt, ctype_opt, declaratorContext,
		   statement_opt_ctor, statement_opt_dtor) =
  begin
    trace "declarator_fun(";

    (* a description of the (return) type *)
    iDeclarator_fun iDeclarator;  (* only for tracing *)

    declaratorContext_fun declaratorContext;  (* only for tracing *)

    (match declaratorContext with
      | DC_UNKNOWN          -> assert false;

      (* function declaration *)
      | DC_FUNCTION         ->

	  assert (not (Option.isSome init_opt));
	  assert (Option.isSome variable_opt);
	  assert (Option.isSome ctype_opt);
	  assert (not (Option.isSome statement_opt_ctor));
	  assert (not (Option.isSome statement_opt_dtor));

	  Format.printf "@[<2>";
	  (* name of function *)
	  Option.app variable_fun variable_opt;
	  (* append the class name in case it's a constructor or assignment
	     operator *)
	  if (Option.valOf variable_opt).var_name = Some "constructor_special"
	    || (Option.valOf variable_opt).var_name = Some "operator="
	  then
	    begin
	      Format.print_string "_";
	      Format.print_string
		(enclosing_classname_of_member (Option.valOf variable_opt));
	    end;
	  (* arguments of function *)
	  (match ctype_opt with
	    | Some (FunctionType (_, _, _, variable_list, _)) ->
		let 
		    var_opt_list = List.map (fun x -> Some x) variable_list
		in let
		    var_list = if (Option.valOf variable_opt).var_name =
		      Some "constructor_special" then
		      None :: var_opt_list else var_opt_list
		in
		  if List.length var_list > 0 then
		    begin
		      Format.printf "@[<2>(";
		      separate (fun v ->
			Format.printf "@[<2>";
			(match v with
			  | Some var -> variable_fun var;
			  | None     -> Format.print_string "receiver");
			Format.printf "@ :@ Address@]")
			(fun () -> Format.printf ",@ ") var_list;
		      Format.printf ")@]";
		    end;
	    | _ ->
		assert false);
	  Format.printf "@ :@ ST[State,@ Semantics_";
	  (* remainder of return value type *)
	  (* TODO: is this different from the return type? What does
	     "remainder" mean? *)
	  (match ctype_opt with
	    | Some (FunctionType (_, _, cType, _, _)) ->
		cType_fun cType;
	    | _ ->
		assert false);
	  Format.printf "]@ =@]@\n";

      | DC_TF_DECL          -> assert false;
      | DC_TF_EXPLICITINST  -> assert false;
      | DC_MR_DECL          -> assert false;

      (* declaration statement *)
      | DC_S_DECL           ->

	  Format.printf "assign(pm, dt_";
	  assert (Option.isSome variable_opt);
	  assert (Option.isSome !((Option.valOf variable_opt).var_type));
	  Option.app cType_fun !((Option.valOf variable_opt).var_type);
	  Format.printf ")(@[<2>id(";
	  Option.app variable_fun variable_opt;
	  Format.printf "),@ ";
	  (* TODO: default initialization (§8.5) not modelled yet *)
	  if Option.isSome init_opt then
	    Option.app init_fun init_opt
	  else
	    Format.print_string ">>>>>default-initialization<<<<<";
	  Format.printf "@])";
	  
	  (* TODO: what is this ? *)
	  Option.app (fun s -> Format.print_string ">>>>>ctor:"; statement_fun s; Format.print_string "<<<<<") statement_opt_ctor;

	  (* TODO: what is this ? *)
	  Option.app (fun s -> Format.print_string ">>>>>dtor:"; statement_fun s; Format.print_string "<<<<<") statement_opt_dtor;

      | DC_TD_DECL          -> assert false;

      (* full expression annotation *)
      | DC_FEA		    ->

	  assert (not (Option.isSome init_opt));
	  assert (Option.isSome variable_opt);
	  assert (Option.isSome ctype_opt);
	  assert (not (Option.isSome statement_opt_ctor));
	  assert (Option.isSome statement_opt_dtor);

	  assert false;

      | DC_D_FUNC           -> assert false;
      | DC_EXCEPTIONSPEC    -> assert false;
      | DC_ON_CONVERSION    -> assert false;
      | DC_CN_DECL          -> assert false;
      | DC_HANDLER          -> assert false;
      | DC_E_CAST           -> assert false;
      | DC_E_SIZEOFTYPE     -> assert false;
      | DC_E_NEW            -> assert false;
      | DC_E_KEYWORDCAST    -> assert false;
      | DC_E_TYPEIDTYPE     -> assert false;
      | DC_TP_TYPE          -> assert false;
      | DC_TP_NONTYPE       -> assert false;
      | DC_TA_TYPE          -> assert false;
      | DC_TS_TYPEOF_TYPE   -> assert false;
      | DC_E_COMPOUNDLIT    -> assert false;
      | DC_E_ALIGNOFTYPE    -> assert false;
      | DC_E_BUILTIN_VA_ARG -> assert false);

    trace ")";
  end


and iDeclarator_fun x = 
  match x with
    | D_name (annot, sourceLoc, pQName_opt) ->
	trace "D_name(";
	(*
	  Option.app pQName_fun pQName_opt;
	*)
	trace ")";

    | D_pointer (annot, sourceLoc, cVFlags, iDeclarator) ->
	trace "D_pointer(";
	(*
	  cVFlags_fun cVFlags;
	  iDeclarator_fun iDeclarator;
	*)
	trace ")";

    | D_reference (annot, sourceLoc, iDeclarator) ->
	trace "D_reference(";
	(*
	  iDeclarator_fun iDeclarator;
	*)
	trace ")";

    | D_func (annot, sourceLoc, iDeclarator, aSTTypeId_list, cVFlags, 
	     exceptionSpec_opt, pq_name_list, bool) ->
	trace "D_func(";
	assert (List.for_all (function PQ_name _ -> true | _ -> false)
		   pq_name_list);
	(*
	  iDeclarator_fun iDeclarator;
	  List.iter aSTTypeId_fun aSTTypeId_list;
	  cVFlags_fun cVFlags;
	  Option.app exceptionSpec_fun exceptionSpec_opt;
	  List.iter pQName_fun pq_name_list;
	  bool_fun bool;
	*)
	trace ")";

    | D_array (annot, sourceLoc, iDeclarator, expression_opt, bool) ->
	assert false;
	iDeclarator_fun iDeclarator;
	Option.app expression_fun expression_opt;
	bool_fun bool;

    | D_bitfield (annot, sourceLoc, pQName_opt, expression, int) -> 
	assert false;
	Option.app pQName_fun pQName_opt;
	expression_fun expression;
	int_fun int;

    | D_ptrToMember (annot, sourceLoc, pQName, cVFlags, iDeclarator) -> 
	assert false;
	pQName_fun pQName;
	cVFlags_fun cVFlags;
	iDeclarator_fun iDeclarator;

    | D_grouping (annot, sourceLoc, iDeclarator) -> 
	assert false;
	iDeclarator_fun iDeclarator;

    | D_attribute (annot, sourceLoc, iDeclarator, attribute_list_list) ->
	trace "D_attribute(";
	assert false;
	iDeclarator_fun iDeclarator;
	List.iter (List.iter attribute_fun) attribute_list_list;
	trace ")";


and exceptionSpec_fun (annot, aSTTypeId_list) =
  begin
    assert false;
    annotation_fun annot;
    List.iter aSTTypeId_fun aSTTypeId_list
  end


and operatorName_fun x =
  match x with
    | ON_newDel (annot, bool_is_new, bool_is_array) -> 
	assert false;
	annotation_fun annot;
	bool_fun bool_is_new;
	bool_fun bool_is_array

    | ON_operator (_, overloadableOp) -> 
	trace "ON_operator(";
	overloadableOp_fun overloadableOp;
	trace ")";

    | ON_conversion (annot, aSTTypeId) -> 
	assert false;
	annotation_fun annot;
	aSTTypeId_fun aSTTypeId


and statement_fun x =
  match x with
    | S_skip (annot, sourceLoc) ->
        Format.print_string "skip";

    | S_label (annot, sourceLoc, stringRef, statement) ->
        assert false; (* labels are not supported, for they are useless *)
                      (* without goto                                   *)
	string_fun stringRef;
	statement_fun statement;

    | S_case (annot, sourceLoc, expression, statement, int32) ->
        Format.print_string "case(";
	intLit_expression_fun expression;
        Format.printf ") ##@\n";
	statement_fun statement;
	(* int32_fun int32 *) (* TODO: what is this for? *)

    | S_default (annot, sourceLoc, statement) ->
        Format.printf "default ##@\n";
	statement_fun statement;

    | S_expr (annot, sourceLoc, fullExpression) ->
        Format.print_string "e2s(";
	fullExpression_fun fullExpression;
        Format.print_string ")";

    | S_compound (annot, sourceLoc, statement_list) ->
	if (List.length statement_list = 0) then
	  Format.print_string "skip"
	else
	  (* collect all variable declarations in this block *)
	  let decls_in_stmt = function
	      S_decl (_, _, declaration) ->
		(match declaration with (_, _, _, declarator_list) ->
		  List.map (fun (_, _, _, variable_opt, _, _, _, _) ->
		    match variable_opt with
			Some v -> v
		      | None -> assert false) declarator_list)
	    | _ -> [] in
	  let variable_list =
	    List.flatten (List.map decls_in_stmt statement_list)
	  in
	    List.iter (fun v ->
	      Format.printf "@[<2>with_new_stackvar(@[<2>lambda@ @[<2>(";
	      variable_fun v;
	      Format.printf "@ :@ Address)@]:@]@\n") variable_list;
	    separate statement_fun (fun () -> Format.printf " ##@\n")
	      statement_list;
	    List.iter (fun v ->
	      Format.printf ")@]") variable_list;

    | S_if(annot, sourceLoc, condition, statement_then, statement_else) -> 
        Format.printf "@[<2>if_else(";
	condition_fun condition;
        Format.printf ",@\n";
	statement_fun statement_then;
        Format.printf ",@\n";
	statement_fun statement_else;
        Format.printf ")@]";

    | S_switch(annot, sourceLoc, condition, statement) -> 
        Format.printf "switch@[<2>(";
	condition_fun condition;
        Format.printf ",@ @[(: ";
	let rec cases_in_stmt = function
            S_case (_, _, expression, _, _) ->
	      [expression]
	  | S_compound (_, _, statement_list) ->
	      List.flatten (List.map cases_in_stmt statement_list)
	  | _ -> (* case labels within substatements are ignored *) []
	in
	  separate intLit_expression_fun (fun () -> Format.print_string ", ")
	    (cases_in_stmt statement);
        Format.printf " :)@],@\n";
	statement_fun statement;
        Format.printf ")@]";

    | S_while(annot, sourceLoc, condition, statement) ->
        Format.printf "@[<2>while(";
	bool_condition_fun condition;
        Format.printf ",@\n";
	statement_fun statement;
        Format.printf ")@]";

    | S_doWhile(annot, sourceLoc, statement, fullExpression) ->
        Format.printf "@[<2>do_while(@\n";
	statement_fun statement;
        Format.printf ",@]@\n";
	fullExpression_fun fullExpression;
        Format.printf ")";

    | S_for(annot, sourceLoc, statement_init, condition, fullExpression,
	   statement_body) ->
        Format.printf "@[<2>for(";
	statement_fun statement_init;
        Format.printf ",@ ";
	bool_condition_fun condition;
        Format.printf ",@ ";
	fullExpression_fun fullExpression;
        Format.printf ",@\n";
	statement_fun statement_body;
        Format.printf ")@]";

    | S_break(annot, sourceLoc) -> 
        Format.print_string "break"

    | S_continue(annot, sourceLoc) ->
        Format.print_string "continue";

    | S_return(annot, sourceLoc, fullExpression_opt, statement_opt) ->
        (match fullExpression_opt with
          | None ->
              Format.print_string "return_void"
          | Some fullExpression ->
              Format.print_string "return(";
              fullExpression_fun fullExpression;
              Format.print_string ")");
        assert (not (Option.isSome statement_opt)); (* ? *)
	Option.app statement_fun statement_opt;

    | S_goto(annot, sourceLoc, stringRef) ->
        assert false;  (* goto is not supported *)
	string_fun stringRef;

    | S_decl(annot, sourceLoc, declaration) ->
	(* variable declarations are translated to stack allocations before  *)
	(* the entire statement block; here only the initializing assignment *)
	(* (if present) still needs to be considered                         *)
	(*TODO*)
        Format.print_string "e2s(";
	declaration_fun declaration;
        Format.print_string ")";

    | S_try(annot, sourceLoc, statement, handler_list) ->
        assert false;  (* try is not supported *)
	statement_fun statement;
	List.iter handler_fun handler_list;

    | S_asm(annot, sourceLoc, e_stringLit) ->
(* TODO        assert false;  (* asm statements are not supported *) *)
	assert(match e_stringLit with | E_stringLit _ -> true | _ -> false);
        Format.print_string "asm(";
	expression_fun e_stringLit;
        Format.print_string ")";

    | S_namespaceDecl(annot, sourceLoc, namespaceDecl) ->
	assert false;
	namespaceDecl_fun namespaceDecl;

    | S_function(annot, sourceLoc, func) ->
	assert false;
	func_fun func;

    | S_rangeCase(annot, sourceLoc, 
		 expression_lo, expression_hi, statement, 
		 label_lo, label_hi) ->
	(*TODO*)
	trace "S_rangeCase(";
	Format.printf ">>>>>";
	expression_fun expression_lo;
	expression_fun expression_hi;
	statement_fun statement;
	int_fun label_lo;
	int_fun label_hi;
	Format.printf "<<<<<";
	trace ")";

    | S_computedGoto(annot, sourceLoc, expression) ->
	assert false;
	expression_fun expression;


(* boolean conditions *)
and bool_condition_fun x =
  match x with
    | CN_expr(annot, fullExpression) ->
	trace "CN_expr(";
	begin
	  match fullExpression with
	    | (_, expression_opt, _) ->
		assert (Option.isSome (expression_opt));
		Option.app x2b_l2r_expression_fun expression_opt;
	end;
	trace ")";

    | CN_decl(annot, aSTTypeId) ->
	trace "CN_decl(";
	(*TODO: declaration conditions currently unsupported*)
	assert false;
	aSTTypeId_fun aSTTypeId;
	trace ")";


(* integer conditions *)
and condition_fun x =
  match x with
    | CN_expr(annot, fullExpression) ->
	trace "CN_expr(";
	begin
	  match fullExpression with
	    | (_, expression_opt, _) ->
		assert (Option.isSome (expression_opt));
		Option.app l2r_expression_fun expression_opt;
	end;
	trace ")";

    | CN_decl(annot, aSTTypeId) ->
	trace "CN_decl(";
	(*TODO: declaration conditions currently unsupported*)
	assert false;
	aSTTypeId_fun aSTTypeId;
	trace ")";


and handler_fun (annot, aSTTypeId, statement_body, variable_opt, 
		 fullExpressionAnnot, expression_opt, statement_gdtor) =
  begin
    assert false;
    annotation_fun annot;
    aSTTypeId_fun aSTTypeId;
    statement_fun statement_body;
    Option.app variable_fun variable_opt;
    fullExpressionAnnot_fun fullExpressionAnnot;
    Option.app expression_fun expression_opt;
    Option.app statement_fun statement_gdtor
  end


(* applies the lvalue-to-rvalue conversion to an expression if necessary *)
and l2r_expression_fun x =
  let is_lvalue_expression = function
    | E_variable _
    | E_assign _ -> true
    | _ -> false
  in
    if is_lvalue_expression x then
      begin
	Format.printf "l2r(pm, dt_int)@[<2>(";
	expression_fun x;
        Format.printf ")@]";
      end
    else
      expression_fun x;


(* applies the int/address-to-bool and lvalue-to-rvalue conversion to an
   expression if necessary *)
and x2b_l2r_expression_fun x =
  let is_int_expression = function
    | _ -> true  (*TODO*)
  in let is_address_expression = function
    | _ -> false  (*TODO*)
  in
    if is_int_expression x then
      begin
	Format.printf "n2b@[<2>(";
	l2r_expression_fun x;
        Format.printf ")@]";
      end
    else if is_address_expression x then
      begin
	Format.printf "a2b@[<2>(";
	l2r_expression_fun x;
        Format.printf ")@]";
      end
    else
      begin
	(*TODO: assert(is_bool_expression x)*)
	l2r_expression_fun x;
      end;


(* expects an int literal; prints the literal without any conversions *)
and intLit_expression_fun x =
  match x with
    | E_intLit (_, _, stringRef, _) ->
	string_fun stringRef
    | _->
	assert false;


and expression_fun x = 
  match x with
    | E_boolLit(annot, type_opt, bool) ->
        trace "E_boolLit(";
        assert (Option.isSome type_opt);
	(*Option.app cType_fun type_opt;*)
	Format.print_string "literal(";
	bool_fun bool;
	Format.print_string ")";
        trace ")";

    | E_intLit(annot, type_opt, stringRef, ulong) ->
        trace "E_intLit(";
	(*Option.app cType_fun type_opt;*)
	Format.print_string "literal(";
	string_fun stringRef;
	Format.print_string ")";
	(*int32_fun ulong*) (*?*)
        trace ")";

    | E_floatLit(annot, type_opt, stringRef, double) ->
	assert false;
	annotation_fun annot;
	Option.app cType_fun type_opt;
	string_fun stringRef
	(*float_fun double*) (*?*)

    | E_stringLit(annot, type_opt, stringRef, 
		 e_stringLit_opt, stringRef_opt) ->
	trace "E_stringLit(";
	(*TODO*)
	assert(match e_stringLit_opt with 
	  | Some(E_stringLit _) -> true 
	  | None -> true
	  | _ -> false);
	(*Option.app cType_fun type_opt;*) (*?*)
	string_fun stringRef; (*?*)
	(*Option.app expression_fun e_stringLit_opt;*) (*?*)
	(*Option.app string_fun stringRef_opt;*) (*?*)
	trace ")";

    | E_charLit(annot, type_opt, stringRef, int32) ->
	trace "E_charLit(";
	Format.printf ">>>>>";
	Option.app cType_fun type_opt;
	string_fun stringRef;
	(*int32_fun int32*) (*?*)
	Format.printf "<<<<<";
	trace ")";

    | E_this(annot, type_opt, variable) ->
        trace "E_this(";
	(*Option.app cType_fun type_opt;*) (* type of variable ? *)
	Format.print_string "id(";
	variable_fun variable;
	Format.print_string ")";
        trace ")";

    | E_variable(annot, type_opt, pQName, var_opt, nondep_var_opt) ->
        trace "E_variable(";
	(*Option.app cType_fun type_opt;*)
	Format.print_string "id(";
	Option.app variable_fun var_opt;
	Format.print_string ")";
        assert (not (Option.isSome nondep_var_opt));
	Option.app variable_fun nondep_var_opt;
        trace ")";

    | E_funCall(annot, type_opt, expression_func, 
	       argExpression_list, expression_retobj_opt) ->
        trace "E_funCall(";
	(* TODO: what is this? The return type? Why is it optional? *)
	assert (Option.isSome type_opt);
	(*Option.app cType_fun type_opt;*)

	Format.print_string "call_";
	(* TODO: is this always just the name of a function? What about
	   function pointers? *)
	(function
	  | E_variable (_, _, _, var_opt, _) ->
	      assert (Option.isSome var_opt);
	      Option.app variable_fun var_opt
	  | E_fieldAcc _ ->
	      expression_fun expression_func
	  | _ ->
	      assert false) expression_func;

	(* for member functions: prepend receiver object to argument list *)
	let receiver_argExpression_list = (function
	  | E_fieldAcc (_, _, expression, _, _) ->
	      (annot, expression) :: argExpression_list
	  | _ ->
	      argExpression_list) expression_func
	in
	  if (List.length receiver_argExpression_list > 0) then
	    begin
	      Format.printf "(@[<2>";
	      separate argExpression_fun (fun () -> Format.printf ",@ ")
		receiver_argExpression_list;
	      Format.printf "@])";
	    end;

	(* TODO: what is this? *)
	assert (not (Option.isSome expression_retobj_opt));
	(*Option.app expression_fun expression_retobj_opt;*)
        trace ")";

    | E_constructor(annot, type_opt, typeSpecifier, argExpression_list, 
		   var_opt, bool, expression_opt) ->
        trace "E_constructor(";
	(*TODO*)
	Option.app cType_fun type_opt;
	typeSpecifier_fun typeSpecifier;
	List.iter argExpression_fun argExpression_list;
	Option.app variable_fun var_opt;
	bool_fun bool;
	Option.app expression_fun expression_opt;
        trace ")";

    | E_fieldAcc(annot, type_opt, expression, pQName, var_opt) ->
        trace "E_fieldAcc(";
	let isFunctionType = function
	  | FunctionType _ -> true
	  | _ -> false
	in
	  (* result type (?) *)
	  (*Option.app cType_fun type_opt;*)
	  if isFunctionType (Option.valOf type_opt) then
	    begin
	      (* field name (difference between this and var_opt?) *)
	      (*pQName_fun pQName;*)
	      assert (Option.isSome var_opt);
	      Option.app variable_fun var_opt;  (* field name *)
	    end
	  else
	    begin
	      Format.printf "@[<2>member(";
	      expression_fun expression;  (* receiver object *)
	      Format.printf ",@ offsets_";
	      assert (Option.isSome (expression_type expression));
	      Option.app (fun t -> cType_fun (derefType t))
		(expression_type expression);  (* class type *)
	      Format.print_string "`";
	      (* field name (difference between this and var_opt?) *)
	      (*pQName_fun pQName;*)
	      assert (Option.isSome var_opt);
	      Option.app variable_fun var_opt;  (* field name *)
	      Format.printf ")@]";
	    end;
          trace ")";

    | E_sizeof(annot, type_opt, expression, int) ->
	(*TODO*)
        trace "E_sizeof(";
	Option.app cType_fun type_opt;
        Format.print_string "sizeof(";
	expression_fun expression;
        Format.print_string ")";
	int_fun int;
        trace ")";

    | E_unary(annot, type_opt, unaryOp, expression) ->
        trace "E_unary(";
        assert (Option.isSome type_opt);
	(*Option.app cType_fun type_opt;*)
	unaryOp_fun unaryOp;
        Format.print_string "(";
	expression_fun expression;
        Format.print_string ")";
        trace ")";

    | E_effect(annot, type_opt, effectOp, expression) ->
        trace "E_effect(";
        assert (Option.isSome type_opt);
	(*Option.app cType_fun type_opt;*)
	effectOp_fun effectOp;
        Format.printf "(pm, dt_int)(";
	expression_fun expression;
        Format.print_string ")";
        trace ")";

    | E_binary(annot, type_opt, expression_left, binaryOp, expression_right) ->
        trace "E_binary(";
        assert (Option.isSome type_opt);
	(*Option.app cType_fun type_opt;*)
	binaryOp_fun binaryOp;
        Format.printf "@[<2>(";
	expression_fun expression_left;
        Format.printf ",@ ";
	expression_fun expression_right;
        Format.printf ")@]";
        trace ")";

    | E_addrOf(annot, type_opt, expression) ->
        trace "E_addrOf(";
	(* Only the address of lvalues can be taken, and an lvalue is an
	   address -- we just need to suppress lvalue-to-rvalue conversion. *)
	(*Option.app cType_fun type_opt;*)
	expression_fun expression;
        trace ")";

    | E_deref(annot, type_opt, expression) ->
        trace "E_deref(";
	(* TODO: do we have to do anything here??? *)
	(*Option.app cType_fun type_opt;*) (* type of expression? *)
	expression_fun expression;
        trace ")";

    | E_cast(annot, type_opt, aSTTypeId, expression, bool) ->
        trace "E_cast(";
	Option.app cType_fun type_opt;
	aSTTypeId_fun aSTTypeId;
	expression_fun expression;
	bool_fun bool;
        trace ")";

    | E_cond(annot, type_opt, expression_cond, expression_true, expression_false) ->
        trace "E_cond(";
	Option.app cType_fun type_opt;
        Format.print_string "(";
	expression_fun expression_cond;
        Format.print_string " ? ";
	expression_fun expression_true;
        Format.print_string " : ";
	expression_fun expression_false;
        Format.print_string ")";
        trace ")";

    | E_sizeofType(annot, type_opt, aSTTypeId, int, bool) ->
	trace "E_sizeofType";
	Format.printf ">>>>>";
	Option.app cType_fun type_opt;
	aSTTypeId_fun aSTTypeId;
	int_fun int;
	bool_fun bool;
	Format.printf "<<<<<";
	trace ")";

    | E_assign(annot, type_opt, expression_target, binaryOp, expression_src) ->
        trace "E_assign(";
        assert (Option.isSome type_opt);
	assignOp_fun binaryOp;
        Format.print_string "(pm, dt_";
	Option.app (fun t -> cType_fun (derefType t)) type_opt;
	Format.printf ")@[<2>(";
	expression_fun expression_target;
        Format.printf ",@ ";
	l2r_expression_fun expression_src;
        Format.printf ")@]";
        trace ")";

    | E_new(annot, type_opt, bool, argExpression_list, aSTTypeId, 
	   argExpressionListOpt_opt, array_size_opt, ctor_opt, 
	   statement_opt, heep_var_opt) ->
        trace "E_new(";
	(*TODO*)
	Option.app cType_fun type_opt;
	bool_fun bool;
	List.iter argExpression_fun argExpression_list;
	aSTTypeId_fun aSTTypeId;
	Option.app argExpressionListOpt_fun argExpressionListOpt_opt;
	Option.app expression_fun array_size_opt;
	Option.app variable_fun ctor_opt;
	Option.app statement_fun statement_opt;
	Option.app variable_fun heep_var_opt;
	trace ")";

    | E_delete(annot, type_opt, bool_colon, bool_array, 
	      expression_opt, statement_opt) ->
	assert false;
	annotation_fun annot;
	Option.app cType_fun type_opt;
	bool_fun bool_colon;
	bool_fun bool_array;
	Option.app expression_fun expression_opt;
	Option.app statement_fun statement_opt

    | E_throw(annot, type_opt, expression_opt, var_opt, statement_opt) ->
	assert false;
	annotation_fun annot;
	Option.app cType_fun type_opt;
	Option.app expression_fun expression_opt;
	Option.app variable_fun var_opt;
	Option.app statement_fun statement_opt;

    | E_keywordCast(annot, type_opt, castKeyword, aSTTypeId, expression) ->
	trace "E_keywordCast(";
	(*TODO*)
	Option.app cType_fun type_opt;
	castKeyword_fun castKeyword;
	aSTTypeId_fun aSTTypeId;
	expression_fun expression;
	trace ")";

    | E_typeidExpr(annot, type_opt, expression) ->
	assert false;
	annotation_fun annot;
	Option.app cType_fun type_opt;
	expression_fun expression

    | E_typeidType(annot, type_opt, aSTTypeId) ->
	assert false;
	annotation_fun annot;
	Option.app cType_fun type_opt;
	aSTTypeId_fun aSTTypeId

    | E_grouping(annot, type_opt, expression) ->
	assert false;
	annotation_fun annot;
	Option.app cType_fun type_opt;
	expression_fun expression

    | E_arrow(annot, type_opt, expression, pQName) ->
	assert false;
	annotation_fun annot;
	Option.app cType_fun type_opt;
	expression_fun expression;
	pQName_fun pQName

    | E_statement(annot, type_opt, s_compound) ->
        assert false;  (* statement expressions are not supported *)
	assert (match s_compound with | S_compound _ -> true | _ -> false);
	annotation_fun annot;
	Option.app cType_fun type_opt;
	statement_fun s_compound

    | E_compoundLit(annot, type_opt, aSTTypeId, in_compound) ->
	assert(match in_compound with | IN_compound _ -> true | _ -> false);
	annotation_fun annot;
	Option.app cType_fun type_opt;
	aSTTypeId_fun aSTTypeId;
	init_fun in_compound

    | E___builtin_constant_p(annot, type_opt, sourceLoc, expression) ->
	assert false;
	annotation_fun annot;
	Option.app cType_fun type_opt;
	sourceLoc_fun sourceLoc;
	expression_fun expression

    | E___builtin_va_arg(annot, type_opt, sourceLoc, expression, aSTTypeId) ->
	trace "E___builtin_va_arg(";
	(*TODO*)
	Format.printf ">>>>>";
	Option.app cType_fun type_opt;
	sourceLoc_fun sourceLoc;
	expression_fun expression;
	aSTTypeId_fun aSTTypeId;
	Format.printf "<<<<<";
	trace ")";

    | E_alignofType(annot, type_opt, aSTTypeId, int) ->
	assert false;
	annotation_fun annot;
	Option.app cType_fun type_opt;
	aSTTypeId_fun aSTTypeId;
	int_fun int

    | E_alignofExpr(annot, type_opt, expression, int) ->
	assert false;
	annotation_fun annot;
	Option.app cType_fun type_opt;
	expression_fun expression;
	int_fun int

    | E_gnuCond(annot, type_opt, expression_cond, expression_false) ->
	assert false;
	annotation_fun annot;
	Option.app cType_fun type_opt;
	expression_fun expression_cond;
	expression_fun expression_false

    | E_addrOfLabel(annot, type_opt, stringRef) ->
	assert false;
	annotation_fun annot;
	Option.app cType_fun type_opt;
	string_fun stringRef

(* TODO
    | E_stdConv (annot, type_opt, expression, stdConv) ->
	(* standard conversion sequence: contains at most one conversion each
	   from 3 different groups of conversions *)
	let is_present sc = List.mem sc stdConv in 
	  begin
	    (* conversion group 3 (comes last conceptually) *)
	    (* 4.4: int* -> int const* *)
	    if is_present SC_QUAL_CONV then
	      (* TODO *)
	      assert false;

	    (* conversion group 2 (goes in the middle) *)
	    if is_present SC_INT_PROM then
              (* 4.5: int... -> int..., no info loss possible *)
	      (* TODO *)
	      assert false
	    else if is_present SC_FLOAT_PROM then
	      (* 4.6: float -> double, no info loss possible *)
	      (* TODO *)
	      assert false
	    else if is_present SC_INT_CONV then
	      (* 4.7: int... -> int..., info loss possible *)
	      (* TODO *)
	      assert false
	    else if is_present SC_FLOAT_CONV then
	      (* 4.8: float... -> float..., info loss possible *)
	      (* TODO *)
	      assert false
	    else if is_present SC_FLOAT_INT_CONV then
	      (* 4.9: int... <-> float..., info loss possible *)
	      (* TODO *)
	      assert false
	    else if is_present SC_PTR_CONV then
	      (* 4.10: 0 -> Foo*, Child* -> Parent* *)
	      (* TODO *)
	      assert false
	    else if is_present SC_PTR_MEMB_CONV then
	      (* 4.11: int Child::* -> int Parent::* *)
	      (* TODO *)
	      assert false
	    else if is_present SC_BOOL_CONV then
	      (* 4.12: various types <-> bool *)
	      (* TODO *)
	      assert false
	    else if is_present SC_DERIVED_TO_BASE then
	      (* 13.3.3.1p6: Child -> Parent *)
	      (* TODO *)
	      assert false;

	    (* conversion group 1 (comes first) *)
	    if is_present SC_LVAL_TO_RVAL then
	      (* 4.1: int& -> int *)
	      (* TODO *)
	      assert false
	    else if is_present SC_ARRAY_TO_PTR then
	      (* 4.2: char[] -> char* *)
	      (* TODO *)
	      assert false
	    else if is_present SC_FUNC_TO_PTR then
	      (* 4.3: int ()(int) -> int ( * )(int) *)
	      (* TODO *)
	      assert false;
	    expression_fun expression;
	  end
*)

and fullExpression_fun (annot, expression_opt, fullExpressionAnnot) =
  begin
    trace "fullExpression_fun(";
    assert (Option.isSome (expression_opt));
    Option.app expression_fun expression_opt;
    fullExpressionAnnot_fun fullExpressionAnnot;
    trace ")"
  end


and argExpression_fun (annot, expression) =
  begin
    trace "argExpression_fun(";
    expression_fun expression;
    trace ")";
  end


and argExpressionListOpt_fun (annot, argExpression_list) =
  begin
    trace "argExpressionListOpt_fun(";
    (*TODO*)
    Format.printf ">>>>>";
    List.iter argExpression_fun argExpression_list;
    Format.printf "<<<<<";
    trace ")";
  end


and init_fun x = 
  match x with
      IN_expr(annot, sourceLoc, fullExpressionAnnot, expression_opt) ->
	trace "IN_expr(";
	fullExpressionAnnot_fun fullExpressionAnnot;
	assert (Option.isSome expression_opt);
	Option.app expression_fun expression_opt;
	trace ")";

    | IN_compound(annot, sourceLoc, fullExpressionAnnot, init_list) ->
	assert false;
	annotation_fun annot;
	sourceLoc_fun sourceLoc;
	fullExpressionAnnot_fun fullExpressionAnnot;
	List.iter init_fun init_list

    | IN_ctor(annot, sourceLoc, fullExpressionAnnot, 
	     argExpression_list, var_opt, bool) ->
	trace "IN_ctor(";
	assert (Option.isSome var_opt);
	Format.print_string "call_";
	Option.app variable_fun var_opt;
	Format.printf "(@[<2>";
	separate argExpression_fun (fun () -> Format.printf ",@ ")
	  argExpression_list;
	Format.printf "@])";
	(* What is this? *)
	fullExpressionAnnot_fun fullExpressionAnnot;
	(*bool_fun bool;*)
	trace ")";

    | IN_designated(annot, sourceLoc, fullExpressionAnnot, 
		   designator_list, init) -> 
	assert false;
	annotation_fun annot;
	sourceLoc_fun sourceLoc;
	fullExpressionAnnot_fun fullExpressionAnnot;
	List.iter designator_fun designator_list;
	init_fun init


and templateDeclaration_fun x =
  match x with
    | TD_func(annot, templateParameter_opt, func) ->
	assert false;
	annotation_fun annot;
	Option.app templateParameter_fun templateParameter_opt;
	func_fun func;

    | TD_decl(annot, templateParameter_opt, declaration) ->
	trace "TD_decl";
	(*TODO*)
	Format.printf ">>>>>";
	Option.app templateParameter_fun templateParameter_opt;
	declaration_fun declaration;
	Format.printf "<<<<<";
	trace ")";

    | TD_tmember(annot, templateParameter_opt, templateDeclaration) ->
	assert false;
	annotation_fun annot;
	Option.app templateParameter_fun templateParameter_opt;
	templateDeclaration_fun templateDeclaration


and templateParameter_fun x = 
  match x with
    | TP_type(annot, sourceLoc, variable, stringRef, 
	     aSTTypeId_opt, templateParameter_opt) ->
	trace "TP_type(";
	(*TODO*)
	Format.printf ">>>>>";
	variable_fun variable;
	string_fun stringRef;
	Option.app aSTTypeId_fun aSTTypeId_opt;
	Option.app templateParameter_fun templateParameter_opt;
	Format.printf "<<<<<";
	trace ")";

    | TP_nontype(annot, sourceLoc, variable,
		aSTTypeId, templateParameter_opt) ->
	assert false;
	annotation_fun annot;
	sourceLoc_fun sourceLoc;
	variable_fun variable;
	aSTTypeId_fun aSTTypeId;
	Option.app templateParameter_fun templateParameter_opt


and templateArgument_fun x =
  match (assert false; x) with
    | TA_type(annot, aSTTypeId, templateArgument_opt) -> 
	annotation_fun annot;
	aSTTypeId_fun aSTTypeId;
	Option.app templateArgument_fun templateArgument_opt

    | TA_nontype(annot, expression, templateArgument_opt) -> 
	annotation_fun annot;
	expression_fun expression;
	Option.app templateArgument_fun templateArgument_opt

    | TA_templateUsed(annot, templateArgument_opt) -> 
	annotation_fun annot;
	Option.app templateArgument_fun templateArgument_opt


and namespaceDecl_fun x = 
  match x with
    | ND_alias(annot, stringRef, pQName) ->
	assert false;
	string_fun stringRef;
	pQName_fun pQName;

    | ND_usingDecl(annot, pQName) ->
	trace "ND_usingDecl(";
	(*TODO*)
	pQName_fun pQName;
	trace ")";

    | ND_usingDir(annot, pQName) ->
	trace "ND_usingDir(";
	(*TODO*)
	pQName_fun pQName;
	trace ")";


and fullExpressionAnnot_fun (annot, declaration_list) =
  begin
    trace "fullExpressionAnnot_fun(";
    List.iter declaration_fun declaration_list;
    trace ")";
  end


and aSTTypeof_fun x = 
  match x with
    | TS_typeof_expr(annot, ctype, fullExpression) ->
	assert false;
	annotation_fun annot;
	cType_fun ctype;
	fullExpression_fun fullExpression

    | TS_typeof_type(annot, ctype, aSTTypeId) ->
	assert false;
	annotation_fun annot;
	cType_fun ctype;
	aSTTypeId_fun aSTTypeId


and designator_fun x =
  match x with
      FieldDesignator(annot, sourceLoc, stringRef) ->
	assert false;
	annotation_fun annot;
	sourceLoc_fun sourceLoc;
	string_fun stringRef;

    | SubscriptDesignator(annot, sourceLoc, expression, expression_opt,
			     idx_start, idx_end) ->
	assert false;
	annotation_fun annot;
	sourceLoc_fun sourceLoc;
	expression_fun expression;
	Option.app expression_fun expression_opt;
	int_fun idx_start;
	int_fun idx_end;


and attribute_fun x =
  match x with
      AT_empty(annot, sourceLoc) ->
	assert false;
	annotation_fun annot;
	sourceLoc_fun sourceLoc;

    | AT_word(annot, sourceLoc, stringRef) ->
	trace "AT_word(";
	(*TODO*)
	string_fun stringRef;
	trace ")";

    | AT_func(annot, sourceLoc, stringRef, argExpression_list) ->
	trace "AT_func(";
	(*TODO*)
	string_fun stringRef;
	List.iter argExpression_fun argExpression_list;
	trace ")";


(**************************************************************************
 *
 * end of astiter.ml 
 *
 **************************************************************************)


(* ------------------------------------------------------------------------- *)
(* ast_node_fun                                                              *)
(* ------------------------------------------------------------------------- *)

open Superast

let ast_node_fun = function
  | NoAstNode -> assert false
  | Variable v -> assert false; variable_fun v
  | TemplateInfo ti -> assert false; templ_info_fun ti
  | InheritedTemplateParams itp -> assert false; inherited_templ_params_fun itp
  | BaseClass bc -> assert false; baseClass_fun bc
  | Compound_info ci -> assert false; compound_info_fun ci
  | EnumType_Value_type et -> assert false; enum_value_fun et
  | AtomicType at ->
      assert false;
      (* does not occur: instead of (AtomicType(CompoundType...)) *)
      (* the ast_array contains a (Compound_info ...)             *)
      assert (match at with CompoundType _ -> false | _ -> true);
      atomicType_fun at
  | CType ct -> assert false; cType_fun ct
  | STemplateArgument ta -> assert false; sTemplateArgument_fun ta
  | Scope s -> assert false; scope_fun s
  | TranslationUnit_type tu ->
      (* this is the only case possible *)
      translationUnit_fun tu
  | TopForm_type tf -> assert false; topForm_fun tf
  | Function_type fn -> assert false; func_fun fn
  | MemberInit_type mi -> assert false; memberInit_fun mi
  | Declaration_type decl -> assert false; declaration_fun decl
  | ASTTypeId_type ast -> assert false; aSTTypeId_fun ast
  | PQName_type name -> assert false; pQName_fun name
  | TypeSpecifier_type ts -> assert false; typeSpecifier_fun ts
  | BaseClassSpec_type bcs -> assert false; baseClassSpec_fun bcs
  | Enumerator_type e -> assert false; enumerator_fun e
  | MemberList_type ml -> assert false; memberList_fun ml
  | Member_type m -> assert false; member_fun m
  | Declarator_type d -> assert false; declarator_fun d
  | IDeclarator_type id -> assert false; iDeclarator_fun id
  | ExceptionSpec_type es -> assert false; exceptionSpec_fun es
  | OperatorName_type on -> assert false; operatorName_fun on
  | Statement_type s -> assert false; statement_fun s
  | Condition_type c -> assert false; condition_fun c
  | Handler_type h -> assert false; handler_fun h
  | Expression_type e -> assert false; expression_fun e
  | FullExpression_type fe -> assert false; fullExpression_fun fe
  | ArgExpression_type ae -> assert false; argExpression_fun ae
  | ArgExpressionListOpt_type ael -> assert false; argExpressionListOpt_fun ael
  | Initializer_type i -> assert false; init_fun i
  | TemplateDeclaration_type td -> assert false; templateDeclaration_fun td
  | TemplateParameter_type tp -> assert false; templateParameter_fun tp
  | TemplateArgument_type ta -> assert false; templateArgument_fun ta
  | NamespaceDecl_type nd -> assert false; namespaceDecl_fun nd
  | FullExpressionAnnot_type fea -> assert false; fullExpressionAnnot_fun fea
  | ASTTypeof_type at -> assert false; aSTTypeof_fun at
  | Designator_type dt -> assert false; designator_fun dt
  | Attribute_type at -> assert false; attribute_fun at

(* ------------------------------------------------------------------------- *)
(* main                                                                      *)
(* ------------------------------------------------------------------------- *)

let arguments = []

let usage_msg =
  "Usage: ast2pvs input.ast [output.pvs]\n\
   \n\
   Recognized options are:"

let usage () =
  Arg.usage arguments usage_msg;
  exit(1)

let in_file_name = ref ""
let in_file_set  = ref false

let out_file_name = ref ""
let out_file_set  = ref false

let set_files s =
  if not !in_file_set then
    begin
      in_file_name := s;
      in_file_set  := true
    end
  else if not !out_file_set then
    begin
      out_file_name := s;
      out_file_set  := true
    end
  else
    begin
      Printf.eprintf
        "Too many arguments: don't know what to do with %s.\n" s;
      usage()
    end

let main () =
  Arg.parse arguments set_files usage_msg;
  if not !in_file_set then
    usage();  (* does not return *)
  if not !out_file_set then
    out_file_name := !in_file_name ^ ".pvs";
  let ast_array = Superast.load_marshaled_ast_array !in_file_name in
  let theory_name = try
          String.sub !out_file_name 0 (String.index !out_file_name '.')
        with Not_found -> !out_file_name in
  let lemma_name = theory_name ^ "_spec" in
    begin
      assert (Array.length ast_array >= 2);
      assert (ast_array.(0) == NoAstNode);
      assert (match ast_array.(1) with TranslationUnit_type _ -> true
                                     | _ -> false);
      out := open_out !out_file_name;
      Format.set_margin 79;
      Format.set_max_indent 60;
      Format.set_max_boxes max_int;
      Format.set_formatter_out_channel !out;
      Format.printf "@[@[<2>%s[State@ :@ Type]@ :@ THEORY@]@\n\
        BEGIN@\n\
        @[<2>@\n\
          @[<2>IMPORTING@ Abstract_Read_Write_Plain,@ Cpp_Types,@ Expressions,@ Statements,@ Conversions,@ Hoare@]@\n\
          @\n\
          @[pm@ :@ Plain_Memory[State]@]@\n\
          @\n" theory_name;
      ast_node_fun ast_array.(1);
      (* this only works for int main(), not for int main(argc, argv)
	 (cf. C++ Standard, 3.6.1.2) *)
      Format.printf "@[<2>PRECONDITION@ :@ PRED[State]@]@\n\
          @\n\
          @[<2>POSTCONDITION@ :@ PRED[State]@]@\n\
          @\n\
          @[<2>@[<4>%s@ :@ LEMMA@]@\n\
            @[<2>valid(PRECONDITION,@\n\
              main,@]@\n\
            POSTCONDITION)@]@]@\n\
          @\n\
        END@ %s@]@." lemma_name theory_name;
      close_out !out;
    end;;

main ();;
