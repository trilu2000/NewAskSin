use strict;
use warnings;

use destillRegs;
use subs qw(DEBUG);

my $CONFIG_FROM_PERL_FILE=1;
my $CONFIG_FROM_JSON_FILE=0;
my $CONFIG_FROM_JSON_HANDLE=0;

## generates a new xml file based on a json device definition
my %config;
$config{'base_config'}{'hm_ID'} = 0x001122;
$config{'base_config'}{'serial_ID'} = "HBremote01";
$config{'base_config'}{'model_ID'} = 0x00A9;
$config{'base_config'}{'firmware_ver'} = 11;

$config{'extended_config'}{'id'} = "testID";
$config{'extended_config'}{'name'} = "testName";
#$config{'extended_config'}{'dutycycle'} = 1;
$config{'extended_config'}{'conf_key_mode'} = 2;
$config{'extended_config'}{'power_mode'} = 0;

$config{'extended_config'}{'lowbat_signal'} = 1;      # bool
$config{'extended_config'}{'lowbat_limit'} = 30;      # in V * 10
$config{'extended_config'}{'lowbat_timer'} = 3600000; # in ms

$config{'extended_config'}{'lowbat_register'} = 1;    # bool
$config{'extended_config'}{'lowbat_min'} = 10;        # in V * 10
$config{'extended_config'}{'lowbat_max'} = 40;        # in V * 10


$config{'channels'}[0] = {type => "Master",    peers => 0, hidden => 0, linked => 0 };
$config{'channels'}[1] = {type => "xmlRemote", peers => 6, hidden => 0, linked => 0 };
$config{'channels'}[2] = {type => "xmlRemote", peers => 8, hidden => 0, linked => 0 };
$config{'channels'}[3] = {type => "xmlRemote", peers => 4, hidden => 0, linked => 0 };
$config{'channels'}[4] = {type => "xmlRemote", peers => 4, hidden => 0, linked => 0 };
#$config{'channels'}[4] = {type => "xmlSwitch", peers => 4, hidden => 0, linked => 0 };
#$config{'channels'}[5] = {type => "xmlRemote", peers => 4, hidden => 0, linked => 0 };
#$config{'channels'}[6] = {type => "xmlSwitch", peers => 4, hidden => 0, linked => 0 };

#$config{'channels'}[1] = {type => "xmlRemote", peers => 4, hidden => 0, linked => 0 };
#$config{'channels'}[1] = {type => "xmlSwitch", peers => 4, hidden => 0, linked => 0 };
#$config{'channels'}[1] = {type => "xmlDimmer", peers => 4, hidden => 0, linked => 0 };
#$config{'channels'}[1] = {type => "xmlBlind", peers => 4, hidden => 0, linked => 0 };
#$config{'channels'}[1] = {type => "xmlMotion", peers => 4, hidden => 0, linked => 0 };
#$config{'channels'}[1] = {type => "xmlWeather", peers => 4, hidden => 0, linked => 0 };


###############################################################
## ++ working ++
## generates a xml device config file out of a given hash 
## describing the functionallity
## returns a string with the xml device content
my $dev_info_xml_string = gen_xml_device_info(\%config);
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
my $re_config = gen_reg_h_device_info(\%config);


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


