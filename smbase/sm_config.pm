# sm_config.pm
# Perl module for package configuration
#
# For now, the idea is simply to have a common place to put
# a library of routines that my individual directory configure
# scripts can use.

# autoflush so progress reports work
$| = 1;

# smbase config version number
sub get_sm_config_version {
  return 1.02;
}

# use smbase's $BASE_FLAGS if I can find them
sub get_smbase_BASE_FLAGS {
  my $smbase_flags = `$main::SMBASE/config.summary 2>/dev/null | grep BASE_FLAGS`;
  if (defined($smbase_flags)) {
    ($main::BASE_FLAGS = $smbase_flags) =~ s|^.*: *||;
    chomp($main::BASE_FLAGS);
  }
}

# detect whether we need PERLIO=crlf
sub getPerlEnvPrefix {
  # It's important to do this test on a file in the current
  # directory, since the behavior under cygwin can change
  # depending on the mount charactersics of the file's location.
  #
  # It is also important that the file be opened by the shell
  # instead of perl; when perl opens the file itself, things 
  # work differently.
  if (0!=system("perl -e 'print(\"a\\n\")' >tmp.txt")) {
    die;
  }

  my $sz1 = getFileSize("tmp.txt");
  if ($sz1 == 2) {
    # no LF->CRLF translation done on output, I think we're ok
    #print("(case 1) ");
    return "";
  }
  if ($sz1 != 3) {
    die("expected file size of 2 or 3");
  }

  open(TMP, "<tmp.txt") or die;
  my $line = <TMP>;
  close(TMP) or die;
  unlink("tmp.txt");

  if (length($line) == 2) {
    # reading did CRLF->LF, so again we're ok
    #print("(case 2) ");
    return "";
  }
  elsif (length($line) == 3) {
    # reading was raw, so if we wrote again then we'd have
    # CRCRLF; this is the condition that "PERLIO=crlf" fixes
    #print("(case 3) ");
    
    # make this the default for *this* perl session too;
    # but it does not work ...
    #use open OUT => ":crlf";

    return "PERLIO=crlf ";
  }
  else {
    die("expected line length of 2 or 3");
  }
}

sub getFileSize {
  my ($fname) = @_;

  my @details = stat($fname);

  # how do I tell if 'stat' failed?

  return $details[7];
}

sub get_PERL_variable {
  print("checking how to run perl... ");

  my $ret = getPerlEnvPrefix() . "perl";

  print($ret . "\n");
  return $ret;
}

                      
1;
# EOF
