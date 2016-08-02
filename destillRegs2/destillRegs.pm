my $DEBUG=1;

use warnings;
use strict;

use XML::LibXML;
use XML::Hash;

use JSON;

use Time::HiRes qw(gettimeofday tv_interval);

use Data::Dumper::Simple;
$Data::Dumper::Sortkeys = 1;

package destillRegs;

use vars qw($VERSION @ISA @EXPORT);
require Exporter;

#                                                                                                                                                                                                                                                                                

@ISA = qw(Exporter);
@EXPORT = qw(DEBUG print_library print_stage print_hm_serial_data print_device_ident print_channel_registers print_channel_defaults print_channel_table print_device_description print_module_register_table print_every_time_start print_first_time_start print_channel_structs print_device_frames load_file_by_name get_file_list gen_xml_dev_info_hash gen_xml_device_info gen_reg_h_device_info gen_xml_dev_list);
$VERSION = 1.0;
 
use userModuls;


## -- debug function ----------------------------------------------------------------------------------------
sub DEBUG {
	if (!$DEBUG) {return;}
	print join("",@_) ."\n";
}

## -- print out functions -----------------------------------------------------------------------------------
##
# @brief converts a char string into an hex output
# @param    prnHexStr("0102",2)
# @return   0x01,0x02
##
sub prnHexStr {
	my $in = shift;
	my $len = shift;
	$in = $in ."0"x(($len*2) - length($in));
		
	#$in = sprintf("%.".$len."x", $in);
	$in =~ s/(..)/0x$&,/g;
	return $in;
}
##
# @brief converts a char string into a comma seperated output
# @param    prnASCIIStr("test")
# @return   't','e','s','t',
##
sub prnASCIIStr {
	my $in = shift;
	$in =~ s/(.)/'$&',/g;
	return $in;
}

## -- file functions ----------------------------------------------------------------------------------------
##
# @brief simply loads a file and returns the content in a string variable
# @param    load_file_by_name("c:\test.txt")
# @return   $content
##
sub load_file_by_name {
    my $file  = shift;

	# check if it is multiline, if yes step out
	return '' if ($file =~ /\n/);
	# check if the file exist	
	return '' if not -e $file;
	
	# open the file and read the content
	open( my $fh, $file );
    my $content = do { local $/; <$fh> };
    close $fh;
	return $content;
}

##
# @brief returns a file list for a given directory
# @param    path      get_file_list("c:\")
# @return   $content
##
sub get_file_list {
	my $dir = shift;
	my $startTime = time;
	
	opendir(DIR, $dir) or die $!;																# open the directory
	our @filelist = grep { $_ ne "." && $_ ne ".."  } readdir(DIR);								# step through the files
	closedir(DIR);																				# close the directory again
	#DEBUG "get_file_list, took ", sprintf("%.4f", time - $startTime), " seconds";
	return \@filelist;
}

##
# @brief returns a xml file as hash
# @param    path/file  the path information and filename 
# @return   a hash reference with the information
##
sub get_xml_file_as_hash {
	my $dir = shift;
	my $startTime = time;
	
	my $ret = load_file_by_name($dir);															# open the file

	my $self = XML::Hash->new();																# stage the helper
	#DEBUG "get_xml_file_as_hash, took ", sprintf("%.4f", time - $startTime), " seconds";
	return $self->fromXMLStringtoHash($ret);													# convert and return
}




##
# @brief generates a list of all xml device definition files
# @param    directory path
# @return   a list with all information 
##
sub gen_xml_dev_list {
	# information is in 
	# check for index and size <parameter index="10.0" size="2.0" const_value="11"/>
	# const value can be in different formats "11", '0x00DD' so normalize

	my $startTime = [Time::HiRes::gettimeofday];
	my $file_list = get_file_list('devicetypes');												# get a filelist
	my $self = XML::Hash->new();																# stage the helper
	my $ret;

	#DEBUG "DeviceID;Version;Support AES;RX modes;Cyclic Timeout;Device Name;Updateable;Priority;Model ID;FW Version;FW Opcond;;Filename\n";  
	$ret = "DeviceID;Version;Support AES;RX modes;Cyclic Timeout;Device Name;Updateable;Priority;Model ID;FW Version;FW Opcond;;Filename\n";
	
	foreach my $file_name (@{$file_list}) 	{													# loop through the filelist
		# getting everything between <supported_types></supported_types> in a string variable
		my $xml_file = load_file_by_name("devicetypes/$file_name");								# load the file

#		$xml_file =~ s/<supported_types>(.*?)<\/supported_types>//gs;							# filter the part we need
#		$xml_hash{'types'} = $self->fromXMLStringtoHash("<types>$1</types>");					# convert and return, types is neccasary because of xml limitation

		my $xml_hash = $self->fromXMLStringtoHash($xml_file);										# convert and return, types is neccasary because of xml limitation
		$$xml_hash{'filenamepath'} = "devicetypes/$file_name";									# add file path info

		# get some device information
		my $dev_version         = $$xml_hash{'device'}{'version'};
		my $dev_support_aes     = ($$xml_hash{'device'}{'supports_aes'})? $$xml_hash{'device'}{'supports_aes'}:"false";
		my $dev_rx_modes        = ($$xml_hash{'device'}{'rx_modes'})? $$xml_hash{'device'}{'rx_modes'}:"";
		my $dev_cyclic_timeout  = ($$xml_hash{'device'}{'cyclic_timeout'})? $$xml_hash{'device'}{'cyclic_timeout'}:"";
		
		# work on the type section, check if we have an array, if not convert hash to an array
		$$xml_hash{'device'}{'supported_types'}{'type'} = [$$xml_hash{'device'}{'supported_types'}{'type'}]         if (ref($$xml_hash{'device'}{'supported_types'}{'type'}) eq 'HASH');
		#DEBUG "ref: ", ref($$xml_hash{'device'}{'supported_types'}{'type'});
		
		# loop through the supported_types type info
		foreach my $type (@{$$xml_hash{'device'}{'supported_types'}{'type'}}) {												# loop through the found elements
			#DEBUG "file: ", $xml_hash{'filenamepath'};

			# get the needed information
			my $type_id         = $$type{'id'};
			my $type_name       = $$type{'name'};
			my $type_updateable = ($$type{'updatable'})?    $$type{'updatable'}   :"false";
			my $type_priority   = ($$type{'priority'})?     $$type{'priority'}    :"";
			
			my ($type_device_id, $type_fw_version, $type_fw_opcond) = ("","0x00","GE"); 

			# within the type there is a parameter section, check if parameter is an array, if not convert it
			$$type{'parameter'} = [$$type{'parameter'}]    if (ref($$type{'parameter'}) eq 'HASH');

			# step through the parameters to get id, name, etc
			# on some devices there are two parameter sections per type, one for device id and the firmware version
			foreach my $parameter (@{$$type{'parameter'}}) {
				#<device version="6" rx_modes="CONFIG,WAKEUP" supports_aes="true" cyclic_timeout="88200">

				#<type name="WinMatic white" id="HM-Sec-Win" priority="2">
				#<parameter index="10.0" size="2.0" const_value="40"/>
				if (( eval($$parameter{'index'}) == 9 ) && ( eval($$parameter{'size'}) == 1 )) {
					$type_fw_version = sprintf("0x%.2x", eval($$parameter{'const_value'}),1);	
					$type_fw_opcond  = $$parameter{'cond_op'};

				} elsif (( eval($$parameter{'index'}) == 10 ) && ( eval($$parameter{'size'}) == 2 )) {
					$type_device_id = sprintf("0x%.4x", eval($$parameter{'const_value'}));
					
				} else {
					$type_device_id = "$$parameter{'index'} $$parameter{'size'} $$parameter{'const_value'}";
					
				}
			} 

			#DEBUG "$type_id;$dev_version;$dev_support_aes;$dev_rx_modes;$dev_cyclic_timeout;$type_name;$type_updateable;$type_priority;$type_device_id;$type_fw_version;$type_fw_opcond;;$file_name";  
			$ret .= "$type_id;$dev_version;$dev_support_aes;$dev_rx_modes;$dev_cyclic_timeout;$type_name;$type_updateable;$type_priority;$type_device_id;$type_fw_version;$type_fw_opcond;;$file_name\n";  
			
		}
		#DEBUG "";
	}

	DEBUG "\ngen_xml_dev_list, took ", Time::HiRes::tv_interval($startTime), " seconds\n\n";
	return $ret;
	
}

