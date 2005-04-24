#!/usr/bin/perl -w
# configure script for smbase

use strict 'subs';

require sm_config;

$dummy = get_sm_config_version();
print("sm_config version: $dummy\n");

@originalArgs = @ARGV;

sub usage {
  standardUsage();

  print(<<"EOF");
package options:
  -no-dash-g         disable -g
  -no-dash-O2        disable -O2
  -prof              enable profiling
  -devel             add options useful while developing smbase
  -debugheap         turn on heap usage debugging
  -traceheap         print messages on each malloc and free
EOF
# this option is obscure, so I won't print it in the usage string
# -icc               turn on options for Intel's compiler
}

# defaults (also see sm_config.pm)
$DEBUG_HEAP = 0;
$TRACE_HEAP = 0;
$use_dash_g = 1;
$allow_dash_O2 = 1;
$debug = $debug;   # silence warning


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

  elsif ($arg eq "-no-dash-g") {
    $use_dash_g = 0;
  }
  elsif ($arg eq "-no-dash-O2") {
    $allow_dash_O2 = 0;
  }

  elsif ($arg eq "-prof") {
    push @CCFLAGS, "-pg";
  }

  elsif ($arg eq "-devel") {
    push @CCFLAGS, "-Werror";
  }

  elsif ($arg eq "-debugheap") {
    $DEBUG_HEAP = 1;
  }
  elsif ($arg eq "-traceheap") {
    $TRACE_HEAP = 1;
  }

  # 9/19/04: I spent some time getting smbase to build under
  # the Intel C++ 8.1 compiler; these are the options I used.
  elsif ($arg eq "-icc") {
    # compiler executables
    $CC = "icc";
    $CXX = "icpc";

    # diagnostic suppression:
    #  444: Wants virtual destructors
    #  1418: external definition with no prior declaration
    #  810: Conversion might lose sig.digs (can't suppress with cast!)
    #  271: trailing comma is nonstandard
    #  981: operands are evaluated in unspecified order
    #  279: controlling expression is constant
    #  383: value copied to temporary, reference to temporary used
    #  327: NULL reference is not allowed
    #  1419: external declaration in primary source file
    push @CCFLAGS, "-wd444,1418,810,271,981,279,383,327,1419";
  }
  
  else {
    print STDERR ("unknown option: $arg\n");
    exit(2);
  }
}

if (!$debug) {
  if ($allow_dash_O2) {
    push @CCFLAGS, "-O2";
  }
  push @CCFLAGS, "-DNDEBUG";
}

if ($use_dash_g) {
  push @CCFLAGS, "-g";
}

finishedOptionProcessing();


# -------------- does the C++ compiler work? --------------
$wd = `pwd`;
chomp($wd);

$testcout = "testcout" . $exe;
print("Testing C++ compiler ...\n");
$cmd = "$CXX -o $testcout @CCFLAGS testcout.cc";
if (system($cmd)) {
  # maybe problem is -Wno-deprecated?
  printf("Trying without -Wno-deprecated ...\n");
  @CCFLAGS = grep { $_ ne "-Wno-deprecated" } @CCFLAGS;
  $cmd = "$CXX -o $testcout @CCFLAGS testcout.cc";
  if (system($cmd)) {
    print(<<"EOF");

I was unable to compile a really simple C++ program.  I tried:
  cd $wd
  $cmd

Please double-check your compiler installation.

Until this is fixed, smbase (and any software that depends on it) will
certainly not compile either.
EOF
    exit(2);
  }
}

if (!$target) {
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

Until this is fixed, smbase (and any software that depends on it) will
certainly not run either.
EOF
    exit(2);
  }

  print("C++ compiler seems to work\n");
}
else {
  print("because we are in cross mode, I will not try running '$testcout'\n",
        "but it might be a good idea to try that yourself\n");
}

print("\n");

# etags: see elsa/configure.pl


# ------------------ config.summary -----------------
# create a program to summarize the configuration
open(OUT, ">config.summary") or die("can't make config.summary");
print OUT (<<"EOF");
#!/bin/sh
# config.summary

echo "smbase configuration summary:"
echo "  command:     $0 @originalArgs"
echo ""
echo "Compile flags:"
echo "  CC:          $CC"
echo "  CXX:         $CXX"
echo "  CCFLAGS:     @CCFLAGS"
EOF

if ($DEBUG_HEAP) {
  print OUT ("echo \"  DEBUG_HEAP:  $DEBUG_HEAP\"\n");
}
if ($TRACE_HEAP) {
  print OUT ("echo \"  TRACE_HEAP:  $TRACE_HEAP\"\n");
}
if ($target) {
  print OUT ("echo \"  CROSSTARGET: $target\"\n");
}
if ($exe) {
  print OUT ("echo \"  EXE:         $exe\"\n");
}

print OUT ("echo \"\"\n");

close(OUT) or die;
chmod 0755, "config.summary";


# ------------------- config.status ------------------
# make a variant, CFLAGS, that doesn't include -Wno-deprecated
@CFLAGS = grep { $_ ne "-Wno-deprecated" } @CCFLAGS;

# create a program which will create the Makefile
open(OUT, ">config.status") or die("can't make config.status");
print OUT (<<"__OUTER_EOF__");
#!/bin/sh
# config.status

# this file was created by ./configure

# report on configuration
./config.summary

echo "creating Makefile ..."

# overcome my chmod below
rm -f Makefile

cat >Makefile <<EOF
# Makefile for smbase
# NOTE: generated by ./configure, do not edit

EOF


# substitute the CCFLAGS
sed -e "s|\@CCFLAGS\@|@CCFLAGS|g" \\
    -e "s|\@CFLAGS\@|@CFLAGS|g" \\
    -e "s|\@DEBUG_HEAP\@|$DEBUG_HEAP|g" \\
    -e "s|\@TRACE_HEAP\@|$TRACE_HEAP|g" \\
    -e "s|\@CC\@|$CC|g" \\
    -e "s|\@CXX\@|$CXX|g" \\
    -e "s|\@CROSSTARGET\@|$target|g" \\
    -e "s|\@EXE\@|$exe|g" \\
  <Makefile.in >>Makefile

# discourage editing ..
chmod a-w Makefile


__OUTER_EOF__

close(OUT) or die;
chmod 0755, "config.status";


# ----------------- final actions -----------------
# run the output file generator
run("./config.status");

print("\nYou can now run make, usually called 'make' or 'gmake'.\n");

exit(0);


# the code below is never executed as part of smbase/configure.pl;
# it is here so it can be easily found to copy into the client
# configuration scripts

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
