
(* 
 * make List.rev accessible from the C side 
 * 
 * not much code, but it has to go somewhere
 *)

let register_ast_util_callbacks () =
  Callback.register "List.rev" List.rev
