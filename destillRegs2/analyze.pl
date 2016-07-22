use strict;
use warnings;
use XML::LibXML;
use Data::Dumper::Simple;
use JSON::XS;
use Time::HiRes;

use Time::HiRes qw(time);

use myModules;
use subs qw(DEBUG);

my $CONFIG_FROM_PERL_FILE=1;
my $CONFIG_FROM_JSON_FILE=0;
my $CONFIG_FROM_JSON_HANDLE=0;

my $AUTO_FIRMWARE_FROM_XML_FILE=1;


## get a file list of devicetypes
#my $fileList = get_file_list("devicetypes");
#DEBUG Dumper($fileList);

## returns a hash with the xml device file information
#my $xmlHash = get_xml_file_as_hash("devicetypes/rf_4dis.xml");
#DEBUG Dumper($xmlHash);

## generates an overview of all existing xml device descriptions
#my $ret = gen_xml_dev_list();
#DEBUG Dumper($ret);


## generates a new xml file based on a json device definition
#    "serial":       "HBremote01",
#    "hmID":         "",
#    "hmKEY":        "0102030405060708090a0b0c0d0e0f10",
	

my %config;
$config{'base_config'}{'modelID'} = 0x0011;
$config{'extended_config'}{'id'} = "testID";
$config{'extended_config'}{'name'} = "testName";
#$config{'extended_config'}{'dutycycle'} = 1;
$config{'extended_config'}{'powerMode'} = 2;

$config{'extended_config'}{'lowbat_signal'} = 1;      # bool
$config{'extended_config'}{'lowbat_limit'} = 30;      # in V * 10
$config{'extended_config'}{'lowbat_timer'} = 3600000; # in ms

$config{'extended_config'}{'lowbat_register'} = 1;    # bool
$config{'extended_config'}{'lowbat_min'} = 10;        # in V * 10
$config{'extended_config'}{'lowbat_max'} = 40;        # in V * 10


#$config{'channels'}[0] = {type => "Master",    peers => 0, hidden => 0, linked => 0 };
#$config{'channels'}[1] = {type => "xmlRemote", peers => 6, hidden => 0, linked => 0 };
#$config{'channels'}[2] = {type => "xmlRemote", peers => 8, hidden => 0, linked => 0 };
#$config{'channels'}[3] = {type => "xmlRemote", peers => 4, hidden => 0, linked => 0 };
#$config{'channels'}[5] = {type => "xmlRemote", peers => 4, hidden => 0, linked => 0 };

$config{'channels'}[1] = {type => "xmlRemote", peers => 4, hidden => 0, linked => 0 };
#$config{'channels'}[1] = {type => "xmlSwitch", peers => 4, hidden => 0, linked => 0 };
#$config{'channels'}[1] = {type => "xmlDimmer", peers => 4, hidden => 0, linked => 0 };
#$config{'channels'}[1] = {type => "xmlBlind", peers => 4, hidden => 0, linked => 0 };
#$config{'channels'}[1] = {type => "xmlMotion", peers => 4, hidden => 0, linked => 0 };
#$config{'channels'}[1] = {type => "xmlWeather", peers => 4, hidden => 0, linked => 0 };

my $ret = gen_xml_device_info(\%config);

###############################################################


## generates a hash with all information to build a register.h
#my $ret = gen_xml_dev_hash('./devicetypes/rf_rc-4-2.xml');
#my $ret = gen_xml_dev_hash('./devicetypes/rf_dim_2t_644_le_v2_4.xml');
#my $ret = gen_xml_dev_hash('./devicetypes/rf_central.xml');
#my $ret = gen_xml_dev_hash('./devicetypes/rf_4dis.xml');

#print_device_ident($$ret{'devIdnt'});
#print_channel_registers($$ret{'devCnlAddr'});
#print_channel_table($$ret{'devCnlAddr'});
#print_channel_structs($$ret{'devCnlLst'});
#print_device_frames($$ret{'devFrames'});
###############################################################



#DEBUG Dumper($$ret{'devCnlLst'});
#DEBUG Dumper($$ret{'devCnlAddr'});
#DEBUG Dumper($$ret{'devCnlAddrDef'});

