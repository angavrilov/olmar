#!/usr/bin/perl -w
# configure script for elsa

@elsa_args = grep { !(/^-cxx=/ || /^-memcheck/ || /^-no-ocamlopt/) } @ARGV;
$elsa_arg_vec = join(" ", map {"'" . $_ . "'" } @elsa_args);

@olmar_args = @ARGV;
$cxx_env=$ENV{"CXX"};
if(defined($cxx_env)) {
    push @olmar_args, ("-cxx=" . $cxx_env);
}
$olmar_arg_vec = join(" ", map {"'" . $_ . "'" } @olmar_args);

#print "elsa: $elsa_arg_vec\n", "olmar: $olmar_arg_vec\n";

print "\n\n=========================================================\n\n";
system("cd smbase && ./configure " . $elsa_arg_vec) == 0 || die;
print "\n\n=========================================================\n\n";
system("cd ast && ./configure " . $elsa_arg_vec) == 0 || die;
print "\n\n=========================================================\n\n";
system("cd elkhound && ./configure " . $elsa_arg_vec) == 0 || die;
print "\n\n=========================================================\n\n";
system("cd elsa && ./configure " . $elsa_arg_vec) == 0 || die;
print "\n\n=========================================================\n\n";
system("cd olmar && ./configure " . $olmar_arg_vec) == 0 || die;

