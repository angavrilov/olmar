
(* type declarations for cc.ast *)

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


(*** Local Variables: ***)
(*** compile-command : "ocamlc.opt -c cc_ml_types.ml" ***)
(*** End: ***)
