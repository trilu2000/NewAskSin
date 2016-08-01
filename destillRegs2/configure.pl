###############################################################################
# configure.pl is part of the destillregs packet in AskSin library.
# With configure.pl you are able to destill device information by
# a given device configuration. The device configuration has to be in a 
# common structure as you can see in the enclosed examples.
# Wit configure.pl and the asksin lib you can design new HM compatible 
# devices with a mixed functionallity - combing dimmers with switches or 
# remotes or sensors. configure.pl generates the necassary register.h file 
# for the arduino part, but also the xml-file for your CCU2 or Homegear
# installation.
###############################################################################

use strict;
use warnings;

use Data::Dumper::Simple;
use JSON;
use destillRegs;
use subs qw(DEBUG);

## check if all needed libraries installed
die "XML::LibXML missing, please install via cpan...\n" if not eval{require XML::LibXML};
die "XML::Hash missing, please install via cpan...\n" if not eval{require XML::Hash};
die "JSON missing, please install via cpan...\n" if not eval{require JSON};
die "Time::HiRes missing, please install via cpan...\n" if not eval{require Time::HiRes};
die "Data::Dumper::Simple missing, please install via cpan...\n" if not eval{require Data::Dumper::Simple};

## generates a new xml file based on a json device definition
my $testconfig =  << 'END_LINE';
{"base_config": {
    "@comment":        "Das ist ein test", 
    "hm_ID":           "001122",
    "serial_ID":       "HBremote01",
    "model_ID":        "00A9",
    "firmware_ver":    "11"
    },

 "extended_config": {
    "id":              "testID",
    "name":            "testName",
    "dutycycle":       1,
    "conf_key_mode":   2,
    "power_mode":      0,

    "lowbat_signal":   1,
    "lowbat_limit":    30,
    "lowbat_timer":    3600000,

    "lowbat_register": 1, 
    "lowbat_min":      10,
    "lowbat_max":      40
    },

 "channels": [
    { "type": "Master",    "peers": 0, "hidden": 0, "linked": 0 },
    { "type": "xmlRemote", "peers": 6, "hidden": 0, "linked": 0 },
    { "type": "xmlRemote", "peers": 8, "hidden": 0, "linked": 0 },
    { "type": "xmlRemote", "peers": 4, "hidden": 0, "linked": 0 },
    { "type": "xmlRemote", "peers": 4, "hidden": 0, "linked": 0 }
    ]

}
END_LINE

## for debugging purpose only, use hash defined above, or load the commandline
#my $filename = $testconfig;
my $filename = shift or die "Usage: configure.pl FILENAME or JSON config string\n";

## check if we got a valid filename/path and load
my $filecontent = load_file_by_name($filename); 
## if $filecontent is undef it could be because we got the json-config as a string 
$filecontent = $filename   if (!$filecontent);
## decode the json-config we got 
my $config = eval{decode_json $filecontent};
die "Cannot load json-config, please double check...\n" if (!$config);


###############################################################
## ++ working ++
## generates a xml device config file out of a given hash 
## describing the functionallity
## returns a string with the xml device content
my $dev_info_xml_string = gen_xml_device_info($config);
## todo: implement a download or automatic file creation function

## ++ in progress ++ 
## generates a hash with all information to build a register.h
## todo: some default conversations to be implemented
my $dev_info_hash = gen_xml_dev_info_hash($dev_info_xml_string);
#my $dev_info_hash = gen_xml_dev_info_hash('./devicetypes/rf_rc-4-2.xml');

## ++ working ++
## generates the device declaration and configuration for a
## register.h out of a given hash describing the functionallity
## returns a hash with a stage and config section and therin an
## array to add it to the respective register.h section
my $re_config = gen_reg_h_device_info($config);


## ++ in progress ++
## print register.h functions
## todo: implement a download or automatic file creation function
print "--------------------------------------------------------------------------------------------\n";
print " This are the xml file information for your created device \n";
print " copy and paste into the xml folder of the CCU or Homegear \n";
print "--------------------------------------------------------------------------------------------\n";
print "\n";
print "$dev_info_xml_string\n";
print "--------------------------------------------------------------------------------------------\n";
print "\n\n";

print "--------------------------------------------------------------------------------------------\n";
print " This is the register.h content for your created device \n";
print " copy and paste into the register.h file of your sketch \n";
print "--------------------------------------------------------------------------------------------\n";
print "\n";
print "#ifndef _REGISTER_h \n";
print " "x4 ."#define _REGISTER_h \n\n";
print_library($$re_config{'library'});
print_stage($$re_config{'stage'});
print_hm_serial_data($$re_config{'general'});
print_device_ident($$dev_info_hash{'devIdnt'});
print_channel_registers($$dev_info_hash{'devCnlAddr'});
print_channel_table($$dev_info_hash{'devCnlAddr'}, $$re_config{'peers'});
print_device_description($$dev_info_hash{'devInfo'});
print_module_register_table($$dev_info_hash{'devInfo'});
print_every_time_start($$re_config{'config'});
print_first_time_start();
print "#endif \n\n";

print "--------------------------------------------------------------------------------------------\n";
print " This are some additional information especially for developers of user modules \n";
print " The channel structs reflecting the register content, the frame section shows \n";
print " which message types are used for the device functionallity \n";
print "--------------------------------------------------------------------------------------------\n";
print "\n";
print_channel_structs($$dev_info_hash{'devCnlLst'}, $$dev_info_hash{'devCnlAddr'});
print_device_frames($$dev_info_hash{'devFrames'});