##
# @brief generates a hash from a xml deive file with all information 
#        needed to build an register.h
# @param    file path
# @return   a list with all information 
##
sub gen_xml_dev_info_hash {
	my $file_path = shift;
	my $startTime = [Time::HiRes::gettimeofday];
	my %ret;
	
	## check if file_path is a path or a xml_string by checking of multiline
	#'./devicetypes/rf_rc-4-2.xml'
	$file_path = load_file_by_name($file_path) if ($file_path !~ /\n/);
	
	## convert it into an hash
	my $self = XML::Hash->new();																# stage the helper
	my $xml_hash = $self->fromXMLStringtoHash($file_path);													# convert and return

	## get the overall device info
	$ret{'devInfo'}{'version'}        = $$xml_hash{'device'}{'version'};
	$ret{'devInfo'}{'rx_modes'}       = ($$xml_hash{'device'}{'rx_modes'})?$$xml_hash{'device'}{'rx_modes'}:"";
	$ret{'devInfo'}{'supports_aes'}   = ($$xml_hash{'device'}{'supports_aes'})?$$xml_hash{'device'}{'supports_aes'}:"false";
	$ret{'devInfo'}{'cyclic_timeout'} = ($$xml_hash{'device'}{'cyclic_timeout'})?$$xml_hash{'device'}{'cyclic_timeout'}:"";


	## get the type info section
	$$xml_hash{'device'}{'supported_types'}{'type'} = [$$xml_hash{'device'}{'supported_types'}{'type'}]         if (ref($$xml_hash{'device'}{'supported_types'}{'type'}) eq 'HASH');

	foreach my $type (@{$$xml_hash{'device'}{'supported_types'}{'type'}}) {												# loop through the found elements
		my ($type_device_id, $type_fw_version, $type_fw_opcond) = ("",0,"GE"); 

		# within the type there is a parameter section, check if parameter is an array, if not convert it
		$$type{'parameter'} = [$$type{'parameter'}]    if (ref($$type{'parameter'}) eq 'HASH');

		# step through the parameters to get id, name, etc
		# on some devices there are two parameter sections per type, one for device id and the firmware version
		foreach my $parameter (@{$$type{'parameter'}}) {

				if (( eval($$parameter{'index'}) == 9 ) && ( eval($$parameter{'size'}) == 1 )) {
					$type_fw_version = eval($$parameter{'const_value'});	
					$type_fw_opcond  = $$parameter{'cond_op'};
				} elsif (( eval($$parameter{'index'}) == 10 ) && ( eval($$parameter{'size'}) == 2 )) {
					$type_device_id = eval($$parameter{'const_value'});
				} else {
					$type_device_id = "$$parameter{'index'} $$parameter{'size'} $$parameter{'const_value'}";
				}
		}

		# collect the infos we found into the hash to return
		push @{$ret{'devIdnt'}}, {name => $$type{'name'}, id => $$type{'id'}, fw_opcond => $type_fw_opcond, fw_version => $type_fw_version, model_id => $type_device_id};

	}
	

	## get the frame section as info
	$file_path =~ s/<frames>(.*?)<\/frames>//gs;	
	foreach my $line (split /\n/, $1) {
		# if line starts with <frame then we search for the id and open a hash with it
		$line =~ s/^\s+|\s+$//g;
		push @{$ret{'devFrames'}}, $line            if ($line =~ /<frame /);
		push @{$ret{'devFrames'}}, "     $line"     if ($line =~ /<parameter /);
	}



	## go through the paramset section
	DEBUG "paramset <> Master gefunden", Dumper($$xml_hash{'device'}{'paramset'}) if ($$xml_hash{'device'}{'paramset'}{'type'} ne "MASTER" );	
	## step through the paramsets
	$$xml_hash{'device'}{'paramset'} = [$$xml_hash{'device'}{'paramset'}]    if (ref($$xml_hash{'device'}{'paramset'}) eq 'HASH');
	foreach my $paramset (@{$$xml_hash{'device'}{'paramset'}}) {												# loop through the found elements
		#DEBUG "device - ", Dumper($paramset);
		my %hash = get_parameter_data($paramset,0,1);
		@{$ret{'devCnlLst'}}{keys %hash} = values %hash;
	}
	
	## go through the channel section in channels
	foreach my $channel (@{$$xml_hash{'device'}{'channels'}{'channel'}}) {												# loop through the found elements
		#<channel index="3" type="VIRTUAL_DIMMER" count="4">
		#<channel autoregister="true" index="1" type="KEY" count="20" pair_function="BA" function="A" paired="true" aes_default="true">

		## collect within the channel section the paramsets
		my $channel_index = $$channel{'index'};
		my $channel_count = $$channel{'count'};

		## if count_from_sysinfo is given, we do not know how many channels we have. set it to 1
		$channel_count = 6 if $$channel{'count_from_sysinfo'};
		#DEBUG "cnl, $channel_index, cnt: $channel_count; ", Dumper($$channel{'paramset'});

		$$channel{'paramset'} = [$$channel{'paramset'}]    if (ref($$channel{'paramset'}) eq 'HASH');
		foreach my $paramset (@{$$channel{'paramset'}}) {												# loop through the found elements
			if (ref($$paramset{'subset'}) eq 'ARRAY') {
            	#'id' => 'dimmer_ch_master',
            	#'subset' => [ {
                #	'ref' => 'physical_parameters'
                #	'ref' => 'general_parameters'

				## stepping through the subsets
				foreach my $subset (@{$$paramset{'subset'}}) {
					## find the fitting entry in param_defs
					$$xml_hash{'device'}{'paramset_defs'}{'paramset'} = [$$xml_hash{'device'}{'paramset_defs'}{'paramset'}]   if ( ref( $$xml_hash{'device'}{'paramset_defs'}{'paramset'}) eq "HASH");
					my ($index) = grep { $$xml_hash{'device'}{'paramset_defs'}{'paramset'}[$_]{'id'} eq $$subset{'ref'} } (0 .. @{$$xml_hash{'device'}{'paramset_defs'}{'paramset'}}-1);
					#DEBUG "ARRAY - ref: ", $$subset{'ref'}, " idx: ", $index, " defs: ", $$xml_hash{'device'}{'paramset_defs'}{'paramset'}[$index]{'id'};
	
					## get the details
					my $temp_paramset = $$xml_hash{'device'}{'paramset_defs'}{'paramset'}[$index];
					
					my %hash = get_parameter_data($temp_paramset,$channel_index,$channel_count);
					@{$ret{'devCnlLst'}}{keys %hash} = values %hash;

				}

			} elsif (ref($$paramset{'subset'}) eq 'HASH') {
            	#'id' => 'dimmer_ch_link',
            	#'subset' => {
            	#	'ref' => 'dimmer_linkset'

				## find the fitting entry in param_defs
				$$xml_hash{'device'}{'paramset_defs'}{'paramset'} = [$$xml_hash{'device'}{'paramset_defs'}{'paramset'}]   if ( ref( $$xml_hash{'device'}{'paramset_defs'}{'paramset'}) eq "HASH");
				my ($index) = grep { $$xml_hash{'device'}{'paramset_defs'}{'paramset'}[$_]{'id'} eq $$paramset{'subset'}{'ref'} } (0 .. @{$$xml_hash{'device'}{'paramset_defs'}{'paramset'}}-1);
				#DEBUG "HASH - ref: ", $$paramset{'subset'}{'ref'}, " idx: ", $index, " defs: ", $$xml_hash{'device'}{'paramset_defs'}{'paramset'}[$index]{'id'};

				## get the details
				my $temp_paramset = $$xml_hash{'device'}{'paramset_defs'}{'paramset'}[$index];
				
				my %hash = get_parameter_data($temp_paramset,$channel_index,$channel_count);
				@{$ret{'devCnlLst'}}{keys %hash} = values %hash;
				
			} else {

				## get the details
				my %hash = get_parameter_data($paramset,$channel_index,$channel_count);
				@{$ret{'devCnlLst'}}{keys %hash} = values %hash;

			}

		}
		
	}
	
	## add the master id section in channel 0, list 0
	$ret{'devCnlLst'}{'00 00 0a.0'} = {'channel' => 0, 'default_val' => 0, 'default_type' => 'integer', 'id' => 'MASTER_ID', 'index' => 10, 'index_bit' => 0, 'list' => 0, 'size_bit' => 24, 'type' => 'integer', 'default_sys' => '', 'default_unit' => ''};
	#DEBUG Dumper($ret{'devCnlLst'});



	## do some sanity on the collected infos
	## collect index per channel and list in $ret{'devCnlLst'}
	my %missing_bits;
	my ($prev_channel, $prev_list, $prev_index, $prev_index_bit, $array_index) = (-1,-1,-1,8,-1);
	foreach my $key (sort keys %{$ret{'devCnlLst'}}) {
		## get channel, list and index from the record and push it into the array
		my $channel = $ret{'devCnlLst'}{$key}{'channel'};
		my $list = $ret{'devCnlLst'}{$key}{'list'};
		my $index = $ret{'devCnlLst'}{$key}{'index'};
		my $index_bit = $ret{'devCnlLst'}{$key}{'index_bit'};
		my $size_bit = $ret{'devCnlLst'}{$key}{'size_bit'};
		my $default = $ret{'devCnlLst'}{$key}{'default_val'};

		## set it to -1 while it is a new channel or list
		$array_index = -1 if (($prev_channel != $channel) || ($prev_list != $list));
		#DEBUG "\naind: $array_index, cnl: $channel, lst: $list, ind: $index, bit: $index_bit, size: $size_bit";
		#DEBUG "pcnl: $prev_channel, plst: $prev_list, pind: $prev_index, pbit: $prev_index_bit";

		## fill uncovered bits in devCnlLst
		if (($array_index == -1 ) && ($index_bit > 0)) {
			## search for missing upfront bits
			my $idx = sprintf("%.2d %.2d %.2x.%d", $channel, $list, $index, 0);
			$missing_bits{$idx} = {'channel' => $channel, 'list' => $list, 'index' => $index, 'index_bit' => 0, 'size_bit' => $index_bit, 'type' => 'fill', 'id' => '', 'default_val' => '', 'default_sys' => '', 'default_unit' => ''};
		} 

		if (($array_index == -1 ) && ($prev_index_bit < 8)) {
			## search on the last entry in a channel/list
			my $idx = sprintf("%.2d %.2d %.2x.%d", $prev_channel, $prev_list, $prev_index, $prev_index_bit);
			$missing_bits{$idx} = {'channel' => $prev_channel, 'list' => $prev_list, 'index' => $prev_index, 'index_bit' => $prev_index_bit, 'size_bit' => (8-$prev_index_bit), 'type' => 'fill', 'id' => '', 'default_val' => '', 'default_sys' => '', 'default_unit' => ''};
		} 
		
		if (($prev_index == $index) && ($prev_index_bit < $index_bit)) {
			## fill in between
			my $idx = sprintf("%.2d %.2d %.2x.%d", $channel, $list, $index, $prev_index_bit);
			$missing_bits{$idx} = {'channel' => $channel, 'list' => $list, 'index' => $index, 'index_bit' => $prev_index_bit, 'size_bit' => ($index_bit - $prev_index_bit), 'type' => 'fill', 'id' => '', 'default_val' => '', 'default_sys' => '', 'default_unit' => ''};

		} elsif (($prev_index != $index) && ($prev_index_bit < 8)) {
			my $idx = sprintf("%.2d %.2d %.2x.%d", $prev_channel, $prev_list, $prev_index, $prev_index_bit);
			$missing_bits{$idx} = {'channel' => $prev_channel, 'list' => $prev_list, 'index' => $prev_index, 'index_bit' => $prev_index_bit, 'size_bit' => (8-$prev_index_bit), 'type' => 'fill', 'id' => '', 'default_val' => '', 'default_sys' => '', 'default_unit' => ''};

		}

		## take care of the size bit - in cnl0, lst0 there is the masterid, starts at 0x0a and covers 3 byte
		for (my $i=0; $i < $size_bit; $i+=8) {
			## increase the position counter while index had changed
			$array_index++ if ($prev_index != $index);

			#DEBUG "i: $i, cnl: $channel, lst: $list, aind: $array_index, ", ($index+($i/8)), "   ind: $index, pind: $prev_index";
			
			## store the index identifier in an array
			my $short = sprintf("%02d %02d",$channel,$list);
			$ret{'devCnlAddr'}{$short}{'reg'}[$array_index] = ($index+($i/8));
			$ret{'devCnlAddr'}{$short}{'channel'} = $channel;
			$ret{'devCnlAddr'}{$short}{'list'} = $list;
			
			## store the default value in an array
			## todo: not sure is there is a index bigger a byte with a default value bigger then a byte
			$ret{'devCnlAddrDef'}{$short}{'reg'}[$array_index] |= ($default << $index_bit);
			$ret{'devCnlAddrDef'}{$short}{'channel'} = $channel;
			$ret{'devCnlAddrDef'}{$short}{'list'} = $list;
			
		}

		## remember the previous values to identify a change
		($prev_channel, $prev_list, $prev_index, $prev_index_bit) = ($channel, $list, $index, $index_bit+$size_bit);
	}
	@{$ret{'devCnlLst'}}{keys %missing_bits} = values %missing_bits;
	#DEBUG Dumper(%missing_bits);

	## complete the devCnlAddr table by stepping through the channel lists 
	## enhance by start and len flag per list 
	
	## reduce to the minimum by deleting double lists
	my $last_slice_index = 0;
	$ret{'devInfo'}{'max_config'} = scalar keys %{$ret{'devCnlAddr'}};
	
	foreach my $cnl_lst (sort keys %{$ret{'devCnlAddr'}}) {
		$ret{'devInfo'}{'max_channel'} = $ret{'devCnlAddr'}{$cnl_lst}{'channel'};
		next if ($ret{'devCnlAddr'}{$cnl_lst}{'link'});
		
		## store slice len and start
		$ret{'devCnlAddr'}{$cnl_lst}{'slice_idx'} = $last_slice_index;
		$ret{'devCnlAddr'}{$cnl_lst}{'slice_len'} = @{$ret{'devCnlAddr'}{$cnl_lst}{'reg'}};
		$last_slice_index += $ret{'devCnlAddr'}{$cnl_lst}{'slice_len'};
		my $cnl_lst_string = join(",",@{$ret{'devCnlAddr'}{$cnl_lst}{'reg'}});
				
		foreach my $x_cnl_lst (sort keys %{$ret{'devCnlAddr'}}) {
			next if ($cnl_lst eq $x_cnl_lst);

			my $x_cnl_lst_string = join(",",@{$ret{'devCnlAddr'}{$x_cnl_lst}{'reg'}});
			
			# check for doubles, set a link, delete content and fix the slice index
			if($cnl_lst_string eq $x_cnl_lst_string) {
				$ret{'devCnlAddr'}{$x_cnl_lst}{'link'} = $cnl_lst;
				@{$ret{'devCnlAddr'}{$x_cnl_lst}{'reg'}} = ();
				$ret{'devCnlAddr'}{$x_cnl_lst}{'slice_idx'} = $ret{'devCnlAddr'}{$cnl_lst}{'slice_idx'};
				$ret{'devCnlAddr'}{$x_cnl_lst}{'slice_len'} = $ret{'devCnlAddr'}{$cnl_lst}{'slice_len'};
			}
		}
	}
	DEBUG "\ngen_xml_dev_hash, took ", Time::HiRes::tv_interval($startTime), " seconds\n\n";
	return \%ret;
}

