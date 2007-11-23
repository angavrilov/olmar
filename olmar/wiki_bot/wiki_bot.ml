(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Stdlib_missing
open More_string
open Wiki_http
open Ast_config
open Meta_ast

let overview_page_name = "Elsa_ast_nodes"

let overview_separation_line =
  "<!-- OLMAR SEPARATION LINE: ADD YOUR MODIFICATIONS OBOVE! -->"

let overview_node_list_start_line =
  "<!-- easy to parse list of all node types"

let renamings_start_line = "Renamings: "

let wiki_page_prefix = "wiki page name prefix: "

let overview_node_list_end_line =
  "end of node type list -->"

let node_page_separation_line =
  "<!-- OLMAR SEPARATION LINE: ADD YOUR MODIFICATIONS BELOW! -->"

let ignore_create_filled_pages = ref false

(******************************************************************************
 ******************************************************************************
 *
 * separate nodes
 *
 ******************************************************************************
 ******************************************************************************)

type node_to_do =
  | New_node
  | Update_node
  | Delete_node
  | Renamed_node


let extract_intro_and_old_nodes overview_page =
  (* let _ = prerr_endline "extr start" in *)
  let res = ref [] in
  let renamings = ref [] in
  let prefix = ref "" in
  let lines = ref(split '\n' overview_page) in
  let intro_lines = ref []
  in
    while !lines <> [] && List.hd !lines <> overview_separation_line do
      intro_lines := (List.hd !lines) :: !intro_lines;
      lines := List.tl !lines
    done;
    while !lines <> [] && List.hd !lines <> overview_node_list_start_line do
      lines := List.tl !lines
    done;
    if !lines = [] or List.tl !lines = [] then begin
      prerr_endline 
	"Cannot find node list start line in the overview page!\nExiting.";
      exit 2;
    end;
    lines := List.tl !lines;
    if string_match_left (List.hd !lines) renamings_start_line then
      renamings := 
	List.filter (fun s -> s <> "")
	  (split ' '
	     (String.sub (List.hd !lines) 
		(String.length renamings_start_line)
		((String.length (List.hd !lines)) - 
		   (String.length renamings_start_line))))
    else begin
      prerr_endline "Renaming line missing";
      exit 1
    end;
    lines := List.tl !lines;
    if string_match_left (List.hd !lines) wiki_page_prefix then 
      begin
	prefix := 
	  String.sub (List.hd !lines) 
	    (String.length wiki_page_prefix)
	    (String.length (List.hd !lines) - (String.length wiki_page_prefix))
      end
    else
      begin
	prerr_endline "Page prefix line missing";
	exit 1
      end;
    lines := List.tl !lines;
    while !lines <> [] && List.hd !lines <> overview_node_list_end_line do
      res := 
	List.rev_append
	  (List.filter
	     (fun s -> s <> "")
	     (split ' ' (List.hd !lines)))
	  !res;
      lines := List.tl !lines;
    done;
    if !lines = [] then
      prerr_endline "Node list end line missing";
    (* 
     * print_endline "old nodes found:";
     * List.iter 
     *   (fun n -> print_string "  "; print_endline n)
     *   !res;
     *)
    (String.concat "\n" (List.rev !intro_lines),
     !prefix, !res, !renamings)


let process_overview_page ast overview_page =
  (* let _ = prerr_endline "sep start" in *)
  let h = Hashtbl.create 383 in
  let new_renamings = ref [] in
  let node_changes = ref [] in
  let (intro, prefix, old_nodes, old_renamings) =
    extract_intro_and_old_nodes overview_page 
  (* in let _ = Printf.eprintf "got %s old nodes\n%!"  *)
  in
    List.iter
      (fun super ->
	 List.iter 
	   (fun cl -> 
	      let cl_name = translated_class_name cl
	      in
		Hashtbl.add h cl_name New_node;
		if cl_name <> cl.ac_name then begin
		  Hashtbl.add h cl.ac_name Renamed_node;
		  new_renamings := cl.ac_name :: !new_renamings;
		end
	   )
	   (super::super.ac_subclasses))
      ast;
    List.iter
      (fun old ->
	 try
	   match Hashtbl.find h old with
	     | New_node -> Hashtbl.replace h old Update_node
	     | Renamed_node -> ()
	     | Update_node ->
		 Printf.printf "Warning: found old node %s twice\n%!" old;
		 Hashtbl.replace h old Update_node
	     | Delete_node -> assert false
	 with
	   | Not_found -> Hashtbl.add h old Delete_node)
      old_nodes;
    List.iter
      (fun rename ->
	 try
	   match Hashtbl.find h rename with
	     | Renamed_node -> ()
	     | New_node ->
		 Printf.printf "Warning: update renaming node %s\n%!" rename;
		 Hashtbl.replace h rename Update_node
	     | Update_node 
	     | Delete_node -> 
		 Printf.eprintf 
		   "Error: old renaming %s set to update/delete. Ignore.\n%!"
		   rename
	 with
	   | Not_found -> Hashtbl.replace h rename Delete_node)
      old_renamings;
    Hashtbl.iter
      (fun node code -> node_changes := (node, code) :: !node_changes)
      h;
    (intro, prefix, !node_changes, !new_renamings)



(******************************************************************************
 ******************************************************************************
 *
 * create content
 *
 ******************************************************************************
 ******************************************************************************)

let class_type_link name =
  Printf.sprintf "[[Elsa Ast Type %s|%s]]" name name

let print_sub_node_list 
    (pf : ('a, Buffer.t, unit) format -> 'a) item_start subs item_end =
  let sub_names = List.sort compare (List.map translated_class_name subs)
  in
    List.iter
      (fun sub_name ->
	 pf (format_of_string "%s%s%s") item_start
	   (class_type_link sub_name)
	   item_end)
      sub_names


let rec print_type (pf : ('a, Buffer.t, unit) format -> 'a) pr = function
  | AT_list(LK_string_ref_map, el_type, _) ->
      pr "(string, ";
      print_type pf pr el_type;
      pr ") Hashtbl.t";
  | AT_list(_, el_type, _) -> 
      print_type pf pr el_type;
      pr " list";
  | AT_option(el_type, _) -> 
      print_type pf pr el_type;
      pr " option";
  | AT_ref ref_type -> 
      print_type pf pr ref_type;
      pr " ref";
  | AT_node cl -> 
      if cl.ac_super = None or cl.ac_record
      then
	pr (class_type_link (translated_class_name cl))
      else
	pf "%s (* = %s *)"
	  (class_type_link (translated_class_name (the cl.ac_super)))
	  (class_type_link (translated_class_name cl))

  | AT_base type_name -> 
      pr (translate_olmar_name None (String.uncapitalize type_name))


let generate_node_page node_name comments =
  let cl = 
    let (none, orig) = translate_back None node_name
    in
      assert(none = None);
      get_node orig
  in
  let cl_name = assert(translated_class_name cl = node_name); node_name in
  let cl_renamed = cl.ac_name <> cl_name in
  let all_fields = get_all_fields cl in
  let is_record = ref false in
  let buf = Buffer.create 8191 in
  let pf format = Printf.bprintf buf format in
  let pr = Buffer.add_string buf
  in
    pr "<!-- Besides the comments at the end this page has been generated\n";
    pr "  -- with the olmar wiki bot on behalf of Hendrik Tews.\n";
    pr "  --> __NOTOC__\n";

    pr cl_name;
    if cl_renamed then
      pf " (original Elsa name: %s)" (class_type_link cl.ac_name);

    if cl.ac_super = None && cl.ac_subclasses = [] then 
      begin
	pr " is a Ast type node (without any subtype nodes). ";
	pf "%s is a record.\n\n" cl_name;
	is_record := true
      end
    else if cl.ac_super = None then 
      begin
	pf " is a Ast super type node with %d subtype nodes. " 
	  (List.length cl.ac_subclasses);
	pf "Concreate ast nodes do always belong to one of the subtypes.\n\n";
	(* is_record not used below *)
      end
    else begin
      pf " is a Ast subtype node belonging to %s. " 
	(class_type_link (translated_class_name (the cl.ac_super)));
      if cl.ac_record then 
	begin
	  pf "%s is a record.\n\n" cl_name;
	  is_record := true;
	end
      else begin
	pf "%s is a variant.\n\n" cl_name;
	is_record := false;
      end
    end;
    

    if cl.ac_subclasses = []
    then
      pf "=== %s Data Fields ===\n\n"
	(if !is_record then "Record" else "Variant")
    else if all_fields <> [[];[];[]] 
    then
      pr "=== Data Fields in all Subclasses ===\n\n"
    else
      ();

    List.iter
      (List.iter 
	 (fun field -> 
	    let field_name = translated_field_name cl field in
	    let _field_renamed = field_name <> field.af_name
	    in
	      pf "* %s : " field_name;
	      print_type pf pr field.af_mod_type;
	      pr "\n";
	 )) 
      all_fields;

    if cl.ac_super = None then begin
      pr "=== Subtype Nodes (Subclasses) ===\n\n";
      if cl.ac_subclasses <> [] then begin
	pf "%s has %d subtype nodes:\n" cl_name (List.length cl.ac_subclasses);
	print_sub_node_list pf "*" cl.ac_subclasses "\n";
	pr "\n";
	end
      else
	pf "%s has no subtype nodes.\n" cl_name;
    end;

    pr "=== Comments ===\n\n";

    pr "''Please make your modifications only below, because the content ";
    pr "above will be regenerated automatically from time to time.''\n\n";
    pr node_page_separation_line;
    pr "\n\n";
    pr comments;
    Buffer.contents buf


let generate_del_page node comments =
  if comments = "" then ""
  else
    let buf = Buffer.create 8191 in
    let pf format = Printf.bprintf buf format in
    let pr = Buffer.add_string buf
    in
      pr "<!-- Besides the comments at the end this page has been generated\n";
      pr "  -- with the olmar wiki bot on behalf of Hendrik Tews.\n";
      pr "  -->\n\n";

      pf "%s is not a ast node type any longer.\n" node;
      pr "This page will be deleted (made empty) as soon as the comments\n";
      pr "below have been moved.\n\n";

      pr "=== Comments ===\n\n";

      pr "''Please make your modifications only below, because the content ";
      pr "above will be regenerated automatically from time to time.''\n\n";
      pr node_page_separation_line;
      pr "\n\n";
      pr comments;
      Buffer.contents buf


let generate_rename_page node comments =
  let cl = get_node node in
  let olmar_name = translated_class_name cl in
  let buf = Buffer.create 8191 in
  let pf format = Printf.bprintf buf format in
  let pr = Buffer.add_string buf
  in
    pr "<!-- Besides the comments at the end this page has been generated\n";
    pr "  -- with the olmar wiki bot on behalf of Hendrik Tews.\n";
    pr "  -->\n\n";

    pf "The Elsa ast class type %s has been renamed to %s in Olmar.\n"
      node (class_type_link olmar_name);
    pr "See there for further information.\n\n";

    pr "=== Comments ===\n\n";

    pr "''Please make your modifications only below, because the content ";
    pr "above will be regenerated automatically from time to time.''\n\n";
    pr node_page_separation_line;
    pr "\n\n";
    pr comments;
    Buffer.contents buf


let print_node_types buf ast with_subnodes =
  let pf format = Printf.bprintf buf format in
  let pr = Buffer.add_string buf
  in
    List.iter
      (fun super ->
	 pf "* %s" (class_type_link (translated_class_name super));
	 if with_subnodes then 
	   begin
	     if super.ac_subclasses = []
	     then
	       pr " : no subnodes\n"
	     else begin
	       pr " subnodes:\n";
	       print_sub_node_list pf "** " super.ac_subclasses "\n";
	     end
	   end
	 else
	   pr "\n"
      )
      (List.sort 
	 (fun cl1 cl2 -> 
	    compare (translated_class_name cl1) (translated_class_name cl2))
	 ast)



let generate_overview_page ast renamings intro prefix =
  let buf = Buffer.create 8191 in
  let pf format = Printf.bprintf buf format in
  let pr = Buffer.add_string buf
  in
    pr intro;
    pr "\n";
    pr overview_separation_line;
    pr "\n== Olmar Ast Supertype nodes without subtypes ==\n\n";
    print_node_types buf ast false;
    pr "\n== Olmar Ast Supertype nodes with subtypes ==\n\n";
    print_node_types buf ast true;
    pr "\n";
    pr overview_node_list_start_line;
    pr "\n";
    pf "%s " renamings_start_line;
    List.iter (fun s -> pf "%s " s) renamings;
    pr "\n";
    pf "%s%s\n" wiki_page_prefix prefix;
    List.iter
      (fun super ->
	 List.iter 
	   (fun cl -> pf "%s " (translated_class_name cl))
	   (super :: super.ac_subclasses);
	 pr "\n";)
      ast;
    pr overview_node_list_end_line;
    pr "\n";
    Buffer.contents buf
    




(******************************************************************************
 ******************************************************************************
 *
 * page modifications
 *
 ******************************************************************************
 ******************************************************************************)

(* ignore trailing newlines in the comparison, because they are cut *)
let compare_content a b =
  let a_len = ref(String.length a) in
  let b_len = ref(String.length b)
  in
    while !a_len > 0 && a.[!a_len -1] = '\n' do decr a_len done;
    while !b_len > 0 && b.[!b_len -1] = '\n' do decr b_len done;
    (String.sub a 0 !a_len) = (String.sub b 0 !b_len)

let wiki_page prefix page =
  translate " " "_" (prefix ^ page)

let extract_page_comments page_name page =
  let lines = ref (split '\n' page)
  in
    while !lines <> [] && List.hd !lines <> node_page_separation_line do
      (* Printf.printf "No %s\n" (List.hd !lines); *)
      lines := List.tl !lines;
    done;
    if !lines = [] then begin
      Printf.eprintf 
	"Cannot find comment separation line in page %s\n" page_name;
      exit 1
    end;
    lines := List.tl !lines;
    while !lines <> [] && List.hd !lines = "" do
      lines := List.tl !lines;
    done;
    String.concat "\n" !lines    


let mod_strings modification =
  match modification with
    | Delete_node -> "deleted", "delete"
    | Update_node -> "updated", "update"
    | New_node -> "created", "create"
    | Renamed_node -> "renamed", "renaming"

let do_node_page wiki_session counter total prefix modification 
    page node watchit summary =
  let (succ_code, normal_code) = mod_strings modification in
  let wiki_page_name = wiki_page prefix page in
  let _ = 
    incr counter;
    Printf.printf "%d/%d: %s %s... %!" 
      !counter total
      normal_code wiki_page_name 
  in
  let old_content = match get_page wiki_session wiki_page_name with
    | Error(status, msg) ->
	if (modification = New_node && status = `Not_found) or
	  (modification = Renamed_node && status = `Not_found)
	then ""
	else begin
	  Printf.eprintf "GETTING OLD CONTENT FAILED\n%s\n" msg;
	  exit 4;
	end
    | OK old_content -> 
	if !ignore_create_filled_pages = false &&
	  modification = New_node && old_content <> ""
	then 
	  begin
	    Printf.eprintf
	      "\nPage %s was not empty when trying to create it\n"
	      wiki_page_name;
	    exit 3
	  end
	else
	  old_content
  in
    Printf.printf "got old content%!";
    let comments = match modification with
      | Delete_node
      | Update_node -> extract_page_comments page old_content
      | Renamed_node -> 
	  if old_content = "" then "" 
	  else extract_page_comments page old_content
      | New_node -> ""
    in
    let _ = match modification with
      | Update_node
      | New_node -> Printf.printf "...%!"
      | Delete_node
      | Renamed_node ->
	  if comments <> "" then 
	    Printf.printf " COMMENTS ON %s PAGE...%!"
	      (match modification with
		 | Update_node 
		 | New_node -> assert false
		 | Delete_node -> "DELETED"
		 | Renamed_node -> "RENAMING")
	  else
	    Printf.printf "...%!"
    in
      (* 
       * let _ = Printf.printf
       *           "Extracted comments from old page\n%s\nEndComment\n"
       *   comments in
       *)
    let new_content = match modification with
      | Delete_node -> generate_del_page node comments
      | Update_node
      | New_node -> generate_node_page node comments 
      | Renamed_node -> generate_rename_page node comments

    in 
      if compare_content old_content new_content
      then
	Printf.printf "No change!\n%!"
      else
	match
	  write_wiki_page wiki_session wiki_page_name 
	    new_content watchit summary
	with
	  | OK _ -> Printf.printf " successfully %s\n%!" succ_code
	  | Error(_, msg) -> 
	      Printf.printf " %s FAILED\n%s\n%!" normal_code msg


let do_overview_page 
    wiki_session ast renamings page intro prefix watchit summary =
  let new_content = generate_overview_page ast renamings intro prefix in
  let _ = Printf.printf "overview page... %!" 
  in match write_wiki_page wiki_session page new_content watchit summary with
    | OK _ -> Printf.printf "updated\n%!"
    | Error(_, msg) -> Printf.printf "update FAILED\n%s\n%!" msg


(******************************************************************************
 ******************************************************************************
 *
 * main
 *
 ******************************************************************************
 ******************************************************************************)

let summary_opt = ref None

let test_update_page = ref false

let test_new_page = ref false

let test_del_page = ref false

let test_rename_page = ref false

let test_overview_page = ref false

let arguments = [
  ("-summary", Arg.String (fun s -> summary_opt := Some s),
   "summary set wiki editing summary");
  ("-create-filled", Arg.Set ignore_create_filled_pages,
   " do not stop when creating a nonempty page");
  ("-test-update", Arg.Set test_update_page,
   "page test updating page into the Sandbox");
  ("-test-new", Arg.Set test_new_page,
   "page test writing a new page into the Sandbox");
  ("-test-del", Arg.Set test_del_page,
   " test deleting the Sandbox");
  ("-test-rename", Arg.Set test_rename_page,
   " test a renaming page in the Sandbox");
  ("-test-overview", Arg.Set test_overview_page,
   " put the overview page into the Sandbox");
]

let do_node_test wiki_session =
  let modification = 
    if !test_update_page then Update_node
    else if !test_new_page then New_node
    else if !test_rename_page then Renamed_node
    else (assert !test_del_page; Delete_node)
  in
    do_node_page wiki_session (ref 0) 1 "" modification
      "Sandbox" "STA_ATOMIC" false "wiki bot test"


let do_test wiki_session ast renamings intro prefix =
  if !test_overview_page
  then 
    do_overview_page wiki_session ast renamings "Sandbox" intro prefix false ""
  else 
    do_node_test wiki_session


let read_user_passwd () =
  let ic = open_in "credentials" in
  let user = input_line ic in
  let passwd = input_line ic
  in
    close_in ic;
    (user, passwd)


let main () =
  let (_oast_file, ast) = setup_ml_ast arguments "wiki_bot" in
  let (user, passwd) = read_user_passwd () in
  let counter = ref 0 in
  let summary = match !summary_opt with
    | Some s -> s
    | None ->
	prerr_endline "Need summary specified via -summary";
	exit 1
  in match wiki_login "http://wiki.mozilla.org/" user passwd with
    | Error(_, msg) ->
	Printf.eprintf "Unable to log into the wiki\n%s\n" msg;
	exit 6
    | OK wiki_session ->
	print_endline "logged into wiki";
	match get_page wiki_session overview_page_name with
	  | Error(_, msg) ->
	      Printf.eprintf "Error getting overview page %s\n%s\n" 
		overview_page_name msg;
	      exit 5
	  | OK ov_page ->
	      let (intro, prefix, node_changes, renamings) = 
		process_overview_page ast ov_page
	      in
	      let total = List.length node_changes
	      in
		if !test_update_page or !test_new_page or !test_del_page 
		  or !test_rename_page or !test_overview_page
		then
		  do_test wiki_session ast renamings intro prefix
		else
		  begin
      		    List.iter
		      (fun (node, mod_code) ->
      		    	 ignore(
		    	   do_node_page wiki_session counter total
			     prefix mod_code node node 
		    	     true summary))
		      node_changes;
		    ignore(
		      do_overview_page wiki_session ast
			renamings overview_page_name
			intro prefix true summary);
		  end 
;;


main()
