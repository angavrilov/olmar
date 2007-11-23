(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Http_client
open Netstring_ext


(******************************************************************************
 ******************************************************************************
 *
 * types
 *
 ******************************************************************************
 ******************************************************************************)

type 'a result =
  | OK of 'a
  | Error of Nethttp.http_status * string

type wiki_session = {
  url : string;
  cookies : Nethttp.cookie list;
}

(******************************************************************************
 ******************************************************************************
 *
 * misc / state
 *
 ******************************************************************************
 ******************************************************************************)


let pipeline = new pipeline


(******************************************************************************
 ******************************************************************************
 *
 * functionality
 *
 ******************************************************************************
 ******************************************************************************)

let wiki_login url user password =
  (* url must end with a slash *)
  let _ = assert(url.[String.length url -1] = '/') in
  let post_call = new post 
    (url ^ "index.php?title=Special:Userlogin&action=submitlogin&type=login")
    [("wpName", user);
     ("wpPassword", password);
     ("wpLoginattempt", "Log in")]
  in
    pipeline#add post_call;
    pipeline#run();
    if post_call#response_status = `Found 
    then 
      OK { url = url;
	   cookies = get_set_cookies post_call#response_header;
	 }
    else 
      Error(post_call#response_status, post_call#response_status_text)


let download cookies url =
  let get_call = new get url in
  let header = get_call#request_header `Base
  in
    set_cookies header cookies;
    pipeline#add get_call;
    pipeline#run();
    if get_call#response_status = `Ok
    then 
      OK(get_call#response_body#value)
    else
      Error(get_call#response_status, get_call#response_status_text)


let get_page session title =
  download 
    session.cookies 
    (session.url ^ "index.php?title=" ^ title ^ "&action=raw")



let write_wiki_page session title content watchit summary =
  match 
    download session.cookies 
      (session.url ^ "index.php?title=" ^ title ^ "&action=edit")
  with
    | Error(status, msg) -> Error(status, "Unable to access edit page.\n" ^ msg)
    | OK edit_page ->
	let (action_url, inputs) = Form_values.get "editform" edit_page in
	let hidden_inputs = 
	  List.filter
	    (function
  	       | Form_values.Form_input(_, Form_values.Form_input_hidden, _) -> 
		   true
  	       | Form_values.Form_input _ -> false
  	       | Form_values.Form_textarea _ -> false)
	    inputs
	in
	  (* let action = Form_values.find_input "wpPreview" inputs in *)
	let action = Form_values.find_input "wpSave" inputs in

	let post_call = 
	  new post (session.url ^ action_url)
	    (
	      (if watchit then
		 [("wpWatchthis", "1")]
	       else
		 []
	      ) 
	      @ [ 
		("wpSummary", summary);
		(Form_values.name_and_val_of_form_input action);
		("wpTextbox1" , content)
	      ]
	      @ (List.map Form_values.name_and_val_of_form_input hidden_inputs)
	    )
	in
	let header = post_call#request_header `Base
	in
	  set_cookies header session.cookies;
	  pipeline#add post_call;
	  pipeline#run();

	  if post_call#response_status = `Found 
	  then
	    OK(post_call#response_body#value)
	  else
	    Error(post_call#response_status,
		  "Unable to submit new content.\n" 
		  ^ post_call#response_status_text)

