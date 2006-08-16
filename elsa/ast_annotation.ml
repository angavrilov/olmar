(* define the annotation type that gets inserted 
 * into every node of the ocaml ast
 *)

(* the first int is a unique number for identification, 
 * the second is the C++ address (shifted left to fit)
 *)
type annotated = int * int

(* accessor Functions *)

let id_annotation ((id,_) : annotated) = id

let caddr_annotation ((_, caddr) : annotated) = caddr

let addr_hash = Hashtbl.create 10093

let next_id = ref 1

let create_ast_annotation (c_addr : int) = 
  try
    let res = (Hashtbl.find addr_hash c_addr, c_addr)
    in
      (* currently we never ask a second time for an annotation *)
      assert false;
      res
  with
    | Not_found ->
	let id = !next_id
	in
	  Hashtbl.add addr_hash c_addr id;
	  incr next_id;
	  (id, c_addr)

let max_annotation() = !next_id -1

let register_ast_annotation_callbacks () =
  Callback.register "create_ast_annotation" create_ast_annotation;
  Callback.register "ocaml_max_annotation" max_annotation
  


let last_pseudo_id = ref 0

let pseudo_annotation () =
  incr last_pseudo_id;
  (- !last_pseudo_id, 0)
