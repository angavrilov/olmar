#!/usr/bin/perl -w
# run ccparse on an input file, then (if the input has some
# stylized comments) re-run, expecting various kinds of errors

use strict 'subs';
use Config;

if (@ARGV == 0) {
  print(<<"EOF");
usage: $0 ./ccparse [-tr flags] input.cc

This will first invoke the command line as given, expecting
that to succeed.

Then, it will scan input.cc for any lines of the form:

  //ERROR(n): <some C++ code>

If it finds them, then for each such 'n' the lines ERROR(n)
will be uncommented (and "ERROR(n)" removed), and the original
command executed again.  These additional runs should fail.
EOF

  exit(0);
}


# excerpt from perlipc man page
defined $Config{sig_name} || die "No sigs?";
$i = 0;
foreach $name (split(' ', $Config{sig_name})) {
  $signo{$name} = $i;
  $signame[$i] = $name;
  $i++;
}

$sigint = $signo{INT};
$sigquit = $signo{QUIT};
#print("sigint: $sigint\n");


$fname = $ARGV[@ARGV - 1];
#print("fname: $fname\n");

# try once with no modifications
$code = mysystem(@ARGV);
if ($code != 0) {
  exit($code);
}

# read the input file
open(IN, "<$fname") or die("can't open $fname: $!\n");
@lines = <IN>;
close(IN) or die;

# see what ERROR lines are present
%codes = ();
foreach $line (@lines) {
  my ($code) = ($line =~ m|^\s*//ERROR\((\d+)\):|);
  if (defined($code)) {
    $codes{$code} = 1;
  }
}

# get sorted keys
@allkeys = (sort {$a <=> $b} (keys %codes));
$numkeys = @allkeys;
if ($numkeys == 0) {
  # no error tags
  exit(0);
}

# consider each in turn
foreach $selcode (@allkeys) {
  print("-- selecting ERROR($selcode) --\n");

  # run through the lines in the file, generating a new file
  # that has the selected lines uncommented
  open(OUT, ">multitest.tmp") or die("can't create multitest.tmp: $!\n");
  foreach $line (@lines) {
    my ($code, $rest) = ($line =~ m|^\s*//ERROR\((\d+)\):(.*)$|);
    if (defined($code) && $selcode == $code) {
      print OUT ($rest, "\n");   # uncomment
    }
    else {
      print OUT ($line);         # emit as-is
    }
  }
  close(OUT) or die;

  # run the command on the new input file
  @args = @ARGV;
  $args[@ARGV - 1] = "multitest.tmp";

  #print("command: ", join(' ', @args), "\n");
  $code = mysystem(@args);
  if ($code == 0) {
    print("ERROR($selcode): expected this to fail:\n",
          "  ", join(' ', @args), "\n");
    exit(4);
  }
  else {
    print("$selcode: failed as expected\n");
  }
}

unlink("multitest.tmp");

print("\nmultitest: success: all $numkeys variations failed as expected\n");

exit(0);


# like 'system', except return a proper exit code, and
# propagate fatal signals (esp. ctrl-C)
sub mysystem {
  my @args = @_;

  my $code = system(@args);
  if ($code == 0) { return $code; }

  my $sig = $code & 127;
  if ($sig != 0) {
    if ($sig == $sigquit || $sig == $sigint) {
      # subprocess died to user-originated signal; kill myself
      # the same way
      #print("killing myself with $sig\n");
      kill $sig, $$;
    }

    # some other signal
    die "child died with signal $signame[$sig]\n";
  }

  return $code >> 8;
}


# EOF