sub print_library {
	my $input = shift;
	
	print " "x4 ."/** \n";
	print " "x4 ." * \@brief Libraries needed to run AskSin library \n";
	print " "x4 ." */ \n";
	
	## input is an array, step through
	for my $i (0 .. $#{$input}) {
		# there could be more then one line in each array element
		for (split /\n/, $$input[$i]) {
			print " "x4 ."$_\n";
		}
	}
	print "\n\n";
}
sub print_stage {
	my $input = shift;
	
	print " "x4 ."/** \n";
	print " "x4 ." * \@brief Stage the modules and declare external functions. \n";
	print " "x4 ." *  \n";
	print " "x4 ." * This functions are the call backs for user modules, \n";
	print " "x4 ." * declaration is in the register.h but the functions needs \n";
	print " "x4 ." * to be defined in the user sketch. \n";
	print " "x4 ." */ \n";
	
	## input is an array, step through
	for my $i (0 .. $#{$input}) {
		# there could be more then one line in each array element
		for (split /\n/, $$input[$i]) {
			print " "x4 ."$_\n";
		}
	}
	print "\n\n";
}
sub print_hm_serial_data {
	my $input = shift;
	
	print " "x4 ."/* \n";
	print " "x4 ." * \@brief HMID, Serial number, HM-Default-Key, Key-Index \n";
	print " "x4 ." */ \n";
	print " "x4 ."const uint8_t HMSerialData[] PROGMEM = { \n";
	print " "x8 ."/* HMID */            ", $$input{'hm_ID'} =~ s/(..)/0x$&,/gr," \n";
	print " "x8 ."/* Serial number */   ", $$input{'serial_ID'} =~ s/(.)/'$&',/gr,"		// $$input{'serial_ID'} \n";
	print " "x8 ."/* Default-Key */     HM_DEVICE_AES_KEY, \n";
	print " "x8 ."/* Key-Index */       HM_DEVICE_AES_KEY_INDEX, \n";
	print " "x4 ."}; \n";
	print "\n\n";
}
sub print_device_ident {
	my $input = shift;

	print " "x4 ."/** \n";
	print " "x4 ." * \@brief Settings of HM device \n";
	print " "x4 ." * firmwareVersion: The firmware version reported by the device \n";
	print " "x4 ." *                  Sometimes this value is important for select the related device-XML-File \n";
	print " "x4 ." * \n";
	print " "x4 ." * modelID:         Important for identification of the device. \n";
	print " "x4 ." *                  \@See Device-XML-File /device/supported_types/type/parameter/const_value \n";
	print " "x4 ." * \n";
	print " "x4 ." * subType:         Identifier if device is a switch or a blind or a remote \n";
	print " "x4 ." * DevInfo:         Sometimes HM-Config-Files are referring on byte 23 for the amount of channels. \n";
	print " "x4 ." *                  Other bytes not known. \n";
	print " "x4 ." *                  23:0 0.4, means first four bit of byte 23 reflecting the amount of channels. \n";
	print " "x4 ." */ \n";
	
	foreach my $devIdnt (@$input) {
		print  " "x4 ."const uint8_t devIdnt[] PROGMEM = {               // $$devIdnt{'id'} \n";
		printf " "x8 ."/* firmwareVersion 1 byte */  0x%02x,           // or %s \n", $$devIdnt{'fw_version'}, $$devIdnt{'fw_opcond'}; 	#	0x11,
		printf " "x8 ."/* modelID         2 byte */  0x%02x,0x%02x, \n", ($$devIdnt{'model_id'}&0xFF00)>>8, $$devIdnt{'model_id'}&0xFF ;

		printf " "x8 ."/* subTypeID       1 byte */  0x%02x, \n", $$devIdnt{'sub_type'}  if ($$devIdnt{'sub_type'});
		print  " "x8 ."/* subTypeID       1 byte */  0x__,           // replace __ by a valid type id \n"    if (!$$devIdnt{'sub_type'});

		print  " "x8 ."/* deviceInfo      3 byte */  0x00,0x00,0x00, // device info not found, replace by valid values \n"     if (!$$devIdnt{'device_info'});
		print " "x4 ."}; \n\n";
	}
}
sub print_channel_registers {
	my $input = shift;

	my $size = 0;
	
	print " "x4 ."/** \n";
	print " "x4 ." * Register definitions \n";
	print " "x4 ." * The values are adresses in relation to the start adress defines in cnlTbl \n";
	print " "x4 ." * Register values can found in related Device-XML-File. \n";
	print " "x4 ." * \n";
	print " "x4 ." * Spechial register list 0: 0x0A, 0x0B, 0x0C \n";
	print " "x4 ." * Spechial register list 1: 0x08 \n";
	print " "x4 ." * \n";
	print " "x4 ." * \@See Defines.h \n";
	print " "x4 ." * \n";
	print " "x4 ." * \@See: cnlTbl \n";
	print " "x4 ." */ \n";
	print " "x4 ."const uint8_t cnlAddr[] PROGMEM = { \n";

	## step through channels, lists
	foreach my $cnl_lst (sort keys %$input) {
		print " "x8 ."// channel: $$input{$cnl_lst}{'channel'}, list: $$input{$cnl_lst}{'list'}";
		printf "%s",($$input{$cnl_lst}{'link'})? ", link to $$input{$cnl_lst}{'link'}\n" : "\n";
		
		## if we have content go further
		next if($$input{$cnl_lst}{'link'});

		printf " "x8 ."0x%02x," x @{$$input{$cnl_lst}{'reg'}} . "\n", @{$$input{$cnl_lst}{'reg'}};
		$size += @{$$input{$cnl_lst}{'reg'}};
	}
	print " "x4 ."}; // $size byte\n\n";	
}
sub print_channel_table {
	my ($input, $peers) = @_;
	
	print " "x4 ."/** \n";
	print " "x4 ." * \@brief Channel - List translation table\n";
	print " "x4 ." * channel, list, startIndex, start address in EEprom, hidden\n";
	print " "x4 ." * do not edit the table, if you need more peers edit the defines accordingly. \n";
	print " "x4 ." */\n";
	print " "x4 ."#define PHY_ADDR_START 0x20\n";

	## default peer amount per channel
	foreach my $cnl_lst (sort keys %$input) {
		next if (($$input{$cnl_lst}{'list'} != 3) && ($$input{$cnl_lst}{'list'} != 4));
		my $cnl_num = $$input{$cnl_lst}{'channel'};
		my $peer_num = ($$peers[$cnl_num])? $$peers[$cnl_num] : 1;
		printf " "x4 ."#define CNL_%02d_PEERS   %d \n", $cnl_num, $peer_num;
	}	
	print "\n";
	
	## print the table now
	print " "x4 ."EE::s_cnlTbl cnlTbl[] = { \n";
	print " "x8 ."// cnl, lst, sIdx, sLen, hide, pAddr \n";

	my $row = 0; my $last_cnl_lst;
	foreach my $cnl_lst (sort keys %$input) {
		## channel and slice table infos
		printf " "x8 ."{ %4s, %3s, %4s, %4s,    0,", $$input{$cnl_lst}{'channel'}, $$input{$cnl_lst}{'list'}, $$input{$cnl_lst}{'slice_idx'}, $$input{$cnl_lst}{'slice_len'};

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
	printf " "x4 ."}; // %d byte \n", $row*7;
	print "\n";
	
	print " "x4 ."/** \n";
	print " "x4 ." * Peer-Device-List-Table \n";
	print " "x4 ." * channel, maximum allowed peers, start address in EEprom \n";
	print " "x4 ." */ \n";
	
	print " "x4 ."EE::s_peerTbl peerTbl[] = { \n";
	print " "x8 ."// pMax, pAddr; \n";
	## first row needed, while missing a list3 or list4
	if (($$input{$last_cnl_lst}{'list'} == 3) || ($$input{$last_cnl_lst}{'list'} == 4)) {
		## last channel entry was a peer rows
		printf " "x8 ."{ 0, cnlTbl[%d].pAddr + (cnlTbl[%d].sLen * CNL_%02d_PEERS) }, \n", $row-1, $row-1, $$input{$last_cnl_lst}{'channel'};	
	} else {
		##  last channel entry was a non peer rows
		printf " "x8 ."{ 0, cnlTbl[%d].pAddr + cnlTbl[%d].sLen }, \n", $row-1, $row-1;	
	}
	## remaining rows
	$row = 0;
	foreach my $cnl_lst (sort keys %$input) {
		next if (($$input{$cnl_lst}{'list'} != 3) && ($$input{$cnl_lst}{'list'} != 4));
		printf " "x8 ."{ CNL_%02d_PEERS, peerTbl[%d].pAddr + (peerTbl[%d].pMax * 4) }, \n", $$input{$cnl_lst}{'channel'}, $row, $row;
		$row += 1;
	}	
	print " "x4 ."}; // ", $row*3, " byte\n\n";	
}
sub print_device_description {
	my $input = shift;

	print " "x4 ."/** \n";
	print " "x4 ." * \@brief Struct with basic information for the AskSin library. \n";
	print " "x4 ." * amount of user channels, amount of lines in the channel table, \n";
	print " "x4 ." * link to devIdent byte array, link to cnlAddr byte array \n";
	print " "x4 ." */ \n";
	print " "x4 ."EE::s_devDef devDef = { \n";
	print " "x8 , $$input{'max_channel'}, ", ", $$input{'max_config'},", devIdnt, cnlAddr, \n";
	print " "x4 ."}; \n\n";
}
sub print_module_register_table {
	my $input = shift;

	print " "x4 ."/** \n";
	print " "x4 ." * \@brief Sizing of the user module register table. \n";
	print " "x4 ." * Within this register table all user modules are registered to make \n";
	print " "x4 ." * them accessible for the AskSin library	 \n";
	print " "x4 ." */ \n";
	print " "x4 ."RG::s_modTable modTbl[", $$input{'max_channel'} ,"]; \n\n";
}
sub print_every_time_start {
	my $input = shift;

	print " "x4 ."/** \n";
	print " "x4 ." * \@brief Regular start function \n";
	print " "x4 ." * This function is called by the main function every time when the device starts, \n"; 
	print " "x4 ." * here we can setup everything which is needed for a proper device operation \n";
	print " "x4 ." */ \n";
	print " "x4 ."void everyTimeStart(void) { \n\n";
	#print " "x8 ."/* \n";
	#print " "x8 ." * Place here everything which should be done on each start or reset of the device. \n";
	#print " "x8 ." * Typical use case are loading default values or user class configurations. \n";
	#print " "x8 ." */ \n";
	
	## input is an array, step through
	for my $i (0 .. $#{$input}) {
		print " "x8 ."// channel $i section \n";
		# there could be more then one line in each array element
		for (split /\n/, $$input[$i]) {
			print " "x8 ."$_\n";
		}
	}
	print " "x4 ."} \n\n";
}
sub print_first_time_start {
	print " "x4 ."/** \n";
	print " "x4 ." * \@brief First time start function \n";
	print " "x4 ." * This function is called by the main function on the first boot of a device.  \n";
	print " "x4 ." * First boot is indicated by a magic byte in the eeprom. \n";
	print " "x4 ." * Here we can setup everything which is needed for a proper device operation, like cleaning  \n";
	print " "x4 ." * of eeprom variables, or setting a default link in the peer table for 2 channels \n";
	print " "x4 ." */ \n";
	print " "x4 ."void firstTimeStart(void) { \n\n";
	#print " "x8 ."/*  \n";
	#print " "x8 ." * Place here everything which should be done on the first start of the device.  \n";
	#print " "x8 ." * Typical use case are loading default values for the user channels. \n";
	#print " "x8 ." */  \n\n";

	print " "x4 ."} \n\n";
}
sub print_channel_structs {
	my ($input, $cnl_addr) = @_;
	my ($linklist, $prev_cnl_lst,  $byte_cnt) = ("", "", 0);
	
	print "/** \n";
	print "* \@brief Channel structs (for developers)\n";
	print "* Within the channel struct you will find the definition of the respective registers per channel and list.\n";
	print "* These information is only needed if you want to develop your own channel module, for pre defined\n";
	print "* channel modules all this definitions enclosed in the pre defined module.  \n";
	print "*/ \n\n";

	foreach my $key (sort keys %$input) {
		## make it easier to compare if we had this channel list combi already
		my $cnl_lst_idx = sprintf("%02d %02d", $$input{$key}{'channel'}, $$input{$key}{'list'});

		## skip if the channel/list is linked to another channel/list
		if ($$cnl_addr{$cnl_lst_idx}{'link'}) {
			$linklist .= "// struct s_cnl" .$$input{$key}{'channel'} ."_lst" .$$input{$key}{'list'} ." linked to " .$$cnl_addr{$cnl_lst_idx}{'link'} ."\n"  if ($cnl_lst_idx ne $prev_cnl_lst);
			$prev_cnl_lst = $cnl_lst_idx;
			next ;
		}
		
		## otherwise print the channel/list struct
		if ($cnl_lst_idx ne $prev_cnl_lst) {
			print "}; // " .($byte_cnt/8) ." byte\n\n" if ($prev_cnl_lst ne "");
			print "struct s_cnl" .$$input{$key}{'channel'} ."_lst" .$$input{$key}{'list'} ." { \n";
			$byte_cnt = 0;
		}

		$byte_cnt += $$input{$key}{'size_bit'};
		printf("%10s %-25s :%-3s // 0x%02x.%d, s:%-3d d: %s %s \n", 'uint8_t', $$input{$key}{'id'}, $$input{$key}{'size_bit'}.";", $$input{$key}{'index'}, $$input{$key}{'index_bit'}, $$input{$key}{'size_bit'}, $$input{$key}{'default_sys'}, $$input{$key}{'default_unit'} );

		$prev_cnl_lst = $cnl_lst_idx;
	}

	print "}; // " .($byte_cnt/8) ." byte\n\n";
	print $linklist, "\n" if (length($linklist) > 0);
}
sub print_device_frames {
	my $input = shift;

	print "/** \n";
	print " * \@brief Message description: \n";
	print " * \n";
	print " *        00        01 02    03 04 05  06 07 08  09  10  11   12     13 \n";
	print " * Length MSG_Count    Type  Sender__  Receiver  ACK Cnl Stat Action RSSI \n";
	print " * 0F     12        80 02    1E 7A AD  23 70 EC  01  01  BE   20     27    dimmer \n";
	print " * 0E     5C        80 02    1F B7 4A  63 19 63  01  01  C8   00     42    pcb relay \n";
	print " * \n";
	print " * Needed frames: \n";
	print " * \n";

	# step through the array and print lines
	printf " * %s\n" x @{$input}, @{$input};
	print " */ \n\n";
}


