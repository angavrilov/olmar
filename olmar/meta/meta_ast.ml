(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open More_string
open Ast_annotation
open Ast_ml_types
open Ast_reflect_type
open Ast_config
	  

(******************************************************************************
 ******************************************************************************
 *
 * ml_ast type declaration
 *
 ******************************************************************************
 ******************************************************************************)

(* kinds of ast lists *)
type list_kind =
  | LK_ast_list
  | LK_fake_list

(* type of fields and constructor arguments *)
type ast_type =
  | AT_base of string
  | AT_node of ast_class
  | AT_ref of ast_type
  | AT_option of ast_type
      (* AT_list( ast or fake list, inner kind, inner c-type string ) *)
  | AT_list of list_kind * ast_type * string

(* a field or constructor argument *)
and ast_field = {
  af_name : string;
  af_modifiers : fieldFlag list;
  af_type : ast_type;
  af_mod_type : ast_type;
  af_is_pointer : bool;
  af_is_base_field : bool;
}

(* a superclass or a subclass *)
and ast_class = {
  ac_id : int;
  ac_name : string;
  mutable ac_args : ast_field list;
  mutable ac_last_args : ast_field list;
  mutable ac_fields : ast_field list;
  ac_super : ast_class option;
  mutable ac_subclasses : ast_class list;
}


(******************************************************************************
 ******************************************************************************
 *
 * some astgen parsing functions
 *
 ******************************************************************************
 ******************************************************************************)

(* parse strings in fieldFlag list
 *
 * recognized access modifiers (see also ASTClass::init_fields in ast.ast):
 * field -> FF_FIELD
 * xml.* -> FF_XML
 * owner -> FF_IS_OWNER
 * nullable -> FF_NULLABLE
 * circular -> FF_CIRCULAR
 *
 * Other modifiers are silently droped on the floor (at least here)
 *)
let fieldFlag_of_string s =
  if String.length s >= 1
  then
    match s.[0] with
      | 'o' -> if s = "owner" then Some(FF_IS_OWNER) else None
      | 'n' -> if s = "nullable" then Some(FF_NULLABLE) else None
      | 'f' -> if s = "field" then Some(FF_FIELD) else None
      | 'c' -> if s = "circular" then Some(FF_CIRCULAR) else None
      | 'x' -> 
	  if String.length s >= 3 
	    && String.sub s 0 3 = "xml" 
	  then Some(FF_XML)
	  else None
      | _ -> 
	  None
  else
    None

let fieldFlags_of_strings =
  List.fold_left
    (fun res s -> match fieldFlag_of_string s with
       | None -> res
       | Some flag -> flag :: res)
    []


(* split a field declaration into identifier and type string *)
let split_field_type_name decl_str = 
  let decl_str = trim_white_space decl_str in
  (* take them from astgen.cc function parseFieldDecl *)
  let delimiter_chars = " \t*()[]<>," in
  let rindices = ref [] in
  let _ = 
    for i = 0 to String.length delimiter_chars -1 do
      try
	rindices := (String.rindex decl_str delimiter_chars.[i]) :: !rindices
      with
	| Not_found -> ()
    done
  in
  let ofs = match List.sort (fun x y -> compare y x) !rindices with
    | [] -> 0
    | ofs :: _ -> ofs
  in
    ((trim_white_space (String.sub decl_str 0 (ofs +1))),
     (trim_white_space 
	(String.sub decl_str (ofs +1) (String.length decl_str - ofs -1))))


let is_pointer_type type_string =
  type_string.[String.length type_string -1] = '*'


(* only use this on a non-mod type without ref or option *)
let field_is_pointer ml_type type_string =
  match ml_type with
    | AT_list _
    | AT_base _ ->
	if is_pointer_type type_string then true else false
    | AT_node _ -> true

    | AT_ref _
    | AT_option _ -> assert false

(******************************************************************************
 ******************************************************************************
 *
 * node hash, is_ast_node
 *
 ******************************************************************************
 ******************************************************************************)

let ast_node_hash : (string, ast_class) Hashtbl.t = Hashtbl.create 1201

let is_node_type node =
  Hashtbl.mem ast_node_hash node

let get_node name =
  Hashtbl.find ast_node_hash name

(******************************************************************************
 ******************************************************************************
 *
 * astgen to ml_ast translation
 *
 ******************************************************************************
 ******************************************************************************)

let iter_oast_nodes f (ast_file : 'a aSTSpecFile_type) = 
  List.iter
    (function 
       | TF_class(_annot, super, sublist) ->
	   f super sublist
       | TF_verbatim _
       | TF_impl_verbatim _
       | TF_ocaml_type_verbatim _
       | TF_xml_verbatim _
       | TF_option _
       | TF_custom _
       | TF_enum _ -> ())
    ast_file.forms


let f_id x = x

let comp f g x = g(f(x))


let extract_pointer_type type_string =
  if type_string.[String.length type_string -1] = '*'
  then
    (true,
     trim_white_space 
       (String.sub type_string 0 (String.length type_string -1)))
  else
    (false, type_string)
     

let fake_list_el_transform type_string =
  let (is_pointer_type, type_string_ptr) = extract_pointer_type type_string
  in
    if not is_pointer_type then begin
      prerr_endline "Non-pointer FakeList";
      assert false
    end;
    type_string_ptr


(* Abstraction data structure for all possible list types.
 * First element is the name of the list.
 * Second element is a function taking a string and an ast_type.
 *   The string is passed immediately, it is the C++ type string of 
 *   the element type (needed for C++ code generation).
 *   Then the inner type is converted and once completed passed as 
 *   a second element, to form the complete type. The second argument 
 *   makes nested lists possible.
 * Third element is transformation/check applied to the inner element 
 * type. It is used to cope with the following difference:
 * Fakelists are always pointers while the other lists are not.
 *)
let ast_lists = 
  [| 
    ("ASTList", (fun s -> fun t -> AT_list(LK_ast_list, t, s)),
       f_id);
    ("FakeList", (fun s -> fun t -> AT_list(LK_fake_list, t, s)),
       fake_list_el_transform);
  |]

let ast_list_name (n,_,_) = n
let ast_list_build (_,b,_) = b
let ast_list_trans (_,_,t) = t


let rec extract_list_type type_string =
  let result = ref None in
  let index = ref 0 
  in
    while !index < Array.length ast_lists && !result = None do
      if string_match_left type_string (ast_list_name ast_lists.(!index))
      then begin
	let list_type_len = String.length (ast_list_name ast_lists.(!index)) in
	let el_with_angles =
	  (ast_list_trans ast_lists.(!index))
	    (trim_white_space (String.sub type_string list_type_len
				 (String.length type_string - list_type_len)))
	in
	let _ = 
	  assert(el_with_angles.[0] = '<' 
	      && el_with_angles.[String.length el_with_angles -1] = '>') 
	in
	let el_type = trim_white_space (String.sub el_with_angles 1
					  (String.length el_with_angles -2))
	in
	  result := 
	    Some( ((ast_list_build ast_lists.(!index)) el_type, el_type) )
      end;
      incr index
    done;
    match !result with
      | Some (f, el_type) -> 
	  let (g, el_el_type) = extract_list_type el_type
	  in
	    (comp f g, el_el_type)
      | None -> (f_id, type_string)


let make_ast_type type_string_in =
  let (list_fun, type_string_list_el) = extract_list_type type_string_in in
  let (is_pointer_type, type_string_ptr) = 
    extract_pointer_type type_string_list_el 
  in
  let simple_type =
    try 
      (* ignore node pointer types *)
      AT_node(get_node type_string_ptr)
    with
      | Not_found ->
	  if is_basic_type type_string_ptr then
	    if is_pointer_type 
	    then 
	      begin
		Printf.eprintf "Pointer to base type %s\n" type_string_ptr;
		assert false
	      end
	    else
	      (* non pointer base type: ok *)
	      ()
	  else
	    Printf.eprintf "Warning: unrecognized type %s\n" 
	      type_string_ptr;
	  AT_base type_string_ptr
  in
    list_fun simple_type


(* call this only with a basic, non-modified ast_type *)
let is_list_type = function
  | AT_base _ -> false
  | AT_node _ -> false
  | AT_list _ -> true

  | AT_ref _
  | AT_option _ -> assert false


let make_mod_type ast_type modifiers =
  match (is_list_type ast_type,
	 List.mem FF_NULLABLE modifiers, 
	 List.mem FF_CIRCULAR modifiers)
  with
        (* list, nullable, circular *)
        (* list, _ *)
      | (_, false, false) -> ast_type
        (* nullable object *)
      | (false, true, false) -> AT_option ast_type
        (* circular object *)
        (* circular nullable object *)
      | (false, _, true) -> AT_ref(AT_option ast_type)
        (* nullable list *)
      | (true, true, false) -> assert false
	(* circular list *)
      | (true, false, true) -> AT_ref ast_type
	(* circular nullable list *)
      | (true, true, true) -> assert false


(* only use this on a non-mod type without ref or option *)
let rec is_base_field = function
  | AT_base _ -> true
  | AT_node _ -> false
  | AT_list(_, inner, _) -> is_base_field inner

  | AT_ref _
  | AT_option _ -> assert false


let type_modifier_p = function
  | FF_NULLABLE
  | FF_CIRCULAR -> true
  | FF_IS_OWNER
  | FF_FIELD   
  | FF_XML     
  | FF_PRIVAT  -> false


let extract_fields cl =
  let res = ref [] in
  let extract_field = function
    | UserDecl(_annot, access_mod, code, _init) ->
	let modifiers = fieldFlags_of_strings access_mod.mods
	in
	  if List.mem FF_FIELD modifiers or List.mem FF_XML modifiers
	  then
	    let modifiers =
	      if access_mod.acc = AC_PRIVATE 
	      then FF_PRIVAT :: modifiers
	      else modifiers
	    in
	    let (field_type, field_name) = split_field_type_name code in
	    let typ = make_ast_type field_type in
	    let field =
	      { af_name = field_name;
		af_modifiers = modifiers;
		af_type = typ;
		af_mod_type = make_mod_type typ modifiers;
		af_is_pointer = field_is_pointer typ field_type;
		af_is_base_field = is_base_field typ;
	      }
	    in
	      res := field :: !res
    | CustomCode _ -> ()
  in
    List.iter extract_field cl.decls;
    List.rev !res


let make_ast_arg arg =
  let t = make_ast_type arg.field_type
  in
    { af_name = arg.field_name;
      af_modifiers = arg.flags;
      af_type = t;
      af_mod_type = make_mod_type t arg.flags;
      af_is_pointer = field_is_pointer t arg.field_type;
      af_is_base_field = is_base_field t;
    }


let update_fields cl =
  let ast_cl = get_node cl.cl_name 
  in
    ast_cl.ac_args <- List.map make_ast_arg cl.args;
    ast_cl.ac_last_args <- List.map make_ast_arg cl.lastArgs;
    ast_cl.ac_fields <- extract_fields cl


let make_ast_class super cl =
  { ac_id = id_annotation cl.aSTClass_annotation;
    ac_name = cl.cl_name;
    ac_args = [];
    ac_last_args = [];
    ac_fields = [];
    ac_super = super;
    ac_subclasses = [];
  }

let ml_ast_of_oast oast =
  let ml_ast = ref []
  in
    iter_oast_nodes
      (fun super sublist ->
	 let super = make_ast_class None super in
	 let sublist = List.map (make_ast_class (Some super)) sublist 
	 in
	   super.ac_subclasses <- sublist;
	   Hashtbl.add ast_node_hash super.ac_name super;
	   List.iter
	     (fun sub -> Hashtbl.add ast_node_hash sub.ac_name sub)
	     sublist;
	   ml_ast := super :: !ml_ast)
      oast;
    iter_oast_nodes
      (fun super sublist -> List.iter update_fields (super::sublist))
      oast;
    List.rev !ml_ast


(*****************************************************************************
 *****************************************************************************
 *
 * ml ast utilities
 *
 *****************************************************************************
 *****************************************************************************)

(* name of the annotation field *)
let annotation_field_name cl =
  (String.uncapitalize
     (translate_olmar_name (Some cl.ac_name) cl.ac_name))
  ^ "_annotation"


let annotation_access_fun cl =
  let super = match cl.ac_super with
    | None -> cl
    | Some super -> super
  in
    (String.uncapitalize (translate_olmar_name None super.ac_name)) 
    ^ "_annotation"


let source_loc_access_fun cl =
  let super = match cl.ac_super with
    | None -> cl
    | Some super -> super
  in
    (String.uncapitalize (translate_olmar_name None super.ac_name)) 
    ^ "_loc"

let source_loc_meta_fun = "super_source_loc"

let translated_class_name cl = match cl.ac_super with
  | None -> translate_olmar_name None cl.ac_name
  | Some super -> translate_olmar_name (Some super.ac_name) cl.ac_name

(* name of the variant constructor for a subclass *)
let variant_name sub =
  match sub.ac_super with
    | None -> assert false
    | Some _ -> String.capitalize(translated_class_name sub)


let node_ml_type_name cl =
  (String.uncapitalize (translate_olmar_name None cl.ac_name)) ^ "_type"

let translated_field_name cl field =
  translate_olmar_name (Some cl.ac_name) field.af_name


let superast_constructor cl =
  String.capitalize (node_ml_type_name cl)

let name_of_superast_no_ast = "No_ast_node"


let get_all_fields cl =
  match cl.ac_super with
    | None -> [cl.ac_args; cl.ac_last_args; cl.ac_fields]
    | Some super ->
	assert(cl.ac_subclasses = []);
	assert(cl.ac_last_args = []);
	[super.ac_args; super.ac_fields; cl.ac_args; cl.ac_fields; 
	 super.ac_last_args]
	


let get_all_fields_flat cl = List.flatten (get_all_fields cl)
  

let count_fields ll =
  List.fold_left (fun sum l -> sum + List.length l) 0 ll



let get_source_loc_field cl =
  let res = ref None
  in
    List.iter 
      (List.iter
	 (fun f -> 
	    if f.af_mod_type = AT_base "SourceLoc"
	    then
	      res := Some f))
      (get_all_fields cl);
    !res


(* string_of_ast_type with_tick_a type 
 * makes node types polymorphic with 'a if with_tick_a
 *)
let rec string_of_ast_type with_tick_a = function
  | AT_list(_, el_type, _) -> (string_of_ast_type with_tick_a el_type) ^ " list"
  | AT_ref el_type -> (string_of_ast_type with_tick_a el_type) ^ " ref"
  | AT_option el_type -> (string_of_ast_type with_tick_a el_type) ^ " option"
  | AT_node cl -> 
      (match cl.ac_super with
	 | None ->
	     if with_tick_a then
	       "'a " ^ (node_ml_type_name cl)
	     else
	       node_ml_type_name cl
	 | Some super ->
	     Printf.sprintf "%s%s (* = %s *)"
	       (if with_tick_a then "'a " else "")
	       (node_ml_type_name super)
	       (variant_name cl)
      )
  | AT_base type_name -> 
      (translate_olmar_name
	 None
	 (String.uncapitalize type_name))



(*****************************************************************************
 *****************************************************************************
 *
 * arguments and input
 *
 *****************************************************************************
 *****************************************************************************)

let translation_file = ref None

let arguments = 
  [
    ("-tr", Arg.String (fun s -> translation_file := Some s),
     "file use identifier translations in file");
  ]


let setup_oast_translation other_args app_name =
  let (oast_file, size, ast) =
    Ast_oast_header.setup_oast (arguments @ other_args) app_name
  in
    (match !translation_file with
       | None -> ()
       | Some tr_file -> parse_config_file tr_file
    );
    (oast_file, size, ast)


let setup_ml_ast other_args app_name =
  let (oast_file, _size, ast) = setup_oast_translation other_args app_name
  in
    (oast_file, ml_ast_of_oast ast)
