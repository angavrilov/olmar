#!/usr/bin/perl -w
# configure script for smbase

use strict 'subs';

sub usage {
  print(<<"EOF");
usage: ./configure [options]
options:
  -h:                print this message
  -debug,-nodebug:   enable/disable debugging options [disabled]
  -no-dash-g         disable -g
  -no-dash-O2        disable -O2
  -prof              enable profiling
  -devel             add options useful while developing smbase
  -debugheap         turn on heap usage debugging
  -traceheap         print messages on each malloc and free
  -ccflag <arg>      add <arg> to gcc command line
EOF
}

# autoflush so progress reports work
$| = 1;

# defaults
$BASE_FLAGS = "-Wall -Wno-deprecated -D__UNIX__";
$CCFLAGS = ();
$DEBUG_HEAP = 0;
$TRACE_HEAP = 0;
$debug = 0;
$use_dash_g = 1;
$allow_dash_O2 = 1;


# process command-line arguments
while (@ARGV) {
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
  }
  elsif ($arg eq "-ccflag") {
    push @CCFLAGS, $ARGV[0];
    shift @ARGV;
  }

  elsif ($arg eq "-d" ||
         $arg eq "-debug") {
    $debug = 1;
  }
  elsif ($arg eq "-nodebug") {
    $debug = 0;
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

$os = `uname -s`;
chomp($os);
if ($os eq "Linux") {
  push @CCFLAGS, "-D__LINUX__";
}

# smash the list together to make a string
$CCFLAGS = join(' ', @CCFLAGS);


# -------------- does the C++ compiler work? --------------
$wd = `pwd`;
chomp($wd);

print("Testing C++ compiler ...\n");
$cmd = "g++ -o testcout $BASE_FLAGS $CCFLAGS testcout.cc";
if (system($cmd)) {
  # maybe problem is -Wno-deprecated?
  printf("Trying without -Wno-deprecated ...\n");
  $BASE_FLAGS =~ s| -Wno-deprecated||;
  $cmd = "g++ -o testcout $BASE_FLAGS $CCFLAGS testcout.cc";
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

if (system("./testcout")) {
  print(<<"EOF");

I was able to compile testcout.cc, but it did not run.  I tried:
  cd $wd
  $cmd

and then
  ./testcout      (this one failed)

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

print("C++ compiler seems to work\n\n");


# ---------------------- etags? ---------------------
print("checking for etags... ");
if (system("type etags >/dev/null 2>&1")) {
  # doesn't have etags; cygwin is an example of such a system
  print("not found\n");
  $ETAGS = "true";       # 'true' is a no-op
}
elsif (system("etags --help | grep -- --members >/dev/null")) {
  # has it, but it does not know about the --members option
  print("etags\n");
  $ETAGS = "etags";
}
else {
  # assume if it knows about --members it knows about --typedefs too
  print("etags --members --typedefs\n");
  $ETAGS = "etags --members --typedefs";
}


# ------------------ config.summary -----------------
# create a program to summarize the configuration
open(OUT, ">config.summary") or die("can't make config.summary");
print OUT (<<"EOF");
#!/bin/sh
# config.summary

echo "smbase configuration summary:"
echo "  debug:       $debug"
echo ""
echo "Compile flags:"
echo "  BASE_FLAGS:  $BASE_FLAGS"
echo "  CCFLAGS:     $CCFLAGS"
echo "  DEBUG_HEAP:  $DEBUG_HEAP"
echo "  TRACE_HEAP:  $TRACE_HEAP"
echo ""
EOF

close(OUT) or die;
chmod 0755, "config.summary";


# ------------------- config.status ------------------
# from here on, combine BASE_FLAGS and CCFLAGS
$CCFLAGS = "$BASE_FLAGS $CCFLAGS";

# make a variant, CFLAGS, that doesn't include -Wno-deprecated
$CFLAGS = $CCFLAGS;
$CFLAGS =~ s| -Wno-deprecated||;

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
sed -e "s|\@CCFLAGS\@|$CCFLAGS|g" \\
    -e "s|\@CFLAGS\@|$CFLAGS|g" \\
    -e "s|\@DEBUG_HEAP\@|$DEBUG_HEAP|g" \\
    -e "s|\@TRACE_HEAP\@|$TRACE_HEAP|g" \\
    -e "s|\@ETAGS\@|$ETAGS|g" \\
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


# ---------------- subroutines -------------
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
