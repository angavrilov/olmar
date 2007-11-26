#!/usr/bin/perl -w
# configure script for elsa

use strict 'subs';


print "Configuring Olmar ...\n\n";


# defaults
#@LDFLAGS = ("-g -Wall");
$CXX = "g++";
@CCFLAGS = ("-g", "-Wall", "-Wno-deprecated", "-D__UNIX__");

$ELSA_BASE_BASE = "..";
$SMBASE = "$ELSA_BASE_BASE/smbase";
$MEMCHECK = "../../memcheck";

# try to detect ocamlopt
$try_ocamlopt=1;
$debug=0;



sub usage {
  print(<<"EOF");
usage: ./configure [options]

influential environment variables:
  CXX                C++ compiler [$CXX]

package options:
  -h:                print this message
  -cxx=ccomp         specify the C++ compiler [$CXX]
  -memcheck=<dir>:   specify where to find the memcheck library [$MEMCHECK]
  -no-dash-O2:       disable -O2
  -no-ocamlopt:      disable ocamlopt (for bytecode compilation testing only)
EOF
}


# =========================================================================
# =================== command line options ================================
# =========================================================================

# get an argument to an option
sub getOptArg($$) {
  my ($arg, $value) = @_;
  if (!$value) {
    die("option -$arg requires an argument\n");
  }
  return $value;
}

#save the argument vector with embedded quotes
$arg_vector = join(" ", map { "'" . $_ . "'" } @ARGV);


foreach $optionAndValue (@ARGV) {
  # ignore leading '-' characters, and split at first '=' (if any)
  my ($arg, $value) =
    ($optionAndValue =~ m/^-*([^-][^=]*)=?(.*)$/);
                      #      option     = value

  if ($arg eq "h" ||
      $arg eq "help") {
    usage();
    exit(0);
  }

  elsif ($arg eq "cxx") {
    $CXX = getOptArg($arg, $value);
  }

  elsif ($arg eq "memcheck") {
    $MEMCHECK = getOptArg($arg, $value);
  }

  elsif ($arg eq "no-dash-O2") {
    $no_dash_O2 = 1;
  }

  elsif ($arg eq "no-ocamlopt") {
    $try_ocamlopt = 0;
    # successful return !!
    1;
  }

  else {
    die "unknown option: $arg\n";
  }
}


if (!$debug) {
  push @CCFLAGS, ("-DNDEBUG");
}

if (!$no_dash_O2) {
  push @CCFLAGS, ("-O2");
}

$os = `uname -s`;
chomp($os);
if ($os eq "Linux") {
  push @CCFLAGS, "-D__LINUX__";
} elsif ($os =~ /CYGWIN/) {
  push @CCFLAGS, "-D__CYGWIN__";
}



# =========================================================================
# ========================== ocaml compiler ===============================
# =========================================================================

sub test_ocaml_compiler {
  my($ocamlc,$ocamlopt,$ocamlcc,$camlp4o,$native,$obj_ext,$lib_ext,$lib);
  print("Searching ocaml compiler... ");

  if(open(OCAML, "ocamlc.opt -where|")){
    print("ocamlc.opt found\n");
    $lib = <OCAML>;
    chomp($lib);
    close OCAML;
    $ocamlc = "ocamlc.opt";
    $ocamlcc = $ocamlc;
  }
  elsif(open(OCAML, "ocamlc -where|")){
    print("ocamlc found\n");
    $lib = <OCAML>;
    chomp($lib);
    close OCAML;
    $ocamlc = "ocamlc";
    $ocamlcc = $ocamlc;
  }
  else {
    print(<<"EOF");

Could neither find ocamlc nor ocamlc.opt. 
Please check your path, the error messages above and retry.
EOF

    exit(1);
  }

  if(!-r ($lib . "/caml/mlvalues.h")){
    print(<<"EOF");

Cannot read "caml/mlvalues.h" in 
  $lib 
(returned from ocamlc -where). Please check your ocaml installation.
EOF

    exit(1);
  }

  print("Searching native ocaml compiler... ");
  if($try_ocamlopt && open(OCAMLOPT, "ocamlopt.opt -where|")){
    print("ocamlopt.opt found\n");
    close OCAMLOPT;
    $ocamlopt="ocamlopt.opt";
    $ocamlcc = $ocamlopt;
    $native=1
  }
  elsif($try_ocamlopt && open(OCAMLOPT, "ocamlopt -where|")){
    print("ocamlopt found\n");
    close OCAMLOPT;
    $ocamlopt="ocamlopt";
    $ocamlcc = $ocamlopt;
    $native=1
  }
  else {
    print(
      "\nNeither ocamlopt not ocamlopt.opt found. Use bytecode compilation.\n");
    $ocamlopt="[ocamlopt not found]";
    $native=0
  }

  if ($native) {
    $obj_ext="cmx";
    $lib_ext="cmxa";
  }
  else {
    $obj_ext="cmo";
    $lib_ext="cma";
  }

  print("Searching camlp4o... ");
  if(open(OCAML, "camlp4o -where|")) {
    print("camlp4o found\n");
    $camlp4o="camlp4o";
    close OCAML;
  }
  else {
    print(<<"EOF");

Could not find camlp4o.
Please check your path, the error messages above and retry.
EOF

    exit(1);
  }
    

  return($lib, $ocamlc, $ocamlopt, $ocamlcc, $camlp4o, 
    $native, $obj_ext, $lib_ext);
}



