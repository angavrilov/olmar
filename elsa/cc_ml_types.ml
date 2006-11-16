(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

(* simple types needed in the ocaml ast *)

type unsigned_long = int32
type double = float
type unsigned_int = int32

(* SourceLoc is defined as an enum in srcloc.h, here we take the
 * xml representation, which is file * line * char
 *
 * xmlserilaization: hardwired in astgen:
 * toXml_SourceLoc -> sourceLocManager->getString(loc)
 *)
type sourceLoc = string * int * int

(* DeclFlags from cc_flags.h
 *)

(* ----------------------- DeclFlags ----------------------
 * These flags tell what keywords were attached to a variable when it
 * was declared.  They also reflect classifications of various kinds;
 * in particular, they distinguish the various roles that Variables
 * can play (variable, type, enumerator, etc.).  This can be used as
 * a convenient way to toss a new boolean into Variable, though it's
 * close to using all 32 bits, so it might need to be split.
 *
 * NOTE: Changes to this enumeration must be accompanied by
 * updates to 'declFlagNames' in cc_flags.cc.
 *)

type declFlag =
  (* | DF_NONE        (\* = 0x00000000 *\) *)

  (* syntactic declaration modifiers (UberModifiers) *)
  | DF_AUTO        (* = 0x00000001 *)
  | DF_REGISTER    (* = 0x00000002 *)
  | DF_STATIC      (* = 0x00000004 *)
  | DF_EXTERN      (* = 0x00000008 *)
  | DF_MUTABLE     (* = 0x00000010 *)
  | DF_INLINE      (* = 0x00000020 *)
  | DF_VIRTUAL     (* = 0x00000040 *)
  | DF_EXPLICIT    (* = 0x00000080 *)
  | DF_FRIEND      (* = 0x00000100 *)
  | DF_TYPEDEF     (* = 0x00000200 *)

  | DF_NAMESPACE   (* = 0x04000000    // names of namespaces *)
  (* | DF_SOURCEFLAGS (\* = 0x040003FF    // all flags that come from keywords in the source *\) *)

  (* semantic flags on Variables *)
  | DF_ENUMERATOR  (* = 0x00000400    // true for values in an 'enum' (enumerators in the terminology of the C++ standard) *)
  | DF_GLOBAL      (* = 0x00000800    // set for globals, unset for locals *)
  | DF_INITIALIZED (* = 0x00001000    // true if has been declared with an initializer (or, for functions, with code) *)
  | DF_BUILTIN     (* = 0x00002000    // true for e.g. __builtin_constant_p -- don't emit later *)
  | DF_PARAMETER   (* = 0x00010000    // true if this is a function parameter or a handler "parameter" *)
  | DF_MEMBER      (* = 0x00080000    // true for members of classes (data, static data, functions); *not* true for namespace members *)
  | DF_DEFINITION  (* = 0x00100000    // set once we've seen this Variable's definition *)
  | DF_INLINE_DEFN (* = 0x00200000    // set for inline function definitions on second pass of tcheck *)
  | DF_IMPLICIT    (* = 0x00400000    // set for C++ implicit typedefs (if also DF_TYPEDEF),
                  *                 // and implicit compiler-supplied member decls (if not DF_TYPEDEF) *)
  | DF_FORWARD     (* = 0x00800000    // for syntax which only provides a forward declaration *)
  | DF_TEMPORARY   (* = 0x01000000    // temporary variable introduced by elaboration *)
  | DF_GNU_EXTERN_INLINE (* = 0x02000000 // dsw: was extern inline (record since might be changed to static inline) *)
  | DF_EXTERN_C    (* = 0x08000000    // name is marked extern "C" *)
  | DF_SELFNAME    (* = 0x10000000    // section 9 para 2: name of class inside its own scope *)
  | DF_BOUND_TPARAM(* = 0x00004000    // template parameter bound to a concrete argument *)
  | DF_TEMPL_PARAM (* = 0x20000000    // template parameter (bound only to itself) *)
  | DF_USING_ALIAS (* = 0x40000000    // this is a 'using' alias *)
  | DF_BITFIELD    (* = 0x80000000    // this is a bitfield *)

  (* These flags are used by the old (direct C -> VC) verifier client
   * analysis; I will remove them once I finish transitioning to the
   * VML-based verifier.  In a pinch, one of these values could be
   * re-used for something that only occurs in C++ code, since the old
   * verifier only works with C code.
   *)
  | DF_ADDRTAKEN   (* = 0x00008000    // true if it's address has been (or can be) taken *)
  | DF_UNIVERSAL   (* = 0x00020000    // universally-quantified variable *)
  | DF_EXISTENTIAL (* = 0x00040000    // existentially-quantified *)

  (*
   * ALL_DECLFLAGS  = 0xFFFFFFFF
   * NUM_DECLFLAGS  = 32             // # bits set to 1 in ALL_DECLFLAGS
   *)


let is_DF_SOURCEFLAGS = function
  | DF_AUTO        (* = 0x00000001 *)
  | DF_REGISTER    (* = 0x00000002 *)
  | DF_STATIC      (* = 0x00000004 *)
  | DF_EXTERN      (* = 0x00000008 *)
  | DF_MUTABLE     (* = 0x00000010 *)
  | DF_INLINE      (* = 0x00000020 *)
  | DF_VIRTUAL     (* = 0x00000040 *)
  | DF_EXPLICIT    (* = 0x00000080 *)
  | DF_FRIEND      (* = 0x00000100 *)
  | DF_TYPEDEF     (* = 0x00000200 *)

  | DF_NAMESPACE   (* = 0x04000000    // names of namespaces *)
  (* | DF_SOURCEFLAGS (\* = 0x040003FF    // all flags that come from keywords in the source *\) *)
      -> true
  | _ -> false


(*
 * xmlserilaization via string toXml(DeclFlags df) as int
 *)
type declFlags = declFlag list


let string_of_declFlag = function
  | DF_AUTO         		-> "auto"
  | DF_REGISTER     		-> "register"
  | DF_STATIC       		-> "static"
  | DF_EXTERN       		-> "extern"
  | DF_MUTABLE      		-> "mutable"
  | DF_INLINE       		-> "inline"
  | DF_VIRTUAL      		-> "virtual"
  | DF_EXPLICIT     		-> "explicit"
  | DF_FRIEND       		-> "friend"
  | DF_TYPEDEF      		-> "typedef"
  | DF_NAMESPACE    		-> "namespace"
  | DF_ENUMERATOR   		-> "enumerator"
  | DF_GLOBAL       		-> "global"
  | DF_INITIALIZED  		-> "initialized"
  | DF_BUILTIN      		-> "builtin"
  | DF_PARAMETER    		-> "parameter"
  | DF_MEMBER       		-> "member"
  | DF_DEFINITION   		-> "definition"
  | DF_INLINE_DEFN  		-> "inline_defn"
  | DF_IMPLICIT     		-> "implicit"
  | DF_FORWARD      		-> "forward"
  | DF_TEMPORARY    		-> "temporary"
  | DF_EXTERN_C     		-> "extern_c"
  | DF_SELFNAME     		-> "selfname"
  | DF_BOUND_TPARAM 		-> "bound_tparam"
  | DF_TEMPL_PARAM  		-> "templ_param"
  | DF_USING_ALIAS  		-> "using_alias"
  | DF_BITFIELD     		-> "bitfield"
  | DF_GNU_EXTERN_INLINE 	-> "gnu extern inline"
  | DF_ADDRTAKEN		-> "addrtaken"
  | DF_UNIVERSAL		-> "universal"
  | DF_EXISTENTIAL		-> "existential"

let string_of_declFlags l = Elsa_util.string_of_flag_list string_of_declFlag l



(* from strtable.h
 * xmlserilaization as string, hardwired in astgen
 *)
type stringRef = string


(* from cc_flags.h
 * xmlserilaization as int via cpp generated toXml
 *)
(*
 * ------------------------- SimpleTypeId ----------------------------
 * C's built-in scalar types; the representation deliberately does
 * *not* imply any orthogonality of properties (like long vs signed);
 * separate query functions can determine such properties, or signal
 * when it is meaningless to query a given property of a given type
 * (like whether a floating-point type is unsigned)
 *)
type simpleTypeId =
  (*  types that exist in C++ *)
  | ST_CHAR
  | ST_UNSIGNED_CHAR
  | ST_SIGNED_CHAR
  | ST_BOOL
  | ST_INT
  | ST_UNSIGNED_INT
  | ST_LONG_INT
  | ST_UNSIGNED_LONG_INT
  | ST_LONG_LONG              (*  GNU/C99 extension *)
  | ST_UNSIGNED_LONG_LONG     (*  GNU/C99 extension *)
  | ST_SHORT_INT
  | ST_UNSIGNED_SHORT_INT
  | ST_WCHAR_T
  | ST_FLOAT
  | ST_DOUBLE
  | ST_LONG_DOUBLE
  | ST_FLOAT_COMPLEX          (*  GNU/C99 (see doc/complex.txt) *)
  | ST_DOUBLE_COMPLEX         (*  GNU/C99 *)
  | ST_LONG_DOUBLE_COMPLEX    (*  GNU/C99 *)
  | ST_FLOAT_IMAGINARY        (*  C99 *)
  | ST_DOUBLE_IMAGINARY       (*  C99 *)
  | ST_LONG_DOUBLE_IMAGINARY  (*  C99 *)
  | ST_VOID                   (*  last concrete type (see 'isConcreteSimpleType') *)

  (*  codes I use as a kind of implementation hack *)
  | ST_ELLIPSIS               (*  used to encode vararg functions *)
  | ST_CDTOR                  (*  "return type" for ctors and dtors *)
  | ST_ERROR                  (*  this type is returned for typechecking errors *)
  | ST_DEPENDENT              (*  depdenent on an uninstantiated template parameter type *)
  | ST_IMPLINT                (*  implicit-int for K&R C *)
  | ST_NOTFOUND               (*  delayed ST_ERROR *)

  (*  for polymorphic built-in operators (cppstd 13.6) *)
  | ST_PROMOTED_INTEGRAL      (*  int,uint,long,ulong *)
  | ST_PROMOTED_ARITHMETIC    (*  promoted integral + float,double,longdouble *)
  | ST_INTEGRAL               (*  has STF_INTEGER *)
  | ST_ARITHMETIC             (*  every simple type except void *)
  | ST_ARITHMETIC_NON_BOOL    (*  every simple type except void & bool *)
  | ST_ANY_OBJ_TYPE           (*  any object (non-function, non-void) type *)
  | ST_ANY_NON_VOID           (*  any type except void *)
  | ST_ANY_TYPE               (*  any type, including functions and void *)

  (*  for polymorphic builtin *return* ("PRET") type algorithms *)
  | ST_PRET_STRIP_REF         (*  strip reference and volatileness from 1st arg *)
  | ST_PRET_PTM               (*  ptr-to-member: union CVs, 2nd arg atType *)
  | ST_PRET_ARITH_CONV        (*  "usual arithmetic conversions" (5 para 9) on 1st, 2nd arg *)
  | ST_PRET_FIRST             (*  1st arg type *)
  | ST_PRET_FIRST_PTR2REF     (*  1st arg ptr type -> ref type *)
  | ST_PRET_SECOND            (*  2nd arg type *)
  | ST_PRET_SECOND_PTR2REF    (*  2nd arg ptr type -> ref type *)

  (*
   * NUM_SIMPLE_TYPES
   * ST_BITMASK = 0xFF          // for extraction for OR with CVFlags
   *)


let string_of_simpleTypeId = function
  | ST_CHAR                  -> "char"
  | ST_UNSIGNED_CHAR         -> "unsigned char"
  | ST_SIGNED_CHAR           -> "signed char"
  | ST_BOOL                  -> "bool"
  | ST_INT                   -> "int"
  | ST_UNSIGNED_INT          -> "unsigned int"
  | ST_LONG_INT              -> "long int"
  | ST_UNSIGNED_LONG_INT     -> "unsigned long int"
  | ST_LONG_LONG             -> "long long"
  | ST_UNSIGNED_LONG_LONG    -> "unsigned long long"
  | ST_SHORT_INT             -> "short int"
  | ST_UNSIGNED_SHORT_INT    -> "unsigned short int"
  | ST_WCHAR_T               -> "wchar t"
  | ST_FLOAT                 -> "float"
  | ST_DOUBLE                -> "double"
  | ST_LONG_DOUBLE           -> "long double"
  | ST_FLOAT_COMPLEX         -> "float complex"
  | ST_DOUBLE_COMPLEX        -> "double complex"
  | ST_LONG_DOUBLE_COMPLEX   -> "long double complex"
  | ST_FLOAT_IMAGINARY       -> "float imaginary"
  | ST_DOUBLE_IMAGINARY      -> "double imaginary"
  | ST_LONG_DOUBLE_IMAGINARY -> "long double imaginary"
  | ST_VOID                  -> "void"
  | ST_ELLIPSIS              -> "ellipsis"
  | ST_CDTOR                 -> "cdtor"
  | ST_ERROR                 -> "error"
  | ST_DEPENDENT             -> "dependent"
  | ST_IMPLINT               -> "implint"
  | ST_NOTFOUND              -> "notfound"
  | ST_PROMOTED_INTEGRAL     -> "promoted integral"
  | ST_PROMOTED_ARITHMETIC   -> "promoted arithmetic"
  | ST_INTEGRAL              -> "integral"
  | ST_ARITHMETIC            -> "arithmetic"
  | ST_ARITHMETIC_NON_BOOL   -> "arithmetic non bool"
  | ST_ANY_OBJ_TYPE          -> "any obj type"
  | ST_ANY_NON_VOID          -> "any non void"
  | ST_ANY_TYPE              -> "any type"
  | ST_PRET_STRIP_REF        -> "pret strip ref"
  | ST_PRET_PTM              -> "pret ptm"
  | ST_PRET_ARITH_CONV       -> "pret arith conv"
  | ST_PRET_FIRST            -> "pret first"
  | ST_PRET_FIRST_PTR2REF    -> "pret first ptr2ref"
  | ST_PRET_SECOND           -> "pret second"
  | ST_PRET_SECOND_PTR2REF   -> "pret second ptr2ref"


(* also cc_flags.h
 * xmlserilaization as int via cpp generated toXml
 *)
(* ----------------------- TypeIntr ----------------------
 * type introducer keyword
 * NOTE: keep consistent with CompoundType::Keyword (cc_type.h)
 *)
type typeIntr =
  | TI_STRUCT
  | TI_CLASS
  | TI_UNION
  | TI_ENUM
  (* NUM_TYPEINTRS *)


let string_of_typeIntr = function
  | TI_STRUCT -> "struct"
  | TI_CLASS  -> "class"
  | TI_UNION  -> "union"
  | TI_ENUM   -> "enum"

(* also cc_flags.h
 * xmlserilaization as int via cpp generated toXml
 *
 * ---------------- access control ------------
 * these are listed from least restrictive to most restrictive,
 * so < can be used to test for retriction level
 *)
type accessKeyword =
  | AK_PUBLIC
  | AK_PROTECTED
  | AK_PRIVATE
  | AK_UNSPECIFIED (* not explicitly specified; typechecking changes it later *)

  (* NUM_ACCESS_KEYWORDS *)


let string_of_accessKeyword = function
  | AK_PUBLIC      -> "public"
  | AK_PROTECTED   -> "protected"
  | AK_PRIVATE	   -> "private"
  | AK_UNSPECIFIED -> "unspecified"



(* cc_flags.h
 *
 * --------------------- CVFlags ---------------------
 * set: which of "const" and/or "volatile" is specified;
 * I leave the lower 8 bits to represent SimpleTypeId, so I can
 * freely OR them together during parsing;
 * values in common with UberModifier must line up
 *)
type cVFlag =
  (* | CV_NONE     (\* = 0x0000, *\) *)
  | CV_CONST    (* = 0x0400, *)
  | CV_VOLATILE (* = 0x0800, *)
  | CV_RESTRICT (* = 0x1000,     // C99 *)
  | CV_OWNER    (* = 0x2000,     // experimental extension *)
  (*
   * CV_ALL      = 0x2C00,
   *
   * CV_SHIFT_AMOUNT = 10,     // shift right this many bits before counting for cvFlagNames
   * NUM_CVFLAGS = 4           // # bits set to 1 in CV_ALL
   *)


(*
 * xmlserilaization as int via cpp generated toXml
 *)
type cVFlags = cVFlag list

let string_of_cVFlag = function
  | CV_CONST    -> "const"
  | CV_VOLATILE -> "volatile"
  | CV_RESTRICT -> "restrict"
  | CV_OWNER    -> "owner"

let string_of_cVFlags l = Elsa_util.string_of_flag_list string_of_cVFlag l

(* cc_flags.h
 * xmlserilaization as int via cpp generated toXml
 *
 * --------------- overloadable operators -------------
 * This is all of the unary and binary operators that are overloadable
 * in C++.  While it repeats operators that are also declared above in
 * some form, it makes the design more orthogonal: the operators above
 * are for use in *expressions*, while these are for use in operator
 * *names*, and there is not a 1-1 correspondence.  In a few places,
 * I've deliberately used different names (e.g. OP_STAR) than the name
 * of the operator above, to emphasize that there's less intrinsic
 * meaning to the operator names here.
 *)
type overloadableOp =
  (*  unary only *)
  | OP_NOT          (*  ! *)
  | OP_BITNOT       (*  ~ *)

  (*  unary, both prefix and postfix; latter is declared as 2-arg function *)
  | OP_PLUSPLUS     (*  ++ *)
  | OP_MINUSMINUS   (*  -- *)

  (*  unary or binary *)
  | OP_PLUS         (*  + *)
  | OP_MINUS        (*  - *)
  | OP_STAR         (*  * *)
  | OP_AMPERSAND    (*  & *)

  (*  arithmetic *)
  | OP_DIV          (*  / *)
  | OP_MOD          (*  % *)
  | OP_LSHIFT       (*  << *)
  | OP_RSHIFT       (*  >> *)
  | OP_BITXOR       (*  ^ *)
  | OP_BITOR        (*  | *)

  (*  arithmetic+assignment *)
  | OP_ASSIGN       (*  = *)
  | OP_PLUSEQ       (*  += *)
  | OP_MINUSEQ      (*  -= *)
  | OP_MULTEQ       (*  *= *)
  | OP_DIVEQ        (*  /= *)
  | OP_MODEQ        (*  %= *)
  | OP_LSHIFTEQ     (*  <<= *)
  | OP_RSHIFTEQ     (*  >>= *)
  | OP_BITANDEQ     (*  &= *)
  | OP_BITXOREQ     (*  ^= *)
  | OP_BITOREQ      (*  |= *)

  (*  comparison *)
  | OP_EQUAL        (*  == *)
  | OP_NOTEQUAL     (*  != *)
  | OP_LESS         (*  < *)
  | OP_GREATER      (*  > *)
  | OP_LESSEQ       (*  <= *)
  | OP_GREATEREQ    (*  >= *)

  (*  logical *)
  | OP_AND          (*  && *)
  | OP_OR           (*  || *)

  (*  arrows *)
  | OP_ARROW        (*  -> *)
  | OP_ARROW_STAR   (*  ->* *)

  (*  misc *)
  | OP_BRACKETS     (*  [] *)
  | OP_PARENS       (*  () *)
  | OP_COMMA        (*  , *)
  | OP_QUESTION     (*  ?:  (not overloadable, but resolution used nonetheless) *)

  (*  gcc extensions *)
  | OP_MINIMUM      (*  <? *)
  | OP_MAXIMUM      (*  >? *)

  (* NUM_OVERLOADABLE_OPS *)


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
  | OP_LSHIFTEQ	   -> "lshifteq"
  | OP_RSHIFTEQ	   -> "rshifteq"
  | OP_BITANDEQ	   -> "bitandeq"
  | OP_BITXOREQ	   -> "bitxoreq"
  | OP_BITOREQ	   -> "bitoreq"
  | OP_EQUAL	   -> "equal"
  | OP_NOTEQUAL	   -> "notequal"
  | OP_LESS	   -> "less"
  | OP_GREATER	   -> "greater"
  | OP_LESSEQ	   -> "lesseq"
  | OP_GREATEREQ   -> "greatereq"
  | OP_AND	   -> "and"
  | OP_OR	   -> "or"
  | OP_ARROW	   -> "arrow"
  | OP_ARROW_STAR  -> "arrow_star"
  | OP_BRACKETS	   -> "brackets"
  | OP_PARENS	   -> "parens"
  | OP_COMMA	   -> "comma"
  | OP_QUESTION	   -> "question"
  | OP_MINIMUM	   -> "minimum"
  | OP_MAXIMUM	   -> "maximum"


(* cc_flags.h
 * xmlserilaization as int via cpp generated toXml
 *
 * ---------------------------- UnaryOp ---------------------------
 *)
type unaryOp =
  | UNY_PLUS      (*  + *)
  | UNY_MINUS     (*  - *)
  | UNY_NOT       (*  ! *)
  | UNY_BITNOT    (*  ~ *)
  (* NUM_UNARYOPS *)


let string_of_unaryOp = function
  | UNY_PLUS    -> "+"
  | UNY_MINUS   -> "-"
  | UNY_NOT     -> "!"
  | UNY_BITNOT  -> "~"


(* cc_flags.h
 * xmlserilaization as int via cpp generated toXml
 *
 * ------------------------- EffectOp -------------------------
 * unary operator with a side effect
 *)
type effectOp =
  | EFF_POSTINC   (*  ++ (postfix) *)
  | EFF_POSTDEC   (*  -- (postfix) *)
  | EFF_PREINC    (*  ++ *)
  | EFF_PREDEC    (*  -- *)
  (* NUM_EFFECTOPS *)


let string_of_effectOp = function
  | EFF_POSTINC -> "postinc"
  | EFF_POSTDEC	-> "postdec"
  | EFF_PREINC	-> "preinc"
  | EFF_PREDEC	-> "predec"


(* cc_flags.h
 * xmlserilaization as int via cpp generated toXml
 *
 * ------------------------ BinaryOp --------------------------
 *)
type binaryOp =
  (*
   * the relationals come first, and in this order, to correspond
   * to RelationOp in predicate.ast (which is now in another
   * repository entirely...)
   *)
  | BIN_EQUAL     (*  == *)
  | BIN_NOTEQUAL  (*  != *)
  | BIN_LESS      (*  < *)
  | BIN_GREATER   (*  > *)
  | BIN_LESSEQ    (*  <= *)
  | BIN_GREATEREQ (*  >= *)

  | BIN_MULT      (*  * *)
  | BIN_DIV       (*  / *)
  | BIN_MOD       (*  % *)
  | BIN_PLUS      (*  + *)
  | BIN_MINUS     (*  - *)
  | BIN_LSHIFT    (*  << *)
  | BIN_RSHIFT    (*  >> *)
  | BIN_BITAND    (*  & *)
  | BIN_BITXOR    (*  ^ *)
  | BIN_BITOR     (*  | *)
  | BIN_AND       (*  && *)
  | BIN_OR        (*  || *)
  | BIN_COMMA     (*  , *)

  (*  gcc extensions *)
  | BIN_MINIMUM   (*  <? *)
  | BIN_MAXIMUM   (*  >? *)

  (*  this exists only between parsing and typechecking *)
  | BIN_BRACKETS  (*  [] *)

  | BIN_ASSIGN    (*  = (used to denote simple assignments in AST, as opposed to (say) "+=") *)

  (*  C++ operators *)
  | BIN_DOT_STAR    (*  .* *)
  | BIN_ARROW_STAR  (*  ->* *)

  (*  theorem prover extension *)
  | BIN_IMPLIES     (*  ==> *)
  | BIN_EQUIVALENT  (*  <==> *)

  (* NUM_BINARYOPS *)


let string_of_binaryOp = function
  | BIN_EQUAL       -> "equal"
  | BIN_NOTEQUAL    -> "notequal"
  | BIN_LESS	    -> "less"
  | BIN_GREATER	    -> "greater"
  | BIN_LESSEQ	    -> "lesseq"
  | BIN_GREATEREQ   -> "greatereq"
  | BIN_MULT	    -> "mult"
  | BIN_DIV	    -> "div"
  | BIN_MOD	    -> "mod"
  | BIN_PLUS	    -> "plus"
  | BIN_MINUS	    -> "minus"
  | BIN_LSHIFT	    -> "lshift"
  | BIN_RSHIFT	    -> "rshift"
  | BIN_BITAND	    -> "bitand"
  | BIN_BITXOR	    -> "bitxor"
  | BIN_BITOR	    -> "bitor"
  | BIN_AND	    -> "and"
  | BIN_OR	    -> "or"
  | BIN_COMMA	    -> "comma"
  | BIN_MINIMUM	    -> "minimum"
  | BIN_MAXIMUM	    -> "maximum"
  | BIN_BRACKETS    -> "brackets"
  | BIN_ASSIGN	    -> "assign"
  | BIN_DOT_STAR    -> "dot_star"
  | BIN_ARROW_STAR  -> "arrow_star"
  | BIN_IMPLIES	    -> "implies"
  | BIN_EQUIVALENT  -> "equivalent"



(* cc_flags.h
 * xmlserilaization as int via cpp generated toXml
 *
 * ---------------- cast keywords -------------
 *)
type castKeyword =
  | CK_DYNAMIC
  | CK_STATIC
  | CK_REINTERPRET
  | CK_CONST

  (* NUM_CAST_KEYWORDS *)



let string_of_castKeyword = function
  | CK_DYNAMIC     -> "dynamic"
  | CK_STATIC	   -> "static"
  | CK_REINTERPRET -> "reinterpret"
  | CK_CONST	   -> "const"



(* was cc_tcheck.ast, now cc_flags.h
*)

type declaratorContext =
  | DC_UNKNOWN         (* dummy value; nothing should have this after tcheck *)

  | DC_FUNCTION        (* Function::nameAndParams *)

                       (* inside Declaration *)
  | DC_TF_DECL         (*   TF_decl::decl *)
  | DC_TF_EXPLICITINST (*   TF_explicitInst::d *)
  | DC_MR_DECL         (*   MR_decl::d *)
  | DC_S_DECL          (*   S_decl::decl *)
  | DC_TD_DECL         (*   TD_decl::d *)
  | DC_FEA             (*   FullExpressionAnnot::declarations *)

                       (* inside ASTTypeId *)
  | DC_D_FUNC          (*   D_func::params *)
  | DC_EXCEPTIONSPEC   (*   ExceptionSpec::types *)
  | DC_ON_CONVERSION   (*   ON_conversion::type *)
  | DC_CN_DECL         (*   CN_decl::typeId *)
  | DC_HANDLER         (*   Handler::typeId *)
  | DC_E_CAST          (*   E_cast::ctype *)
  | DC_E_SIZEOFTYPE    (*   E_sizeofType::atype *)
  | DC_E_NEW           (*   E_new::atype (new) *)
  | DC_E_KEYWORDCAST   (*   E_keywordCast::ctype *)
  | DC_E_TYPEIDTYPE    (*   E_typeidType::ttype *)
  | DC_TP_TYPE         (*   TP_type::defaultType *)
  | DC_TP_NONTYPE      (*   TP_nontype::param *)
  | DC_TA_TYPE         (*   TA_type::type *)

  (* additional contexts in the GNU extensions *)
                           (* inside ASTTypeId *)
  | DC_TS_TYPEOF_TYPE      (*   TS_typeof_type::atype *)
  | DC_E_COMPOUNDLIT       (*   E_compoundLit::stype *)
  | DC_E_ALIGNOFTYPE       (*   E_alignofType::atype *)
  (* | DC_E_OFFSETOF,         (\*   E_offsetof::atype *\) *)
  | DC_E_BUILTIN_VA_ARG    (*   E___builtin_va_arg::atype *)



let string_of_declaratorContext = function
  | DC_UNKNOWN          -> "DC_UNKNOWN"
  | DC_FUNCTION         -> "DC_FUNCTION"
  | DC_TF_DECL          -> "DC_TF_DECL"
  | DC_TF_EXPLICITINST  -> "DC_TF_EXPLICITINST"
  | DC_MR_DECL          -> "DC_MR_DECL"
  | DC_S_DECL           -> "DC_S_DECL"
  | DC_TD_DECL          -> "DC_TD_DECL"
  | DC_FEA		-> "DC_FEA"
  | DC_D_FUNC           -> "DC_D_FUNC"
  | DC_EXCEPTIONSPEC    -> "DC_EXCEPTIONSPEC"
  | DC_ON_CONVERSION    -> "DC_ON_CONVERSION"
  | DC_CN_DECL          -> "DC_CN_DECL"
  | DC_HANDLER          -> "DC_HANDLER"
  | DC_E_CAST           -> "DC_E_CAST"
  | DC_E_SIZEOFTYPE     -> "DC_E_SIZEOFTYPE"
  | DC_E_NEW            -> "DC_E_NEW"
  | DC_E_KEYWORDCAST    -> "DC_E_KEYWORDCAST"
  | DC_E_TYPEIDTYPE     -> "DC_E_TYPEIDTYPE"
  | DC_TP_TYPE          -> "DC_TP_TYPE"
  | DC_TP_NONTYPE       -> "DC_TP_NONTYPE"
  | DC_TA_TYPE          -> "DC_TA_TYPE"
  | DC_TS_TYPEOF_TYPE   -> "DC_TS_TYPEOF_TYPE"
  | DC_E_COMPOUNDLIT    -> "DC_E_COMPOUNDLIT"
  | DC_E_ALIGNOFTYPE    -> "DC_E_ALIGNOFTYPE"
  | DC_E_BUILTIN_VA_ARG -> "DC_E_BUILTIN_VA_ARG"



(* from cc_flags.h *)

type scopeKind =
  | SK_UNKNOWN         (* hasn't been registered in a scope yet *)
  | SK_GLOBAL          (* toplevel names *)
  | SK_PARAMETER       (* parameter list *)
  | SK_FUNCTION        (* includes local variables *)
  | SK_CLASS           (* class member scope *)
  | SK_TEMPLATE_PARAMS (* template paramter list (inside the '<' and '>') *)
  | SK_TEMPLATE_ARGS   (* bound template arguments, during instantiation *)
  | SK_NAMESPACE       (* namespace *)


let string_of_scopeKind = function
  | SK_UNKNOWN        -> "SK_UNKNOWN"
  | SK_GLOBAL         -> "SK_GLOBAL"
  | SK_PARAMETER      -> "SK_PARAMETER"
  | SK_FUNCTION       -> "SK_FUNCTION"
  | SK_CLASS          -> "SK_CLASS"
  | SK_TEMPLATE_PARAMS-> "SK_TEMPLATE_PARAMS"
  | SK_TEMPLATE_ARGS  -> "SK_TEMPLATE_ARGS"
  | SK_NAMESPACE      -> "SK_NAMESPACE"
