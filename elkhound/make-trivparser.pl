#!/usr/bin/perl -w
# given a partial grammar spec on input, write a grammar spec
# on output which is well-formed, and whose actions just print
# the productions that drive them

use strict 'subs';

if (@ARGV < 1) {
  print("usage: $0 grammarName\n");
  exit(0);
}
$name = $ARGV[0];


print("// automatically produced by $0\n",
      "// do not edit directly\n",
      "\n");


sub preamble {
  # insert standard preamble
  print(<<"EOF");

    verbatim $name [
      #include <iostream.h>     // cout

      class $name : public UserActions {
        #include "$name.gr.gen.h"
      };

      UserActions *makeUserActions()
      {
        return new $name;
      }
    ]

EOF
}


# add actions
while (defined($line = <STDIN>)) {
  # insert preamble right before terminals
  if ($line =~ /^\s*terminals/) {
    preamble();
    print($line);
    
    # add EOF terminal
    print("  0 : EOF ;\n");
    next;
  }

  # remember last-seen nonterm
  ($nonterm) = ($line =~ m/nonterm\s+(\S+)\s+/);
  if (defined($nonterm)) {
    if (!defined($curNT)) {
      # this is the first nonterminal; insert dummy start rule
      print("// dummy first rule\n",
            "nonterm DummyStart -> $nonterm EOF;\n",
            "\n");
    }

    $curNT = $nonterm;
    print($line);
    next;
  }

  # add actions to rules without them
  ($space, $rule) = ($line =~ /^(\s*)(->[^;]*);\s*$/);
  if (defined($rule)) {
    $len = length($space) + length($rule);
    print($space, $rule, " " x (30-$len),
          "[ cout << \"reduced by $curNT $rule\\n\"; return 0; ]\n");
    next;
  }
  
  # expand terminals (single letter with *no* semicolon, and possibly a comment)
  ($letter, $comment) = ($line =~ m'^\s*([a-z])\s*(//.*)?$');   #'
  if (defined($letter)) {
    if (!defined($comment)) {
      $comment = "";
    }
    printf("  %d : $letter ;   $comment\n", ord(uc($letter)));
    next;
  }
  
  print($line);
}

exit(0);
