(*
 * CIL: An intermediate language for analyzing C progams.
 *
 * Version Tue Dec 12 15:21:52 PST 2000 
 * Scott McPeak, George Necula, Wes Weimer
 *
 * This version hacked up by Wes: this is what ocaml people see after Scott 
 * creates his C version.
 *
 * Note: you may *NOT* change the order of the fields or the order in
 * which disjoint union choices are presented. The C translation code
 * has those values hard-wired.
 *)

(* where did some construct originally appear in the source code? *)
type location = { 
	line: int;							(* -1 means "do not know" *)
	col: int;
	file: string 
}

(* information about a variable *)
type varinfo = { 
	vid: int;								(* unique integer indentifier, one per decl *)
	vname: string;				
	vglob: bool; 						(* is this a global variable? *)
												  (* FIXME: currently always false *)
	vtype: typ; 			
	vdecl: location;			  (* where was this variable declared? *)
} 

(* information about a field access *)
and fieldinfo = { 
	struct_name: string;	(* "CilLval *record"-> *)
	field_name: string;   (* "Variable *field"->name *)
	t: typ;
}

(* what is the type of an expression? *)
(* FIXME: currently no type information is available *)
and typ =
		Void
	| TInt
  | TFloat
  | TDouble
  | TChar
  | Typedef of string * typ ref
  | TPtr of typ  (* cv? *)
  | TArray of typ  (* len *)
  | TStruct of string * fieldinfo list
  | TUnion of string * fieldinfo list
(* missing: enum, functions *)

(* literal constants *)
type constant =
    Int of int
  | Str of string
  | Chr of char
  | Real of float

(* unary operations *)
type unop =
    Neg                                 (* unary - *)
  | LNot                                (* ! *)
  | BNot                                (* ~ *)

(* binary operations *)
type binop =
    Plus
  | Minus
  | Mult
  | Div
  | Mod

  | Shiftlt                             (* shift left *)
  | Shiftrt

  | Lt
  | Gt
  | Le
  | Ge

  | Eq
  | Ne

  | BAnd
  | BXor                                (* exclusive-or *)
  | BOr                                 (* inclusive-or *)

  | LAnd    (* don't need these it turns out *)
  | LOr
                                        (* Comparison operations *)

(* expressions, no side effects *)
type exp =
    Const      of constant * location
  | Lval       of lval                  (* l-values *)
  | UnOp       of unop * exp * location
  | BinOp      of binop * exp * exp * location
  | CastE      of typ * exp * location
  | AddrOf     of lval * location

(* L-Values *)
and lval =
  | Var        of varinfo * location     (* an identifier and some other info*)
  | Deref      of exp * location          (* *e *)
  | Field      of lval * fieldinfo * location(* e.f *)
  | CastL      of typ * exp * location
  | ArrayElt   of exp * exp             (* array, index *)

(* Instructions. May cause effects directly but may not have control flow. *)
type instr =
    Set        of lval * exp * location  (* An assignment *)
  | Call       of lval * exp * exp list * location
						 (* result, function, argument list, location *)

  | Asm        of string

and stmt = 
		Compound of stmt list 
	| While of exp * stmt 	(* while loop *)
	| IfThenElse of exp * stmt * stmt (* if *)
	| Label of string 
	| Jump of string
	| Return of exp
	| Switch of exp * stmt (* no work done by scott, sigh*)
	| Case of int 
	| Default 
	| Instruction of instr

type fun_decl = string * typ * stmt * (varinfo list) * (varinfo list)
	(* function name, return value type, body, formal args, local vars *)

type program = (fun_decl list) * (varinfo list)
	(* global function decls, global variable decls *)

val parse : string -> program list
