# sm_config.pm
# Perl module for package configuration
# 
# For now, the idea is simply to have a common place to put
# a library of routines that my individual directory configure
# scripts can use.

# smbase config version number
sub get_sm_config_version {
  return 1.01;
}

# use smbase's $BASE_FLAGS if I can find them
sub get_smbase_BASE_FLAGS {
  my $smbase_flags = `$main::SMBASE/config.summary 2>/dev/null | grep BASE_FLAGS`;
  if (defined($smbase_flags)) {
    ($main::BASE_FLAGS = $smbase_flags) =~ s|^.*: *||;
    chomp($main::BASE_FLAGS);
  }
}


1;
# EOF