sub print_device_ident {
	my $input = shift;

	print "/* \n";
	print "* Settings of HM device \n";
	print "* firmwareVersion: The firmware version reported by the device \n";
	print "*                  Sometimes this value is important for select the related device-XML-File \n";
	print "* \n";
	print "* modelID:         Important for identification of the device. \n";
	print "*                  \@See Device-XML-File /device/supported_types/type/parameter/const_value \n";
	print "* \n";
	print "* subType:         Identifier if device is a switch or a blind or a remote \n";
	print "* DevInfo:         Sometimes HM-Config-Files are referring on byte 23 for the amount of channels. \n";
	print "*                  Other bytes not known. \n";
	print "*                  23:0 0.4, means first four bit of byte 23 reflecting the amount of channels. \n";
	print "*/ \n";
	
	foreach my $devIdnt (@$input) {
		print  "const uint8_t devIdnt[] PROGMEM = {              // $$devIdnt{'id'} \n";
		printf "   /* firmwareVersion 1 byte */  0x%02x,           // or %s \n", $$devIdnt{'fw_version'}, $$devIdnt{'fw_opcond'}; 	#	0x11,
		printf "   /* modelID         2 byte */  0x%02x,0x%02x, \n", ($$devIdnt{'model_id'}&0xFF00)>>8, $$devIdnt{'model_id'}&0xFF ;

		printf "   /* subTypeID       1 byte */  0x%02x, \n", $$devIdnt{'sub_type'}  if ($$devIdnt{'sub_type'});
		print  "   /* subTypeID       1 byte */  0x__,           // replace __ by a valid type id \n"    if (!$$devIdnt{'sub_type'});

		print  "   /* deviceInfo      3 byte */  0x__,0x00,0x00, // device info not found, replace by valid values \n"     if (!$$devIdnt{'device_info'});
		print "}; \n\n";
	}
}


sub print_channel_registers {
	my $input = shift;
	my $size = 0;
	
	print "/* \n";
	print "* Register definitions \n";
	print "* The values are adresses in relation to the start adress defines in cnlTbl \n";
	print "* Register values can found in related Device-XML-File. \n";
	print "* \n";
	print "* Spechial register list 0: 0x0A, 0x0B, 0x0C \n";
	print "* Spechial register list 1: 0x08 \n";
	print "* \n";
	print "* \@See Defines.h \n";
	print "* \n";
	print "* \@See: cnlTbl \n";
	print "*/ \n";
	print "const uint8_t cnlAddr[] PROGMEM = { \n";

	## step through channels, lists
	foreach my $cnl_lst (sort keys %$input) {
		print "   // channel: $$input{$cnl_lst}{'channel'}, list: $$input{$cnl_lst}{'list'}";
		printf "%s",($$input{$cnl_lst}{'link'})? ", link to $$input{$cnl_lst}{'link'}\n" : "\n";
		
		## if we have content go further
		next if($$input{$cnl_lst}{'link'});

		print "   ";
		printf "0x%02x," x @{$$input{$cnl_lst}{'reg'}} . "\n", @{$$input{$cnl_lst}{'reg'}};
		$size += @{$$input{$cnl_lst}{'reg'}};
	}
	print "}; // $size byte\n\n";	
}

