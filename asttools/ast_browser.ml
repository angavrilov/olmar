(* A primitive GUI tree browser. Uses LablGTK2

   The AST, which is actually a cyclic graph, is
   here represented as an infinite tree, constructed
   on demand when it's nodes are expanded. Thus, it's
   extremely unwise to call expand_all.
  *)

open Gobject.Data
open Cc_ml_types
open Ml_ctype
open Cc_ast_gen_type
open Ast_meta

let _ = GMain.Main.init ()

(* Load the AST *)
let tree = load_node_array Sys.argv.(1)

(* Construct the store *)
let cols = new GTree.column_list
let col_text = cols#add string
let col_id = cols#add int

let tree_store = GTree.tree_store cols

(* Construct a tree node corresponding to an AST node *)
let bind_ast_node node field ast =
    let name, _childs = node_meta ast in
    let id = NodeArray.node_id ast in
    tree_store#set ~row:node ~column:col_text (field^": "^name^" #"^(string_of_int id));
    tree_store#set ~row:node ~column:col_id (NodeArray.node_id ast)

(* Compute a string representation for a POD field *)
let rec eval_str_value = function
  | NodeField _ 
  | OptNodeField _
  | NodeListField _
  | NodeListListField _
  | OptNodeListField _
  | AnnotField _
  | StringNodeTableField _ -> assert false (* not POD *)
  | OptField None -> "(unset)"
  | OptField (Some x) -> eval_str_value x
  | ListField [] -> "(empty)"
  | ListField (h::l') ->
    List.fold_left (fun s e -> s ^ ", " ^ (eval_str_value e)) (eval_str_value h) l'
  | IntField iv -> string_of_int iv
  | Int32Field iv -> Int32.to_string iv
  | FloatField fv -> string_of_float fv
  | NativeintField iv -> Nativeint.to_string iv
  | StringField s -> "'"^s^"'"
  | SourceLocField (f, l, r) -> "("^f^", "^(string_of_int l)^", "^(string_of_int r)^")"
  | BoolField true -> "true"
  | BoolField false -> "false"
  | DeclFlagField df -> string_of_declFlags df
  | STypeIDField st -> string_of_simpleTypeId st
  | TypeIntrField ti -> string_of_typeIntr ti
  | AccessKwdField ak -> string_of_accessKeyword ak
  | CVFlagsField cf -> string_of_cVFlags cf
  | OverldOpField op -> string_of_overloadableOp op
  | UnaryOpField op -> string_of_unaryOp op
  | EffectOpField op -> string_of_effectOp op
  | BinaryOpField op -> string_of_binaryOp op
  | CastKwdField ck -> string_of_castKeyword ck
  | FunFlagsField ff -> string_of_function_flags ff
  | DeclCtxField cx -> string_of_declaratorContext cx
  | SkopeKindField ck -> string_of_scopeKind ck
  | ArraySizeField NO_SIZE -> "NO"
  | ArraySizeField DYN_SIZE -> "DYN"
  | ArraySizeField (FIXED_SIZE sz) -> string_of_int sz
  | CompoundTypeField K_STRUCT -> "struct"
  | CompoundTypeField K_CLASS -> "class"
  | CompoundTypeField K_UNION -> "union"
  | TemplKindField tk -> string_of_templateThingKind tk

(* Pick first n nodes of a list *)
let rec pick_head n l =
  if n <= 0 then [], l
  else 
  match l with
  | [] -> [], []
  | h :: l' ->
    let l'', tail = pick_head (n-1) l' in
    (h :: l''), tail

(* Spawn children for the node - implements on-demand construction *)
let populate_node node =
    if tree_store#iter_has_child node then () (* already done *)
    else
    let id = tree_store#get ~row:node ~column:col_id in
    if id < 0 then ()                         (* not AST, i.e. a POD node *)
    else
    let ast = NodeArray.get tree id in
    let _name, childs =
        match ast with 
        (* CompoundType nodes do not have a separate ID *)
        | AtomicType (CompoundType ct) -> 
            node_meta (Compound_info ct)
        | _ -> 
            node_meta ast
    in
    List.iter (fun (nm, f) -> 
        let node2 = tree_store#append ~parent:node () in
        tree_store#set ~row:node2 ~column:col_id (-1);
        match f with 
        | AnnotField _ -> 
            ignore(tree_store#remove node2)
        | NodeField n
        | OptNodeField (Some n) ->
            bind_ast_node node2 nm n
        | OptNodeField (None) ->
            tree_store#set ~row:node2 ~column:col_text (nm^": (null)")
        | NodeListField nl 
        | OptNodeListField (Some nl) ->
            tree_store#set ~row:node2 ~column:col_text (nm^": (list)");
            List.iter (fun n -> bind_ast_node (tree_store#append ~parent:node2 ()) "*" n) nl
        | OptNodeListField (None) ->
            tree_store#set ~row:node2 ~column:col_text (nm^": (null list)")
        | NodeListListField nll ->
            tree_store#set ~row:node2 ~column:col_text (nm^": (list list)");
            List.iter (fun nl -> 
                let node3 = tree_store#append ~parent:node2 () in
                tree_store#set ~row:node3 ~column:col_id (-1);
                tree_store#set ~row:node3 ~column:col_text ("*: (list)");
                List.iter (fun n -> bind_ast_node (tree_store#append ~parent:node3 ()) "*" n) nl
            ) nll;
        | StringNodeTableField nl ->
            (* Tables are used in scopes and can be very big, so trim them if too long *)
            let nl', tail = pick_head 100 nl in
            tree_store#set ~row:node2 ~column:col_text (nm^": (table)");
            List.iter (fun (s,n) -> bind_ast_node (tree_store#append ~parent:node2 ()) s n) nl';
            if tail != [] then 
             begin
                let tnode = tree_store#append ~parent:node2 () in
                tree_store#set ~row:tnode ~column:col_id (-1);
                tree_store#set ~row:tnode ~column:col_text ("... (skipping "^(string_of_int (List.length tail))^")")
             end
        | _ ->
            tree_store#set ~row:node2 ~column:col_text (nm^": "^(eval_str_value f));
    ) childs

let inproc = ref false
let update_node iter _path =
    if !inproc then () (* Just in case - prevent recursion *)
    else
    if tree_store#iter_has_child iter then (
    inproc := true;
    for i = 0 to tree_store#iter_n_children (Some iter) - 1 do    
      let ci = tree_store#iter_children ~nth:i (Some iter) in
      populate_node ci
    done;
    inproc := false;
    )

(* Main GUI code *)
let create_view_and_model ~packing () =
  let scrolled_window = GBin.scrolled_window ~packing
    ~width:500 ~height:500
    ~hpolicy:`AUTOMATIC ~vpolicy:`AUTOMATIC () in
  let view = GTree.view ~packing:(scrolled_window#add_with_viewport) () in

  let col = GTree.view_column ~title:"Tree"
      ~renderer:(GTree.cell_renderer_text [], ["text", col_text]) () in
  ignore (view#append_column col);

  view#set_model (Some (tree_store#coerce));

  view#collapse_all ();
  view#selection#set_mode `BROWSE;
  ignore (view#connect#row_expanded ~callback:update_node); (* Invoke on-demand construction *)
  view

let main () =
  let window = GWindow.window ~title:("AST Browser: "^Sys.argv.(1)) ~border_width:1 () in
  ignore (window#connect#destroy ~callback:GMain.Main.quit);
  let root_tln = tree_store#append () in
  bind_ast_node root_tln "ROOT" (NodeArray.get_root tree);
  populate_node root_tln;
  let _view = create_view_and_model ~packing:window#add () in
  window#show ();
  GMain.Main.main ()

let _ = Printexc.print main ()
