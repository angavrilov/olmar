

                                        (* An intermediate language for 
                                         * analyzing C programs  *)

type constant =
    Int of int
  | Str of identifier
  | Chr of identifier
  | Real of identifier
  
                                        (* Unary operators *)
type unop = 
    Neg                                 (* unary - *)
  | Not                                 (* ! *)
  | BitNot                              (* ~ *)

                                        (* Commutative binary operators *)
type combinop = 
    Plus
  | Mult
  | And
  | Ior                                 (* inclusive-or *)
  | Xor                                 (* exclusive-or *)

                                        (* Other binary operations *)
and binop = 
    Minus
  | Div
  | Udiv                                (* unsigned division *)
  | Mod
  | Umod                                (* unsigned modulo *)

  | Ashift
  | Lshiftrt
  | Ashiftrt
  | Rotate
  | Rotatert
                                        (* Comparison operations *)
  | Eq
  | Ne
  | Gt
  | Gtu
  | Lt
  | Ltu
  | Ge
  | Geu
  | Le
  | Leu


                                        (* Expressions. No side-effects ! *)
type exp =
    Const      of constant
  | Lval       of lval                  (* l-values *)
  | UnOp       of unop * exp
  | BinOp      of binop * exp * exp
  | ComBinOp   of combinop * exp * exp
  | CastE      of typeinfo * exp
  | AddrOf     of lval                  (* & e *)

                                        (* L-values *)
and lval =
    Global     of string
  | Local      of int * varinfo         (* an indentifier and some other info*)
  | Deref      of exp                   (* *e *)
  | Field      of exp * fieldinfo       (* e.f *)
  | CastL      of typeinfo * lval
  | ArrayElt   of exp * exp             (* array, index *)

                                        (* Instructions. Can have side-effects
                                         * but no control flow *)
and instr = 
    Set        of lval * exp            (* An assignment *)
                                        (* result, function address, argument 
                                         * list *)
  | Call       of lval * exp * exp list
                  (* Represents one output of an ASM. The template, the 
                   * output operand constraint, the index of the output 
                   * operand and the inputs *)
  | Asm        of string * string * (exp * string) list
  
and 'a bblock = 
    { id:      int;                     (* An identifier *)
      ins:     instr list;
      succ:    'a successor;
      info:    'a;                      (* Some custom information *)
    } 

    
