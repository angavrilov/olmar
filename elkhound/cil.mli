

                                        (* An intermediate language for 
                                         * analyzing C programs  *)

type linecol =
    { line: int;
      col: int;
      file: string }

type varinfo =
    { vid: int;
      vname: string;
      vglob: bool;
      vtyp: typeinfo;
      vdecl: linecol;
    }

and fieldinfo =
    { fname: string;
      name: string;
      t: typ;
    }

and typ =
    TInt
  | TFloat
  | TDouble
  | TChar
  | Typedef of string * typ ref
  | TPtr of typ  (* cv? *)
  | TArray of typ  (* len *)
  | TStruct of string * fieldinfo list
  | TUnion of string * fieldinfo list
(* missing: enum, functions *)


type constant =
    Int of int
  | Str of string
  | Chr of char
  | Real of float

                                        (* Unary operators *)
type unop =
    Neg                                 (* unary - *)
  | BNot                                (* ~ *)
  | LNot                                (* ! *)


                                        (* Other binary operations *)
and binop =
    Plus
  | Minus
  | Mult
  | Div
  | Mod

  | BAnd
  | BOr                                 (* inclusive-or *)
  | BXor                                 (* exclusive-or *)

  | Shiftlt                             (* shift left *)
  | Shiftrt

  | LAnd    (* don't need these it turns out *)
  | LOr
                                        (* Comparison operations *)
  | Eq
  | Ne
  | Gt
  | Lt
  | Ge
  | Le


                                        (* Expressions. No side-effects ! *)
type exp =
    Const      of constant * linecol
  | Lval       of lval                  (* l-values *)
  | UnOp       of unop * exp * linecol
  | BinOp      of binop * exp * exp * linecol
  | CastE      of typeinfo * exp * linecol
  | AddrOf     of lval * linecol

                                        (* L-values *)
and lval =
  | Var        of varinfo * linecol     (* an identifier and some other info*)
  | Deref      of exp * linecol          (* *e *)
  | Field      of lval * fieldinfo * linecol(* e.f *)
  | CastL      of typeinfo * exp * linecol
  | ArrayElt   of exp * exp             (* array, index *)

                                        (* Instructions. Can have side-effects
                                         * but no control flow *)
and instr =
    Set        of lval * exp * linecol  (* An assignment *)
                                        (* result, function address, argument
                                         * list *)
  | Call       of lval * exp * exp list * linecol

  | Asm        of string

and successor =
    Return of exp * linecol
  | If of exp * bblock * bblock * bool(*loop hint*) * linecol
  | Switch of exp * (constant * bblock) list * bblock(*default*) * linecol
  | Jump of bblock ref * linecol    (*leaving?*)


and bblock =
    {          id:     int;            (* An identifier. Starts at 0 and
                                         * grows breadth-first through If and
                                         * Switch  *)
              ins:     instr list;
              succ:    successor;
      mutable nrpred:  int
    }

and program =
   list of top_level_declations

top_level_declarations =
   Function of string * typeinfo * bblock * (locals list, push local initializers into body)
 | Global of string * typeinfo * whatever * initializer * ...
