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


1;
# EOF
