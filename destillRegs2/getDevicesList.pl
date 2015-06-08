use strict;
use warnings;
use XML::LibXML;

## --------------import constants---------------------
use destillRegsModules;
usrMods::searchXMLFiles('');


exit 0;


#my $idxID; 																								# holds the text
#my $idxIx;																									# holds the hex register value combined with starting position
#my $idxVal;																								# holds the dec register value
#my $idxBgn; 																								# holds the starting bit
#my $idxSze;																								# holds the size of the register
#my $idxLst;																								# holds the list

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

