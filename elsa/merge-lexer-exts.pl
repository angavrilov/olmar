#!/usr/bin/perl -w
# merge a base flexer lexer description with zero or more extensions

if (@ARGV == 0) {
  print(<<"EOF");
usage: $0 base.lex [extension.lex [...]] >merged.lex
EOF
  exit(0);
}

$base = $ARGV[0];
shift @ARGV;
                     
open(IN, "<$base") or die("cannot open $base: $!\n");
while (defined($line = <IN>)) {                                               
  # re-echo all, including marker line, to allow composition via
  # multiple runs
  print($line);

  if ($line =~ m/EXTENSION RULES GO HERE/) {
    # insert all extension modules; insert in reverse order to
    # preserve the idea that later files are extending earlier
    # files, and the last-added extension should come first so
    # it has total control
    for ($i = @ARGV-1; $i >= 0; $i--) {
      my $ext = $ARGV[$i];
      open(EXT, "<$ext") or die("cannot open $ext: $!\n");
      while (defined($extline = <EXT>)) {
        print($extline);
      }
      close(EXT) or die;
    }
  }
}

close(IN) or die;
exit(0);