##
# @brief replacement list, there are some registers marked as internal
#        without a valid register index, size, list and so on
# @param    no param
# @return   simply a hash with the id as index 
##
my $replace;
$$replace{'AES_ACTIVE'} = {'index' => '8.0','interface' => 'config','list' => '1','size' => '0.1','type' => 'boolean'};
sub get_parameter_data {
	my $paramset = shift;	
	my $channel = shift;	
	my $count = shift;	
	my %ret;
	
	## step through the parameters in the paramset
	$$paramset{'parameter'} = [$$paramset{'parameter'}]    if (ref($$paramset{'parameter'}) eq 'HASH');
	foreach my $parameter (@{$$paramset{'parameter'}}) {												# loop through the found elements

		## check for any internal paramsets and replace them with a version incl. index
		if ( $$parameter{'ui_flags'} && $$parameter{'ui_flags'} eq "internal" &&  $$parameter{'physical'}{'interface'} && $$parameter{'physical'}{'interface'} eq "internal") {
			DEBUG "-- internal parameter without index found ---------------------";
			DEBUG Dumper($parameter);
			DEBUG "---------------------------------------------------------------";
			$$parameter{'physical'} = $$replace{$$parameter{'id'}} if $$replace{$$parameter{'id'}};
		}

		next if (!$$parameter{'physical'});
		next if (!$$parameter{'physical'}{'index'});
		
			
		## collect the physical information
		for (my $i=$channel; $i < $channel+$count; $i++) {
		
			my $hash = get_physical_section($parameter, $i, $count);
		
			my $idx = sprintf("%.2d %.2d %.2x.%d", $$hash{'channel'}, $$hash{'list'}, $$hash{'index'}, $$hash{'index_bit'});
			next if (!$idx);
			#DEBUG Dumper($hash);
		
			$ret{$idx} = $hash;
			$ret{$idx}{'id'} = $$parameter{'id'};
				
			## get the default information
			my ($default, $type, $unit, $default_sys) = get_default($parameter);
			$ret{$idx}{'default_val'} = $default;
			$ret{$idx}{'default_type'} = $type;
			$ret{$idx}{'default_unit'} = $unit;
			$ret{$idx}{'default_sys'} = $default_sys;
			
		}
	}

	#DEBUG Dumper(%ret);
	return %ret;
}
sub get_default {
	#<logical type="float" min="0.5" max="15.5" default="2.0" unit="s">
	#   <special_value id="NOT_USED" value="0.0"/>
	#</logical>	
	#<conversion type="float_integer_scale" factor="2"/>

	my $parameter = shift;
	my ($default, $type, $unit, $default_sys);
	
	$type = $$parameter{'logical'}{'type'};
	$unit = $$parameter{'logical'}{'unit'};
	
	if      ($$parameter{'logical'}{'type'} eq "boolean") {
		$default_sys = $$parameter{'logical'}{'default'};
		$default_sys = "false" if (!$default_sys);
		$default = 1    if $default_sys =~ m/true|TRUE|True/;
		$default = 0    if $default_sys =~ m/false|FALSE|False/;

	} elsif ($$parameter{'logical'}{'type'} eq "string") {
		$default_sys = "";
		$default = 0;

	} elsif ($$parameter{'logical'}{'type'} eq "option") {
		## step throug the list of options and search for the default= flag
		$$parameter{'logical'}{'option'} = [$$parameter{'logical'}{'option'}]    if (ref($$parameter{'logical'}{'option'}) eq 'HASH');
		foreach my $i (0 .. $#{$$parameter{'logical'}{'option'}}) {												# loop through the found elements
			next if(!$$parameter{'logical'}{'option'}[$i]{'default'});
			next if($$parameter{'logical'}{'option'}[$i]{'default'} =~ m/false|FALSE|False/);
			$default = $i;
			$default_sys = $$parameter{'logical'}{'option'}[$i]{'id'};
			#DEBUG Dumper($$parameter{'logical'}{'option'}[$i]);
		}
		
	} elsif (($$parameter{'logical'}{'type'} eq "float") && ($$parameter{'conversion'}{'type'} eq 'float_integer_scale')){
    	#'conversion' => {
        #	'factor' => '200',
        #	'offset' => '-0.1',		
		#value->floatValue = ((double)value->integerValue / factor) - offset;
		#value->integerValue = std::lround((value->floatValue + offset) * factor);
		#float_integer_scale 0.05 offset: 0, factor: 200
		
		my $input = $$parameter{'logical'}{'default'};    $input = 0  if (!$input);		
		my $factor = $$parameter{'conversion'}{'factor'}; $factor = 0 if(!$factor);
		my $offset = $$parameter{'conversion'}{'offset'}; $offset = 0 if(!$offset);

		$default_sys = $input;
		$default = ($input + $offset) * $factor;
		#DEBUG "float_integer_scale ", $input, " offset: $offset, factor: $factor ", "out: $default";		

	} elsif (($$parameter{'logical'}{'type'} eq "float") && ($$parameter{'conversion'}{'type'} eq 'float_configtime')){
		#	int32_t factorIndex = (value->integerValue & 0xFF) >> 5;
		# 	switch(factorIndex); case 0: factor = 0.1;
		#						 case 1: factor = 1;
		#						 case 2: factor = 5;
		#						 case 3: factor = 10;
		#						 case 4: factor = 60;
		#						 case 5: factor = 300;
		#						 case 6: factor = 600;
		# 						 case 7: factor = 3600;
		#	value->floatValue = (value->integerValue & 0x1F) * factor;
		
		#int32_t factorIndex = 0;
		#double factor = 0.1;
		#if(value->floatValue < 0) value->floatValue = 0;
		#if(value->floatValue <= 3.1) { factorIndex = 0; factor = 0.1; }
		#else if(value->floatValue <= 31) { factorIndex = 1; factor = 1; }
		#else if(value->floatValue <= 155) { factorIndex = 2; factor = 5; }
		#else if(value->floatValue <= 310) { factorIndex = 3; factor = 10; }
		#else if(value->floatValue <= 1860) { factorIndex = 4; factor = 60; }
		#else if(value->floatValue <= 9300) { factorIndex = 5; factor = 300; }
		#else if(value->floatValue <= 18600) { factorIndex = 6; factor = 600; }
		#else { factorIndex = 7; factor = 3600; }
		#value->integerValue = ((factorIndex << 5) | std::lround(value->floatValue / factor)) & 0xFF;
		
		my $input = $$parameter{'logical'}{'default'};    $input = 0  if ((!$input) || ($input < 0));		
		my $factor = 0.1; 
		my $factor_index;
		
		if($input <= 3.1) { $factor_index = 0; $factor = 0.1; }
		elsif($input <= 31) { $factor_index = 1; $factor = 1; }
		elsif($input <= 155) { $factor_index = 2; $factor = 5; }
		elsif($input <= 310) { $factor_index = 3; $factor = 10; }
		elsif($input <= 1860) { $factor_index = 4; $factor = 60; }
		elsif($input <= 9300) { $factor_index = 5; $factor = 300; }
		elsif($input <= 18600) { $factor_index = 6; $factor = 600; }
		else { $factor_index = 7; $factor = 3600; }
		$default = (($factor_index << 5) | int(($input / $factor)+0.5)) & 0xFF;
		$default_sys = $input;
		#DEBUG "float_configtime ", "in: $input, out: $default";		
		#DEBUG Dumper($parameter);

	} elsif (($$parameter{'logical'}{'type'} eq "integer") && (!$$parameter{'conversion'})){
		$default = $$parameter{'logical'}{'default'};
		$default = 0   if (!$default);		
		$default_sys = $default;

	} else {
		DEBUG "Found paramset with a special type...", Dumper($parameter);
	}
	$unit = ''   if (!$unit);
	$unit = '%'  if ($unit eq '100%');
	return ($default, $type, $unit, $default_sys);
}
sub get_physical_section {
	my $parameter = shift;
	my $channel = shift;
	
	my $para_index     = get_index($$parameter{'physical'}{'index'});
	my $para_index_bit = get_index_bit($$parameter{'physical'}{'index'});
	my $para_size_bit  = get_size_bit($$parameter{'physical'}{'size'});
	my $para_channel   = $channel;
	my $para_list      = $$parameter{'physical'}{'list'};
	my $para_type      = $$parameter{'physical'}{'type'};;
	
	## take care of the count flag
	my $ret = {'channel' => $para_channel, 'list' => $para_list, 'index' => $para_index, 'index_bit' => $para_index_bit, 'size_bit' => $para_size_bit, 'type' => $para_type};
	#DEBUG Dumper($ret);

	return $ret;
}
sub get_index {
	## we got something like '0x15' or '2.7', this needs to be cleaned up and the return has to be a decimal number	
	## reflecting the number in front of the dot or converting the hex figure into dec
	my $number = shift;
	my ( $int, $rest ) = split /\./, $number, 2;
	$int  = 0 if !defined $int  || length $int  == 0;
	$rest = 0 if !defined $rest || length $rest == 0;
	return eval $int;
}
sub get_index_bit {
	## we got something like '0x15' or '2.7', this needs to be cleaned up and the return has to be a decimal number	
	## between 0 and 7 reflecting the starting bit
	my $number = shift;
	my ( $int, $rest ) = split /\./, $number, 2;
	$int  = 0 if !defined $int  || length $int  == 0;
	$rest = 0 if !defined $rest || length $rest == 0;
	return $rest;
}
sub get_size_bit {
	# size could be something like '0.1' or '1', but we want to store the bit value, so we have to sort out
	my $number = shift;
	my ( $int, $rest ) = split /\./, $number, 2;
	$int  = 0 if !defined $int  || length $int  == 0;
	$rest = 0 if !defined $rest || length $rest == 0;
	$int  = hex($int) if $int =~ /0x/;
	return ($int * 8) + $rest;
}


