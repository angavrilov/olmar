#!/usr/bin/perl -w
# scan some C++ source files for their module dependencies,
# output a description in the Dot format

# NOTE: throughout, when file names are passed or stored, they
# are stored *without* path qualification; all files are in
# either the current directory or one of the directories 
# specified with -I (and hence in @incdirs)

# list of directories to search for #include files in
@incdirs = ();

# hashtable (value 1 when inserted) of files to exclude
%excluded = ();

# when I print the info for a node, I put a 1 in this hashtable
%nodes = ();

# when I explore the edges from a node, I put a 1 in this table
%explored = ();
$recursive = 0;

# list of files to explore
@toExplore = ();

# true to print debug messages
$debug = 0;

# true to suppress warning messages
$quiet = 0;


sub usage {
  print(<<"EOF");
usage: $0 [options] sourcefile [sourcefile ...]

This program reads each of the source files specified on the command
line, and emits a graph of their interdependencies in the Dot graph
file format.  The source files should *not* be qualified with a path,
but instead be inside some directory specified with -I.

Options:
  -I<dir>     Add <dir> to the list of directories to search when
              looking for \#included files (files which are included
              but not found are not printed as dependencies).

  -X<name>    Exclude module <name> from the graph.

  -r          Recursively follows dependencies for files we find.

  -q          Suppress warnings.
  
  -d          Enable debug messages.

  -help       Print this usage string.

EOF
  exit(0);
}


# error messages
sub error {
  print STDERR ("error: ", @_, "\n");
  exit(2);
}

# warnings the user might be interested in
sub warning {
  if (!$quiet) {
    print STDERR ("warning: ", @_, "\n");
  }
}

# debug messages
sub diagnostic {
  if ($debug) {
    print STDERR ("diagnostic: ", @_, "\n");
  }
}



# return true if the argument file exists in the current directory
# or any of the directories in @incdirs; return value is the name
# under which the file was found, if it was
sub fileExists {
  my ($fname) = @_;

  if (-e $fname) {
    return $fname;
  }

  foreach $dir (@incdirs) {
    if (-e "$dir/$fname") {
      return "$dir/$fname";
    }
  }

  diagnostic("didn't find $fname");
  return "";   # false
}


sub isHeader {
  my ($filename) = @_;
  return ($filename =~ m/\.h$/);
}


# print the node info for the argument filename, if it
# hasn't already been printed
sub addNode {
  my ($filename) = @_;

  if (!defined($nodes{$filename})) {
    # print info for this node
    $nodes{$filename} = 1;
    print("  \"$filename\" [\n");

    if (isHeader($filename)) {
      print("    color = white\n");
    }

    print("  ]\n");
  }
}


# given a file name, strip its path if there is one (this is what
# /bin/basename does, also)
sub basename {
  my ($fname) = @_;

  my ($ret) = ($fname =~ m,/([^/]*)$,);
  if (defined($ret)) {
    # there was a slash, and we stripped it and everything before it
    return $ret;
  }
  else {
    # no slash
    return $fname;
  }
}


# given a file name, possibly with path, return the string of
# characters after the last slash (if any) but before the first dot
# which follows the last slash (if any)
sub prefix {
  my ($fname) = @_;
  $fname = basename($fname);

  # no slash now; get everything up to first dot
  ($ret) = ($fname =~ m,^([^.]*),);
  #diagnostic("prefix($fname) = $ret");
  return $ret;
}


# add an edge between the two nodes; if the third argument
# is true, then this is a link-time dependency (which is
# printed with a dashed line)
sub addEdge {
  my ($src, $dest, $isLinkTime) = @_;

  if (defined($excluded{$dest})) {
    diagnostic("skipping $dest because it is excluded");
    return;
  }

  if ($recursive) {
    # arrange to explore this later
    @toExplore = (@toExplore, $dest);
  }

  addNode($src);
  addNode($dest);

  print("  \"$src\" -> \"$dest\" [\n");

  if ($isLinkTime) {
    print("    style = dashed\n");
  }

  # if src and dest have the same base name, use a large
  # weight so they will be close together visually
  if (prefix($src) eq prefix($dest)) {
    print("    weight = 10\n");
  }

  print("  ]\n");
}


# explore the edges emanating from this file; expect a filename
# without path
sub exploreFile {
  my ($filename) = @_;
  my $withPath = fileExists($filename);
  if (!$withPath) {
    warning("does not exist: $filename");
    return;
  }

  if (defined($explored{$filename})) {
    return;
  }
  $explored{$filename} = 1;

  addNode($filename);

  # if it's a header, and the corresponding .cc file exists,
  # then add a link-time dependency edge
  if (isHeader($filename)) {
    my $corresp = $filename;
    $corresp =~ s/\.h$/\.cc/;
    $corresp = basename($corresp);
    if (fileExists($corresp)) {
      addEdge($filename, $corresp, 1);
    }
    else {
      # try .cpp too
      $corresp = $filename;
      $corresp =~ s/\.h$/\.cpp/;
      $corresp = basename($corresp);
      if (fileExists($corresp)) {
        addEdge($filename, $corresp, 1);
      }
    }
  }

  # scan the file for its edges
  if (!open(IN, "<$withPath")) {
    warning("cannot read $withPath: $!");
    next;    # skip it
  }

  my $line;
  while (defined($line = <IN>)) {
    # is this an #include line?
    my ($target) = ($line =~ m/^\#include \"([^\"]*)\"/);
    if (defined($target)) {
      $target = basename($target);
      if (fileExists($target)) {
        # add a compile-time edge
        addEdge($filename, $target, 0);
      }
    }

    # is this a comment specifying a link-time dependency?
    ($target) = ($line =~ m/linkdepend: (.*)$/);
    if (defined($target)) {
      $target = basename($target);

      # add a link-time edge; do so even if the file can't
      # be found, because it's clearly important if the
      # programmer went to the trouble of mentioning it
      addEdge($filename, $target, 1);

      if (!fileExists($target)) {
        warning("could not find linkdepend: $target\n");
      }
    }
  }

  close(IN) or die;
}


# ---------------------- main ------------------------
# process arguments
while (@ARGV >= 1 &&
       ($ARGV[0] =~ m/^-/)) {
  my $option = $ARGV[0];
  shift @ARGV;

  my ($arg) = ($option =~ m/-I(.*)/);
  if (defined($arg)) {
    @incdirs = (@incdirs, $arg);
    next;
  }

  ($arg) = ($option =~ m/-X(.*)/);
  if (defined($arg)) {
    $excluded{$arg} = 1;
    next;
  }

  if ($option eq "-r") {
    $recursive = 1;
    next;
  }

  if ($option eq "-q") {
    $quiet = 1;
    next;
  }

  if ($option eq "-d") {
    $debug = 1;
    next;
  }

  if ($option eq "-help") {
    usage();
  }

  error("unknown option: $option");
}

if (@ARGV == 0) {
  usage();
}


# graph preamble
print(<<"EOF");
// dependency graph automatically produced by $0

digraph "Dependencies" {
EOF

# nodes and edges
@toExplore = @ARGV;
while (@toExplore != 0) {
  my $filename = $toExplore[0];
  shift @toExplore;

  if ($filename =~ m,/,) {
    warning("should not use paths in arguments: $filename\n");
    $filename = basename($filename);
  }

  exploreFile($filename);
}

# finish the graph
print("}\n");

exit(0);

