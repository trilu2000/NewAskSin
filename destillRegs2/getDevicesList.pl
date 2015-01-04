use strict;
use warnings;
use XML::LibXML;

my $dir = 'devicetypes';																					# directory with the HM device files
opendir(DIR, $dir) or die $!;																				# open the directory

while (my $file = readdir(DIR)) {																			# step through the files
	
	next unless (-f "$dir/$file");																			# we only want files
	next unless ($file =~ m/\.xml$/);        																# use a regular expression to find files ending in .xml

	#-- create parser object --------------------------------------------------------------
	my $parser = XML::LibXML->new();																		# create the xml object
	my $doc    = $parser->parse_file("$dir/$file");															# open the file
	my $xc = XML::LibXML::XPathContext->new( $doc->documentElement()  );									# create parser object
	

	#-- get the device version ------------------------------------------------------------
	my($sections) = $xc->findnodes('/device');																# set pointer to device
	my $devVer = $sections->getAttribute('version');														# get the device version	


	#-- get the device name and id --------------------------------------------------------
	foreach $sections ($xc->findnodes('/device/supported_types/type')) {									# set pointer to type

		my $devName = $sections->getAttribute('name');														# get the device name	
		my $devID = $sections->getAttribute('id');															# get the device id	
		#print "$devName, $devID\n";

		
		my ($devIdx, $devCVal) = (0,0);
		my $devFW = 0;

		foreach my $param ($sections->findnodes('./parameter')) {											# set pointer to parameter
			$devIdx = $param->getAttribute('index');														# get the device index	
			
			if ($devIdx ==  '9.0') {																		# reflects the firmware
				
				my $condOP = $param->getAttribute('cond_op');
				if (( $condOP eq "GE" ) || ( $condOP eq "LE" ) || ( $condOP eq "EQ" )) {
					$devFW = eval $param->getAttribute('const_value');
					#print "$devFW\n";
				}

			}

			if ($devIdx != '10.0') {																		# only 10.0 is interesting
				next;
			}

			$devCVal = eval $param->getAttribute('const_value');											# getting the const value
			
			
			#-- generating output ---------------------------------------------------------
			print sprintf("0x%.4x   0x%.2x    %-25s   %-65s", $devCVal, $devFW, $devID, $devName) ."   $file\n";
			
		}


	}


}
closedir(DIR);																								# close the directory again



exit 0;


#my $idxID; 																									# holds the text
#my $idxIx;																									# holds the hex register value combined with starting position
#my $idxVal;																									# holds the dec register value
#my $idxBgn; 																								# holds the starting bit
#my $idxSze;																									# holds the size of the register
#my $idxLst;																									# holds the list

#foreach my $sections ($xc->findnodes('/paramset/parameter')) {
#
#	#-- get the right node in the file ----------------------------------------------------
#	$idxID = $sections->getAttribute('id');																	# get the index name
#
#	my($physical) = $sections->findnodes('./physical');														# pointer to physical line
#	my($index) = $physical->getAttribute('index');															# all content in physical is an attribute, get the index
#	my($size) =  $physical->getAttribute('size');															# all content in physical is an attribute, get the size
#	
#	$idxLst = $physical->getAttribute('list');																# getting the list information
#
#
#	#-- get the starting bit position if exits, otherwise assume it is 0 ------------------
#	$idxBgn = 0;																							# default is bit 0 
#
#	if (index($index, '.') != -1) {																			# check for starting bit position
#		$idxBgn = substr $index, -1;																		# bit position is the last number
#		$index = substr($index, 0, -2);																		# shorten the index to get it converted to a dec number
#	} 
#	$idxVal = eval $index;																					# convert from hex to dec if necessary
#
#
#	#-- getting the size in bit -----------------------------------------------------------
#	my ($hiValue, $loValue) = (0, 0);
#	my $i = index($size, '.');
#	
#	if ($i == -1) {																							# no dot in size
#		$hiValue = $size;
#
#	} else {																								# we have to split up 
#		$hiValue = substr($size, 0, $i);																	# get the bytes in front of the dot
#		$loValue = substr($size, $i+1, 1);																	# and after the dot
#	}
#	$idxSze = ($hiValue*8) + $loValue;																		# recalculate the amount of bit
#
#
#	print "ID: $idxID \nV: $idxVal \t B: $idxBgn \t S: $idxSze \t L: $idxLst  \n\n";
#}

