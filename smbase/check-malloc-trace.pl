#!/usr/bin/perl -w
# check the output of a malloc trace for malloc usage bugs

use strict 'subs';

while ($line = <STDIN>) {
  if (($addr) = ($line =~ /yielded (0x.*)$/)) {
    #print ("malloc yielded $addr\n");

    if ($valid{$addr}) {
      print ("strange: malloc yielded already-valid $addr\n");
    }
    $valid{$addr} = 1;
  }

  elsif (($addr) = ($line =~ /free\((0x.*)\)$/)) {
    #print ("free of $addr\n");

    if ($valid{$addr}) {
      $valid{$addr} = 0;     # not valid now
    }
    else {
      print ("free of invalid addr $addr\n");
    }
  }
}