($OCDIR, $OCC, $OCOPT, $OCCC, $OCP4O,
 $OC_NATIVE, $OC_OBJ_EXT, $OC_LIB_EXT) = 
    test_ocaml_compiler();


# =========================================================================
# ========================== memcheck lib =================================
# =========================================================================

# memcheck
print("Searching the memcheck library... ");
if (-f "$MEMCHECK/generate_type_descr.cmo" && 
    -f "$MEMCHECK/memcheck.$OC_OBJ_EXT") {
  print("found in $MEMCHECK\n");
}
else {
  print("not found\n",
	"Unable to find $MEMCHECK/generate_type_descr.cmo ",
	"and $MEMCHECK/memcheck.$OC_OBJ_EXT.\n",
	"Configure and compile the memcheck library and try again",
	"with the -memcheck switch.\n",
	"You can also compile without memcheck, then check_oast ",
	"won't be build\n",
	"DISABLING memcheck\n");
  $MEMCHECK="";
}


# =========================================================================
# ========================== ocamlfind ====================================
# =========================================================================

print("Searching ocamlfind with the netclient package ...");
if (system("ocamlfind query netclient >/dev/null 2>&1") == 0) {
  print "found\n";
  $wiki_bot = 1;
} 
else {
  print("NOT FOUND\ndisable building wiki_bot\n");
  $wiki_bot = 0;
}
      


# =========================================================================
# ========================== C++ compiler =================================
# =========================================================================


sub test_CXX_compiler {
  my $wd = `pwd`;
  chomp($wd);

  my $testcout = "testcout";
  print("Testing C++ compiler ...\n");
  my $cmd = "$CXX -o $testcout @CCFLAGS $SMBASE/testcout.cc";
  if (system($cmd)) {
    # maybe problem is -Wno-deprecated?
    printf("Trying without -Wno-deprecated ...\n");
    @CCFLAGS = grep { $_ ne "-Wno-deprecated" } @CCFLAGS;
    $cmd = "$CXX -o $testcout @CCFLAGS $SMBASE/testcout.cc";
    if (system($cmd)) {
      print(<<"EOF");

I was unable to compile a really simple C++ program.  I tried:
  cd $wd
  $cmd

Please double-check your compiler installation.

I doubt anything will compile.
EOF
      exit(2);
    }
  }

  if (system("./$testcout")) {
    print(<<"EOF");

I was able to compile testcout.cc, but it did not run.  I tried:
  cd $wd
  $cmd

and then
  ./$testcout      (this one failed)

A frequent cause for this error is a misconfiguration of the language
runtime libraries.

For example, by default g++ installs libstdc++ into /usr/local/lib,
but on many systems this directory is not searched by the loader.
Solutions would include symlinking or copying the files into /usr/lib,
adding /usr/local/lib to the library search path, or reinstalling g++
with a different --prefix argument to its configuration script.

I doubt anything will compile.
EOF
    exit(2);
  }

  print("C++ compiler seems to work\n");

  # make a variant, CFLAGS, that doesn't include -Wno-deprecated
  #@CFLAGS = grep { $_ ne "-Wno-deprecated" } @CCFLAGS;
}


test_CXX_compiler();


# =========================================================================
# ========================== config summary ===============================
# =========================================================================


if($wiki_bot) {
  $wiki_bot_text="enabled";
  $OLMARSUBDIRS="util build meta wiki_bot"
}
else {
  $wiki_bot_text="disabled";
  $OLMARSUBDIRS="util build meta"
}




