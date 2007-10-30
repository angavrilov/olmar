(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)


let create ast_array =
  let up = Array.create (Array.length ast_array) [] in
  let down = Array.create (Array.length ast_array) [] 
  in
    Superast.iteri (ast_node_fun up down) ast_array;
    (up, down)
