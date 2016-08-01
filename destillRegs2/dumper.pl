###############################################################################
# dumper.pl is part of the destillregs packet in AskSin library.
# With dumper.pl you are able to generate an overview of all xml device files
# in a given directory. The overview includes device name, id, model id, etc. 
###############################################################################

use strict;
use warnings;

use destillRegs;
use subs qw(DEBUG);

## check if all needed libraries installed
die "XML::LibXML missing, please install via cpan...\n" if not eval{require XML::LibXML};
die "XML::Hash missing, please install via cpan...\n" if not eval{require XML::Hash};
die "JSON missing, please install via cpan...\n" if not eval{require JSON};
die "Time::HiRes missing, please install via cpan...\n" if not eval{require Time::HiRes};
die "Data::Dumper::Simple missing, please install via cpan...\n" if not eval{require Data::Dumper::Simple};

## handover commandline
#my $filename = shift or die "Usage: dumper.pl FILEPATH\n";


## generates an overview of all existing xml device descriptions
print "please wait some seconds, your request is processed...\n";
my $ret = gen_xml_dev_list();
print "--------------------------------------------------------------------------------------------\n";
print " Overview of all available xml device file \n";
print " copy and paste into excel to make it easier to read \n";
print "--------------------------------------------------------------------------------------------\n";
print "\n";
print $ret;
print "--------------------------------------------------------------------------------------------\n";

