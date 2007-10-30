(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

let into_array max_node ast =
  let ast_array = Array.create (max_node +1) No_ast_node in
  let ast_order = Array.create (max_node +1) 0 in
  let order_index = ref 0 in
  let order_fun i =
    begin
      ast_order.(!order_index) <- i;
      incr order_index
    end
  in
  let visited_nodes = Dense_set.make ()
  in
    compilationUnit_type_into_array ast_array order_fun visited_nodes ast;
    assert(let res = ref true
	   in
	     for i = 0 to max_node do
	       if ast_array.(i) = No_ast_node then begin
		 Printf.eprintf "Superast.into_array: node id %d missing\n" i;
		 res := false
	       end
	     done;
	     !res);
    (ast_array, ast_order)

let iteri f ast_array =
  for i = 0 to (Array.length ast_array -1) do
    f i ast_array.(i)
  done

let load_marshaled_ast_array file =
  let (max_node, ast) = Elsa_oast_header.unmarshal_oast file
  in
    into_array max_node ast
