#!/usr/bin/perl -w
# configure script for elkhound

use strict 'subs';

$| = 1;     # autoflush

# defaults
$BASE_FLAGS = "-g -Wall -D__UNIX__";
$CCFLAGS = ();
$debug = 0;
$loc = 1;
%flags = (
  "eef" => 0,
  "gcs" => 0
);
$subconfigure = 1;
$SMBASE = "../smbase";
$AST = "../ast";

# arguments to pass to sub-configures
@c_args = ();


sub usage {
  print(<<"EOF");
usage: ./configure [options]
options:
  -h:                print this message
  -debug,-nodebug:   enable/disable debugging options [disabled]
  -prof              enable profiling
  -devel             add options useful while developing
  -loc,-noloc:       enable/disable source location tracking [enabled]
  -action:           enable use of "-tr action" to see parser actions
  -eef=y/n           enable/disable EEF compression [disabled]
  -gcs=y/n           enable/disable GCS compression [disabled]
  -fastest:          turn off all Elkhound features that are not present
                     in Bison, for the purpose of performance comparison
                     (note that Elsa will not work in this mode)
  <op>:              add a given option to the gcc command line,
                       including forms: -W*, -pg, -D*, -O*
  -nosub:            do not invoke subdirectory configure scripts
  -smbase=<dir>:     specify where the smbase library is [$SMBASE]
  -ast=<dir>:        specify where the ast library is [$AST]
EOF
}


# process command-line arguments
$originalArgs = join(' ', @ARGV);
while (@ARGV) {
  my $tmp;
  my $arg = $ARGV[0];
  shift @ARGV;

  # treat leading "--" uniformly with leading "-"
  $arg =~ s/^--/-/;

  if ($arg eq "-h" ||
      $arg eq "-help") {
    usage();
    exit(0);
  }

  # things that look like options to gcc should just
  # be added to CCFLAGS
  elsif ($arg =~ m/^(-W|-pg$|-D|-O)/) {
    push @CCFLAGS, $arg;
    push @c_args, $arg;
  }

  elsif ($arg eq "-d" ||
         $arg eq "-debug") {
    $debug = 1;
    push @c_args, $arg;
  }
  elsif ($arg eq "-nodebug") {
    $debug = 0;
    push @c_args, $arg;
  }

  elsif ($arg eq "-prof") {
    push @CCFLAGS, "-pg";
    push @c_args, $arg;
  }

  elsif ($arg eq "-devel") {
    push @CCFLAGS, "-Werror";
    push @c_args, $arg;
  }

  elsif ($arg eq "-loc") {
    $loc = 1;
  }
  elsif ($arg eq "-noloc") {
    $loc = 0;
  }

  elsif ($arg eq "-action") {
    push @CCFLAGS, "-DACTION_TRACE=1";
  }

  elsif ($arg eq "-fastest") {
    # the idea is I can say
    #   $ ./configure -fastest
    #   $ make clean; make
    #   $ ./perf -tests c -iters 5
    # to verify that I'm still within 3% of Bison (at least
    # when compiled with gcc-2.95.3)
    $loc = 0;
    $debug = 0;
    $flags{eef} = 0;
    push @CCFLAGS,
      ("-DUSE_RECLASSIFY=0",        # no token reclassification
       "-DUSE_KEEP=0",              # don't call keep() functions
       "-DNDEBUG_NO_ASSERTIONS",    # disable all xassert() calls
       "-DDO_ACCOUNTING=0",         # don't count stack nodes, etc.
       "-DENABLE_YIELD_COUNT=0");   # don't check for yield-then-merge at runtime
    push @c_args, "-DUSE_RECLASSIFY=0";
  }

  elsif ($arg eq "-nosub") {
    $subconfigure = 0;
  }

  elsif (($tmp) = ($arg =~ m/^-smbase=(.*)$/)) {
    $SMBASE = $tmp;
    if ($tmp !~ m|^/|) {
      push @c_args, "-smbase=../$tmp";   # modify relative path
    }
  }
  elsif (($tmp) = ($arg =~ m/^-ast=(.*)$/)) {
    $AST = $tmp;
    if ($tmp !~ m|^/|) {
      push @c_args, "-ast=../$tmp";
    }
  }

  elsif (($opt, $val) = ($arg =~ m/^-(.*)=(y|n|yes|no)$/)) {
    if (!defined($flags{$opt})) {
      die "unknown flag: $opt\n";
    }              
    $flags{$opt} = ($val eq "y" || $val eq "yes")? 1 : 0;
  }

  else {
    die "unknown option: $arg\n";
  }
}

if (!$debug) {
  push @CCFLAGS, ("-O2", "-DNDEBUG");
}

$os = `uname -s`;
chomp($os);
if ($os eq "Linux") {
  push @CCFLAGS, "-D__LINUX__";
}

# summarize compression flags
@compflags = ();
for $k (keys %flags) {
  if ($flags{$k}) {
    push @compflags, $k;
  }
}
if (@compflags) {
  $compflags = join(',', @compflags);
}
else {
  $compflags = "<none>";
}

# individual flags, for substituting later
$eef = $flags{eef};
$gcs = $flags{gcs};
if ($gcs && !$eef) {
  die "GCS requires EEF\n";
}