##########################################################
## + working +
## generate xml device info and return it as string
sub gen_xml_device_info {
	## config data should come from outside per hash
	my %config = %{(shift)};
	
	## step throug the channels and create xml string
	my $cnl_max = scalar @{$config{'channels'}};
	my $ret;

	## channel 0 should be Master anyhow and need more configuration
	## header
	my $user_master = UserModuls::get("Master");	
	$ret = $$user_master{'header'};

	## device section
	# rx_modes are depending on powerMode
	my $rx_flags = "";
	$rx_flags = "BURST" if ($config{'extended_config'}{'power_mode'} == 1);
	$rx_flags = "WAKEUP,LAZY_CONFIG" if ($config{'extended_config'}{'power_mode'} >= 2);
	#<device version="1" rx_modes="CONFIG,$rx_mode_flags" peering_sysinfo_expect_channel="false" supports_aes="true">
	$$user_master{'device'} =~ s/\$rx_mode_flags/$rx_flags/g;
	$ret .= $$user_master{'device'};

	## supported type section
	#<supported_types>
	#	<type name="$type_name" id="$type_id" updatable="true" priority="2">
	#		<parameter index="10.0" size="2.0" const_value="$type_const"/>
	$$user_master{'supported_types'} =~ s/\$type_id/$config{'extended_config'}{'id'}/g;
	$$user_master{'supported_types'} =~ s/\$type_name/$config{'extended_config'}{'name'}/g;
	my $model_id = sprintf("0x%04x", hex($config{'base_config'}{'model_ID'}));
	$$user_master{'supported_types'} =~ s/\$type_const/$model_id/g;
	$ret .= $$user_master{'supported_types'};
	
	## paramset master, check low_bat_limit first
	my ($lowbat_register, $lowbat_min, $lowbat_max, $lowbat_def) = ("",25,50,30);
	$lowbat_register = $$user_master{'lowbat_limit'}     if ($config{'extended_config'}{'lowbat_register'});
	$lowbat_min =  ($config{'extended_config'}{'lowbat_min'}/10)     if ($config{'extended_config'}{'lowbat_min'});
	$lowbat_max =  ($config{'extended_config'}{'lowbat_max'}/10)     if ($config{'extended_config'}{'lowbat_max'});
	$lowbat_def =  ($config{'extended_config'}{'lowbat_limit'}/10)   if ($config{'extended_config'}{'lowbat_limit'});
	$lowbat_register =~ s/\$lowbat_min/$lowbat_min/g; 
	$lowbat_register =~ s/\$lowbat_max/$lowbat_max/g; 
	$lowbat_register =~ s/\$lowbat_def/$lowbat_def/g;  
	
	$$user_master{'paramset_dev_master'} =~ s/\$low_bat_limit/$lowbat_register/g;
	$ret .= $$user_master{'paramset_dev_master'};
	
	## now we are in the channels section
	## check low_bat and duty cycle?
	my ($low_bat, $dutycycle) = ("","");
	$low_bat =   $$user_master{'param_lowbat'}     if ($config{'extended_config'}{'lowbat_signal'});
	$dutycycle = $$user_master{'param_dutycycle'}  if ($config{'extended_config'}{'dutycycle'});
	$$user_master{'param_default'} =~ s/\$lowbat/$low_bat/g;
	$$user_master{'param_default'} =~ s/\$dutycycle/$dutycycle/g;
	$ret .= $$user_master{'param_default'};

	## step through the user module array and put together the channel information
	my $channel_total = scalar @{$config{'channels'}};
	my @frames; my @paramsets;

	for(my $i = 1; $i < $channel_total; $i++) {
		next if (!$config{'channels'}[$i]);
		
		## looking forward to identify similar channels
		my $index = $i;	my $counter = 1;
		while ( ($config{'channels'}[$i+1]) && ($config{'channels'}[$i]{'type'} eq $config{'channels'}[$i+1]{'type'})) {
			$counter++;
			$i++;
		}
		
		## now we see only defined channels, lets search for a predefined modul - if not
		## available, choose a placeholder
		my $user_channel = UserModuls::get($config{'channels'}[$i]{'type'});	
		$user_channel = UserModuls::get('xmlDummy')   if(!$user_channel);

		## replace variables with real values and copy to the return value
		my $user_text = $$user_channel{'channels'};
		$user_text =~ s/\$channel_index/$index/g;
		$user_text =~ s/\$channel_count/$counter/g;
		$ret .= $user_text;

		## collect frames and parasets in an array, because we have to cleanup any doubles later
		my $user_frames = $$user_channel{'frames'};
		push @frames,$user_frames;
		my $user_paramsets = $$user_channel{'paramsets'};
		push @paramsets,$user_paramsets;
	} 
	
	$ret .= "	</channels>\n";
	## channels are done now

	## sorting out doubles in frames and paramsets	
	my @frames_clean = do { my %seen; grep { !$seen{$_}++ } @frames };
	my @paramsets_clean = do { my %seen; grep { !$seen{$_}++ } @paramsets };
	
	## put together the frames
	$ret .= "	<frames>\n";
	$ret .= join "", @frames_clean;
	$ret .= "	</frames>\n";
		
	## and now the paramsets	
	$ret .= "	<paramset_defs>\n";
	$ret .= join "", @paramsets_clean;
	$ret .= "	</paramset_defs>\n";
	
	my $user_modules             =UserModuls::get("xmlRemote");
	$ret .= "</device>\n";
	
	# xml content will be collected as a string and returned
	return $ret;
}
##########################################################

