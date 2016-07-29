## input via JSON
## several functions to read content of the XML file

use strict;
use warnings;
use XML::LibXML;


#-------------------------------------------------------------------------------------------------------
#-- quit unless we have the correct number of command-line args ----------------------------------------
my $num_args = $#ARGV + 1;
if ($num_args != 2) {
    print "\nUsage: createRegisterFromFile.pl rf_file dev_id\n";
    exit;
}
 
#-- we got two command line args, so assume they are the rf_file and dev_id ----------------------------
my $rf_file=$ARGV[0];
my $dev_id=$ARGV[1];


## select mode and open json input file, or a XML device description file
 