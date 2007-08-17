(*  Copyright 2006-2007, Hendrik Tews, All rights reserved.            *)
(*  See file license.txt for terms of use                              *)
(***********************************************************************)

open Cfg_type

type error_report_level =
  | Report_all
  | Ignore_unimplemented
  | Report_nothing

val do_file_list : string list -> error_report_level list -> cfg_type
