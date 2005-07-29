#!/usr/bin/perl -w
use strict;

# filter out the source locations from the printAST
while(<STDIN>) {
  next if /^\s*loc =/;
  s|, at in/t\S+\.cc:\d+:\d+ \(0x.*\)$||;
  s|, at <noloc>:\d+:\d+ \(0x.*\)$||;
  # well, we filter other junk too
  s| \(\d+ overloadings?\)$||;
  next if /^\s*parse=.* tcheck=.* integ=.* elab=.*/;
  next if /^\s*typechecking results:/;
  next if /^\s*errors:/;
  next if /^\s*succ=/;
  next if /^\s*warnings:/;
  next if /^\s*no-typecheck/;
  next if /^\s*no-elaborate/;
  print;
}