# ------------------ compiler tests ---------------
# get g++ version (it's a lot easier just to test the version
# number than to craft various kinds of compilation tests)
@lines = `g++ --version`;
checkExitCode($?);
$gccver = $lines[0];

# g++-3?
if ($gccver =~ m/\(GCC\) 3\./) {
  # suppress warnings about deprecation
  push @CCFLAGS, "-Wno-deprecated";
}


# smash the list together to make a string
$CCFLAGS = join(' ', @CCFLAGS);


# does the compiler want me to pass "-I."?  unfortunately, some versions
# of gcc-3 will emit an annoying warning if I pass "-I." when I don't need to
print("checking whether compiler needs \"-I.\"... ");
if (0!=system("g++ -c $CCFLAGS cc2/testprog.cc >/dev/null 2>&1")) {
  # failed without "-I.", so try adding it
  if (0!=system("g++ -c -I. $CCFLAGS cc2/testprog.cc >/dev/null 2>&1")) {
    my $wd = `pwd`;
    chomp($wd);
    die "\n" .
        "I was unable to compile a simple test program.  I tried:\n" .
        "  cd $wd\n" .
        "  g++ -c -I. $CCFLAGS cc2/testprog.cc\n" .
        "Try it yourself to see the error message.  This needs be fixed\n" .
        "before Elkhound will compile.\n";
  }

  # adding "-I." fixed the problem
  print("yes\n");
  push @CCFLAGS, "-I.";
  $CCFLAGS = join(' ', @CCFLAGS);
}
else {
  print("no\n");
}


# ------------------ check for needed components ----------------
# smbase
if (! -f "$SMBASE/nonport.h") {
  die "I cannot find nonport.h in `$SMBASE'.\n" .
      "The smbase library is required for elkhound.\n" .
      "If it's in a different location, use the -smbase=<dir> option.\n";
}

# ast
if (! -f "$AST/asthelp.h") {
  die "I cannot find asthelp.h in `$AST'.\n" .
      "The ast library is required for elkhound.\n" .
      "If it's in a different location, use the -ast=<dir> option.\n";
}


# ------------------ config.summary -----------------
# create a program to summarize the configuration
open(OUT, ">config.summary") or die("can't make config.summary");
print OUT (<<"OUTER_EOF");
#!/bin/sh
# config.summary

cat <<EOF
./configure command:
  $0 $originalArgs

Elkhound configuration summary:
  debug:       $debug
  loc:         $loc
  compression: $compflags

Compile flags:
  BASE_FLAGS:  $BASE_FLAGS
  CCFLAGS:     $CCFLAGS
  SMBASE:      $SMBASE
  AST:         $AST

EOF

OUTER_EOF

close(OUT) or die;
chmod 0755, "config.summary";


# ------------------- config.status ------------------
# from here on, combine BASE_FLAGS and CCFLAGS
$CCFLAGS = "$BASE_FLAGS $CCFLAGS";

# create a program which will create the Makefile
open(OUT, ">config.status") or die("can't make config.status");
print OUT (<<"OUTER_EOF");
#!/bin/sh
# config.status

# this file was created by ./configure

# report on configuration
./config.summary


echo "creating Makefile ..."

# overcome my chmod below
rm -f Makefile

cat >Makefile <<EOF
# Makefile for elkhound
# NOTE: do not edit; generated by:
#   $0 $originalArgs

EOF

# substitute variables
sed -e "s|\@CCFLAGS\@|$CCFLAGS|g" \\
    -e "s|\@SMBASE\@|$SMBASE|g" \\
    -e "s|\@AST\@|$AST|g" \\
  <Makefile.in >>Makefile || exit

# discourage editing
chmod a-w Makefile


cat >glrconfig.h.tmp <<EOF
// glrconfig.h
// do not edit; generated by ./configure

EOF

sed -e "s|\@GLR_SOURCELOC\@|$loc|g" \\
    -e "s|\@eef\@|$eef|g" \\
    -e "s|\@gcs\@|$gcs|g" \\
  <glrconfig.h.in >>glrconfig.h.tmp

# see if the new glrconfig.h differs from the old; if not, then
# leave the old, so 'make' won't think something needs to be rebuilt
if diff glrconfig.h glrconfig.h.tmp >/dev/null 2>&1; then
  # leave it
  echo "glrconfig.h is unchanged"
else
  echo "creating glrconfig.h ..."

  # overwrite it, and make it read-only
  mv -f glrconfig.h.tmp glrconfig.h
  chmod a-w glrconfig.h
fi


OUTER_EOF

close(OUT) or die;
chmod 0755, "config.status";


# ----------------- final actions -----------------
# invoke sub-configures
if ($subconfigure) {
  chdir("c") or die;
  my $tmp = join(' ', ("./configure", @c_args));
  print("Invoking $tmp in 'c' directory..\n");
  run("./configure", @c_args);
  chdir("..") or die;
}

# run the output file generator
run("./config.status");

print("\nYou can now run make, usually called 'make' or 'gmake'.\n");


exit(0);


# ------------------ subroutines --------------
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


sub slurpFile {
  my ($fname) = @_;
  open(IN, "<$fname") or die("can't open $fname: $!\n");
  my @ret = <IN>;
  close(IN) or die;
  return @ret;
}
