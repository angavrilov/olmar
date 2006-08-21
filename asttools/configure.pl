#!/usr/bin/perl -w
# configure script for elsa

use strict 'subs';

# default location of smbase relative to this package
$SMBASE = "../smbase";
$req_smcv = 1.03;            # required sm_config version number
$thisPackage = "semantic";

print "Configuring $thisPackage ...\n\n";

# -------------- BEGIN common block ---------------
# do an initial argument scan to find if smbase is somewhere else
for (my $i=0; $i < @ARGV; $i++) {
  my ($d) = ($ARGV[$i] =~ m/-*smbase=(.*)/);
  if (defined($d)) {
    $SMBASE = $d;
  }
}

# try to load the sm_config module
eval {
  push @INC, ($SMBASE);
  require sm_config;
};
if ($@) {
  die("$@" .     # ends with newline, usually
      "\n" .
      "I looked for smbase in \"$SMBASE\".\n" .
      "\n" .
      "You can explicitly specify the location of smbase with the -smbase=<dir>\n" .
      "command-line argument.\n");
}

# check version number
$smcv = get_sm_config_version();
if ($smcv < $req_smcv) {
  die("This package requires version $req_smcv of sm_config, but found\n" .
      "only version $smcv.\n");
}
# -------------- END common block ---------------

# defaults
$AST = "../ast";
$ELSA = "../elsa";


sub usage {
  standardUsage();

  print(<<"EOF");
package options:
  -ast=<dir>:        specify where the ast system is [$AST]
  -elsa=<dir>:       specify where the elsa system is [$ELSA]
EOF
}


# -------------- BEGIN common block 2 -------------
# global variables holding information about the current command-line
# option being processed
$option = "";
$value = "";

# process command-line arguments
foreach $optionAndValue (@ARGV) {
  # ignore leading '-' characters, and split at first '=' (if any)
  ($option, $value) =
    ($optionAndValue =~ m/^-*([^-][^=]*)=?(.*)$/);
                      #      option     = value

  my $arg = $option;

  if (handleStandardOption()) {
    # handled by sm_config.pm
  }
  # -------------- END common block 2 -------------

  elsif ($arg eq "ast") {
    $AST = getOptArg();
  }
  elsif ($arg eq "elsa") {
    $ELSA = getOptArg();
  }

  else {
    die "unknown option: $arg\n";
  }
}

finishedOptionProcessing();


# ------------------ check for needed components ----------------
test_smbase_presence();

# ast
if (! -f "$AST/ast_util.ml") {
  die "I cannot find ast_util.ml in `$AST'.\n" .
      "The ast system is required for this package.\n" .
      "If it's in a different location, use the -ast=<dir> option.\n";
}

if (! -f "$ELSA/ast_annotation.ml") {
  die "I cannot find ast_annotation.ml in `$ELSA'.\n" .
      "The elsa system is required for this package.\n" .
      "If it's in a different location, use the -ast=<dir> option.\n";
}


# ocaml
($OCAMLDIR, $OCAMLC, $OCAMLOPT, $OCAMLCC, $OCAML_NATIVE, 
 $OCAML_OBJ_EXT, $OCAML_LIB_EXT) = 
    test_ocaml_compiler();

# ------------------ config.summary -----------------
$summary = getStandardConfigSummary();

$summary .= <<"OUTER_EOF";
cat <<EOF
  SMBASE:        $SMBASE
  AST:           $AST
  ELSA:          $ELSA
  OCAMLC:        $OCAMLC
  OCAMLOPT:      $OCAMLOPT
  OCAMLCC:       $OCAMLCC
  OCAML LIB DIR: $OCAMLDIR
  OCAML_OBJ_EXT: $OCAML_OBJ_EXT
  OCAML_LIB_EXT: $OCAML_LIB_EXT
EOF
OUTER_EOF

writeConfigSummary($summary);


# ------------------- config.status ------------------
writeConfigStatus("SMBASE" => "$SMBASE",
                  "AST" => "$AST",
                  "ELSA" => "$ELSA",
		  "OCAMLC" => "$OCAMLC",
		  "OCAMLOPT" => "$OCAMLOPT",
		  "OCAMLCC" => "$OCAMLCC",
		  "OCAMLDIR" => "$OCAMLDIR",
		  "OCAML_OBJ_EXT" => "$OCAML_OBJ_EXT",
		  "OCAML_LIB_EXT" => "$OCAML_LIB_EXT");


# ----------------- final actions -----------------
# run the output file generator
run("./config.status");

print("\nYou can now run make, usually called 'make' or 'gmake'.\n");

exit(0);


# silence warnings
pretendUsed($thisPackage, $OCAML_NATIVE);
