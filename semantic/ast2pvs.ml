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

(* TODO: many AST constructors are not translated yet.  For some, "assert    *)
(*       false" will raise an exception, but others are just ignored         *)
(*       silently. This is certainly more harmful than good ...              *)
(*       Where we don't want to abort by raising an assertion, we sometimes  *)
(*       produce output of the form ">>>>>...<<<<<" (which shouldn't be      *)
(*       valid PVS).                                                         *)

(* TODO: - type casts, conversions (eg. int to bool)?                        *)
(*       - __attribute__                                                     *)
(*       - Pretty-printing currently ignores the layout of the C++ program.  *)
(*         To obtain more readable output, one might want to preserve        *)
(*         whitespace, certain comments etc.                                 *)
(*       - It would be nice if the pre-/postcondition was taken from an      *)
(*         annotation in the C++ program.                                    *)
(*       - arrays                                                            *)
(*       - multiple variable declarations in one line, e.g.                  *)
(*         "int a = 0, b = 0;"                                               *)

(* flag for debug output *)

let debug = false

let trace s = if debug then Format.print_string s else ()

(* output channel for PVS file *)

let out = ref stdout

(* ------------------------------------------------------------------------- *)
(* AST translation code                                                      *)
(* ------------------------------------------------------------------------- *)

open Elsa_reflect_type
open Elsa_ml_base_types
open Elsa_ml_flag_types

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
  | ST_ELLIPSIS              -> ">>>>>ellipsis<<<<<"  (*TODO*)
  | ST_CDTOR                 -> ">>>>>cdtor<<<<<"  (*TODO*)
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

(*open Cc_ast_gen_type*)
(*open Ml_ctype*)
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
  Format.printf "(%d)" (id_annotation a)

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

let castKeyword_fun = function
  | CK_DYNAMIC     -> assert false
  | CK_STATIC      -> assert false
  | CK_REINTERPRET -> Format.print_string "reinterpret_cast"
  | CK_CONST       -> assert false

let function_flags_fun (_ : functionFlags) =
  trace "function_flags_fun()"  (*TODO*)

let declaratorContext_fun (dc : declaratorContext) =
  trace "declaratorContext_fun(";
  trace (string_of_declaratorContext dc);
  trace ")"

let scopeKind_fun (_ : scopeKind) = ()

let array_size_fun = function
  | NO_SIZE      -> assert false
  | DYN_SIZE     -> assert false
  | FIXED_SIZE i -> assert false; int_fun i

let compoundType_Keyword_fun = function
  | K_STRUCT -> Format.print_string "struct"
  | K_CLASS  -> Format.print_string "class"
  | K_UNION  -> Format.print_string "union"

let templ_kind_fun (kind : templateThingKind) =
  assert false

let implicitConversion_Kind_fun = function
  | IC_NONE         -> trace "IC_NONE"; assert false;
  | IC_STANDARD     -> trace "IC_STANDARD";
  | IC_USER_DEFINED -> trace "IC_USER_DEFINED"
  | IC_ELLIPSIS     -> trace "IC_ELLIPSIS"; assert false;
  | IC_AMBIGUOUS    -> trace "IC_AMBIGUOUS"; assert false

(***************** variable ***************************)

let rec variable_fun (v : annotated variable_type) =
    trace "variable_fun(";
    (match v.variable_name with
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
      assert false;


(************* inheritedTemplateParams ****************)

and inherited_templ_params_fun itp =
      assert false;

(***************** cType ******************************)

and baseClass_fun baseClass =
      assert false;

and compound_info_fun i = 
    begin
      trace "compound_info_fun(";
(*
      annotation_fun i.compound_info_poly;
      Option.app string_fun i.compound_name;
*)
      variable_fun i.compound_type_var;
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
      ATY_Simple(annot, simpleTypeId) ->
	trace "SimpleType(";
	simpleTypeId_fun simpleTypeId;
	trace ")";

    | ATY_Compound(compound_info) ->
	trace "CompoundType(";
        compound_info_fun compound_info;
	trace ")";

    | ATY_PseudoInstantiation _ ->
	assert false;

    | ATY_Enum x ->
	trace "EnumType(";
	assert (Option.isSome x.enum_type_name);  (* anonymous enums not supported yet *)
	Option.app string_fun x.enum_type_name;  (*identical to variable name?*)
	(*Option.app variable_fun variable;
	accessKeyword_fun accessKeyword;
	List.iter enum_value_fun enum_value_list;
	bool_fun has_negatives;*)
	trace ")";

    | ATY_TypeVariable _ ->
	assert false;

    | ATY_DependentQ _ ->
	assert false;


and cType_fun x = 
  match x with
      TY_CVAtomic(annot, atomicType, cv) ->
	trace "CVAtomicType(";
	cVFlags_fun cv;
	atomicType_fun atomicType;
	trace ")";

    | TY_Pointer(annot, cVFlags, cType) ->
	trace "PointerType(";
	(*TODO*)
	cVFlags_fun cVFlags;
	Format.print_string "pointer[dt_";	
	cType_fun cType;
	Format.print_string "]";
	trace ")";

    | TY_Reference(annot, cType) ->
	trace "ReferenceType(";
	(*TODO*)
	Format.print_string "reference[dt_";
	cType_fun cType;
	Format.print_string "]";
	trace ")";

    | TY_Function x ->
	trace "FunctionType(";
	(*TODO*)
	function_flags_fun x.function_type_flags;
	cType_fun x.retType;
	List.iter variable_fun x.function_type_params;
	(*TODO*)
	List.iter cType_fun x.function_type_exn.fun_exn_spec_types;
	trace ")";

    | TY_Array(annot, cType, array_size) ->
	trace "ArrayType(";
	(*TODO*)
	cType_fun cType;
	array_size_fun array_size;
	trace ")";

    | TY_DependentSizedArray (annot, cType, expression) ->
	trace "DependentSizeArrayType(";
	(*TODO*)
	assert false;
	cType_fun cType;
	expression_fun expression;
	trace ")";

    | TY_PointerToMember _ ->
	assert false;


and derefType = function
    TY_Reference (_, cType) -> cType
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
    trace "scope_fun(";
    trace ")";


(***************** generated ast nodes ****************)

and translationUnit_fun x =
    trace "translationUnit_fun(";
    List.iter (fun tf -> topForm_fun tf; Format.printf "@\n@\n") x.topForms;
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

    | TF_explicitInst _ ->
	assert false;

    | TF_linkage x ->
        trace "TF_Linkage(";
	(*TODO*)
	string_fun x.linkage_type;
	translationUnit_fun x.linkage_forms;
        trace ")";

    | TF_one_linkage x ->
        trace "TF_one_linkage(";
	(*string_fun stringRef;*)  (* "C" for "extern C" -- ignore ?! *)
	topForm_fun x.form;
        trace ")";

    | TF_asm (annot, loc, text) ->
	trace "TF_asm";
        assert false;
	expression_fun (E_stringLit text);
	trace ")";

    | TF_namespaceDefn x ->
	trace "TF_namespaceDefn(";
	(*TODO*)
	Option.app string_fun x.name_space_defn_name;
	List.iter topForm_fun x.name_space_defn_forms;
	trace ")";

    | TF_namespaceDecl(annot, sourceLoc, namespaceDecl) ->
	trace "TF_namespaceDecl(";
	namespaceDecl_fun namespaceDecl;
	trace ")";


and func_fun x (*annot, declFlags, typeSpecifier, declarator, memberInit_list,
	     s_compound_opt, handler_list, func, variable_opt_1,
	     variable_opt_2, statement_opt, bool*) =
  begin
    trace "func_fun(";

    (* TODO: static, extern etc.; not supported yet *)
    declFlags_fun x.function_dflags;

    (* type specifier for return value *)
    (* TODO: what's the difference between this and the type in declarator? *)
    (*typeSpecifier_fun typeSpecifier;*)

    (* function definition: f(addresses) = body *)

    Format.printf "@[<2>";

    (* - remainder of return value type *)
    (* - name of function               *)
    (* - names/types of parameters      *)
    declarator_fun x.nameAndParams;

    (* (for ctors only) member initialization list *)
    List.iter memberInit_fun x.function_inits;

    (* TODO: s_compound_opt may be None, but when can this happen (apparently
             in in/t0524.cc, but in general? - seems obscure) *)
    assert (Option.isSome x.function_body);
    Option.app statement_fun x.function_body;

    (let variable_opt = x.nameAndParams.declarator_var in
      if (Option.valOf variable_opt).variable_name = Some "constructor_special" then
	(* append return statement to the function body if this function is a constructor *)
	begin
	  Format.printf " ##@\nreturn(read_data(pm, dt_";
	  Format.print_string
	    (enclosing_classname_of_member (Option.valOf variable_opt));
	  Format.printf ")(receiver))";
	end
      else
	(* append "return_void" to the function body if necessary *)
	(match x.function_body with
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

    (let variable_opt = x.nameAndParams.declarator_var in
    let ctype_opt = x.nameAndParams.declarator_type in

      (* name of function *)
      Option.app variable_fun variable_opt;
      (* append the class name in case it's a constructor or assignment
	 operator *)
      if (Option.valOf variable_opt).variable_name = Some "constructor_special"
	|| (Option.valOf variable_opt).variable_name = Some "operator="
      then
	begin
	  Format.print_string "_";
	  Format.print_string
	    (enclosing_classname_of_member (Option.valOf variable_opt));
	end;
      Format.printf "@[<2>";
      (* arguments of function *)
      (match ctype_opt with
	| Some (TY_Function xx) ->
	    let
		var_opt_list = List.map (fun x -> Some x) xx.function_type_params
	    in let
		var_list = if (Option.valOf variable_opt).variable_name =
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
			     assert (Option.isSome !(var.variable_type));
			     Option.app cType_fun !(var.variable_type);
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
	| TY_Function x ->
	    cType_fun x.retType;
	| _ ->
	    assert false) ctype_opt;
      Format.printf "]@ =@]@\n";

      (* arguments of function *)
      (match ctype_opt with
	| Some (TY_Function xx) ->
	    let 
		var_opt_list = List.map (fun x -> Some x) xx.function_type_params
	    in let
		var_list = if (Option.valOf variable_opt).variable_name =
		  Some "constructor_special" then
		  None :: var_opt_list else var_opt_list
	    in
		 if List.length var_list > 0 then
		   List.iter (fun v ->
		     (match v with
		       | Some var ->
			   (*reference arguments are not copied onto the stack*)
			   let has_reference_type var =
			     match Option.valOf !(var.variable_type) with
			       | TY_Reference _ -> true
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
				 assert (Option.isSome !(var.variable_type));
				 Option.app cType_fun !(var.variable_type);
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
      if (Option.valOf variable_opt).variable_name = Some "constructor_special"
	|| (Option.valOf variable_opt).variable_name = Some "operator="
      then
	begin
	  Format.print_string "_";
	  Format.print_string
	    (enclosing_classname_of_member (Option.valOf variable_opt));
	end;
      (* arguments of function *)
      (match ctype_opt with 
	| Some (TY_Function xx) ->
	    let 
		var_opt_list = List.map (fun x -> Some x) xx.function_type_params
	    in let
		var_list = if (Option.valOf variable_opt).variable_name =
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
    assert (List.length x.function_handlers = 0);
    List.iter handler_fun x.function_handlers;

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
    assert (match x.function_dtor_statement with
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


and enclosing_classname_of_member (v : annotated variable_type) : string =
  Option.valOf
    (Option.valOf (Option.valOf v.scope).scope_compound).compound_name


and memberInit_fun x (*annot, pQName, argExpression_list, 
		    variable_opt_1, compound_opt, variable_opt_2, 
		    full_expr_annot, statement_opt*) =
  begin
    trace "memberInit_fun(";
    Format.printf "e2s(assign(pm, dt_";
    (* type of member *)
    (function
      | PQ_variable (_, _, variable) ->
	  assert (Option.isSome !(variable.variable_type));
	  Option.app cType_fun !(variable.variable_type);
      | _ ->
	  assert false) x.member_init_name;
    Format.printf ")(@[<2>member(@[<2>id(receiver),@ offsets_";
    (* class type *)
    Format.print_string
      (enclosing_classname_of_member (Option.valOf x.member));
    Format.print_string  "`";
    pQName_fun x.member_init_name;  (* name of member *)
    Format.printf "@]),@ ";
    assert (List.length x.member_init_args = 1);
    List.iter argExpression_fun x.member_init_args;
    Format.printf "@])) ##@\n";

    (* TODO: ? *)
    (* is this different from pQName ? *)
    assert (Option.isSome x.member);
    assert (not (Option.isSome x.member_init_base));
    assert (not (Option.isSome x.member_init_ctor_var));
    assert (not (Option.isSome x.member_init_ctor_statement));
    (*
      Option.app variable_fun variable_opt_1;
      Option.app atomicType_fun compound_opt;
      Option.app variable_fun variable_opt_2;
      fullExpressionAnnot_fun full_expr_annot;
      Option.app statement_fun statement_opt;
    *)
    trace ")";
  end


and declaration_fun x (*annot, declFlags, typeSpecifier, declarator_list*) =
  begin
    trace "declaration_fun(";
    declFlags_fun x.declaration_dflags;

    (* TODO: only one declaration at a time supported at the moment *)
    if List.length x.decllist > 1 then
      assert false;

    if List.mem DF_TYPEDEF x.declaration_dflags then
      (* typedef declaration *)
      begin
	List.iter (function d ->
	  Format.print_string "% typedef ";
	  typedef_declarator_fun d;
	  Format.printf "@\n";
	  Format.printf "@[<2>Semantics_";
	  typedef_declarator_fun d;
	  Format.printf "@ :@ TYPE@ =@ Semantics_";
	  typeSpecifier_fun x.declaration_spec;
	  Format.printf "@]@\n";
	  Format.printf "@[<2>dt_";
	  typedef_declarator_fun d;
	  Format.printf "@ :@ (pod_data_type?[Semantics_";
	  typedef_declarator_fun d;
	  Format.printf "])@ =@ dt_";
	  typeSpecifier_fun x.declaration_spec;
          Format.printf "@]") x.decllist;
      end
    else
      (* class/variable/function declaration *)
	(*TODO: declaring a type and at the same time a variable of that type,
	  as in    class C { /* ... */ } c;    is not supported yet.*)
	if List.length x.decllist = 0 then
	  typeSpecifier_fun x.declaration_spec
	else
	  List.iter declarator_fun x.decllist;
    trace ")";
  end


and aSTTypeId_fun x =
  begin
    trace "aSTTypeId_fun(";
    (*TODO*)
    Format.printf ">>>>>aSTTypeId_fun(";
    typeSpecifier_fun x.ast_type_id_spec;
    (*
      declarator_fun x.ast_type_id_decl;
    *)
    Format.printf ")<<<<<";
    trace ")";
  end


and pQName_fun x =
  match x with
      PQ_qualifier xx (*annot, sourceLoc, stringRef_opt, 
		  templateArgument_opt, pQName, 
		  variable_opt, s_template_arg_list*) ->
	trace "PQ_qualifier(";
	Option.app string_fun xx.qualifier;
	Option.app templateArgument_fun xx.pq_qualifier_templ_args;
	pQName_fun xx.qualifier_rest;
	Option.app variable_fun xx.qualifierVar;
	List.iter sTemplateArgument_fun xx.pq_qualifier_sargs;
	trace ")";

    | PQ_name(annot, sourceLoc, stringRef) ->
	trace "PQ_name(";
	string_fun stringRef;
	trace ")";

    | PQ_operator xx (*annot, sourceLoc, operatorName, stringRef*) ->
	trace "PQ_operator(";
	Format.printf ">>>>>";
	operatorName_fun xx.o;
	string_fun xx.fakeName;
	Format.printf "<<<<<";
	trace ")";

    | PQ_template _ ->
	assert false;

    | PQ_variable(annot, sourceLoc, variable) ->
	trace "PQ_variable(";
	variable_fun variable;
	trace ")";


and typeSpecifier_fun x = 
  match x with
    | TS_name xx (*annot, sourceLoc, cVFlags, pQName, bool, 
	     var_opt_1, var_opt_2*) ->
	trace "TS_name(";
	(*TODO*)
(*
	cVFlags_fun cVFlags;
	pQName_fun pQName;
	bool_fun bool;
*)
	assert (Option.isSome xx.type_name_var);
	assert (not (Option.isSome xx.type_name_nondep_var));
	Option.app variable_fun xx.type_name_var;
	Option.app variable_fun xx.type_name_nondep_var;
	trace ")";

    | TS_simple xx (*annot, sourceLoc, cVFlags, simpleTypeId*) ->
	trace "TS_simple(";
	cVFlags_fun xx.simple_type_cv;
	simpleTypeId_fun xx.id;
	trace ")";

    | TS_elaborated xx (*annot, sourceLoc, cVFlags, typeIntr, 
		   pQName, namedAtomicType_opt*) ->
	trace "TS_elaborated(";
	Format.printf ">>>>>";
	assert(match xx.elaborated_atype with
	  | Some(ATY_Simple _) -> false
	  | _ -> true);
	annotation_fun xx.tS_elaborated_annotation;
	sourceLoc_fun xx.elaborated_loc;
	cVFlags_fun xx.elaborated_cv;
	typeIntr_fun xx.elaborated_keyword;
	pQName_fun xx.elaborated_name;
	Option.app atomicType_fun xx.elaborated_atype;
	Format.printf "<<<<<";
	trace ")";

    | TS_classSpec xx (*annot, sourceLoc, cVFlags, typeIntr, pQName_opt, 
		  baseClassSpec_list, memberList, compoundType*) ->
	trace "TS_classSpec(";

	let cVFlags = xx.class_cv in
	let typeIntr = xx.class_keyword in
	let pQName_opt = xx.class_name in
	let baseClassSpec_list = xx.class_bases in
	let memberList = xx.members in
	let compoundType = xx.class_ctype in

	assert (Option.isSome pQName_opt);

	Format.print_string "% ";
	(* keyword: class/struct/union *)
        compoundType_Keyword_fun compoundType.compound_keyword;
	Format.print_string " ";
	Option.app pQName_fun pQName_opt;  (* name of class type *)
	Format.printf "@\n@\n";

	(* collect all field names of this class declaration; ignore member
	   functions *)
	let fields = List.flatten (List.map (function
		| MR_decl (_, _, declaration) ->
		    (let typeSpecifier = declaration.declaration_spec in
		     let declarator_list = declaration.decllist in

			  (* we disallow nested class declarations etc. *)
			  (match typeSpecifier with
			    | TS_name _ -> ();
			    | TS_simple _ -> ();
			    | TS_elaborated _ -> assert false;
			    | TS_classSpec _ -> assert false;
			    | TS_enumSpec _ -> assert false;
			    | TS_type _ -> assert false;
			    | TS_typeof _ -> assert false);

			  List.flatten
			    (List.map
				(fun xx ->
				  match xx.declarator_var with
				    | Some var ->
					(match !(var.variable_type) with
					  | Some cType ->
					      (match cType with
						| TY_CVAtomic _
						| TY_Pointer _ ->
						    [var];
						| TY_Function _ ->
						    (* ignore member functions here *)
						    [];
						| TY_Reference _
						| TY_Array _
						| TY_DependentSizedArray _
						| TY_PointerToMember _ ->
						    (* not supported as member types yet *)
						    assert false);
					  | None ->
					      assert false);
				    | None ->
					assert false) declarator_list));
		| _ ->
		    []) memberList.member_list) in

	(* Semantics_<type> : TYPE = ... *)
	Format.printf "@[<2>Semantics_";
	Option.app pQName_fun pQName_opt;  (* name of class type *)
	Format.printf "@ :@ TYPE";
	(* class/struct: cartesian product of members *)
	(* (unions however not!)                      *)
	if compoundType.compound_keyword <> K_UNION then
	  begin
	    Format.printf "@ =@ @[<2>[#@ ";
	    separate
	      (fun var ->
		Format.printf "@[<2>";
		variable_fun var;  (* name of field *)
		Format.printf ":@ Semantics_";
		assert (Option.isSome !(var.variable_type));
		Option.app cType_fun !(var.variable_type);  (* type of field *)
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
	  (fun var ->
	    Format.printf "@[<2>";
	    variable_fun var;
	    Format.printf ":@ nat@]")
	  (fun () -> Format.printf ",@ ") fields;
	Format.printf "@ #]@]@]@\n";

	cVFlags_fun cVFlags;  (*?*)
	typeIntr_fun typeIntr;  (*?*)

	(*TODO: base classes not supported yet*)
	assert (List.length baseClassSpec_list = 0);
	List.iter baseClassSpec_fun baseClassSpec_list;

	(*additional axioms about memory layout etc.*)
	ignore (List.fold_left (fun prev_opt var ->
	  begin
	    match prev_opt with
	      | None ->
		  Format.printf "@[<2>";
		  Option.app pQName_fun pQName_opt;  (* name of class type *)
		  Format.print_string "_";
		  variable_fun var;  (* name of field *)
		  Format.printf "_axiom@ :@ AXIOM@ offsets_";
		  Option.app pQName_fun pQName_opt;  (* name of class type *)
		  Format.print_string "`";
		  variable_fun var;  (* name of field *)
		  (* the C++ standard only implies >= 0 here, but also *)
		  (* that the amount of padding is the same between    *)
		  (* different classes with identical initial segments *)
		  Format.printf "@ =@ 0@]@\n";
	      | Some prev_var ->
		  Format.printf "@[<2>";
		  Option.app pQName_fun pQName_opt;  (* name of class type *)
		  Format.print_string "_";
		  variable_fun var;  (* name of field *)
		  Format.printf "_axiom@ :@ AXIOM@ offsets_";
		  Option.app pQName_fun pQName_opt;  (* name of class type *)
		  Format.print_string "`";
		  variable_fun var;  (* name of field *)
		  (* the C++ standard only implies >= here, but also   *)
		  (* that the amount of padding is the same between    *)
		  (* different classes with identical initial segments *)
		  Format.printf "@ =@ ";
		  if compoundType.compound_keyword = K_UNION then
		    (* unions: everything is at the same offset *)
		    Format.print_string "0"
		  else
		    begin
		      Format.print_string "offsets_";
		      Option.app pQName_fun pQName_opt;  (* name of class type *)
		      Format.print_string "`";
		      variable_fun prev_var;  (* name of field *)
		      Format.printf "@ +@ size(uidt(dt_";
		      assert (Option.isSome !(var.variable_type));
		      Option.app cType_fun !(var.variable_type);  (* type of field *)
		      Format.print_string "))";
		    end;
		  Format.printf "@]@\n";
	  end;
	  Some var) None fields);

	(* total size of the datatype *)
	(* TODO: for unions, we need to take the maximum of the member sizes *)
	if (List.length fields > 0) then
	  let var = last fields in
	    begin
	      Format.printf "@[<2>dt_";
	      Option.app pQName_fun pQName_opt;  (* name of class type *)
	      Format.printf "_size@ :@ AXIOM@ size(dt_";
	      Option.app pQName_fun pQName_opt;  (* name of class type *)
	      Format.printf ")@ >= offsets_";
	      Option.app pQName_fun pQName_opt;  (* name of class type *)
	      Format.print_string "`";
	      variable_fun var;  (* name of field *)
	      Format.printf "@ +@ size(uidt(dt_";
	      assert (Option.isSome !(var.variable_type));
	      Option.app cType_fun !(var.variable_type);  (* type of field *)
	      Format.printf "))@]@\n";
	  end;
	Format.printf "@\n";

	(* print member functions *)
	memberList_fun memberList;

	(*atomicType_fun compoundType;*)  (* same as pQName_opt ??? *)
	trace ")";

    | TS_enumSpec xx (*annot, sourceLoc, cVFlags, 
		 stringRef_opt, enumerator_list, enumType*) ->
	(* we treat enums as int constants, rather than as a new type *)
	trace "TS_enumSpec(";
	Format.print_string "% enum ";
	atomicType_fun (ATY_Enum xx.etype);
	Format.printf "@\n@\n";
	Format.printf "@[<2>Semantics_";
	atomicType_fun (ATY_Enum xx.etype);
	Format.printf "@ :@ TYPE@ =@ Semantics_int@]@\n";
	Format.printf "@[<2>dt_";
	atomicType_fun (ATY_Enum xx.etype);
	Format.printf "@ :@ (pod_data_type?[Semantics_";
	atomicType_fun (ATY_Enum xx.etype);
	Format.printf "])@ =@ dt_int@]@\n@\n";
	(*cVFlags_fun cVFlags;*)
	(*Option.app string_fun stringRef_opt;*)
	separate enumerator_fun (fun () -> Format.printf "@\n")
	  xx.elts;
	trace ")";

    | TS_type xx (*annot, sourceLoc, cVFlags, cType*) -> 
	trace "TS_type(";
	(*TODO*)
	cVFlags_fun xx.type_cv;
	cType_fun xx.type_type;
	trace ")";

    | TS_typeof _ ->
	assert false;


and baseClassSpec_fun _ =
    assert false;


and enumerator_fun x (*annot, sourceLoc, stringRef, 
		    expression_opt, variable, int32*) =
  begin
    trace "enumerator_fun(";
    Format.printf "@[<2>";
    string_fun x.enumerator_name;  (*same as variable name?*)
    (*Option.app expression_fun expression_opt;*)
    (*variable_fun variable;*)
    Format.printf "@ :@ Semantics_int@ =@ ";
    int32_fun x.enumValue;
    Format.printf "@]";
    trace ")";
  end


and memberList_fun x =
  begin
    trace "memberList_fun(";
    separate member_fun (fun () -> Format.printf "@\n@\n")
      (List.filter (function | MR_func _ -> true | _ -> false) x.member_list);
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


and typedef_declarator_fun x (*annot, iDeclarator, init_opt,
		   variable_opt, ctype_opt, declaratorContext,
		   statement_opt_ctor, statement_opt_dtor*) =
  begin
    trace "typedef_declarator_fun(";

    assert (not (Option.isSome x.declarator_init));
    assert (Option.isSome x.declarator_var);
    assert (Option.isSome x.declarator_type);
    assert (not (Option.isSome x.declarator_ctor_statement));
    assert (not (Option.isSome x.declarator_dtor_statement));

    (* name of new type *)
    Option.app variable_fun x.declarator_var;

    trace ")";
  end;


and declarator_fun x (*annot, iDeclarator, init_opt,
		   variable_opt, ctype_opt, declaratorContext,
		   statement_opt_ctor, statement_opt_dtor*) =
  begin
    trace "declarator_fun(";

    let iDeclarator = x.declarator_decl in
    let init_opt = x.declarator_init in
    let variable_opt = x.declarator_var in
    let ctype_opt = x.declarator_type in
    let declaratorContext = x.context in
    let statement_opt_ctor = x.declarator_ctor_statement in
    let statement_opt_dtor = x.declarator_dtor_statement in

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
	  if (Option.valOf variable_opt).variable_name = Some "constructor_special"
	    || (Option.valOf variable_opt).variable_name = Some "operator="
	  then
	    begin
	      Format.print_string "_";
	      Format.print_string
		(enclosing_classname_of_member (Option.valOf variable_opt));
	    end;
	  (* arguments of function *)
	  (match ctype_opt with
	    | Some (TY_Function xx) ->
		let variable_list = xx.function_type_params in
		let var_opt_list = List.map (fun x -> Some x) variable_list
		in let
		    var_list = if (Option.valOf variable_opt).variable_name =
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
	    | Some (TY_Function xx) ->
		cType_fun xx.retType;
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
	  assert (Option.isSome !((Option.valOf variable_opt).variable_type));
	  Option.app cType_fun !((Option.valOf variable_opt).variable_type);
	  Format.printf ")(@[<2>id(";
	  Option.app variable_fun variable_opt;
	  Format.printf "),@ ";
	  (match init_opt with
	    | Some init ->
		init_fun init;
		(match init with
		  | IN_ctor _ ->
		      (match statement_opt_ctor with
			| Some (S_expr (_, _,fullExpression)) ->
			    fullExpression_fun fullExpression;
			| _ ->
			    assert false);
		  | _ ->
		      assert (not (Option.isSome statement_opt_ctor)));
	    | None ->
		(* TODO: default initialization (§8.5) not modelled yet *)
		Format.print_string ">>>>>default-initialization<<<<<";
	  );
	  Format.printf "@])";
	  
          (* If a destructor is associated with this variable, then it needs to
	     be called at the end of the enclosing block; therefore the
	     destructor call (as well as the allocation of stack memory for
	     this variable) is dealt with in the code for S_compound. *)
	  (*
	    Option.app statement_fun statement_opt_dtor;
	  *)

      | DC_TD_DECL          -> assert false;

      (* full expression annotation (what is this?) *)
      | DC_FEA		    ->

	  assert (not (Option.isSome init_opt));
	  assert (Option.isSome variable_opt);
	  assert (Option.isSome ctype_opt);
	  assert (not (Option.isSome statement_opt_ctor));
	  assert (Option.isSome statement_opt_dtor);

	  Format.print_string ">>>>>DC_FEA(";
	  Option.app variable_fun variable_opt;
	  Option.app cType_fun ctype_opt;
	  Option.app statement_fun statement_opt_dtor;
	  Format.print_string ")<<<<<";

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

    | D_pointer _ ->
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

    | D_func x ->
	trace "D_func(";
	assert (List.for_all (function PQ_name _ -> true | _ -> false)
		   x.kAndR_params);
	(*
	  iDeclarator_fun iDeclarator;
	  List.iter aSTTypeId_fun aSTTypeId_list;
	  cVFlags_fun cVFlags;
	  Option.app exceptionSpec_fun exceptionSpec_opt;
	  List.iter pQName_fun pq_name_list;
	  bool_fun bool;
	*)
	trace ")";

    | D_array _ ->
	assert false;

    | D_bitfield _ -> 
	assert false;

    | D_ptrToMember _ -> 
	assert false;

    | D_grouping _ -> 
	assert false;


and exceptionSpec_fun (annot, aSTTypeId_list) =
  begin
    assert false;
    annotation_fun annot;
    List.iter aSTTypeId_fun aSTTypeId_list
  end


and operatorName_fun x =
  match x with
    | ON_newDel _ ->
	assert false;

    | ON_operator (_, overloadableOp) -> 
	trace "ON_operator(";
	overloadableOp_fun overloadableOp;
	trace ")";

    | ON_conversion _ -> 
	assert false;


and statement_fun x =
  match x with
    | S_skip (annot, sourceLoc) ->
	trace "S_skip(";
        Format.print_string "skip";
	trace ")";

    | S_label _ ->
        assert false; (* labels are not supported, for they are useless *)
                      (* without goto                                   *)

    | S_case xx ->
	trace "S_case(";
        Format.print_string "case(";
	intLit_expression_fun xx.case_expr;
        Format.printf ") ##@\n";
	statement_fun xx.case_stmt;
	(* int32_fun int32 *) (* TODO: what is this for? *)
	trace ")";

    | S_default (annot, sourceLoc, statement) ->
	trace "S_default(";
        Format.printf "default ##@\n";
	statement_fun statement;
	trace ")";

    | S_expr (annot, sourceLoc, fullExpression) ->
	trace "S_expr(";
        Format.print_string "e2s(";
	fullExpression_fun fullExpression;
        Format.print_string ")";
	trace ")";

    | S_compound (annot, sourceLoc, statement_list) ->
	trace "S_compound(";
	if (List.length statement_list = 0) then
	  Format.print_string "skip"
	else
	  begin
	    (* collect all variable declarations in this block *)
	    let decls_in_stmt = function
	      | S_decl (_, _, declaration) ->
		  (List.map (fun xx ->
		      match xx.declarator_var with
			| Some v -> (v, xx.declarator_dtor_statement)
			| None   -> assert false) declaration.decllist)
	      | _ -> [] in
	    let var_dtor_list =
	      List.flatten (List.map decls_in_stmt statement_list)
	    in
	      (* allocate local variables on the stack *)
	      List.iter (fun (v, _) ->
		Format.printf "@[<2>with_new_stackvar(@[<2>lambda@ @[<2>(";
		variable_fun v;
		Format.printf "@ :@ Address)@]:@]@\n") var_dtor_list;
	      (* the compound statement itself *)
	      separate statement_fun (fun () -> Format.printf " ##@\n")
		statement_list;
	      (* destructor calls, in reverse allocation order *)
	      List.iter (fun (_, statement_opt_dtor) ->
		(match statement_opt_dtor with
		  | Some stmt ->
		      Format.printf " ##@\n";
		      Format.print_string "lift_destructor(";
		      statement_fun stmt;
		      Format.print_string ")";
		  | None -> ());
		Format.printf ")@]") (List.rev var_dtor_list);
	  end;
	trace ")";

    | S_if xx ->
	trace "S_if(";
        Format.printf "@[<2>if_else(";
	condition_fun xx.if_cond;
        Format.printf ",@\n";
	statement_fun xx.thenBranch;
        Format.printf ",@\n";
	statement_fun xx.elseBranch;
        Format.printf ")@]";
	trace ")";

    | S_switch xx -> 
	trace "S_switch(";
        Format.printf "switch@[<2>(";
	condition_fun xx.switch_cond;
        Format.printf ",@ @[(: ";
	let rec cases_in_stmt = function
            S_case xxx ->
	      [xxx.case_expr]
	  | S_compound (_, _, statement_list) ->
	      List.flatten (List.map cases_in_stmt statement_list)
	  | _ -> (* case labels within substatements are ignored *) []
	in
	  separate intLit_expression_fun (fun () -> Format.print_string ", ")
	    (cases_in_stmt xx.branches);
        Format.printf " :)@],@\n";
	statement_fun xx.branches;
        Format.printf ")@]";
	trace ")";

    | S_while xx ->
	trace "S_while(";
        Format.printf "@[<2>while(";
	bool_condition_fun xx.while_cond;
        Format.printf ",@\n";
	statement_fun xx.while_body;
        Format.printf ")@]";
	trace ")";

    | S_doWhile xx ->
	trace "S_doWhile(";
        Format.printf "@[<2>do_while(@\n";
	statement_fun xx.do_while_body;
        Format.printf ",@]@\n";
	fullExpression_fun xx.do_while_expr;
        Format.printf ")";
	trace ")";

    | S_for xx ->
	trace "S_for(";
        Format.printf "@[<2>for(";
	statement_fun xx.for_init;
        Format.printf ",@ ";
	bool_condition_fun xx.for_cond;
        Format.printf ",@ ";
	fullExpression_fun xx.after;
        Format.printf ",@\n";
	statement_fun xx.for_body;
        Format.printf ")@]";
	trace ")";

    | S_break(annot, sourceLoc) -> 
	trace "S_break(";
        Format.print_string "break";
	trace ")";

    | S_continue(annot, sourceLoc) ->
	trace "S_continue(";
        Format.print_string "continue";
	trace ")";

    | S_return xx ->
	trace "S_return(";
        (match xx.return_expr with
          | None ->
              Format.print_string "return_void"
          | Some fullExpression ->
              Format.print_string "return(";
              fullExpression_fun fullExpression;
              Format.print_string ")");
        assert (not (Option.isSome xx.return_ctor_statement)); (* ? *)
	Option.app statement_fun xx.return_ctor_statement;
	trace ")";

    | S_goto _ ->
        assert false;  (* goto is not supported *)

    | S_decl(annot, sourceLoc, declaration) ->
	(* variable declarations are translated to stack allocations before  *)
	(* the entire statement block; here only the initializing assignment *)
	(* (if present) still needs to be considered                         *)
	trace "S_decl(";
	(*TODO*)
        Format.print_string "e2s(";
	declaration_fun declaration;
        Format.print_string ")";
	trace ")";

    | S_try _ ->
        assert false;  (* try is not supported *)

    | S_asm(annot, sourceLoc, e_stringLit) ->
	trace "S_asm(";
	(* We do not really model assembler statements at the moment; the *)
	(* string argument is simply passed on to PVS.                    *)
        Format.print_string "asm(";
	expression_fun (E_stringLit e_stringLit);
        Format.print_string ")";
	trace ")";

    | S_namespaceDecl _ ->
	assert false;

    | S_function _ ->
	assert false;

    | S_rangeCase xx ->
	(*TODO*)
	trace "S_rangeCase(";
	Format.printf ">>>>>(";
	expression_fun xx.exprLo;
	expression_fun xx.exprHi;
	statement_fun xx.range_case_stmt;
	int_fun xx.labelValLo;
	int_fun xx.labelValHi;
	Format.printf ")<<<<<";
	trace ")";

    | S_computedGoto _ ->
	assert false;


(* boolean conditions *)
and bool_condition_fun x =
  match x with
    | CN_expr(annot, fullExpression) ->
	trace "CN_expr(";
	assert (Option.isSome (fullExpression.full_expr_expr));
	Option.app x2b_l2r_expression_fun (fullExpression.full_expr_expr);
	trace ")";

    | CN_decl _ ->
	trace "CN_decl(";
	(*TODO: declaration conditions currently unsupported*)
	assert false;
	trace ")";


(* integer conditions *)
and condition_fun x =
  match x with
    | CN_expr(annot, fullExpression) ->
	trace "CN_expr(";
	assert (Option.isSome (fullExpression.full_expr_expr));
	Option.app l2r_expression_fun (fullExpression.full_expr_expr);
	trace ")";

    | CN_decl _ ->
	trace "CN_decl(";
	(*TODO: declaration conditions currently unsupported*)
	assert false;
	trace ")";


and handler_fun _ =
    assert false;


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
    | E_intLit xx ->
	string_fun xx.int_lit_text
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

    | E_intLit xx ->
        trace "E_intLit(";
	(*Option.app cType_fun type_opt;*)
	(*string_fun stringRef;*)
	Format.print_string "literal(";
	int32_fun xx.i;
	Format.print_string ")";
        trace ")";

    | E_floatLit _ ->
	assert false;

    | E_stringLit xx ->
	let rec stringLit2string slit =
	  slit.string_lit_text ^
	    Option.getOpt(Option.map stringLit2string slit.continuation, "")
	in
	  trace "E_stringLit(";
	  string_fun (stringLit2string xx);
	  trace ")";

    | E_charLit xx ->
	trace "E_charLit(";
	assert (match xx.char_lit_type with
	  | Some (TY_CVAtomic(_, ATY_Simple(_, ST_CHAR), [])) -> true
	  | _                                                  -> false);
	(*Option.app cType_fun type_opt;*)  (* type is (always?) char *)
	(*string_fun stringRef;*)  (* string representation of the literal *)
	Format.print_string "literal(";
	int32_fun xx.c;  (* actual value of the literal *)
	Format.print_string ")";
	trace ")";

    | E_this(annot, type_opt, variable) ->
        trace "E_this(";
	(*Option.app cType_fun type_opt;*) (* type of variable ? *)
	Format.print_string "id(";
	variable_fun variable;
	Format.print_string ")";
        trace ")";

    | E_variable xx ->
        trace "E_variable(";
	(*Option.app cType_fun type_opt;*)
	Format.print_string "id(";
	Option.app variable_fun xx.expr_var_var;
	Format.print_string ")";
        assert (not (Option.isSome xx.expr_var_nondep_var));
	Option.app variable_fun xx.expr_var_nondep_var;
        trace ")";

    | E_funCall xx ->
        trace "E_funCall(";
	(* TODO: what is this? The return type? Why is it optional? *)
	assert (Option.isSome xx.fun_call_type);
	(*Option.app cType_fun type_opt;*)

	Format.print_string "call_";
	(* TODO: is this always just the name of a function? What about
	   function pointers? *)
	(function
	  | E_variable xxx ->
	      assert (Option.isSome xxx.expr_var_var);
	      Option.app variable_fun xxx.expr_var_var
	  | E_fieldAcc _ ->
	      expression_fun xx.func
	  | _ ->
	      assert false) xx.func;

	(* for member functions: prepend receiver object to argument list *)
	let receiver_argExpression_list = (function
	  | E_fieldAcc xxx ->
	      {argExpression_annotation = xx.e_funCall_annotation; arg_expr_expr = xxx.field_access_obj} :: xx.fun_call_args
	  | _ ->
	      xx.fun_call_args) xx.func
	in
	  if (List.length receiver_argExpression_list > 0) then
	    begin
	      Format.printf "(@[<2>";
	      separate argExpression_fun (fun () -> Format.printf ",@ ")
		receiver_argExpression_list;
	      Format.printf "@])";
	    end;

	(* TODO: what is this? *)
	assert (not (Option.isSome xx.fun_call_ret_obj));
	(*Option.app expression_fun expression_retobj_opt;*)
        trace ")";

    | E_constructor xx ->
        trace "E_constructor(";
	Format.print_string "call_";
	(* constructor name (?) *)
	assert (Option.isSome xx.constructor_ctor_var);
	Option.app variable_fun xx.constructor_ctor_var;
	(* class name (?) *)
	Format.print_string "_";
	assert (Option.isSome xx.constructor_type);
	Option.app cType_fun xx.constructor_type;
	Format.printf "(@[<2>";
	(* receiver object (?) *)
	assert (Option.isSome xx.constructor_ret_obj);
	Option.app expression_fun xx.constructor_ret_obj;
	(* arguments (?) *)
	if (List.length xx.constructor_args > 0) then
	  Format.printf ",@ ";
	separate argExpression_fun (fun () -> Format.printf ",@ ")
	  xx.constructor_args;
	Format.printf "@])";
	(* TODO: What's the difference between type_opt and typeSpecifier? *)
	(*
	  typeSpecifier_fun typeSpecifier;
	*)
	(* TODO: What is this? *)
	(*
	  bool_fun bool;
	*)
        trace ")";

    | E_fieldAcc xx ->
        trace "E_fieldAcc(";
	let isFunctionType = function
	  | TY_Function _ -> true
	  | _ -> false
	in
	  (* result type (?) *)
	  (*Option.app cType_fun type_opt;*)
	  if isFunctionType (Option.valOf xx.field_access_type) then
	    begin
	      (* field name (difference between this and var_opt?) *)
	      (*pQName_fun pQName;*)
	      assert (Option.isSome xx.field);
	      Option.app variable_fun xx.field;  (* field name *)
	    end
	  else
	    begin
	      Format.printf "@[<2>member(";
	      expression_fun xx.field_access_obj;  (* receiver object *)
	      Format.printf ",@ offsets_";
	      assert (Option.isSome (expression_type xx.field_access_obj));
	      Option.app (fun t -> cType_fun (derefType t))
		(expression_type xx.field_access_obj);  (* class type *)
	      Format.print_string "`";
	      (* field name (difference between this and var_opt?) *)
	      (*pQName_fun pQName;*)
	      assert (Option.isSome xx.field);
	      Option.app variable_fun xx.field;  (* field name *)
	      Format.printf ")@]";
	    end;
          trace ")";

    | E_sizeof xx ->
	(*TODO*)
	trace "E_sizeof(";
	Format.print_string "size(uidt(dt_";
	Option.app cType_fun xx.sizeof_type;
        Format.print_string "))";
	(*expression_fun expression;*)
	(*int_fun int;*)
        trace ")";

    | E_unary xx ->
        trace "E_unary(";
        assert (Option.isSome xx.unary_expr_type);
	(*Option.app cType_fun type_opt;*)
	unaryOp_fun xx.unary_expr_op;
        Format.print_string "(";
	expression_fun xx.unary_expr_expr;
        Format.print_string ")";
        trace ")";

    | E_effect xx ->
        trace "E_effect(";
        assert (Option.isSome xx.effect_type);
	(*Option.app cType_fun type_opt;*)
	effectOp_fun xx.effect_op;
        Format.printf "(pm, dt_int)(";
	expression_fun xx.effect_expr;
        Format.print_string ")";
        trace ")";

    | E_binary xx ->
        trace "E_binary(";
        assert (Option.isSome xx.binary_expr_type);
	(*Option.app cType_fun type_opt;*)
	binaryOp_fun xx.binary_expr_op;
        Format.printf "@[<2>(";
	expression_fun xx.e1; (*left*)
        Format.printf ",@ ";
	expression_fun xx.e2; (*right*)
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

    | E_cast _ ->
        trace "E_cast(";
	assert false;
        trace ")";

    | E_cond xx ->
        trace "E_cond(";
	Option.app cType_fun xx.cond_type;
        Format.print_string "(";
	expression_fun xx.cond_cond;
        Format.print_string " ? ";
	expression_fun xx.th; (*then*)
        Format.print_string " : ";
	expression_fun xx.cond_else;
        Format.print_string ")";
        trace ")";

    | E_sizeofType xx ->
	trace "E_sizeofType(";
	Format.printf ">>>>>";
	Option.app cType_fun xx.sizeof_type_type;
	aSTTypeId_fun xx.sizeof_type_atype;
	int_fun xx.sizeof_type_size;
	bool_fun xx.tchecked; (*?*)
	Format.printf "<<<<<";
	trace ")";

    | E_assign xx ->
        trace "E_assign(";
        assert (Option.isSome xx.assign_type);
	assignOp_fun xx.assign_op;
        Format.print_string "(pm, dt_";
	Option.app (fun t -> cType_fun (derefType t)) xx.assign_type;
	Format.printf ")@[<2>(";
	expression_fun xx.target;
        Format.printf ",@ ";
	l2r_expression_fun xx.src;
        Format.printf ")@]";
        trace ")";

    | E_new _ ->
        trace "E_new(";
	assert false;
	trace ")";

    | E_delete _ ->
	assert false;

    | E_throw _ ->
	assert false;

    | E_keywordCast xx ->
	trace "E_keywordCast(";
	castKeyword_fun xx.key;  (* cast keyword *)
	Format.printf "(@[<2>dt_";
	assert (Option.isSome xx.keyword_cast_type);
	Option.app cType_fun  xx.keyword_cast_type;  (* destination type *)
	(* TODO: what is this? *)
	(*
	aSTTypeId_fun aSTTypeId;
	*)
	Format.printf ",@ dt_";
	assert (Option.isSome (expression_type xx.keyword_cast_expr));
	Option.app cType_fun (expression_type xx.keyword_cast_expr);  (* source type *)
	Format.printf ")(@[<2>";
	expression_fun xx.keyword_cast_expr;  (* expression *)
	Format.printf ")@]@]";
	trace ")";

    | E_typeidExpr _ ->
	assert false;

    | E_typeidType _ ->
	assert false;

    | E_grouping _ ->
	assert false;

    | E_arrow _ ->
	assert false;

    | E_statement _ ->
        assert false;  (* statement expressions are not supported *)

    | E_compoundLit _ ->
	assert false;

    | E___builtin_constant_p _ ->
	assert false;

    | E___builtin_va_arg xx ->
	trace "E___builtin_va_arg(";
	(*TODO*)
	Format.printf ">>>>>";
	Option.app cType_fun xx.va_arg_type;
	expression_fun xx.va_arg_expr;
	aSTTypeId_fun xx.va_arg_atype;
	Format.printf "<<<<<";
	trace ")";

    | E_alignofType _ ->
	assert false;

    | E_alignofExpr _ ->
	assert false;

    | E_gnuCond _ ->
	assert false;

    | E_addrOfLabel _ ->
	assert false;

    | E_stdConv xx ->
	trace "E_stdConv(";
	(* TODO: type_opt currently contains the same type as the expression,
	   which is conceptually wrong, and does not (in general) allow us to
	   figure out which conversions precisely need to be applied.  Instead
	   type_opt should be the type of the expression after all conversions
	   are applied. *)
	assert (Option.isSome xx.std_conversion_type);
	implicitConversion_Kind_fun xx.conversionKind;
	(* TODO: user-defined conversions are currently not recorded properly
	   in the ast by Elsa.  A user-defined conversion should be broken
	   apart into a standard conversion sequence (scs), followed by a
	   function call and another scs. *)
	(match xx.conversionKind with
	  | IC_USER_DEFINED ->
	      Format.print_string ">>>>>IC_USER_DEFINED<<<<<"
	  | _ -> ());
	(* standard conversion sequence: contains at most one conversion each
	   from 3 different groups of conversions *)
	let is_present sc = List.mem sc xx.stdConv in 
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
	      Format.printf ">>>>>SC_INT_PROM<<<<<(@[<2>"
	    else if is_present SC_FLOAT_PROM then
	      (* 4.6: float -> double, no info loss possible *)
	      (* TODO *)
	      assert false
	    else if is_present SC_INT_CONV then
	      (* 4.7: int... -> int..., info loss possible *)
	      (* TODO *)
	      Format.printf ">>>>>SC_INT_CONV<<<<<(@[<2>"
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
	      Format.printf ">>>>>SC_PTR_CONV<<<<<(@[<2>"
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
	      begin
		Format.printf "l2r(@[<2>pm,@ dt_";
		Option.app cType_fun xx.std_conversion_type;
		Format.print_string ")(";
	      end
	    else if is_present SC_ARRAY_TO_PTR then
	      (* 4.2: char[] -> char* *)
	      (* TODO *)
	      Format.printf ">>>>>SC_ARRAY_TO_PTR<<<<<(@[<2>"
	    else if is_present SC_FUNC_TO_PTR then
	      (* 4.3: int ()(int) -> int ( * )(int) *)
	      (* TODO *)
	      assert false;
	  end;
	  expression_fun xx.std_conv_expr;
	  List.iter (fun _ -> Format.printf ")@]") xx.stdConv;
	  trace ")";


and fullExpression_fun x =
  begin
    trace "fullExpression_fun(";
    assert (Option.isSome (x.full_expr_expr));
    Option.app expression_fun x.full_expr_expr;
    fullExpressionAnnot_fun x.full_expr_annot;
    trace ")"
  end


and argExpression_fun x=
  begin
    trace "argExpression_fun(";
    expression_fun x.arg_expr_expr;
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
      IN_expr xx ->
	trace "IN_expr(";
	(* TODO: what is this? *)
	fullExpressionAnnot_fun xx.init_expr_annot;
	assert (Option.isSome xx.e);
	Option.app expression_fun xx.e;
	trace ")";

    | IN_compound _ ->
	assert false;

    | IN_ctor xx ->
	trace "IN_ctor(";
	(* In case we have an IN_ctor in a statement declaration, we also have a
	   constructor statement, which really contains all the necessary
	   information (in particular the constructor's arguments) and is therefore
	   printed instead of this IN_ctor node's data. *)

	(* TODO: what is this? *)
	fullExpressionAnnot_fun xx.init_ctor_annot;

	(* the constructor function *)
	assert (Option.isSome xx.init_ctor_var);
	(*
	  Option.app variable_fun var_opt;
	*)

	(* somewhat surprisingly, this list does NOT seem to contain the
	   constructor's arguments ... I wonder what it does contain then? *)
	assert (List.length xx.init_ctor_args = 0);
	(*
	  List.iter argExpression_fun argExpression_list;
	*)

	(* TODO: what is this? *)
	(*
	  bool_fun bool;
	*)
	trace ")";

    | IN_designated _ -> 
	assert false;


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
    | TP_type _ ->
	assert false;

    | TP_nontype _ ->
	assert false


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


and fullExpressionAnnot_fun x =
  begin
    trace "fullExpressionAnnot_fun(";
    List.iter declaration_fun x.declarations;
    trace ")";
  end


and aSTTypeof_fun x = 
  match x with
    | AST_typeof_expr _ ->
	assert false;

    | AST_typeof_type _ ->
	assert false


and designator_fun x =
  match x with
      FieldDesignator _ ->
	assert false;

    | SubscriptDesignator _ ->
	assert false;


and attribute_fun x =
  match x with
      AT_empty _ ->
	assert false;

    | AT_word (annot, sourceLoc, stringRef) ->
	trace "AT_word(";
	(*TODO*)
	string_fun stringRef;
	trace ")";

    | AT_func xx ->
	trace "AT_func(";
	(*TODO*)
	string_fun xx.f;
	List.iter argExpression_fun xx.function_args;
	trace ")";


and compilationUnit_fun x =
  trace "compilationUnit_fun(";
  translationUnit_fun x.unit;
  trace ")";


(**************************************************************************
 *
 * end of astiter.ml 
 *
 **************************************************************************)

open Superast

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
  let (_, ast) = Elsa_oast_header.unmarshal_oast !in_file_name in
  let theory_name = try
          String.sub !out_file_name 0 (String.index !out_file_name '.')
        with Not_found -> !out_file_name in
  let lemma_name = theory_name ^ "_spec" in
    begin
      out := open_out !out_file_name;
      Format.set_margin 79;
      Format.set_max_indent 60;
      Format.set_max_boxes max_int;
      Format.set_formatter_out_channel !out;
      Format.printf "@[@[<2>%s[State@ :@ Type]@ :@ THEORY@]@\n\
        BEGIN@\n\
        @[<2>@\n\
          @[<2>IMPORTING@ Cpp_Verification@]@\n\
          @\n" theory_name;
      compilationUnit_fun ast;
      (* TODO: this only works for int main(), not for int main(argc, argv)
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