sub writeConfigSummary {
  my ($summary) = @_;

  open (OUT, ">config.summary") or die("cannot write config.summary: $!\n");

  print OUT ($summary);
  print OUT ("echo \"\"\n");

  close(OUT) or die;
  chmod 0755, "config.summary";
}


# =========================================================================
# ========================== config status ================================
# =========================================================================

@dirs= (
	".", 
	"util", 
	"build", 
	"build/general", 
	"build/astgen", 
	"build/elsa", 
	"meta", 
	"meta/generated_elsa", 
	"meta/generated_ast",
	"wiki_bot"
	);


sub writeConfigStatus {
  my @pairs = @_;

  # create a program which will create the Makefile
  open(OUT, ">config.status") or die("can't make config.status");

  # preamble
  print OUT (<<"OUTER_EOF");
#!/bin/sh
# config.status

# this file was created by ./configure

if [ "\$1" = "-reconfigure" -o configure.pl -nt config.status ]; then
  # re-issue the ./configure command
  exec ./configure $arg_vector
fi

# report on configuration
./config.summary

dirs="@dirs"

if [ "\$1" = "-only" ] ; then
   if [ \$# -lt 2 ] ; then
       echo "usage: config.status -only <dir>"
       exit 1
   fi
   dirs="\$2"
fi

for d in \$dirs ; do

    echo "generating \$d/Makefile"

    # overcome my chmod below
    rm -f \$d/Makefile

    echo "# Makefile for \$d" > \$d/Makefile
    echo "# NOTE: generated by ./configure, do not edit" >> \$d/Makefile
    echo >> \$d/Makefile

    # variable substitution
    sed \\
OUTER_EOF

  # variable substitution
  for ($i=0; $i < @pairs; $i += 2) {
    my $a = $pairs[$i];
    my $b = $pairs[$i+1];
    #           -e  "s|@foo@|bar|g" \           (example)
    print OUT ("        -e \"s|\@$a\@|$b|g\" \\\n");
  }

  # postamble
  print OUT (<<"EOF");
    < \$d/Makefile.in >> \$d/Makefile

    # discourage editing ..
    chmod a-w \$d/Makefile
done

EOF

  close(OUT) or die;
  chmod 0755, "config.status";
}


$summary = <<"OUTER_EOF";
#!/bin/sh
# config.summary

echo "Olmar configuration summary:"
echo "  command:     ./configure $arg_vector"
echo ""
echo "Compile flags:"
cat <<EOF
  ELSA_BASE_BASE:  $ELSA_BASE_BASE
  MEMCHECK:        $MEMCHECK
  CXX:             $CXX
  CCFLAGS:         @CCFLAGS
  OCC:             $OCC
  OCCC:            $OCCC
  OCOPT:           $OCOPT
  OCP4O:           $OCP4O
  OC_OBJ_EXT:      $OC_OBJ_EXT
  OC_LIB_EXT:      $OC_LIB_EXT
  OC_NATIVE:       $OC_NATIVE
  OCDIR:           $OCDIR
  wikibot:         $wiki_bot_text
EOF
OUTER_EOF


writeConfigSummary($summary);


writeConfigStatus(
    "ELSA_BASE_BASE" => "$ELSA_BASE_BASE",
    "MEMCHECK" => "$MEMCHECK",
    "CXX" => "$CXX",
    "CCFLAGS" => "@CCFLAGS",
    
    "OCC" => "$OCC",
    "OCCC" => "$OCCC",
    "OCOPT" => "$OCOPT",
    "OCP4O" => "$OCP4O",
    "OC_OBJ_EXT" => "$OC_OBJ_EXT",
    "OC_LIB_EXT" => "$OC_LIB_EXT",
    "OC_NATIVE" => "$OC_NATIVE",
    "OCDIR" => "$OCDIR",
    "OLMARSUBDIRS" => "$OLMARSUBDIRS"
    );



# =========================================================================
# ========================== generate Makefiles ===========================
# =========================================================================

sub run {
  my $code = system(@_);
  checkExitCode($code);
}

sub checkExitCode {
  my ($code) = @_;
  if ($code != 0) {
    # hopefully the command has already printed a message,
    # I'll just relay the status code
    if ($code >> 8) {
      exit($code >> 8);
    }
    else {
      exit($code & 127);
    }
  }
}

run("./config.status");

exit(0);



### Local Variables: ###
### perl-indent-level: 2 ###
### End: ###
