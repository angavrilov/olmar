#!/usr/bin/perl -w
# send predicates and axioms to Simplify, and verify the
# responses are as expected

use strict 'subs';

use FileHandle;          # autoflush?
use IPC::Open2;          # open2 call

if (@ARGV != 1) {
  print("usage: $0 file1.sx\n");
  exit(0);
}

# open Simplify
my $pid = open2(*Reader, *Writer, "./run-Simplify -nosc");
Writer->autoflush();     # default here, actually

# feed it the background predicate
open(BG, "<bgpred.sx") or die("can't open bgpred.sx: $!\n");
while (defined($line = <BG>)) {
  print Writer ($line);
}
close(BG) or die;

# globals
$predicateCt = 0;
$errors = 0;

valid("true", "TRUE\n");
if (0) {
  notvalid("false", "FALSE\n");
  valid("false", "FALSE\n");
  notvalid("true", "TRUE\n");
  valid("true", "TRUE\n");
}


# work through experiments file
open(EXPER, "<$ARGV[0]") or die("can't open $ARGV[0]: $!\n");
while (defined($line = <EXPER>)) {
  chomp($line);

  if (($line =~ /^\s*$/) ||    # blank line
      ($line =~ /^\s*;/)) {    # comment
    next;
  }

  my ($inv, $val, $dummy2, $comment) =
    ($line =~ /^\s*(not)?(valid)\s+(\"(.*)\"\s+)?\(\s*$/);
  if (defined($val)) {     # predicate
    if ($inv) {
      notvalid($comment, collectPred());
    }
    else {
      valid($comment, collectPred());
    }
    next;
  }
  
  if ($line =~ /^\s*bg_push\s+\(\s*$/) {     # bg_push
    my @pred = collectPred();
    print Writer ("(BG_PUSH (AND\n", @pred, "))\n");
    next;
  }

  if ($line =~ /^\s*bg_pop\s*$/) {
    print Writer ("(BG_POP)\n");
    next;
  }

  print("not understood: $line\n");
  pretendUsed($dummy2);
}

# get all remaining responses
close(Writer);
@resp = <Reader>;
print(@resp);

pretendUsed($pid);

if ($errors > 0) {
  print("$errors errors\n");
  exit(2);
}
else {
  exit(0);
}


# ----------------------- subroutines ------------------------
sub collectPred {
  my @ret = ();
  my $line;

  while (defined($line = <EXPER>)) {
    if ($line =~ /^\)\s*$/) {   # done
      return @ret;
    }
    @ret = (@ret, $line);
  }

  print("unexpected EOF reading predicate:\n@ret\n");
  exit(2);
}


sub pretendUsed {
}

# give a single predicate to Simplify, and return 1 if it proves it
# and 0 if it cannot prove it
sub prove {
  my ($comment, @pred) = @_;

  # say what's going on
  $predicateCt++;
  print("$predicateCt: ");
  if ($comment) {
    print("$comment... ");
  }
  else {
    print("predicate... ");
  }
  STDOUT->flush();
                       
  # hand predicate to Simplify
  print Writer (@pred);
  $resp = <Reader>;
  if (defined($resp)) {
    chomp($resp);
    if ($resp =~ / Valid./) {
      print("proved");
      $blank = <Reader>;     # eat blank line
      return 1;
    }
    if ($resp =~ / Invalid./) {
      print("not proved");
      $blank = <Reader>;
      return 0;
    }

    print("\n  unexpected response: $resp\n");
  }
  else {
    print("\n  pipe EOF!\n");
  }
  return 0;
}


# try to prove a predicate; expect success iff $resp is true
sub expect {
  my ($resp, $comment, @pred) = @_;
  if (!$resp != !prove($comment, @pred)) {    # normalize booleans with "!"
    # response is opposite what I expected
    print(" (UNEXPECTED)\n");
    $errors++;

    # print failing predicate
    my @lines = split('\n', join('', @pred));
    for (my $i=0; $i < @lines; $i++) {
      #chomp($lines[$i]);        # I think split removes newlines
      print("  $lines[$i]\n");
    }
  }
  else {
    # ok
    print("\n");
  }
}


# try to prove a predicate we think is valid
sub valid {
  my ($comment, @pred) = @_;
  print("valid:    ");
  expect(1, $comment, @pred);
}

# try to prove a predicate we think is not valid or not provable
# (I avoid the term 'invalid', which connotes "provably false")
sub notvalid {
  my ($comment, @pred) = @_;
  print("notvalid: ");
  expect(0, $comment, @pred);
}


