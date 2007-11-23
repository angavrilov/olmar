(*  Copyright 2006 Hendrik Tews, All rights reserved.                  *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)


type 'a result =
  | OK of 'a
  | Error of Nethttp.http_status * string

type wiki_session = {
  url : string;
  cookies : Nethttp.cookie list;
}


(* wiki_login url user password *)
val wiki_login : string -> string -> string -> wiki_session result

(* get_page session title -> content *)
val get_page : wiki_session -> string -> string result

(* write_wiki_page session page_title new_content watchit summary 
 * -> (success, response) 
 *)
val write_wiki_page : 
  wiki_session -> string -> string -> bool -> string -> string result
