#!/usr/bin/perl

my $subdir;
my %rev;

while(<>) {

    if(/^Exported/ || /^Fetching/) {
	if(/^Fetching/){
	    s|Fetching.*upstream/||;
	    s/\'\n//;
	    if( $subdir eq "") {
		$subdir=$_;
	    } else {
		die "subdir $subdir without revision!!";
	    }
	}
	if(/Exported/){
	    s|Exported.* revision ||;
            s|\.\n||;
	    if( $subdir eq "") {
		if( $top_revision eq "") {
		    $top_revision=$_;
		} else {
		    die "revision $_ without subdir";
		}
	    } else {
		$rev{$subdir}=$_;
		$subdir="";
	    }
	}
    }
}

if(!$top_revision) {
    die "elsa-stack top revision missing";
}

if(keys(%rev) != 4){
    die "more than four subdirectories";
}

if(!($rev{"smbase"} && $rev{"ast"} && $rev{"elsa"} && $rev{"elkhound"})){
    die "missing some subdir";
}

my $smbase_rev=$rev{"smbase"};
my $ast_rev=$rev{"ast"};
my $elk_rev=$rev{"elkhound"};
my $elsa_rev=$rev{"elsa"};

($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);

$mon+=1;
$year+=1900;

#print "year $year mon $mon day $mday\n";

print "import upstream version elsa-stack: $top_revision  ",
    "smbase: $smbase_rev  ",
    "ast: $ast_rev  ",
    "elkhound: $elk_rev  ",
    "elsa: $elsa_rev",
    "\n";

printf "svn-%04d-%02d-%02d-elsa-stack-%s-smbase-%s-ast-%s-elkhound-%s-elsa-%s\n",
    ($year, $mon, $mday, $top_revision, $smbase_rev,
     $ast_rev, $elk_rev, $elsa_rev);
