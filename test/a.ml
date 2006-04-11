(* a.ml *)
(* *** DO NOT EDIT *** *)
(* generated automatically by astgen, from a.ast *)

type a_type = 
  | A_B of int * b_type 
  | A_C of int * c_type list 

and b_type = int * string 

and c_type = 
  | C_A of int 
  | C_B of string 


(*** Local Variables: ***)
(*** compile-command : "ocamlc.opt -c a.ml" ***)
(*** End: ***)
