#!/usr/bin/perl -w
# configure script for ast

use strict 'subs';

# default location of smbase relative to this package
$SMBASE = "../smbase";
$req_smcv = 1.01;            # required sm_config version number

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
$BASE_FLAGS = "-Wall -Wno-deprecated -D__UNIX__";
$CCFLAGS = ();
$debug = 0;
$use_dash_g = 1;
$allow_dash_O2 = 1;

sub usage {
  print(<<"EOF");
usage: ./configure [options]
options:
  -h:                print this message
  -debug,-nodebug:   enable/disable debugging options [disabled]
  -no-dash-g         disable -g
  -no-dash-O2        disable -O2
  -prof              enable profiling
  -devel             add options useful while developing
  -ccflag <arg>      add <arg> to gcc command line
  -smbase=<dir>:     specify where the smbase library is [$SMBASE]
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

  elsif (($tmp) = ($arg =~ m/^-smbase=(.*)$/)) {
    $SMBASE = $tmp;
  }

  else {
    die "unknown option: $arg\n";
  }
}

if (!$debug) {
  if ($allow_dash_O2) {
    push @CCFLAGS, "-O2";
  }
  push @CCFLAGS, "-DNDEBUG";
}

if ($use_dash_g) {
  unshift @CCFLAGS, "-g";
}

$os = `uname -s`;
chomp($os);
if ($os eq "Linux") {
  push @CCFLAGS, "-D__LINUX__";
}

# smash the list together to make a string
$CCFLAGS = join(' ', @CCFLAGS);


# ------------------ check for needed components ----------------
# smbase
if (! -f "$SMBASE/nonport.h") {
  die "I cannot find nonport.h in `$SMBASE'.\n" .
      "The smbase library is required for ast.\n" .
      "If it's in a different location, use the -smbase=<dir> option.\n";
}

# I previously had a check for libsmbase.a, but that prevents
# configuring all software from one master configure script, so
# I've removed that.

# use smbase's $BASE_FLAGS if I can find them
$smbase_flags = `$SMBASE/config.summary 2>/dev/null | grep BASE_FLAGS`;
if (defined($smbase_flags)) {
  ($BASE_FLAGS = $smbase_flags) =~ s|^.*: *||;
  chomp($BASE_FLAGS);
}


# etags: see elsa/configure.pl


# ------------------ config.summary -----------------
# create a program to summarize the configuration
open(OUT, ">config.summary") or die("can't make config.summary");
print OUT (<<"EOF");
#!/bin/sh
# config.summary

echo "./configure command:"
echo "  $0 $originalArgs"
echo ""
echo "ast configuration summary:"
echo "  debug:       $debug"
echo ""
echo "Compile flags:"
echo "  BASE_FLAGS:  $BASE_FLAGS"
echo "  CCFLAGS:     $CCFLAGS"
echo "  SMBASE:      $SMBASE"
echo ""
EOF

close(OUT) or die;
chmod 0755, "config.summary";


# ------------------- config.status ------------------
# from here on, combine BASE_FLAGS and CCFLAGS
$CCFLAGS = "$BASE_FLAGS $CCFLAGS";

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
# Makefile for ast
# NOTE: do not edit; generated by:
#   $0 $originalArgs

EOF


# substitute variables
sed -e "s|\@CCFLAGS\@|$CCFLAGS|g" \\
    -e "s|\@SMBASE\@|$SMBASE|g" \\
  <Makefile.in >>Makefile || exit

# discourage editing ..
chmod a-w Makefile


__OUTER_EOF__

close(OUT) or die;
chmod 0755, "config.status";


# ----------------- final actions -----------------
# run the output file generator
my $code = system("./config.status");
if ($code != 0) {
  # hopefully ./config.status has already printed a message,
  # I'll just relay the status code
  if ($code >> 8) {                
    exit($code >> 8);
  }
  else {
    exit($code & 127);
  }
}


print("\nYou can now run make, usually called 'make' or 'gmake'.\n");

exit(0);
