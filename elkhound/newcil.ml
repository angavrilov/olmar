(*


									     
		 Scott's C/C++ Parser 
 Construct a CFG from Scott's AST. 
*)
open Pretty

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
    line: int;				(* -1 means "do not know" *)
    col: int;
    file: string 
}

val locUnknown = { line = -1; col = -1; file = ""; }

(* information about a variable *)
type varinfo = { 
    vid: int;		(* unique integer indentifier, one per decl *)
    vname: string;				
    vglob: bool;	(* is this a global variable? *)
                        (* FIXME: currently always false *)
    vtype: typ; 			
    mutable vdecl: location;	(* where was this variable declared? *)
    mutable vattr: attribute list;
    mutable vstorage: storage;
} 

                                        (* Storage-class information *)
and storage = 
    NoStorage | Static | Register | Extern

(* information about a field access *)
and fieldinfo = { 
    fstruct: string;                    (* "CilLval *record"-> *)
    fname: string;                      (* "Variable *field"->name *)
    ftype: typ;
    mutable fattr: attribute list;
}

(* what is the type of an expression? *)
(* FIXME: currently no type information is available *)
and typ =
    TVoid * attribute list
  | TInt of ikind * attribute list
  | TBitfield of ikind * int * attribute list
  | TFloat of fkind * attribute list
  | Typedef of string * int * typ ref * attribute list
  | TPtr of typ * attribute list

              (* base type and length *)
  | TArray of typ * exp option * attribute list

               (* name, fields, id, attributes *)
  | TStruct of string * fieldinfo list * int * attribute list
  | TUnion of string * fieldinfo list * int * attribute list
  | TEnum of string * (string * int) list * int * attribute list
               (* result, args, isVarArg, attributes *)
  | TFunc of typ * typ list * bool * attribute list

(* kinds of integers *)
and ikind = 
    IChar | ISChar | IUChar
  | IInt | IUInt
  | IShort | IUShort
  | ILong | IULong
  | ILongLong | IULongLong

and fkind = 
    FFloat | FDouble | FLongDouble

and attribute = 
    AId of string                       (* Atomic attributes *)
  | ACons of string * exp list          (* Constructed attributes *)

(* literal constants *)
and constant =
    CInt of int * string option          (* Also the textual representation *)
  | CLInt of int * int * string option   (* LInt(l,h) = l +signed (h << 31) *)
  | CStr of string
  | CChr of char
  | CReal of float * string option       (* Also give the textual
                                        * representation *)
(* !!! int only holds 31 bits. We need long constants as well *)

(* unary operations *)
and unop =
    Neg                                 (* unary - *)
  | LNot                                (* ! *)
  | BNot                                (* ~ *)

(* binary operations *)
and binop =
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

                                        (* Comparison operations *)

(* expressions, no side effects *)
and exp =
    Const      of constant * location
  | Lval       of lval                  (* l-values *)
  | SizeOf     of typ * location
  | UnOp       of unop * exp * typ * location
  | BinOp      of binop * exp * exp * typ * location (* also have the type of
                                                      * the result *)
  | CastE      of typ * exp * location
  | AddrOf     of lval * location

  (* these next two are needed to handle initializers for globals (locals
   * could be handled by emitting code to do the initialization) and for
   * GNU extension expressions, which include literal aggregate values
   * (the "L" here stands for "literal") *)
  | LStruct of 
      typ *                               (* must be a TStruct *)
      ((fieldinfo * exp) list)            (* pairs of field,init; must be complete *)
  | LArray of 
      typ *                               (* must be a TArray *) 
      exp list                            (* list length == array length *)

(* L-Values *)
and lval =
  | Var        of varinfo * offset * location(* variable + offset *)
  | Mem        of exp * offset * location(* memory location + offset *)

and offset =
  | NoOffset
  | Field      of fieldinfo * offset    (* l.f + offset *)
  | Index      of exp * offset          (* l[e] + offset *)

(**** INSTRUCTIONS. May cause effects directly but may not have control flow.*)
and instr =
    Set        of lval * exp * location  (* An assignment. *)
  | Call       of varinfo option * exp * exp list * location
			 (* result temporary variable,
                            function, argument list, location *)

  | Asm        of string list *         (* templates (CR-separated) *)
                  bool *                (* if it is volatile *)
                  (string * varinfo) list * (* outputs must be variables with
                                             * constraints  *)
                  (string * exp) *      (* inputs with constraints *)
                  string list           (* clobbers *)

(**** STATEMENTS. Mostly structural information ****)
and stmt =
  | Skip                                (* empty statement *)
  | Sequence of stmt list
  | While of exp * stmt                 (* while loop *)
  | IfThenElse of exp * stmt * stmt     (* if *)
  | Label of string
  | Goto of string
  | Return of exp option
  | Switch of exp * stmt                (* no work done by scott, sigh*)
  | Case of int
  | Default
  | Break
  | Continue
  | Instruction of instr

type fundec =
    { sname: string;                    (* function name *)
      slocals: varinfo list;            (* locals *)
      smaxid: int;                      (* max local id. Starts at 0 *)
      sbody: stmt;                      (* the body *)
      stype: typ;                       (* the function type *)
      sstorage: storage;
      sattr: attributes list;
    }

