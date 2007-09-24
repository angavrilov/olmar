
(* type declarations for cc.ast *)

(* SourceLoc is defined as an enum in srcloc.h, here we take the 
 * xml representation, which is file * line * char
 *
 * xmlserilaization: hardwired in astgen: 
 * toXml_SourceLoc -> sourceLocManager->getString(loc)
 *)
type sourceLoc = string * int * int

let create_SourceLoc_tuple s i j : sourceLoc = (s,i,j) 

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

  (* not used *)
  | DF_unused      (* = 0x02000000    // (available) *)
  
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


let df_flag_array = [|
  DF_AUTO;        (* = 0x00000001 *)
  DF_REGISTER;    (* = 0x00000002 *)
  DF_STATIC;      (* = 0x00000004 *)
  DF_EXTERN;      (* = 0x00000008 *)
  DF_MUTABLE;     (* = 0x00000010 *)
  DF_INLINE;      (* = 0x00000020 *)
  DF_VIRTUAL;     (* = 0x00000040 *)
  DF_EXPLICIT;    (* = 0x00000080 *)
  DF_FRIEND;      (* = 0x00000100 *)
  DF_TYPEDEF;     (* = 0x00000200 *)
  DF_ENUMERATOR;  (* = 0x00000400 *)
  DF_GLOBAL;      (* = 0x00000800 *)
  DF_INITIALIZED; (* = 0x00001000 *)
  DF_BUILTIN;     (* = 0x00002000 *)
  DF_BOUND_TPARAM;(* = 0x00004000 *)
  DF_ADDRTAKEN;   (* = 0x00008000 *)
  DF_PARAMETER;   (* = 0x00010000 *)
  DF_UNIVERSAL;   (* = 0x00020000 *)
  DF_EXISTENTIAL; (* = 0x00040000 *)
  DF_MEMBER;      (* = 0x00080000 *)
  DF_DEFINITION;  (* = 0x00100000 *)
  DF_INLINE_DEFN; (* = 0x00200000 *)
  DF_IMPLICIT;    (* = 0x00400000 *)
  DF_FORWARD;     (* = 0x00800000 *)
  DF_TEMPORARY;   (* = 0x01000000 *)
  DF_unused;      (* = 0x02000000 *)
  DF_NAMESPACE;   (* = 0x04000000 *)
  DF_EXTERN_C;    (* = 0x08000000 *)
  DF_SELFNAME;    (* = 0x10000000 *)
  DF_TEMPL_PARAM; (* = 0x20000000 *)
  DF_USING_ALIAS; (* = 0x40000000 *)
  DF_BITFIELD;    (* = 0x80000000 *)
|]

let _ = assert(Array.length df_flag_array = 32)

let declFlag_from_int32 flags =
  let rec doit i accu =
    if i = 32 then accu
    else 
      if Int32.logand (Int32.shift_left Int32.one i) flags <> Int32.zero
      then
	doit (i+1) (df_flag_array.(i) :: accu)
      else
	doit (i+1) accu
  in
  let res = doit 0 []
  in
    assert(List.for_all
	     (function 
		| DF_unused -> false
		| _ -> true)
	     res);
    res
	  
  


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


let create_ST_CHAR_constructor () = ST_CHAR
let create_ST_UNSIGNED_CHAR_constructor () = ST_UNSIGNED_CHAR
let create_ST_SIGNED_CHAR_constructor () = ST_SIGNED_CHAR
let create_ST_BOOL_constructor () = ST_BOOL
let create_ST_INT_constructor () = ST_INT
let create_ST_UNSIGNED_INT_constructor () = ST_UNSIGNED_INT
let create_ST_LONG_INT_constructor () = ST_LONG_INT
let create_ST_UNSIGNED_LONG_INT_constructor () = ST_UNSIGNED_LONG_INT
let create_ST_LONG_LONG_constructor () = ST_LONG_LONG
let create_ST_UNSIGNED_LONG_LONG_constructor () = ST_UNSIGNED_LONG_LONG
let create_ST_SHORT_INT_constructor () = ST_SHORT_INT
let create_ST_UNSIGNED_SHORT_INT_constructor () = ST_UNSIGNED_SHORT_INT
let create_ST_WCHAR_T_constructor () = ST_WCHAR_T
let create_ST_FLOAT_constructor () = ST_FLOAT
let create_ST_DOUBLE_constructor () = ST_DOUBLE
let create_ST_LONG_DOUBLE_constructor () = ST_LONG_DOUBLE
let create_ST_FLOAT_COMPLEX_constructor () = ST_FLOAT_COMPLEX
let create_ST_DOUBLE_COMPLEX_constructor () = ST_DOUBLE_COMPLEX
let create_ST_LONG_DOUBLE_COMPLEX_constructor () = ST_LONG_DOUBLE_COMPLEX
let create_ST_FLOAT_IMAGINARY_constructor () = ST_FLOAT_IMAGINARY
let create_ST_DOUBLE_IMAGINARY_constructor () = ST_DOUBLE_IMAGINARY
let create_ST_LONG_DOUBLE_IMAGINARY_constructor () = ST_LONG_DOUBLE_IMAGINARY
let create_ST_VOID_constructor () = ST_VOID
let create_ST_ELLIPSIS_constructor () = ST_ELLIPSIS
let create_ST_CDTOR_constructor () = ST_CDTOR
let create_ST_ERROR_constructor () = ST_ERROR
let create_ST_DEPENDENT_constructor () = ST_DEPENDENT
let create_ST_IMPLINT_constructor () = ST_IMPLINT
let create_ST_NOTFOUND_constructor () = ST_NOTFOUND
let create_ST_PROMOTED_INTEGRAL_constructor () = ST_PROMOTED_INTEGRAL
let create_ST_PROMOTED_ARITHMETIC_constructor () = ST_PROMOTED_ARITHMETIC
let create_ST_INTEGRAL_constructor () = ST_INTEGRAL
let create_ST_ARITHMETIC_constructor () = ST_ARITHMETIC
let create_ST_ARITHMETIC_NON_BOOL_constructor () = ST_ARITHMETIC_NON_BOOL
let create_ST_ANY_OBJ_TYPE_constructor () = ST_ANY_OBJ_TYPE
let create_ST_ANY_NON_VOID_constructor () = ST_ANY_NON_VOID
let create_ST_ANY_TYPE_constructor () = ST_ANY_TYPE
let create_ST_PRET_STRIP_REF_constructor () = ST_PRET_STRIP_REF
let create_ST_PRET_PTM_constructor () = ST_PRET_PTM
let create_ST_PRET_ARITH_CONV_constructor () = ST_PRET_ARITH_CONV
let create_ST_PRET_FIRST_constructor () = ST_PRET_FIRST
let create_ST_PRET_FIRST_PTR2REF_constructor () = ST_PRET_FIRST_PTR2REF
let create_ST_PRET_SECOND_constructor () = ST_PRET_SECOND
let create_ST_PRET_SECOND_PTR2REF_constructor () = ST_PRET_SECOND_PTR2REF

let register_ST_callbacks () =
  Callback.register "create_ST_CHAR_constructor" create_ST_CHAR_constructor;
  Callback.register
    "create_ST_UNSIGNED_CHAR_constructor" create_ST_UNSIGNED_CHAR_constructor;
  Callback.register
    "create_ST_SIGNED_CHAR_constructor" create_ST_SIGNED_CHAR_constructor;
  Callback.register "create_ST_BOOL_constructor" create_ST_BOOL_constructor;
  Callback.register "create_ST_INT_constructor" create_ST_INT_constructor;
  Callback.register
    "create_ST_UNSIGNED_INT_constructor" create_ST_UNSIGNED_INT_constructor;
  Callback.register
    "create_ST_LONG_INT_constructor" create_ST_LONG_INT_constructor;
  Callback.register
    "create_ST_UNSIGNED_LONG_INT_constructor"
    create_ST_UNSIGNED_LONG_INT_constructor;
  Callback.register
    "create_ST_LONG_LONG_constructor" create_ST_LONG_LONG_constructor;
  Callback.register
    "create_ST_UNSIGNED_LONG_LONG_constructor"
    create_ST_UNSIGNED_LONG_LONG_constructor;
  Callback.register
    "create_ST_SHORT_INT_constructor" create_ST_SHORT_INT_constructor;
  Callback.register
    "create_ST_UNSIGNED_SHORT_INT_constructor"
    create_ST_UNSIGNED_SHORT_INT_constructor;
  Callback.register
    "create_ST_WCHAR_T_constructor" create_ST_WCHAR_T_constructor;
  Callback.register "create_ST_FLOAT_constructor" create_ST_FLOAT_constructor;
  Callback.register
    "create_ST_DOUBLE_constructor" create_ST_DOUBLE_constructor;
  Callback.register
    "create_ST_LONG_DOUBLE_constructor" create_ST_LONG_DOUBLE_constructor;
  Callback.register
    "create_ST_FLOAT_COMPLEX_constructor" create_ST_FLOAT_COMPLEX_constructor;
  Callback.register
    "create_ST_DOUBLE_COMPLEX_constructor" create_ST_DOUBLE_COMPLEX_constructor;
  Callback.register
    "create_ST_LONG_DOUBLE_COMPLEX_constructor"
    create_ST_LONG_DOUBLE_COMPLEX_constructor;
  Callback.register
    "create_ST_FLOAT_IMAGINARY_constructor"
    create_ST_FLOAT_IMAGINARY_constructor;
  Callback.register
    "create_ST_DOUBLE_IMAGINARY_constructor"
    create_ST_DOUBLE_IMAGINARY_constructor;
  Callback.register
    "create_ST_LONG_DOUBLE_IMAGINARY_constructor"
    create_ST_LONG_DOUBLE_IMAGINARY_constructor;
  Callback.register "create_ST_VOID_constructor" create_ST_VOID_constructor;
  Callback.register
    "create_ST_ELLIPSIS_constructor" create_ST_ELLIPSIS_constructor;
  Callback.register "create_ST_CDTOR_constructor" create_ST_CDTOR_constructor;
  Callback.register "create_ST_ERROR_constructor" create_ST_ERROR_constructor;
  Callback.register
    "create_ST_DEPENDENT_constructor" create_ST_DEPENDENT_constructor;
  Callback.register
    "create_ST_IMPLINT_constructor" create_ST_IMPLINT_constructor;
  Callback.register
    "create_ST_NOTFOUND_constructor" create_ST_NOTFOUND_constructor;
  Callback.register
    "create_ST_PROMOTED_INTEGRAL_constructor"
    create_ST_PROMOTED_INTEGRAL_constructor;
  Callback.register
    "create_ST_PROMOTED_ARITHMETIC_constructor"
    create_ST_PROMOTED_ARITHMETIC_constructor;
  Callback.register
    "create_ST_INTEGRAL_constructor" create_ST_INTEGRAL_constructor;
  Callback.register
    "create_ST_ARITHMETIC_constructor" create_ST_ARITHMETIC_constructor;
  Callback.register
    "create_ST_ARITHMETIC_NON_BOOL_constructor"
    create_ST_ARITHMETIC_NON_BOOL_constructor;
  Callback.register
    "create_ST_ANY_OBJ_TYPE_constructor" create_ST_ANY_OBJ_TYPE_constructor;
  Callback.register
    "create_ST_ANY_NON_VOID_constructor" create_ST_ANY_NON_VOID_constructor;
  Callback.register
    "create_ST_ANY_TYPE_constructor" create_ST_ANY_TYPE_constructor;
  Callback.register
    "create_ST_PRET_STRIP_REF_constructor" create_ST_PRET_STRIP_REF_constructor;
  Callback.register
    "create_ST_PRET_PTM_constructor" create_ST_PRET_PTM_constructor;
  Callback.register
    "create_ST_PRET_ARITH_CONV_constructor"
    create_ST_PRET_ARITH_CONV_constructor;
  Callback.register
    "create_ST_PRET_FIRST_constructor" create_ST_PRET_FIRST_constructor;
  Callback.register
    "create_ST_PRET_FIRST_PTR2REF_constructor"
    create_ST_PRET_FIRST_PTR2REF_constructor;
  Callback.register
    "create_ST_PRET_SECOND_constructor" create_ST_PRET_SECOND_constructor;
  Callback.register
    "create_ST_PRET_SECOND_PTR2REF_constructor"
    create_ST_PRET_SECOND_PTR2REF_constructor;
  ()



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

let create_TI_STRUCT_constructor () = TI_STRUCT
let create_TI_CLASS_constructor () = TI_CLASS
let create_TI_UNION_constructor () = TI_UNION
let create_TI_ENUM_constructor () = TI_ENUM

let register_TI_callbacks () =
  Callback.register 
    "create_TI_STRUCT_constructor"
    create_TI_STRUCT_constructor;
  Callback.register 
    "create_TI_CLASS_constructor"
    create_TI_CLASS_constructor;
  Callback.register 
    "create_TI_UNION_constructor"
    create_TI_UNION_constructor;
  Callback.register 
    "create_TI_ENUM_constructor"
    create_TI_ENUM_constructor;
  ()




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


let create_AK_PUBLIC_constructor () = AK_PUBLIC
let create_AK_PROTECTED_constructor () = AK_PROTECTED
let create_AK_PRIVATE_constructor () = AK_PRIVATE
let create_AK_UNSPECIFIED_constructor () = AK_UNSPECIFIED

let register_AK_callbacks () =
  Callback.register 
    "create_AK_PUBLIC_constructor"
    create_AK_PUBLIC_constructor;
  Callback.register 
    "create_AK_PROTECTED_constructor"
    create_AK_PROTECTED_constructor;
  Callback.register 
    "create_AK_PRIVATE_constructor"
    create_AK_PRIVATE_constructor;
  Callback.register 
    "create_AK_UNSPECIFIED_constructor"
    create_AK_UNSPECIFIED_constructor;
  ()




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

let cv_shift_amount = 10

let cv_flag_array = [|
  CV_CONST;    (* = 0x0400 *)
  CV_VOLATILE; (* = 0x0800 *)
  CV_RESTRICT; (* = 0x1000 *)
  CV_OWNER;    (* = 0x2000 *)
|]

let _ = assert(Array.length cv_flag_array = 4)

let cv_mask =
  Int32.lognot(
    Int32.of_int(int_of_float(
	2.0 ** (float_of_int (Array.length cv_flag_array)) -. 1.0) 
	     lsl cv_shift_amount))


let cVFlag_from_int32 flags =
  let rec doit i accu =
    if i = 4 then accu
    else 
      if Int32.logand(Int32.shift_left Int32.one (i + cv_shift_amount)) flags 
	<> Int32.zero
      then
	doit (i+1) (cv_flag_array.(i) :: accu)
      else
	doit (i+1) accu
  in
    assert(Int32.logand cv_mask flags = Int32.zero);
    doit 0 []


(*
 * xmlserilaization as int via cpp generated toXml
 *)
type cVFlags = cVFlag list



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


let create_OP_NOT_constructor () = OP_NOT
let create_OP_BITNOT_constructor () = OP_BITNOT
let create_OP_PLUSPLUS_constructor () = OP_PLUSPLUS
let create_OP_MINUSMINUS_constructor () = OP_MINUSMINUS
let create_OP_PLUS_constructor () = OP_PLUS
let create_OP_MINUS_constructor () = OP_MINUS
let create_OP_STAR_constructor () = OP_STAR
let create_OP_AMPERSAND_constructor () = OP_AMPERSAND
let create_OP_DIV_constructor () = OP_DIV
let create_OP_MOD_constructor () = OP_MOD
let create_OP_LSHIFT_constructor () = OP_LSHIFT
let create_OP_RSHIFT_constructor () = OP_RSHIFT
let create_OP_BITXOR_constructor () = OP_BITXOR
let create_OP_BITOR_constructor () = OP_BITOR
let create_OP_ASSIGN_constructor () = OP_ASSIGN
let create_OP_PLUSEQ_constructor () = OP_PLUSEQ
let create_OP_MINUSEQ_constructor () = OP_MINUSEQ
let create_OP_MULTEQ_constructor () = OP_MULTEQ
let create_OP_DIVEQ_constructor () = OP_DIVEQ
let create_OP_MODEQ_constructor () = OP_MODEQ
let create_OP_LSHIFTEQ_constructor () = OP_LSHIFTEQ
let create_OP_RSHIFTEQ_constructor () = OP_RSHIFTEQ
let create_OP_BITANDEQ_constructor () = OP_BITANDEQ
let create_OP_BITXOREQ_constructor () = OP_BITXOREQ
let create_OP_BITOREQ_constructor () = OP_BITOREQ
let create_OP_EQUAL_constructor () = OP_EQUAL
let create_OP_NOTEQUAL_constructor () = OP_NOTEQUAL
let create_OP_LESS_constructor () = OP_LESS
let create_OP_GREATER_constructor () = OP_GREATER
let create_OP_LESSEQ_constructor () = OP_LESSEQ
let create_OP_GREATEREQ_constructor () = OP_GREATEREQ
let create_OP_AND_constructor () = OP_AND
let create_OP_OR_constructor () = OP_OR
let create_OP_ARROW_constructor () = OP_ARROW
let create_OP_ARROW_STAR_constructor () = OP_ARROW_STAR
let create_OP_BRACKETS_constructor () = OP_BRACKETS
let create_OP_PARENS_constructor () = OP_PARENS
let create_OP_COMMA_constructor () = OP_COMMA
let create_OP_QUESTION_constructor () = OP_QUESTION
let create_OP_MINIMUM_constructor () = OP_MINIMUM
let create_OP_MAXIMUM_constructor () = OP_MAXIMUM

let register_OP_callbacks () =
  Callback.register 
    "create_OP_NOT_constructor"
    create_OP_NOT_constructor;
  Callback.register 
    "create_OP_BITNOT_constructor"
    create_OP_BITNOT_constructor;
  Callback.register 
    "create_OP_PLUSPLUS_constructor"
    create_OP_PLUSPLUS_constructor;
  Callback.register 
    "create_OP_MINUSMINUS_constructor"
    create_OP_MINUSMINUS_constructor;
  Callback.register 
    "create_OP_PLUS_constructor"
    create_OP_PLUS_constructor;
  Callback.register 
    "create_OP_MINUS_constructor"
    create_OP_MINUS_constructor;
  Callback.register 
    "create_OP_STAR_constructor"
    create_OP_STAR_constructor;
  Callback.register 
    "create_OP_AMPERSAND_constructor"
    create_OP_AMPERSAND_constructor;
  Callback.register 
    "create_OP_DIV_constructor"
    create_OP_DIV_constructor;
  Callback.register 
    "create_OP_MOD_constructor"
    create_OP_MOD_constructor;
  Callback.register 
    "create_OP_LSHIFT_constructor"
    create_OP_LSHIFT_constructor;
  Callback.register 
    "create_OP_RSHIFT_constructor"
    create_OP_RSHIFT_constructor;
  Callback.register 
    "create_OP_BITXOR_constructor"
    create_OP_BITXOR_constructor;
  Callback.register 
    "create_OP_BITOR_constructor"
    create_OP_BITOR_constructor;
  Callback.register 
    "create_OP_ASSIGN_constructor"
    create_OP_ASSIGN_constructor;
  Callback.register 
    "create_OP_PLUSEQ_constructor"
    create_OP_PLUSEQ_constructor;
  Callback.register 
    "create_OP_MINUSEQ_constructor"
    create_OP_MINUSEQ_constructor;
  Callback.register 
    "create_OP_MULTEQ_constructor"
    create_OP_MULTEQ_constructor;
  Callback.register 
    "create_OP_DIVEQ_constructor"
    create_OP_DIVEQ_constructor;
  Callback.register 
    "create_OP_MODEQ_constructor"
    create_OP_MODEQ_constructor;
  Callback.register 
    "create_OP_LSHIFTEQ_constructor"
    create_OP_LSHIFTEQ_constructor;
  Callback.register 
    "create_OP_RSHIFTEQ_constructor"
    create_OP_RSHIFTEQ_constructor;
  Callback.register 
    "create_OP_BITANDEQ_constructor"
    create_OP_BITANDEQ_constructor;
  Callback.register 
    "create_OP_BITXOREQ_constructor"
    create_OP_BITXOREQ_constructor;
  Callback.register 
    "create_OP_BITOREQ_constructor"
    create_OP_BITOREQ_constructor;
  Callback.register 
    "create_OP_EQUAL_constructor"
    create_OP_EQUAL_constructor;
  Callback.register 
    "create_OP_NOTEQUAL_constructor"
    create_OP_NOTEQUAL_constructor;
  Callback.register 
    "create_OP_LESS_constructor"
    create_OP_LESS_constructor;
  Callback.register 
    "create_OP_GREATER_constructor"
    create_OP_GREATER_constructor;
  Callback.register 
    "create_OP_LESSEQ_constructor"
    create_OP_LESSEQ_constructor;
  Callback.register 
    "create_OP_GREATEREQ_constructor"
    create_OP_GREATEREQ_constructor;
  Callback.register 
    "create_OP_AND_constructor"
    create_OP_AND_constructor;
  Callback.register 
    "create_OP_OR_constructor"
    create_OP_OR_constructor;
  Callback.register 
    "create_OP_ARROW_constructor"
    create_OP_ARROW_constructor;
  Callback.register 
    "create_OP_ARROW_STAR_constructor"
    create_OP_ARROW_STAR_constructor;
  Callback.register 
    "create_OP_BRACKETS_constructor"
    create_OP_BRACKETS_constructor;
  Callback.register 
    "create_OP_PARENS_constructor"
    create_OP_PARENS_constructor;
  Callback.register 
    "create_OP_COMMA_constructor"
    create_OP_COMMA_constructor;
  Callback.register 
    "create_OP_QUESTION_constructor"
    create_OP_QUESTION_constructor;
  Callback.register 
    "create_OP_MINIMUM_constructor"
    create_OP_MINIMUM_constructor;
  Callback.register 
    "create_OP_MAXIMUM_constructor"
    create_OP_MAXIMUM_constructor;
  ()



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



let create_UNY_PLUS_constructor () = UNY_PLUS
let create_UNY_MINUS_constructor () = UNY_MINUS
let create_UNY_NOT_constructor () = UNY_NOT
let create_UNY_BITNOT_constructor () = UNY_BITNOT

let register_UNY_callbacks () =
  Callback.register 
    "create_UNY_PLUS_constructor"
    create_UNY_PLUS_constructor;
  Callback.register 
    "create_UNY_MINUS_constructor"
    create_UNY_MINUS_constructor;
  Callback.register 
    "create_UNY_NOT_constructor"
    create_UNY_NOT_constructor;
  Callback.register 
    "create_UNY_BITNOT_constructor"
    create_UNY_BITNOT_constructor;
  ()



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


let create_EFF_POSTINC_constructor () = EFF_POSTINC
let create_EFF_POSTDEC_constructor () = EFF_POSTDEC
let create_EFF_PREINC_constructor () = EFF_PREINC
let create_EFF_PREDEC_constructor () = EFF_PREDEC

let register_EFF_callbacks () =
  Callback.register 
    "create_EFF_POSTINC_constructor"
    create_EFF_POSTINC_constructor;
  Callback.register 
    "create_EFF_POSTDEC_constructor"
    create_EFF_POSTDEC_constructor;
  Callback.register 
    "create_EFF_PREINC_constructor"
    create_EFF_PREINC_constructor;
  Callback.register 
    "create_EFF_PREDEC_constructor"
    create_EFF_PREDEC_constructor;
  ()



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


let create_BIN_EQUAL_constructor () = BIN_EQUAL
let create_BIN_NOTEQUAL_constructor () = BIN_NOTEQUAL
let create_BIN_LESS_constructor () = BIN_LESS
let create_BIN_GREATER_constructor () = BIN_GREATER
let create_BIN_LESSEQ_constructor () = BIN_LESSEQ
let create_BIN_GREATEREQ_constructor () = BIN_GREATEREQ
let create_BIN_MULT_constructor () = BIN_MULT
let create_BIN_DIV_constructor () = BIN_DIV
let create_BIN_MOD_constructor () = BIN_MOD
let create_BIN_PLUS_constructor () = BIN_PLUS
let create_BIN_MINUS_constructor () = BIN_MINUS
let create_BIN_LSHIFT_constructor () = BIN_LSHIFT
let create_BIN_RSHIFT_constructor () = BIN_RSHIFT
let create_BIN_BITAND_constructor () = BIN_BITAND
let create_BIN_BITXOR_constructor () = BIN_BITXOR
let create_BIN_BITOR_constructor () = BIN_BITOR
let create_BIN_AND_constructor () = BIN_AND
let create_BIN_OR_constructor () = BIN_OR
let create_BIN_COMMA_constructor () = BIN_COMMA
let create_BIN_MINIMUM_constructor () = BIN_MINIMUM
let create_BIN_MAXIMUM_constructor () = BIN_MAXIMUM
let create_BIN_BRACKETS_constructor () = BIN_BRACKETS
let create_BIN_ASSIGN_constructor () = BIN_ASSIGN
let create_BIN_DOT_STAR_constructor () = BIN_DOT_STAR
let create_BIN_ARROW_STAR_constructor () = BIN_ARROW_STAR
let create_BIN_IMPLIES_constructor () = BIN_IMPLIES
let create_BIN_EQUIVALENT_constructor () = BIN_EQUIVALENT

let register_BIN_callbacks () =
  Callback.register 
    "create_BIN_EQUAL_constructor"
    create_BIN_EQUAL_constructor;
  Callback.register 
    "create_BIN_NOTEQUAL_constructor"
    create_BIN_NOTEQUAL_constructor;
  Callback.register 
    "create_BIN_LESS_constructor"
    create_BIN_LESS_constructor;
  Callback.register 
    "create_BIN_GREATER_constructor"
    create_BIN_GREATER_constructor;
  Callback.register 
    "create_BIN_LESSEQ_constructor"
    create_BIN_LESSEQ_constructor;
  Callback.register 
    "create_BIN_GREATEREQ_constructor"
    create_BIN_GREATEREQ_constructor;
  Callback.register 
    "create_BIN_MULT_constructor"
    create_BIN_MULT_constructor;
  Callback.register 
    "create_BIN_DIV_constructor"
    create_BIN_DIV_constructor;
  Callback.register 
    "create_BIN_MOD_constructor"
    create_BIN_MOD_constructor;
  Callback.register 
    "create_BIN_PLUS_constructor"
    create_BIN_PLUS_constructor;
  Callback.register 
    "create_BIN_MINUS_constructor"
    create_BIN_MINUS_constructor;
  Callback.register 
    "create_BIN_LSHIFT_constructor"
    create_BIN_LSHIFT_constructor;
  Callback.register 
    "create_BIN_RSHIFT_constructor"
    create_BIN_RSHIFT_constructor;
  Callback.register 
    "create_BIN_BITAND_constructor"
    create_BIN_BITAND_constructor;
  Callback.register 
    "create_BIN_BITXOR_constructor"
    create_BIN_BITXOR_constructor;
  Callback.register 
    "create_BIN_BITOR_constructor"
    create_BIN_BITOR_constructor;
  Callback.register 
    "create_BIN_AND_constructor"
    create_BIN_AND_constructor;
  Callback.register 
    "create_BIN_OR_constructor"
    create_BIN_OR_constructor;
  Callback.register 
    "create_BIN_COMMA_constructor"
    create_BIN_COMMA_constructor;
  Callback.register 
    "create_BIN_MINIMUM_constructor"
    create_BIN_MINIMUM_constructor;
  Callback.register 
    "create_BIN_MAXIMUM_constructor"
    create_BIN_MAXIMUM_constructor;
  Callback.register 
    "create_BIN_BRACKETS_constructor"
    create_BIN_BRACKETS_constructor;
  Callback.register 
    "create_BIN_ASSIGN_constructor"
    create_BIN_ASSIGN_constructor;
  Callback.register 
    "create_BIN_DOT_STAR_constructor"
    create_BIN_DOT_STAR_constructor;
  Callback.register 
    "create_BIN_ARROW_STAR_constructor"
    create_BIN_ARROW_STAR_constructor;
  Callback.register 
    "create_BIN_IMPLIES_constructor"
    create_BIN_IMPLIES_constructor;
  Callback.register 
    "create_BIN_EQUIVALENT_constructor"
    create_BIN_EQUIVALENT_constructor;
  ()


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


let create_CK_DYNAMIC_constructor () = CK_DYNAMIC
let create_CK_STATIC_constructor () = CK_STATIC
let create_CK_REINTERPRET_constructor () = CK_REINTERPRET
let create_CK_CONST_constructor () = CK_CONST

let register_CK_callbacks () =
  Callback.register 
    "create_CK_DYNAMIC_constructor" create_CK_DYNAMIC_constructor;
  Callback.register "create_CK_STATIC_constructor" create_CK_STATIC_constructor;
  Callback.register 
    "create_CK_REINTERPRET_constructor" create_CK_REINTERPRET_constructor;
  Callback.register "create_CK_CONST_constructor" create_CK_CONST_constructor;
  ()







let register_cctypes_callbacks () =
  Callback.register "create_SourceLoc_tuple" create_SourceLoc_tuple;
  Callback.register "declFlag_from_int32" declFlag_from_int32;
  Callback.register "cVFlag_from_int32" cVFlag_from_int32;
  register_OP_callbacks();
  register_UNY_callbacks();
  register_EFF_callbacks();
  register_BIN_callbacks();
  register_CK_callbacks();
  ()



(* (query-replace-regexp "//\\(.*\\)" "(* \\1 *)" nil nil nil) *)
(* (query-replace-regexp "\\(=.*\\)" "(* \\1 *)" nil nil nil) *)

(*

to create enum constructors and their callbacks:

(defun ml-of-enum ()
  (interactive)
  (narrow-to-region (point) (mark))
  (beginning-of-buffer)
  (replace-regexp "(\\*.+" "")
  (end-of-buffer)
  (forward-line -1)
  (forward-char 4)
  (kill-rectangle (point-min) (point))
  (beginning-of-buffer)
  (replace-regexp " +" "")
  (beginning-of-buffer)
  (flush-lines "^$")
  (kill-ring-save (point-min) (point-max))
  (beginning-of-buffer)
  (replace-regexp "\\(.+\\)" "let create_\\1_constructor () = \\1")
  (insert "\n\nlet register_")
  (let ((pos (point)))
    (insert "_callbacks () =\n")
    (yank)
    (goto-char (mark))
    (replace-regexp "\\(.+\\)" "  Callback.register 
    \"create_\\1_constructor\"
    create_\\1_constructor;")
    (insert "\n  ()\n\n")
    (goto-char pos)
    (widen)))
*)


(*** Local Variables: ***)
(*** compile-command : "ocamlc.opt -c cctypes.ml" ***)
(*** End: ***)