sub print_channel_table {
	my $input = shift;

	print "/* \n";
	print "* Channel - List translation table\n";
	print "* channel, list, startIndex, start address in EEprom, hidden\n";
	print "* do not edit the table, if you need more peers edit the defines accordingly. \n";
	print "*/\n";
	print "#define PHY_ADDR_START 0x20\n";

	## default peer amount per channel
	my $cnl_lst;
	foreach $cnl_lst (sort keys %$input) {
		next if (($$input{$cnl_lst}{'list'} != 3) && ($$input{$cnl_lst}{'list'} != 4));
		printf "#define CNL_%02d_PEERS   1 \n", $$input{$cnl_lst}{'channel'};
	}	
	print "\n";
	
	## print the table now
	print "EE::s_cnlTbl cnlTbl[] = { \n";
	print "   // cnl, lst, sIdx, sLen, hide, pAddr \n";

	my $row = 0; my $last_cnl_lst;
	foreach $cnl_lst (sort keys %$input) {
		## channel and slice table infos
		printf "   { %4s, %3s, %4s, %4s,    0,", $$input{$cnl_lst}{'channel'}, $$input{$cnl_lst}{'list'}, $$input{$cnl_lst}{'slice_idx'}, $$input{$cnl_lst}{'slice_len'};

		## eeprom address
		if ($row == 0) { 
			## row one is different
			print " PHY_ADDR_START }, \n" ;	
		} elsif (($$input{$last_cnl_lst}{'list'} == 3) || ($$input{$last_cnl_lst}{'list'} == 4)) {
			## peer rows
			printf " cnlTbl[%d].pAddr + (cnlTbl[%d].sLen * CNL_%02d_PEERS) }, \n", $row-1, $row-1, $$input{$cnl_lst}{'channel'}-1;	
		} else {
			## non peer rows
			printf " cnlTbl[%d].pAddr + cnlTbl[%d].sLen }, \n", $row-1, $row-1;	
		}
		$last_cnl_lst = $cnl_lst;
		$row += 1;
	}
	printf "}; // %d byte \n", $row*7;
	print "\n";
	
	print "/* \n";
	print "* Peer-Device-List-Table \n";
	print "* channel, maximum allowed peers, start address in EEprom \n";
	print "*/ \n";
	
	print "EE::s_peerTbl peerTbl[] = { \n";
	print "   // pMax, pAddr; \n";
	## first row needed, while missing a list3 or list4
	if (($$input{$last_cnl_lst}{'list'} == 3) || ($$input{$last_cnl_lst}{'list'} == 4)) {
		## last channel entry was a peer rows
		printf "   { 0, cnlTbl[%d].pAddr + (cnlTbl[%d].sLen * CNL_%02d_PEERS) }, \n", $row-1, $row-1, $$input{$last_cnl_lst}{'channel'};	
	} else {
		##  last channel entry was a non peer rows
		printf "   { 0, cnlTbl[%d].pAddr + cnlTbl[%d].sLen }, \n", $row-1, $row-1;	
	}
	## remaining rows
	$row = 0;
	foreach $cnl_lst (sort keys %$input) {
		next if (($$input{$cnl_lst}{'list'} != 3) && ($$input{$cnl_lst}{'list'} != 4));
		printf "   { CNL_%02d_PEERS, peerTbl[%d].pAddr + (peerTbl[%d].pMax * 4) }, \n", $$input{$cnl_lst}{'channel'}, $row, $row;
		$row += 1;
	}	
	printf "}; // %d byte \n", $row*3;
	print "\n";


	
}

sub print_channel_structs {
	my $input = shift;
	my ($prev_cnl, $prev_lst, $byte_cnt) = (-1, -1, 0);

	print "##- channel structs ----------------------------------------------------------------------------------------------\n";
	print "/* \n";
	print "* Channel structs (for developers)\n";
	print "* Within the channel struct you will find the definition of the respective registers per channel and list.\n";
	print "* These information is only needed if you want to develop your own channel module, for pre defined\n";
	print "* channel modules all this definitions enclosed in the pre defined module.  \n";
	print "*/ \n\n";

	foreach my $key (sort keys %$input) {

		if (($$input{$key}{'channel'} ne $prev_cnl) || ($$input{$key}{'list'} ne $prev_lst)) {
			print "}; // " .($byte_cnt/8) ." byte\n\n"       if ($prev_cnl != -1);
			print "struct s_cnl" .$$input{$key}{'channel'} ."_lst" .$$input{$key}{'list'} ." { \n";
			$byte_cnt = 0;
		}

		$byte_cnt += $$input{$key}{'size_bit'};
		printf("%10s %-25s :%-3s // 0x%02x.%d, s:%-3d d: %s %s \n", 'uint8_t', $$input{$key}{'id'}, $$input{$key}{'size_bit'}.";", $$input{$key}{'index'}, $$input{$key}{'index_bit'}, $$input{$key}{'size_bit'}, $$input{$key}{'default_sys'}, $$input{$key}{'default_unit'} );

		$prev_cnl = $$input{$key}{'channel'};
		$prev_lst = $$input{$key}{'list'};
	}

	print "}; // " .($byte_cnt/8) ." byte\n\n"; 
	print "##----------------------------------------------------------------------------------------------------------------\n";
}

sub print_device_frames {
	my $input = shift;

	print "/* \n";
	print "* Message description: \n";
	print "* \n";
	print "*        00        01 02    03 04 05  06 07 08  09  10  11   12     13 \n";
	print "* Length MSG_Count    Type  Sender__  Receiver  ACK Cnl Stat Action RSSI \n";
	print "* 0F     12        80 02    1E 7A AD  23 70 EC  01  01  BE   20     27    dimmer \n";
	print "* 0E     5C        80 02    1F B7 4A  63 19 63  01  01  C8   00     42    pcb relay \n";
	print "* \n";
	print "* Needed frames: \n";
	print "* \n";

	# step through the array and print lines
	printf "* %s\n" x @{$input}, @{$input};
		
	#DEBUG Dumper($input);	
	print "*/ \n\n";
}