type global = 
    GFun of fundec
  | GType of string * typ               (* A typedef *)
  | GVar of varinfo * exp option        (* A global variable with 
                                         * initializer. Includes function prototypes *)
  | GAsm of string                      (* Global asm statement *)
    
type file = global list
	(* global function decls, global variable decls *)




(* the name of the C function we call to get ccgr ASTs *)
external parse : string -> program = "cil_main"

(* 
  Pretty Printing
 *)

(* location *)
let d_loc () l =
  dprintf "/*(%s:%d:%d)*/" l.file l.line l.col

(* type *)
let rec d_type () t =
  match t with
    Void -> dprintf "void"
  | TInt -> dprintf "int"
  | TFloat -> dprintf "float"
  | TDouble -> dprintf "double"
  | TChar -> dprintf "char"
  | Typedef(s,t) -> dprintf "%s" s
  | TPtr(t) -> dprintf "%a *" d_type t
  | TArray(t) -> dprintf "%a []" d_type t
  | _ -> dprintf "unhandled_t" 
        
(* varinfo *)
let d_varinfo () v =
  dprintf "%s /* %d:%s:%a */ %a" v.vname v.vid 
    (if v.vglob then "global" else "local") d_type v.vtype 
    d_loc v.vdecl
    
let d_fieldinfo () f =
  dprintf "/* %s:%s:%a */" f.struct_name f.field_name d_type f.t
    
(* unop *)
let d_unop () u =
  match u with
    Neg -> text "-"
  | LNot -> text "!"
  | BNot -> text "~"
        
(* binop *)
let d_binop () b =
  match b with
    Plus -> text "+"
  | Minus -> text "-"
  | Mult -> text "*"
  | Div -> text "/"
  | Mod -> text "%"
  | Shiftlt -> text "<<"
  | Shiftrt -> text ">>"
  | Lt -> text "<"
  | Gt -> text ">"
  | Le -> text "<="
  | Ge -> text ">="
  | Eq -> text "=="
  | Ne -> text "!="
  | BAnd -> text "&"
  | BXor -> text "^"
  | BOr -> text "|"
  | LAnd -> text "&&"
  | LOr -> text "||"
        
(* constant *)
let d_const () c =
  match c with
    Int(i) -> dprintf "%d" i
  | Str(s) -> dprintf "\"%s\"" s
  | Chr(c) -> dprintf "'%c'" c
  | Real(f) -> dprintf "%f" f
        
(* exp *)
let rec d_exp () e =
  match e with
    Const(c,l) -> dprintf "%a %a" d_const c d_loc l
  | Lval(l) -> dprintf "%a" d_lval l
  | UnOp(u,e,l) -> dprintf "%a %a %a" d_unop u d_exp e d_loc l
  | BinOp(b,a,c,l) -> dprintf "%a@?%a@?%a %a" d_exp a d_binop b
	d_exp c d_loc l
  | CastE(t,e,l) -> dprintf "(%a) %a %a" d_type t d_exp e d_loc l
  | AddrOf(lv,lo) -> dprintf "& %a %a" d_lval lv d_loc lo
        
(* lvalue *)
and d_lval () l = 
  match l with
    Var(v,l) -> dprintf "%a %a" d_varinfo v d_loc l
  | Deref(e,l) -> dprintf "* %a %a" d_exp e d_loc l
  | Field(lv,fi,lo) -> dprintf "%a . %a %a" d_lval lv
	d_fieldinfo fi d_loc lo
  | CastL(t,e,l) -> dprintf "(%a) %a %a" d_type t d_exp e d_loc l
  | ArrayElt(a,i) -> dprintf "%a [ %a ]" d_exp a d_exp i
        
let rec d_instr () i =
  match i with
    Set(lv,e,lo) -> dprintf "%a = %a; %a@!" d_lval lv d_exp e d_loc lo
  | Call(l,e,args,loc) ->
      dprintf "%a = %a(%a); %a@!" d_lval l d_exp e 
	(docList (chr ',' ++ break) (d_exp ())) args
	d_loc loc
  | Asm(s) -> dprintf "__asm(%s);@!" s
        
let rec d_stmt () s =
  match s with
    Compound(lst) -> dprintf "{%a}" (docList (break) (d_stmt ())) lst
  | While(e,stmt) -> dprintf "while (%a) %a@!" d_exp e d_stmt stmt
  | IfThenElse(e,a,b) -> dprintf "if (%a) %a %a@!" d_exp e d_stmt a d_stmt b
  | Label(s) -> dprintf "%s:@!" s
  | Case(i) -> dprintf "case %d: " i
  | Jump(s) -> dprintf "goto %s;@!" s
  | Return(e) -> dprintf "return (%a);@!" d_exp e
  | Switch(e,s) -> dprintf "switch (%a) %a@!" d_exp e d_stmt s
  | Default -> dprintf "default:@!"
  | Instruction(i) -> d_instr () i
        
let d_fun_decl () (name,retval,stmt,formal,local) =
  dprintf "%a %s(%a)@!%a@!%a" d_type retval
    name (docList (chr ',' ++ break) (d_varinfo ())) formal
    (docList (break) (d_varinfo ())) local
    d_stmt stmt
    
let d_program () (fun_decl_list, global_list) = 
  dprintf "%a@!%a@!"
    (docList (break) (d_varinfo ())) global_list
    (docList (break) (d_fun_decl ())) fun_decl_list
    
