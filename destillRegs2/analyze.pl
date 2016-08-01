###############################################################################
# analyze.pl is part of the destillregs packet in AskSin library.
# With analyze.pl you are able to destill/extract device information 
# of a xml config file of a CCU2. It returns the register content
# as a struct of all channel/lists of a specific device.
# Can be used to understand better a device configuration or to rebuild 
# device functionallity wit the asksin lib.
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
my $filename = shift or die "Usage: analyze.pl FILENAME\n";

## check if it is a valid filepath
die "$filename is not valid, please check\n" if not -e $filename;

## ++ in progress ++ 
## generates a hash with all information to build a register.h
## todo: some default conversations to be implemented
#my $dev_info_hash = gen_xml_dev_info_hash($dev_info_xml_string);
my $dev_info_hash = gen_xml_dev_info_hash($filename);


## ++ in progress ++
## print register.h functions
print "--------------------------------------------------------------------------------------------\n";
print " This is the register.h content for the analysed xml file \n";
print " There are several information missing for a comlete register.h configuration \n";
print "--------------------------------------------------------------------------------------------\n";
print "\n";
print "#ifndef _REGISTER_h \n";
print " "x4 ."#define _REGISTER_h \n\n";
print_device_ident($$dev_info_hash{'devIdnt'});
print_channel_registers($$dev_info_hash{'devCnlAddr'});
print_channel_table($$dev_info_hash{'devCnlAddr'});
print_device_description($$dev_info_hash{'devInfo'});
print_module_register_table($$dev_info_hash{'devInfo'});
print "#endif \n\n";

print "--------------------------------------------------------------------------------------------\n";
print " This are some additional information especially for developers of user modules \n";
print " The channel structs reflecting the register content, the frame section shows \n";
print " which message types are used for the device functionallity \n";
print "--------------------------------------------------------------------------------------------\n";
print "\n";
print_channel_structs($$dev_info_hash{'devCnlLst'}, $$dev_info_hash{'devCnlAddr'});
print_device_frames($$dev_info_hash{'devFrames'});