##########################################################
## + working +
## generate xml device info and return it as string
sub gen_reg_h_device_info {
	## config data should come from outside per hash, define the return object
	my %config = %{(shift)};
	my %ret;

	## channel 0 should be Master anyhow and need more configuration
	## read from user modules file
	my $user_master = UserModuls::get("Master");	
	$ret{'library'} = [$$user_master{'library'}];
	$ret{'stage'} = [$$user_master{'stage'}];
	$ret{'stage_counter'}[0] = 0;
	$ret{'config'}[0] = $$user_master{'config'};
	$ret{'config_index'}[0] = 0;

	## replace the variables in config section of master
	## conf key mode
	$ret{'config'}[0] =~ s/\$conf_keymode/$config{'extended_config'}{'conf_key_mode'}/g;	

	## power mode
	my @pw_mode = ('POWER_MODE_NO_SLEEP', 'POWER_MODE_WAKEUP_ONRADIO', 'POWER_MODE_WAKEUP_32MS', 'POWER_MODE_WAKEUP_64MS', 'POWER_MODE_WAKEUP_250MS', 'POWER_MODE_WAKEUP_8000MS', 'POWER_MODE_WAKEUP_EXT_INT');
	$ret{'config'}[0] =~ s/\$conf_powermode/$pw_mode[$config{'extended_config'}{'power_mode'}]/g;	

	## low battery check, if defined replace placeholders, otherwise delete line
	if ($config{'extended_config'}{'lowbat_signal'}) {
		$ret{'config'}[0] =~ s/\$conf_lowbat_limit/$config{'extended_config'}{'lowbat_limit'}/g;	
		$ret{'config'}[0] =~ s/\$conf_lowbat_timer/$config{'extended_config'}{'lowbat_timer'}/g;	
	} else {
		$ret{'config'}[0] =~ s/.*hm.bt.set.*\n//g;
	}	


	## step through all user channels starting with 1
	for my $i (1 .. $#{$config{'channels'}}) {
		## skip while channel def is empty
		next if (!$config{'channels'}[$i]);
		
		## get the respective library section and double check 
		## if already added to our return object
		my $user_channel = UserModuls::get($config{'channels'}[$i]{'type'});
		
		# search the return object and remember the position
		my ($index) = grep { $ret{'library'}[$_] eq $$user_channel{'library'} } 0..$#{$ret{'library'}};
		# if the entry exist already, increase counter for stage
		# otherwise collect respective information
		if ( $index ) {
			$ret{'stage_counter'}[$index] += 1;
			$ret{'config_index'}[$i] = $ret{'stage_counter'}[$index] - 1;
		} else {
			push @{$ret{'library'}}, $$user_channel{'library'};
			push @{$ret{'stage_counter'}}, 1;
			push @{$ret{'stage'}}, $$user_channel{'stage'};
			push @{$ret{'config_index'}}, 0;
		}

		## take care of the config section
		$ret{'config'}[$i] = $$user_channel{'config'};
		$ret{'config'}[$i] =~ s/\$cm_reg_channel/$i/g;	
		$ret{'config'}[$i] =~ s/\$cm_index/$ret{'config_index'}[$i]/g;	

		## take care of the peer amount per channel
		$ret{'peers'}[$i] = $config{'channels'}[$i]{'peers'};
	}
			
	## loop through the reduced list, starting with 1 while 0 is the maintenance channel
	## and replace the $cm_tot_index variable
	for my $i (1 .. $#{$ret{'stage'}}) {
    	$ret{'stage'}[$i] =~ s/\$cm_tot_index/$ret{'stage_counter'}[$i]/g;
	}	
	
	## generate HMSerialData
	# if hm_id is given, format it accordingly, 
	# if empty, generate a random number
	if ($config{'base_config'}{'hm_ID'}) {
		$ret{'general'}{'hm_ID'} = sprintf("%06x", hex($config{'base_config'}{'hm_ID'}));
	} else {
		$ret{'general'}{'hm_ID'} = randString('H',6);
	}

	# if serial_ID is given, format it accordingly, 
	# if empty, generate a random string
	if ($config{'base_config'}{'serial_ID'}) {
		$ret{'general'}{'serial_ID'} = $config{'base_config'}{'serial_ID'};
		$ret{'general'}{'serial_ID'} =~ s/[^a-zA-Z0-9]//g; 
	} else {
		$ret{'general'}{'serial_ID'} = "HB";
	}
	$ret{'general'}{'serial_ID'} .= randString('D', 10-length($ret{'general'}{'serial_ID'}) );

	

	## cleanup the return object, stage_counter and config_index not needed anymore
	delete($ret{'stage_counter'});
	delete($ret{'config_index'});

	## done, return the data object
	return \%ret;
}
## generates an DEC or HEX string with the given length
## returns the generated string
sub randString {
	# parameter needed by the function
	# randString('DEC or HEX', 'length of string')
	my $chkCnt = shift;	my $chkLen = shift;
	my $ret = ""; my @chars;

	@chars = ();
	@chars = ('a'..'f', '0'..'9') if ($chkCnt eq 'H');
	@chars = ('0'..'9')           if ($chkCnt eq 'D');
	
	while($chkLen--){ $ret .= $chars[rand @chars] };
	return $ret;
}
##########################################################

##########################################################
## + working +
## print functions for register.h and xml device file
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
	print " "x4 ." * \@brief Register definitions \n";
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
sub print_channel_defaults {
	my ($addr,$defs) = @_;
	#DEBUG Dumper($defs);
	my $size = 0;
	
	print " "x4 ."/** \n";
	print " "x4 ." * \@brief Channel, List defaults \n";
	print " "x4 ." * Source of the default values is the respective xml file. \n";
	print " "x4 ." * This values are the defined default values and should be set \n";
	print " "x4 ." * in the first start function. \n";
	print " "x4 ." */ \n";
	print " "x4 ."const uint8_t cnlDefs[] PROGMEM = { \n";

	## step through channels, lists
	foreach my $cnl_lst (sort keys %$addr) {
		print " "x8 ."// channel: $$addr{$cnl_lst}{'channel'}, list: $$addr{$cnl_lst}{'list'}";
		printf "%s",($$addr{$cnl_lst}{'link'})? ", link to $$addr{$cnl_lst}{'link'}\n" : "\n";
		
		## if we have content go further
		next if($$addr{$cnl_lst}{'link'});

		printf " "x8 ."0x%02x," x @{$$defs{$cnl_lst}{'reg'}} . "\n", @{$$defs{$cnl_lst}{'reg'}};
		$size += @{$$defs{$cnl_lst}{'reg'}};
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
	my $input = shift;

	print << 'END_LINE';
    /** 
     * \@brief First time start function 
     * This function is called by the main function on the first boot of a device. 
     * First boot is indicated by a magic byte in the eeprom. 
     * Here we can setup everything which is needed for a proper device operation, like cleaning 
     * of eeprom variables, or setting a default link in the peer table for 2 channels 
     */ 
    void firstTimeStart(void) { 

    #ifdef SER_DBG
        // some debug
        dbg << F("First time start active:\n");
        dbg << F("cnl\tlst\tsIdx\tsLen\thide\tpAddr\n");
        for (uint8_t i = 0; i < devDef.lstNbr; i++) {
            // cnl, lst, sIdx, sLen, hide, pAddr 
            dbg  << cnlTbl[i].cnl << "\t" << cnlTbl[i].lst << "\t" << cnlTbl[i].sIdx << "\t" << cnlTbl[i].sLen << "\t" << cnlTbl[i].vis << "\t" << cnlTbl[i].pAddr << "\n";
        }
    #endif

        // fill register with default values, peer registers are not filled while done in usermodules
END_LINE

	## step through the hash
	foreach my $cnl_lst (sort keys %$input) {
		## skip on list 3 and 4
		next if (($$input{$cnl_lst}{'list'} == 3)||($$input{$cnl_lst}{'list'} == 4));	
		print " "x8 ."hm.ee.setList($$input{$cnl_lst}{'channel'}, $$input{$cnl_lst}{'list'}, 0, (uint8_t*)&cnlDefs[$$input{$cnl_lst}{'slice_idx'}]);\n";
	}

	print << "END_LINE";
\n        // format peer db
        hm.ee.clearPeers();
    } \n;
END_LINE
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
##########################################################


1;
