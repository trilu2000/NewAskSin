##############################################
# CUL HomeMatic handler
# $Id$

#####################################################
# configuration data for CUL_HM -used to split code and configuration
package HMConfig;

use strict;
use warnings;

##----------definitions for register settings-----------------	
	# definition of Register for all devices
	# a: address, incl bits 13.4 4th bit in reg 13
	# s: size 2.0 = 2 byte, 0.5 = 5 bit. Max is 4.0!!
	# l: list number. List0 will be for channel 0
	#     List 1 will set peer to 00000000
	#     list 3 will need the input of a peer!
	# min: minimal input value
	# max: maximal input value
	# c: conversion, will point to a routine for calculation
	# f: factor to be used if c = 'factor'
	# u: unit for description
	# t: txt description
	# lit: if the command is a literal options will be entered here
	# d: if '1' the register will appear in Readings
	#
my %culHmRegDefShLg = (# register that are available for short AND long button press. Will be merged to rgister list at init
#blindActuator mainly   
  ActionType      =>{a=> 10.0,s=>0.2,l=>3,min=>0  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>""             ,lit=>{off=>0,jmpToTarget=>1,toggleToCnt=>2,toggleToCntInv=>3}},
  OffTimeMode     =>{a=> 10.6,s=>0.1,l=>3,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"off time mode",lit=>{absolut=>0,minimal=>1}},
  OnTimeMode      =>{a=> 10.7,s=>0.1,l=>3,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"on time mode" ,lit=>{absolut=>0,minimal=>1}},
  MaxTimeF        =>{a=> 29.0,s=>1.0,l=>3,min=>0  ,max=>25.4    ,c=>''         ,f=>10      ,u=>'s'   ,d=>0,t=>"max time first direction"},
  DriveMode       =>{a=> 31.0,s=>1.0,l=>3,min=>0  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>""             ,lit=>{direct=>0,viaUpperEnd=>1,viaLowerEnd=>2,viaNextEnd=>3}},
#dimmer mainly                                                                                 
  OnDly           =>{a=>  6.0,s=>1.0,l=>3,min=>0  ,max=>111600  ,c=>'fltCvT'   ,f=>''      ,u=>'s'   ,d=>0,t=>"on delay"},
  OnTime          =>{a=>  7.0,s=>1.0,l=>3,min=>0  ,max=>111600  ,c=>'fltCvT'   ,f=>''      ,u=>'s'   ,d=>0,t=>"on time, 111600 = infinite"},
  OffDly          =>{a=>  8.0,s=>1.0,l=>3,min=>0  ,max=>111600  ,c=>'fltCvT'   ,f=>''      ,u=>'s'   ,d=>0,t=>"off delay"},
  OffTime         =>{a=>  9.0,s=>1.0,l=>3,min=>0  ,max=>111600  ,c=>'fltCvT'   ,f=>''      ,u=>'s'   ,d=>0,t=>"off time, 111600 = infinite"},

  ActionTypeDim   =>{a=> 10.0,s=>0.4,l=>3,min=>0  ,max=>8       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>""             ,lit=>{off=>0,jmpToTarget=>1,toggleToCnt=>2,toggleToCntInv=>3,upDim=>4,downDim=>5,toggelDim=>6,toggelDimToCnt=>7,toggelDimToCntInv=>8}},
  OffDlyBlink     =>{a=> 14.5,s=>0.1,l=>3,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>""             ,lit=>{off=>0,on=>1}},
  OnLvlPrio       =>{a=> 14.6,s=>0.1,l=>3,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>""             ,lit=>{high=>0,low=>1}},
  OnDlyMode       =>{a=> 14.7,s=>0.1,l=>3,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>""             ,lit=>{setToOff=>0,NoChange=>1}},
  OffLevel        =>{a=> 15.0,s=>1.0,l=>3,min=>0  ,max=>100     ,c=>''         ,f=>2       ,u=>'%'   ,d=>0,t=>"PowerLevel off"},
  OnMinLevel      =>{a=> 16.0,s=>1.0,l=>3,min=>0  ,max=>100     ,c=>''         ,f=>2       ,u=>'%'   ,d=>0,t=>"minimum PowerLevel"},
  OnLevel         =>{a=> 17.0,s=>1.0,l=>3,min=>0  ,max=>100     ,c=>''         ,f=>2       ,u=>'%'   ,d=>1,t=>"PowerLevel on"},

  OffLevelKm      =>{a=> 15.0,s=>1.0,l=>3,min=>0  ,max=>127.5   ,c=>''         ,f=>2       ,u=>'%'   ,d=>0,t=>"OnLevel 127.5=locked"},
  OnLevelKm       =>{a=> 17.0,s=>1.0,l=>3,min=>0  ,max=>127.5   ,c=>''         ,f=>2       ,u=>'%'   ,d=>0,t=>"OnLevel 127.5=locked"},
  RampOnSp        =>{a=> 34.0,s=>1.0,l=>3,min=>0  ,max=>1       ,c=>''         ,f=>200     ,u=>'s'   ,d=>0,t=>"Ramp on speed"},
  RampOffSp       =>{a=> 35.0,s=>1.0,l=>3,min=>0  ,max=>1       ,c=>''         ,f=>200     ,u=>'s'   ,d=>0,t=>"Ramp off speed"},

  RampSstep       =>{a=> 18.0,s=>1.0,l=>3,min=>0  ,max=>100     ,c=>''         ,f=>2       ,u=>'%'   ,d=>0,t=>"rampStartStep"},
  RampOnTime      =>{a=> 19.0,s=>1.0,l=>3,min=>0  ,max=>111600  ,c=>'fltCvT'   ,f=>''      ,u=>'s'   ,d=>0,t=>"rampOnTime"},
  RampOffTime     =>{a=> 20.0,s=>1.0,l=>3,min=>0  ,max=>111600  ,c=>'fltCvT'   ,f=>''      ,u=>'s'   ,d=>0,t=>"rampOffTime"},
  DimMinLvl       =>{a=> 21.0,s=>1.0,l=>3,min=>0  ,max=>100     ,c=>''         ,f=>2       ,u=>'%'   ,d=>0,t=>"dimMinLevel"},
  DimMaxLvl       =>{a=> 22.0,s=>1.0,l=>3,min=>0  ,max=>100     ,c=>''         ,f=>2       ,u=>'%'   ,d=>0,t=>"dimMaxLevel"},
  DimStep         =>{a=> 23.0,s=>1.0,l=>3,min=>0  ,max=>100     ,c=>''         ,f=>2       ,u=>'%'   ,d=>0,t=>"dimStep"},

  OffDlyNewTime   =>{a=> 25.0,s=>1.0,l=>3,min=>0.1,max=>25.6    ,c=>''         ,f=>10      ,u=>'s'   ,d=>0,t=>"off delay new time"},
  OffDlyOldTime   =>{a=> 26.0,s=>1.0,l=>3,min=>0.1,max=>25.6    ,c=>''         ,f=>10      ,u=>'s'   ,d=>0,t=>"off delay old time"},
  DimElsOffTimeMd =>{a=> 38.6,s=>0.1,l=>3,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>""             ,lit=>{absolut=>0,minimal=>1}},
  DimElsOnTimeMd  =>{a=> 38.7,s=>0.1,l=>3,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>""             ,lit=>{absolut=>0,minimal=>1}},
  DimElsActionType=>{a=> 38.0,s=>0.4,l=>3,min=>0  ,max=>8       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>""             ,lit=>{off=>0,jmpToTarget=>1,toggleToCnt=>2,toggleToCntInv=>3,upDim=>4,downDim=>5,toggelDim=>6,toggelDimToCnt=>7,toggelDimToCntInv=>8}},
#output Unit                                                                                       
  ActTypeMp3      =>{a=> 36  ,s=>1  ,l=>3,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>0,t=>"Tone or MP3 to be played"},
  ActTypeLed      =>{a=> 36  ,s=>1  ,l=>3,min=>0  ,max=>255     ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"LED color"          ,lit=>{no=>0x00,red_short=>0x11,red_long=>0x12,green_short=>0x21,green_long=>0x22,orange_short=>0x31,orange_long=>0x32}},
  ActTypeOuCf     =>{a=> 36  ,s=>1  ,l=>3,min=>0  ,max=>255     ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"type sound or LED"  ,lit=>{no=>0,short=>1,long=>2}},
  ActNum          =>{a=> 37  ,s=>1  ,l=>3,min=>1  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>0,t=>"Number of repetitions"},
  Intense         =>{a=> 43  ,s=>1  ,l=>3,min=>10 ,max=>255     ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Volume",lit=>{vol_100=>255,vol_90=>250,vol_80=>246,vol_70=>240,vol_60=>234,vol_50=>227,vol_40=>218,vol_30=>207,vol_20=>190,vol_10=>162,vol_00=>10}},  
# statemachines
  BlJtOn          =>{a=> 11.0,s=>0.4,l=>3,min=>0  ,max=>9       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from on"      ,lit=>{no=>0,dlyOn=>1,refOn=>2,on=>3,dlyOff=>4,refOff=>5,off=>6,rampOn=>8,rampOff=>9}},
  BlJtOff         =>{a=> 11.4,s=>0.4,l=>3,min=>0  ,max=>9       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from off"     ,lit=>{no=>0,dlyOn=>1,refOn=>2,on=>3,dlyOff=>4,refOff=>5,off=>6,rampOn=>8,rampOff=>9}},
  BlJtDlyOn       =>{a=> 12.0,s=>0.4,l=>3,min=>0  ,max=>9       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from delayOn" ,lit=>{no=>0,dlyOn=>1,refOn=>2,on=>3,dlyOff=>4,refOff=>5,off=>6,rampOn=>8,rampOff=>9}},
  BlJtDlyOff      =>{a=> 12.4,s=>0.4,l=>3,min=>0  ,max=>9       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from delayOff",lit=>{no=>0,dlyOn=>1,refOn=>2,on=>3,dlyOff=>4,refOff=>5,off=>6,rampOn=>8,rampOff=>9}},
  BlJtRampOn      =>{a=> 13.0,s=>0.4,l=>3,min=>0  ,max=>9       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from rampOn"  ,lit=>{no=>0,dlyOn=>1,refOn=>2,on=>3,dlyOff=>4,refOff=>5,off=>6,rampOn=>8,rampOff=>9}},
  BlJtRampOff     =>{a=> 13.4,s=>0.4,l=>3,min=>0  ,max=>9       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from rampOff" ,lit=>{no=>0,dlyOn=>1,refOn=>2,on=>3,dlyOff=>4,refOff=>5,off=>6,rampOn=>8,rampOff=>9}},
  BlJtRefOn       =>{a=> 30.0,s=>0.4,l=>3,min=>0  ,max=>9       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from refOn"   ,lit=>{no=>0,dlyOn=>1,refOn=>2,on=>3,dlyOff=>4,refOff=>5,off=>6,rampOn=>8,rampOff=>9}},
  BlJtRefOff      =>{a=> 30.4,s=>0.4,l=>3,min=>0  ,max=>9       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from refOff"  ,lit=>{no=>0,dlyOn=>1,refOn=>2,on=>3,dlyOff=>4,refOff=>5,off=>6,rampOn=>8,rampOff=>9}},
  
  DimJtOn         =>{a=> 11.0,s=>0.4,l=>3,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from on"      ,lit=>{no=>0,dlyOn=>1,rampOn=>2,on=>3,dlyOff=>4,rampOff=>5,off=>6}},
  DimJtOff        =>{a=> 11.4,s=>0.4,l=>3,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from off"     ,lit=>{no=>0,dlyOn=>1,rampOn=>2,on=>3,dlyOff=>4,rampOff=>5,off=>6}},
  DimJtDlyOn      =>{a=> 12.0,s=>0.4,l=>3,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from delayOn" ,lit=>{no=>0,dlyOn=>1,rampOn=>2,on=>3,dlyOff=>4,rampOff=>5,off=>6}},
  DimJtDlyOff     =>{a=> 12.4,s=>0.4,l=>3,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from delayOff",lit=>{no=>0,dlyOn=>1,rampOn=>2,on=>3,dlyOff=>4,rampOff=>5,off=>6}},
  DimJtRampOn     =>{a=> 13.0,s=>0.4,l=>3,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from rampOn"  ,lit=>{no=>0,dlyOn=>1,rampOn=>2,on=>3,dlyOff=>4,rampOff=>5,off=>6}},
  DimJtRampOff    =>{a=> 13.4,s=>0.4,l=>3,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from rampOff" ,lit=>{no=>0,dlyOn=>1,rampOn=>2,on=>3,dlyOff=>4,rampOff=>5,off=>6}},

  DimElsJtOn      =>{a=> 39.0,s=>0.4,l=>3,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"else Jump from on"      ,lit=>{no=>0,dlyOn=>1,rampOn=>2,on=>3,dlyOff=>4,rampOff=>5,off=>6}},
  DimElsJtOff     =>{a=> 39.4,s=>0.4,l=>3,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"else Jump from off"     ,lit=>{no=>0,dlyOn=>1,rampOn=>2,on=>3,dlyOff=>4,rampOff=>5,off=>6}},
  DimElsJtDlyOn   =>{a=> 40.0,s=>0.4,l=>3,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"else Jump from delayOn" ,lit=>{no=>0,dlyOn=>1,rampOn=>2,on=>3,dlyOff=>4,rampOff=>5,off=>6}},
  DimElsJtDlyOff  =>{a=> 40.4,s=>0.4,l=>3,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"else Jump from delayOff",lit=>{no=>0,dlyOn=>1,rampOn=>2,on=>3,dlyOff=>4,rampOff=>5,off=>6}},
  DimElsJtRampOn  =>{a=> 41.0,s=>0.4,l=>3,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"else Jump from rampOn"  ,lit=>{no=>0,dlyOn=>1,rampOn=>2,on=>3,dlyOff=>4,rampOff=>5,off=>6}},
  DimElsJtRampOff =>{a=> 41.4,s=>0.4,l=>3,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"else Jump from rampOff" ,lit=>{no=>0,dlyOn=>1,rampOn=>2,on=>3,dlyOff=>4,rampOff=>5,off=>6}},
  
  ttJtOn          =>{a=> 11.0,s=>0.4,l=>3,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from on"      ,lit=>{no=>0,on=>2,off=>5}},
  ttJtOff         =>{a=> 11.4,s=>0.4,l=>3,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from off"     ,lit=>{no=>0,on=>2,off=>5}},

  SwJtOn          =>{a=> 11.0,s=>0.4,l=>3,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from on"      ,lit=>{no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}},
  SwJtOff         =>{a=> 11.4,s=>0.4,l=>3,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from off"     ,lit=>{no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}},
  SwJtDlyOn       =>{a=> 12.0,s=>0.4,l=>3,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from delayOn" ,lit=>{no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}},
  SwJtDlyOff      =>{a=> 12.4,s=>0.4,l=>3,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from delayOff",lit=>{no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}},

  KeyJtOn         =>{a=> 11.0,s=>0.4,l=>3,min=>0  ,max=>7       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from on"      ,lit=>{no=>0,dlyUnlock=>1,rampUnlock=>2,unLock=>3,dlyLock=>4,rampLock=>5,lock=>6,open=>8}},
  KeyJtOff        =>{a=> 11.4,s=>0.4,l=>3,min=>0  ,max=>7       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from off"     ,lit=>{no=>0,dlyUnlock=>1,rampUnlock=>2,unLock=>3,dlyLock=>4,rampLock=>5,lock=>6,open=>8}},

  WinJtOn         =>{a=> 11.0,s=>0.4,l=>3,min=>0  ,max=>9       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from off"     ,lit=>{no=>0,rampOnDly=>1,rampOn=>2,on=>3,rampOffDly=>4,rampOff=>5,off=>6,rampOnFast=>8,rampOffFast=>9}},
  WinJtOff        =>{a=> 11.4,s=>0.4,l=>3,min=>0  ,max=>9       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from off"     ,lit=>{no=>0,rampOnDly=>1,rampOn=>2,on=>3,rampOffDly=>4,rampOff=>5,off=>6,rampOnFast=>8,rampOffFast=>9}},
  WinJtRampOn     =>{a=> 13.0,s=>0.4,l=>3,min=>0  ,max=>9       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from off"     ,lit=>{no=>0,rampOnDly=>1,rampOn=>2,on=>3,rampOffDly=>4,rampOff=>5,off=>6,rampOnFast=>8,rampOffFast=>9}},
  WinJtRampOff    =>{a=> 13.4,s=>0.4,l=>3,min=>0  ,max=>9       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jump from off"     ,lit=>{no=>0,rampOnDly=>1,rampOn=>2,on=>3,rampOffDly=>4,rampOff=>5,off=>6,rampOnFast=>8,rampOffFast=>9}},
  
  CtRampOn        =>{a=>  1.0,s=>0.4,l=>3,min=>0  ,max=>5       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jmp on condition from rampOn"   ,lit=>{geLo=>0,geHi=>1,ltLo=>2,ltHi=>3,between=>4,outside=>5}},
  CtRampOff       =>{a=>  1.4,s=>0.4,l=>3,min=>0  ,max=>5       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jmp on condition from rampOff"  ,lit=>{geLo=>0,geHi=>1,ltLo=>2,ltHi=>3,between=>4,outside=>5}},
  CtDlyOn         =>{a=>  2.0,s=>0.4,l=>3,min=>0  ,max=>5       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jmp on condition from delayOn"  ,lit=>{geLo=>0,geHi=>1,ltLo=>2,ltHi=>3,between=>4,outside=>5}},
  CtDlyOff        =>{a=>  2.4,s=>0.4,l=>3,min=>0  ,max=>5       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jmp on condition from delayOff" ,lit=>{geLo=>0,geHi=>1,ltLo=>2,ltHi=>3,between=>4,outside=>5}},
  CtOn            =>{a=>  3.0,s=>0.4,l=>3,min=>0  ,max=>5       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jmp on condition from on"       ,lit=>{geLo=>0,geHi=>1,ltLo=>2,ltHi=>3,between=>4,outside=>5}},
  CtOff           =>{a=>  3.4,s=>0.4,l=>3,min=>0  ,max=>5       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jmp on condition from off"      ,lit=>{geLo=>0,geHi=>1,ltLo=>2,ltHi=>3,between=>4,outside=>5}},
  CtValLo         =>{a=>  4.0,s=>1  ,l=>3,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>0,t=>"Condition value low for CT table"  },
  CtValHi         =>{a=>  5.0,s=>1  ,l=>3,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>0,t=>"Condition value high for CT table" },
  CtRefOn         =>{a=> 28.0,s=>0.4,l=>3,min=>0  ,max=>5       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jmp on condition from refOn"    ,lit=>{geLo=>0,geHi=>1,ltLo=>2,ltHi=>3,between=>4,outside=>5}},
  CtRefOff        =>{a=> 28.4,s=>0.4,l=>3,min=>0  ,max=>5       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jmp on condition from refOff"   ,lit=>{geLo=>0,geHi=>1,ltLo=>2,ltHi=>3,between=>4,outside=>5}},

  CtrlRc          =>{a=> 46  ,s=>0.4,l=>3,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Jmp on condition from refOff"   ,lit=>{no=>0,tempSh=>1,auto=>2,auto_tempSh=>3,manu_tempSh=>4,boost=>5,toggle=>6}},
  TempRC          =>{a=> 45  ,s=>0.6,l=>3,min=>5  ,max=>30      ,c=>''         ,f=>''      ,u=>'C'   ,d=>0,t=>"Jmp on condition from refOff"},
);

my %culHmRegDefine = (
#--- list 0, device  and protocol level-----------------
  burstRx         =>{a=>  1.0,s=>1.0,l=>0,min=>0  ,max=>255     ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>'device reacts on Burst'        ,lit=>{off=>0,on=>200}},# not sure what 'on' is. Also change Tx mode TODO!!
  intKeyVisib     =>{a=>  2.7,s=>0.1,l=>0,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>'visibility of internal channel',lit=>{invisib=>0,visib=>1}},
  pairCentral     =>{a=> 10.0,s=>3.0,l=>0,min=>0  ,max=>16777215,c=>'hex'      ,f=>''      ,u=>''    ,d=>1,t=>'pairing to central'},
#repeater                                                                                      
  compMode        =>{a=> 23.0,s=>0.1,l=>0,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"compatibility moden"     ,lit=>{off=>0,on=>1}},
#remote mainly                                                                                      
  backlOnTime     =>{a=>  5.0,s=>0.6,l=>0,min=>0  ,max=>5       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Backlight ontime[s]"        ,lit=>{0=>0,5=>1,10=>2,15=>3,20=>4,25=>5}},
  backlOnMode     =>{a=>  5.6,s=>0.2,l=>0,min=>0  ,max=>2       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Backlight mode"          ,lit=>{off=>0,auto=>2}},
  ledMode         =>{a=>  5.6,s=>0.2,l=>0,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"LED mode"                ,lit=>{off=>0,on=>1}},
  language        =>{a=>  7.0,s=>1.0,l=>0,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"Language"                ,lit=>{English=>0,German=>1}},
  backAtKey       =>{a=> 13.7,s=>0.1,l=>0,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"Backlight at keystroke"  ,lit=>{off=>0,on=>1}},
  backAtMotion    =>{a=> 13.6,s=>0.1,l=>0,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"Backlight at motion"     ,lit=>{off=>0,on=>1}},
  backAtCharge    =>{a=> 13.5,s=>0.1,l=>0,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"Backlight at Charge"     ,lit=>{off=>0,on=>1}},
  stbyTime        =>{a=> 14.0,s=>1.0,l=>0,min=>1  ,max=>99      ,c=>''         ,f=>''      ,u=>'s'   ,d=>1,t=>"Standby Time"},          
  backOnTime      =>{a=> 14.0,s=>1.0,l=>0,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>'s'   ,d=>1,t=>"Backlight On Time"},     
  btnLock         =>{a=> 15.0,s=>1.0,l=>0,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Button Lock"             ,lit=>{unlock=>0,lock=>1}},
                                                                                                                                       
  confBtnTime     =>{a=> 21.0,s=>1.0,l=>0,min=>1  ,max=>255     ,c=>''         ,f=>''      ,u=>'min' ,d=>0,t=>"255=permanent"},         
# keymatic/winmatic secific register                                                                                                    
  keypressSignal  =>{a=>  3.0,s=>0.1,l=>0,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Keypress beep"           ,lit=>{off=>0,on=>1}},
  signal          =>{a=>  3.4,s=>0.1,l=>0,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Confirmation beep"       ,lit=>{off=>0,on=>1}},
  signalTone      =>{a=>  3.6,s=>0.2,l=>0,min=>0  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>""                        ,lit=>{low=>0,mid=>1,high=>2,veryHigh=>3}},

  brightness      =>{a=>  4.0,s=>0.4,l=>0,min=>0  ,max=>15      ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"Display brightness"},
  energyOpt       =>{a=>  8.0,s=>1.0,l=>0,min=>0  ,max=>127     ,c=>''         ,f=>2       ,u=>'s'   ,d=>1,t=>"energy Option: Duration of ilumination"},
# sec_mdir                                                                                   
  cyclicInfoMsg   =>{a=>  9.0,s=>1.0,l=>0,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"cyclic message"          ,lit=>{off=>0,on=>1}},
  sabotageMsg     =>{a=> 16.0,s=>1.0,l=>0,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"enable sabotage message" ,lit=>{off=>0,on=>1}},
  cyclicInfoMsgDis=>{a=> 17.0,s=>1.0,l=>0,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"cyclic message"},
  lowBatLimit     =>{a=> 18.0,s=>1.0,l=>0,min=>10 ,max=>12      ,c=>''         ,f=>10      ,u=>'V'   ,d=>1,t=>"low batterie limit, step .1V"},
  lowBatLimitBA   =>{a=> 18.0,s=>1.0,l=>0,min=>5  ,max=>15      ,c=>''         ,f=>10      ,u=>'V'   ,d=>0,t=>"low batterie limit, step .1V"},
  lowBatLimitFS   =>{a=> 18.0,s=>1.0,l=>0,min=>2  ,max=>3       ,c=>''         ,f=>10      ,u=>'V'   ,d=>0,t=>"low batterie limit, step .1V"},
  lowBatLimitRT   =>{a=> 18.0,s=>1.0,l=>0,min=>2  ,max=>2.5     ,c=>''         ,f=>10      ,u=>'V'   ,d=>0,t=>"low batterie limit, step .1V"},
  batDefectLimit  =>{a=> 19.0,s=>1.0,l=>0,min=>0.1,max=>2       ,c=>''         ,f=>100     ,u=>'Ohm' ,d=>1,t=>"batterie defect detection"},
  transmDevTryMax =>{a=> 20.0,s=>1.0,l=>0,min=>1  ,max=>10      ,c=>''         ,f=>''      ,u=>''    ,d=>0,t=>"max message re-transmit"},
  localResDis     =>{a=> 24.0,s=>1.0,l=>0,min=>1  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"local reset disable"       ,lit=>{off=>0,on=>1}},
  globalBtnLock   =>{a=> 25.0,s=>1.0,l=>0,min=>1  ,max=>255     ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"global button lock"        ,lit=>{off=>0,on=>200}},
  modusBtnLock    =>{a=> 25.0,s=>1.0,l=>0,min=>1  ,max=>255     ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"mode button lock"          ,lit=>{off=>0,on=>200}},
  paramSel        =>{a=> 27.0,s=>1.0,l=>0,min=>0  ,max=>4       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"data transfered to peer"   ,lit=>{off=>0,T1=>1,T2=>2,T1_T2=>3,T2_T1=>4}},
  RS485IdleTime   =>{a=> 29.0,s=>1.0,l=>0,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>'s'   ,d=>0,t=>"Idle Time"},
#un-identified List0
# addr Dec!!
# SEC-WM55 02:01 (AES on?)
# SEC-WDS  02:01 16:01(sabotage) ?
# HM-SEC-MDIR  02:01 ?
# SEC-SC   02:00 ?
# Blind     9:00 10:00 20:00
# BL1TPBU  02:01 21:FF
# Dim1TPBU 02:01 21:FF 22:00
#Keymatic 3.3 unknown, seen 1 here
  
#--- list 1, Channel level------------------
#blindActuator mainly                                                                             
  sign            =>{a=>  8.0,s=>0.1,l=>1,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"signature (AES)",lit=>{off=>0,on=>1}},

  driveDown       =>{a=> 11.0,s=>2.0,l=>1,min=>0  ,max=>6000.0  ,c=>''         ,f=>10      ,u=>'s'   ,d=>1,t=>"drive time up"},
  driveUp         =>{a=> 13.0,s=>2.0,l=>1,min=>0  ,max=>6000.0  ,c=>''         ,f=>10      ,u=>'s'   ,d=>1,t=>"drive time up"},
  driveTurn       =>{a=> 15.0,s=>1.0,l=>1,min=>0.5,max=>25.5    ,c=>''         ,f=>10      ,u=>'s'   ,d=>1,t=>"engine uncharge - fhem min = 0.5s for protection. HM min= 0s (use regBulk if necessary)"},
  refRunCounter   =>{a=> 16.0,s=>1.0,l=>1,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>0,t=>"reference run counter"},
#remote mainly                                                                                      
  longPress       =>{a=>  4.4,s=>0.4,l=>1,min=>0.3,max=>1.8     ,c=>'m10s3'    ,f=>''      ,u=>'s'   ,d=>0,t=>"time to detect key long press"},
  dblPress        =>{a=>  9.0,s=>0.4,l=>1,min=>0  ,max=>1.5     ,c=>''         ,f=>10      ,u=>'s'   ,d=>0,t=>"time to detect double press"},
  msgShowTime     =>{a=> 45.0,s=>1.0,l=>1,min=>0.0,max=>120     ,c=>''         ,f=>2       ,u=>'s'   ,d=>1,t=>"Message show time(RC19). 0=always on"},
  beepAtAlarm     =>{a=> 46.0,s=>0.2,l=>1,min=>0  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"Beep Alarm"        ,lit=>{none=>0,tone1=>1,tone2=>2,tone3=>3}},
  beepAtService   =>{a=> 46.2,s=>0.2,l=>1,min=>0  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"Beep Service"      ,lit=>{none=>0,tone1=>1,tone2=>2,tone3=>3}},
  beepAtInfo      =>{a=> 46.4,s=>0.2,l=>1,min=>0  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"Beep Info"         ,lit=>{none=>0,tone1=>1,tone2=>2,tone3=>3}},
  backlAtAlarm    =>{a=> 47.0,s=>0.2,l=>1,min=>0  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"Backlight Alarm"   ,lit=>{off=>0,on=>1,blinkSlow=>2,blinkFast=>3}},
  backlAtService  =>{a=> 47.2,s=>0.2,l=>1,min=>0  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"Backlight Service" ,lit=>{off=>0,on=>1,blinkSlow=>2,blinkFast=>3}},
  backlAtInfo     =>{a=> 47.4,s=>0.2,l=>1,min=>0  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"Backlight Info"    ,lit=>{off=>0,on=>1,blinkSlow=>2,blinkFast=>3}},

#dimmer  mainly                                                                                  
  loadErrCalib	  =>{a=> 18.0,s=>1.0,l=>1,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>""    ,d=>0,t=>"Load Error Calibration"},
  transmitTryMax  =>{a=> 48.0,s=>1.0,l=>1,min=>1  ,max=>10      ,c=>''         ,f=>''      ,u=>""    ,d=>0,t=>"max message re-transmit"},
  loadAppearBehav =>{a=> 49.0,s=>0.2,l=>1,min=>0  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>""    ,d=>1,t=>"behavior on load appearence at restart",lit=>{off=>0,last=>1,btnPress=>2,btnPressIfWasOn=>3}},
  ovrTempLvl      =>{a=> 50.0,s=>1.0,l=>1,min=>30 ,max=>100     ,c=>''         ,f=>''      ,u=>"C"   ,d=>0,t=>"overtemperatur level"},
  fuseDelay		  =>{a=> 51.0,s=>1.0,l=>1,min=>0  ,max=>2.55    ,c=>''         ,f=>100     ,u=>"s"   ,d=>0,t=>"fuse delay"},
  redTempLvl      =>{a=> 52.0,s=>1.0,l=>1,min=>30 ,max=>100     ,c=>''         ,f=>''      ,u=>"C"   ,d=>0,t=>"reduced temperatur recover"},
  redLvl          =>{a=> 53.0,s=>1.0,l=>1,min=>0  ,max=>100     ,c=>''         ,f=>2       ,u=>"%"   ,d=>0,t=>"reduced power level"},
  powerUpAction	  =>{a=> 86.0,s=>0.1,l=>1,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>""    ,d=>1,t=>"behavior on power up"                  ,lit=>{off=>0,on=>1}},
  statusInfoMinDly=>{a=> 87.0,s=>0.5,l=>1,min=>0.5,max=>15.5    ,c=>''         ,f=>2       ,u=>"s"   ,d=>0,t=>"status message min delay"},
  statusInfoRandom=>{a=> 87.5,s=>0.3,l=>1,min=>0  ,max=>7       ,c=>''         ,f=>''      ,u=>"s"   ,d=>0,t=>"status message random delay"},
  characteristic  =>{a=> 88.0,s=>0.1,l=>1,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>""    ,d=>1,t=>""                                      ,lit=>{linear=>0,square=>1}},
  logicCombination=>{a=> 89.0,s=>0.5,l=>1,min=>0  ,max=>16      ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>""             ,lit=>{inactive=>0,or=>1,and=>2,xor=>3,nor=>4,nand=>5,orinv=>6,andinv=>7,plus=>8,minus=>9,mul=>10,plusinv=>11,minusinv=>12,mulinv=>13,invPlus=>14,invMinus=>15,invMul=>16}},
#SCD                                                                                  
  msgScdPosA      =>{a=> 32.6,s=>0.2,l=>1,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Message for position A",lit=>{noMsg=>0,lvlNormal=>1}},
  msgScdPosB      =>{a=> 32.4,s=>0.2,l=>1,min=>0  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Message for position B",lit=>{noMsg=>0,lvlNormal=>1,lvlAddStrong=>2,lvlAdd=>3}},
  msgScdPosC      =>{a=> 32.2,s=>0.2,l=>1,min=>0  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Message for position C",lit=>{noMsg=>0,lvlNormal=>1,lvlAddStrong=>2,lvlAdd=>3}},
  msgScdPosD      =>{a=> 32.0,s=>0.2,l=>1,min=>0  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Message for position D",lit=>{noMsg=>0,lvlNormal=>1,lvlAddStrong=>2,lvlAdd=>3}},
#wds - different literals
  msgWdsPosA      =>{a=> 32.6,s=>0.2,l=>1,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Message for position A",lit=>{noMsg=>0,dry=>1}},
  msgWdsPosB      =>{a=> 32.4,s=>0.2,l=>1,min=>0  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Message for position B",lit=>{noMsg=>0,dry=>1,water=>2,wet=>3}},
  msgWdsPosC      =>{a=> 32.2,s=>0.2,l=>1,min=>0  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Message for position C",lit=>{noMsg=>0,       water=>2,wet=>3}},
#rhs - different literals
  msgRhsPosA      =>{a=> 32.6,s=>0.2,l=>1,min=>0  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Message for position A",lit=>{noMsg=>0,closed=>1,open=>2,tilted=>3}},
  msgRhsPosB      =>{a=> 32.4,s=>0.2,l=>1,min=>0  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Message for position B",lit=>{noMsg=>0,closed=>1,open=>2,tilted=>3}},
  msgRhsPosC      =>{a=> 32.2,s=>0.2,l=>1,min=>0  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Message for position C",lit=>{noMsg=>0,closed=>1,open=>2,tilted=>3}},
#SC - different literals
  msgScPosA       =>{a=> 32.6,s=>0.2,l=>1,min=>0  ,max=>2       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Message for position A",lit=>{noMsg=>0,closed=>1,open=>2}},
  msgScPosB       =>{a=> 32.4,s=>0.2,l=>1,min=>0  ,max=>2       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Message for position B",lit=>{noMsg=>0,closed=>1,open=>2}},
# keymatic/winmatic secific register                                                                     
  holdTime        =>{a=> 20  ,s=>1,  l=>1,min=>0  ,max=>8.16    ,c=>''         ,f=>31.25   ,u=>'s'   ,d=>0,t=>"Holdtime for door opening"},
  holdPWM         =>{a=> 21  ,s=>1,  l=>1,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>0,t=>"Holdtime pulse wide modulation"},
  setupDir        =>{a=> 22  ,s=>0.1,l=>1,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"Rotation direction for locking",lit=>{right=>0,left=>1}},
  setupPosition   =>{a=> 23  ,s=>1  ,l=>1,min=>0  ,max=>3000    ,c=>''         ,f=>0.06666 ,u=>'deg' ,d=>1,t=>"Rotation angle neutral position"},
  angelOpen       =>{a=> 24  ,s=>1  ,l=>1,min=>0  ,max=>3000    ,c=>''         ,f=>0.06666 ,u=>'deg' ,d=>1,t=>"Door opening angle"},
  angelMax        =>{a=> 25  ,s=>1  ,l=>1,min=>0  ,max=>3000    ,c=>''         ,f=>0.06666 ,u=>'deg' ,d=>1,t=>"Angle maximum"},
  angelLocked     =>{a=> 26  ,s=>1  ,l=>1,min=>0  ,max=>3000    ,c=>''         ,f=>0.06666 ,u=>'deg' ,d=>1,t=>"Angle Locked position"},
  pullForce       =>{a=> 28  ,s=>1  ,l=>1,min=>0  ,max=>100     ,c=>''         ,f=>2       ,u=>'%'   ,d=>1,t=>"pull force level"},
  pushForce       =>{a=> 29  ,s=>1  ,l=>1,min=>0  ,max=>100     ,c=>''         ,f=>2       ,u=>'%'   ,d=>1,t=>"push force level"},
  tiltMax         =>{a=> 30  ,s=>1  ,l=>1,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"maximum tilt level"},
  ledFlashUnlocked=>{a=> 31.3,s=>0.1,l=>1,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"LED blinks when not locked",lit=>{off=>0,on=>1}},
  ledFlashLocked  =>{a=> 31.6,s=>0.1,l=>1,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"LED blinks when locked"    ,lit=>{off=>0,on=>1}},

  waterUppThr     =>{a=>  6.0,s=>1  ,l=>1,min=>0  ,max=>256     ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"water upper threshold"},
  waterlowThr     =>{a=>  7.0,s=>1  ,l=>1,min=>0  ,max=>256     ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"water lower threshold"},
  caseDesign      =>{a=> 90.0,s=>1  ,l=>1,min=>1  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"case desing"               ,lit=>{verticalBarrel=>1,horizBarrel=>2,rectangle=>3}},
  caseHigh        =>{a=> 94.0,s=>2  ,l=>1,min=>100,max=>10000   ,c=>''         ,f=>''      ,u=>'cm'  ,d=>1,t=>"case hight"},
  fillLevel       =>{a=> 98.0,s=>2  ,l=>1,min=>100,max=>300     ,c=>''         ,f=>''      ,u=>'cm'  ,d=>1,t=>"fill level"},
  caseWidth       =>{a=>102.0,s=>2  ,l=>1,min=>100,max=>10000   ,c=>''         ,f=>''      ,u=>'cm'  ,d=>1,t=>"case width"},
  caseLength      =>{a=>106.0,s=>2  ,l=>1,min=>100,max=>10000   ,c=>''         ,f=>''      ,u=>'cm'  ,d=>1,t=>"case length"},
  meaLength       =>{a=>108.0,s=>2  ,l=>1,min=>110,max=>310     ,c=>''         ,f=>''      ,u=>'cm'  ,d=>1,t=>""},
  useCustom       =>{a=>110.0,s=>1  ,l=>1,min=>110,max=>310     ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"use custom"      ,lit=>{off=>0,on=>1}},

  evtFltrPeriod   =>{a=>  1.0,s=>0.4,l=>1,min=>0.5,max=>7.5     ,c=>''         ,f=>2       ,u=>'s'   ,d=>1,t=>"event filter period"},
  evtFltrNum      =>{a=>  1.4,s=>0.4,l=>1,min=>1  ,max=>15      ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"sensitivity - read sach n-th puls"},
  minInterval     =>{a=>  2.0,s=>0.3,l=>1,min=>0  ,max=>4       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"minimum interval in sec"   ,lit=>{15=>0,30=>1,60=>2,120=>3,240=>4}},
  captInInterval  =>{a=>  2.3,s=>0.1,l=>1,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"capture within interval"   ,lit=>{off=>0,on=>1}},
  brightFilter    =>{a=>  2.4,s=>0.4,l=>1,min=>0  ,max=>7       ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"brightness filter - ignore light at night"},
  eventDlyTime    =>{a=> 33  ,s=>1  ,l=>1,min=>0  ,max=>7620    ,c=>'fltCvT60' ,f=>''      ,u=>'s'   ,d=>1,t=>"event delay time"},
  ledOnTime       =>{a=> 34  ,s=>1  ,l=>1,min=>0  ,max=>1.275   ,c=>''         ,f=>200     ,u=>'s'   ,d=>0,t=>"LED ontime"},
  eventFilterTime =>{a=> 35  ,s=>1  ,l=>1,min=>0  ,max=>7620    ,c=>'fltCvT60' ,f=>''      ,u=>'s'   ,d=>0,t=>"event filter time"},
  eventFilterTimeB=>{a=> 35  ,s=>1  ,l=>1,min=>5  ,max=>7620    ,c=>'fltCvT60' ,f=>''      ,u=>'s'   ,d=>0,t=>"event filter time"},
# - different range
  evtFltrTime     =>{a=> 35.0,s=>1  ,l=>1,min=>600,max=>1200    ,c=>'fltCvT'   ,f=>''      ,u=>'s'   ,d=>0,t=>"event filter time"},

# weather units                                                                                  
  stormUpThresh   =>{a=>  6  ,s=>1  ,l=>1,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"Storm upper threshold"},
  stormLowThresh  =>{a=>  7  ,s=>1  ,l=>1,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"Storm lower threshold"},
# others
  localResetDis   =>{a=>  7  ,s=>1  ,l=>1,min=>0  ,max=>255     ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"LocalReset disable",lit=>{off=>0,on=>1}},
#un-identified List1
# SEC-WM55 08:01 (AES on?)
# SEC-WDS  34:0x64 ?
# SEC-SC   08:00 ?
# RC19     08:00 ?
# Bl1PBU   08:00 09:00 10:00

#  logicCombination=>{a=> 89.0,s=>0.5,l=>1,min=>0  ,max=>16      ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"".
#		                                                                                                      "inactive=>unused\n".
#                                                                                                              "or      =>max(state,chan)\n".
#					                                                                                          "and     =>min(state,chan)\n".
#					                                                                                          "xor     =>0 if both are != 0, else max\n".
#					                                                                                          "nor     =>100-max(state,chan)\n".
#					                                                                                          "nand    =>100-min(state,chan)\n".
#					                                                                                          "orinv   =>max((100-chn),state)\n".
#					                                                                                          "andinv  =>min((100-chn),state)\n".
#					                                                                                          "plus    =>state + chan\n".
#					                                                                                          "minus   =>state - chan\n".
#					                                                                                          "mul     =>state * chan\n".
#					                                                                                          "plusinv =>state + 100 - chan\n".
#					                                                                                          "minusinv=>state - 100 + chan\n".
#					                                                                                          "mulinv  =>state * (100 - chan)\n".
#					                                                                                          "invPlus =>100 - state - chan\n".
#					                                                                                          "invMinus=>100 - state + chan\n".
#					                                                                                          "invMul  =>100 - state * chan\n",lit=>{inactive=>0,or=>1,and=>2,xor=>3,nor=>4,nand=>5,orinv=>6,andinv=>7,plus=>8,minus=>9,mul=>10,plusinv=>11,minusinv=>12,mulinv=>13,invPlus=>14,invMinus=>15,invMul=>16}},
#
#					  
#CC-TC                                                                                        

#--- list 3, link level for actor - mainly in short/long hash, only specials here------------------
  lgMultiExec     =>{a=>138.5,s=>0.1,l=>3,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>0,t=>"multiple execution per repeat of long trigger"    ,lit=>{off=>0,on=>1}},

#--- list 4, link level for Button ------------------                                                                                     
  peerNeedsBurst  =>{a=>  1.0,s=>0.1,l=>4,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"peer expects burst",lit=>{off=>0,on=>1}},
  expectAES       =>{a=>  1.7,s=>0.1,l=>4,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"expect AES"        ,lit=>{off=>0,on=>1}},
  lcdSymb         =>{a=>  2.0,s=>0.1,l=>4,min=>0  ,max=>255     ,c=>'hex'      ,f=>''      ,u=>''    ,d=>0,t=>"bitmask which symbol to display on message"},
  lcdLvlInterp    =>{a=>  3.0,s=>0.1,l=>4,min=>0  ,max=>255     ,c=>'hex'      ,f=>''      ,u=>''    ,d=>0,t=>"bitmask for symbols"},

  fillLvlUpThr    =>{a=>  4.0,s=>1  ,l=>4,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"fill level upper threshold"},
  fillLvlLoThr    =>{a=>  5.0,s=>1  ,l=>4,min=>0  ,max=>255     ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"fill level lower threshold"},

#--- list 5,6 parameter for channel ------------------
  displayMode     =>{a=>  1.0,s=>0.1,l=>5,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>""                ,lit=>{"temp-only"=>0,"temp-hum"=>1}},
  displayTemp     =>{a=>  1.1,s=>0.1,l=>5,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>""                ,lit=>{actual=>0,setpoint=>1}},
  displayTempUnit =>{a=>  1.2,s=>0.1,l=>5,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>""                ,lit=>{celsius=>0,fahrenheit=>1}},
  controlMode     =>{a=>  1.3,s=>0.2,l=>5,min=>0  ,max=>3       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>""                ,lit=>{manual=>0,auto=>1,central=>2,party=>3}},
  decalcDay       =>{a=>  1.5,s=>0.3,l=>5,min=>0  ,max=>7       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"Decalc weekday"  ,lit=>{Sat=>0,Sun=>1,Mon=>2,Tue=>3,Wed=>4,Thu=>5,Fri=>6}},
  mdTempValve     =>{a=>  2.6,s=>0.2,l=>5,min=>0  ,max=>2       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>""                ,lit=>{auto=>0,close=>1,open=>2}},
  "day-temp"      =>{a=>  3  ,s=>0.6,l=>5,min=>6  ,max=>30      ,c=>''         ,f=>2       ,u=>'C'   ,d=>1,t=>"comfort temp value"},
  "night-temp"    =>{a=>  4  ,s=>0.6,l=>5,min=>6  ,max=>30      ,c=>''         ,f=>2       ,u=>'C'   ,d=>1,t=>"comfort temp value"},
  tempWinOpen     =>{a=>  5  ,s=>0.6,l=>5,min=>6  ,max=>30      ,c=>''         ,f=>2       ,u=>'C'   ,d=>1,t=>"Temperature for Win open !chan 3 only!"},
  "party-temp"    =>{a=>  6  ,s=>0.6,l=>5,min=>6  ,max=>30      ,c=>''         ,f=>2       ,u=>'C'   ,d=>1,t=>"Temperature for Party"},
  decalMin        =>{a=>  8  ,s=>0.3,l=>5,min=>0  ,max=>50      ,c=>''         ,f=>0.1     ,u=>'min' ,d=>1,t=>"Decalc min"},
  decalHr         =>{a=>  8.3,s=>0.5,l=>5,min=>0  ,max=>23      ,c=>''         ,f=>''      ,u=>'h'   ,d=>1,t=>"Decalc hour"},

  partyEndHr      =>{a=> 97  ,s=>0.6,l=>6,min=>0  ,max=>23      ,c=>''         ,f=>''      ,u=>'h'   ,d=>1,t=>"Party end hour. Use cmd partyMode to set"},
  partyEndMin     =>{a=> 97.7,s=>0.1,l=>6,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>'min' ,d=>1,t=>"Party end min. Use cmd partyMode to set"   ,lit=>{"00"=>0,"30"=>1}},
  partyEndDay     =>{a=> 98  ,s=>1  ,l=>6,min=>0  ,max=>200     ,c=>''         ,f=>''      ,u=>'d'   ,d=>1,t=>"Party duration days. Use cmd partyMode to set"},
#Thermal-cc-VD                                                                                  
  valveOffset     =>{a=>  9  ,s=>0.5,l=>5,min=>0  ,max=>25      ,c=>''         ,f=>''      ,u=>'%'   ,d=>1,t=>"Valve offset"},             # size actually 0.5
  valveErrorPos   =>{a=> 10  ,s=>1  ,l=>5,min=>0  ,max=>99      ,c=>''         ,f=>''      ,u=>'%'   ,d=>1,t=>"Valve position when error"},# size actually 0.7

  btnNoBckLight   =>{a=>  9.4,s=>0.1,l=>7,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"button response without backlight",lit=>{off=>0,on=>1}},
  tempComfort     =>{a=>  1  ,s=>0.6,l=>7,min=>15 ,max=>30      ,c=>''         ,f=>'2'     ,u=>''    ,d=>1,t=>"comfort temperatur"},
  tempLowering    =>{a=>  2  ,s=>0.6,l=>7,min=>5  ,max=>25      ,c=>''         ,f=>'2'     ,u=>''    ,d=>1,t=>"lowering temperatur"},
  tempMin         =>{a=>  3  ,s=>0.6,l=>7,min=>4.5,max=>25      ,c=>''         ,f=>'2'     ,u=>''    ,d=>1,t=>"minimum temperatur"},
  tempMax         =>{a=>  4  ,s=>0.6,l=>7,min=>15 ,max=>30.5    ,c=>''         ,f=>'2'     ,u=>''    ,d=>1,t=>"maximum temperatur"},
  tempFallWinOpen =>{a=>  5  ,s=>0.6,l=>7,min=>5  ,max=>30      ,c=>''         ,f=>'2'     ,u=>''    ,d=>1,t=>"lowering temp whenWindow is opened"},
  tempFallWinPerio=>{a=>  6  ,s=>0.4,l=>7,min=>0  ,max=>60      ,c=>''         ,f=>'0.2'   ,u=>'min' ,d=>1,t=>"period lowering when window is open"},
  decalcWeekday   =>{a=>  7  ,s=>0.3,l=>7,min=>0  ,max=>7       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"decalcification day"      ,lit=>{Sat=>0,Sun=>1,Mon=>2,Tue=>3,Wed=>4,Thu=>5,Fri=>6}},
  decalcTime      =>{a=>  8  ,s=>0.6,l=>7,min=>0  ,max=>1410    ,c=>''         ,f=>'0.033' ,u=>''    ,d=>1,t=>"decalcification time"},
  tempOffset      =>{a=>  9  ,s=>0.4,l=>7,min=>0  ,max=>15      ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"temperature offset",lit=>{"-3.5K"=>0,"-3.0K"=>1,"-2.5K"=>2,"-2.0K"=>3,"-1.5K"=>4,"-1.0K"=>5,"-0.5K"=>6, 
                                                                                                                                        "0.0K"=>7, "0.5K"=>8, "1.0K"=>10, "1.5K"=>11, "2.0K"=>12, "2.5K"=>13, "3.0K"=>14, "3.5K"=>15}},
  boostPos        =>{a=> 10.0,s=>0.5,l=>7,min=>0  ,max=>100     ,c=>''         ,f=>'0.2'   ,u=>'%'   ,d=>1,t=>"boost period [min]"},
  boostPeriod     =>{a=> 10.5,s=>0.3,l=>7,min=>0  ,max=>6       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"boost position"           ,lit=>{0=>0,5=>1,10=>2,15=>3,20=>4,25=>5,30=>6}},
  boostAftWinOpen =>{a=> 14.5,s=>0.1,l=>7,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"boost after window opened",lit=>{off=>0,on=>1}},
                                    
  daylightSaveTime=>{a=> 14  ,s=>0.1,l=>7,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"set daylight saving time",lit=>{off=>0,on=>1}},
  regAdaptive     =>{a=> 14.1,s=>0.2,l=>7,min=>0  ,max=>2       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"adaptive regulation: offDef, offdetrmine, on",lit=>{off=>0,offDeter=>1,on=>2}},
  showInfo        =>{a=> 14.3,s=>0.2,l=>7,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"show date or time"                           ,lit=>{time=>0,date=>1}},
  noMinMan4Manu   =>{a=> 14.6,s=>0.1,l=>7,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"min/max is irrelevant for manual mode"       ,lit=>{off=>0,on=>1}},
  showWeekday     =>{a=> 14.7,s=>0.1,l=>7,min=>0  ,max=>1       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"show weekday"                                ,lit=>{off=>0,on=>1}},
  valveOffset     =>{a=> 11  ,s=>0.7,l=>7,min=>0  ,max=>100     ,c=>''         ,f=>''      ,u=>'%'   ,d=>1,t=>"offset for valve"},
  valveMaxPos     =>{a=> 12  ,s=>0.7,l=>7,min=>0  ,max=>100     ,c=>''         ,f=>''      ,u=>'%'   ,d=>1,t=>"valve maximum position"},
  valveErrPos     =>{a=> 13  ,s=>0.7,l=>7,min=>0  ,max=>100     ,c=>''         ,f=>''      ,u=>'%'   ,d=>1,t=>"valve error position"},
                                    
  modePrioManu    =>{a=> 18.3,s=>0.3,l=>7,min=>0  ,max=>5       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"allow tempChange for manual by...",lit=>{RT_SC=>0,all=>1,RT_CCU=>3,CCU=>4,self=>5}},
  modePrioParty   =>{a=> 18.0,s=>0.3,l=>7,min=>0  ,max=>5       ,c=>'lit'      ,f=>''      ,u=>''    ,d=>1,t=>"allow tempChange for party by..." ,lit=>{RT_SC=>0,all=>1,RT_CCU=>3,CCU=>4,self=>5}},
                                    
  reguIntI        =>{a=>202.0,s=>1  ,l=>7,min=>10 ,max=>20      ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"regulator I-param internal mode"},
  reguIntP        =>{a=>203.0,s=>1  ,l=>7,min=>25 ,max=>35      ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"regulator P-param internal mode"},
  reguIntPstart   =>{a=>204.0,s=>1  ,l=>7,min=>5  ,max=>45      ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"regulator P-param internal mode start value"},
  reguExtI        =>{a=>205.0,s=>1  ,l=>7,min=>10 ,max=>20      ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"regulator I-param extern mode"},
  reguExtP        =>{a=>206.0,s=>1  ,l=>7,min=>25 ,max=>35      ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"regulator P-param extern mode"},
  reguExtPstart   =>{a=>207.0,s=>1  ,l=>7,min=>5  ,max=>45      ,c=>''         ,f=>''      ,u=>''    ,d=>1,t=>"regulator P-param extern mode start value"},
  );
  
my %culHmRegGeneral = (
  intKeyVisib=>1,pairCentral=>1,
	);
my %culHmRegType = (
  swi               =>{peerNeedsBurst  =>1,expectAES       =>1},
  remote            =>{peerNeedsBurst  =>1,expectAES       =>1,dblPress        =>1,longPress       =>1,
					   sign            =>1
                      },
  blindActuator     =>{driveUp         =>1,driveDown       =>1,driveTurn       =>1,refRunCounter   =>1,
                       sign            =>1,
                       MaxTimeF        =>1,                                    
                       OnDly           =>1,OnTime          =>1,OffDly          =>1,OffTime         =>1,
  	   		           OffLevel        =>1,OnLevel         =>1,                                    
                       ActionType      =>1,OnTimeMode      =>1,OffTimeMode     =>1,DriveMode       =>1,
				       BlJtOn          =>1,BlJtOff         =>1,BlJtDlyOn       =>1,BlJtDlyOff      =>1,
                       BlJtRampOn      =>1,BlJtRampOff     =>1,BlJtRefOn       =>1,BlJtRefOff      =>1,
                       CtValLo         =>1,CtValHi         =>1,
                       CtOn            =>1,CtDlyOn         =>1,CtRampOn        =>1,CtRefOn         =>1,
				       CtOff           =>1,CtDlyOff        =>1,CtRampOff       =>1,CtRefOff        =>1,
				       lgMultiExec     =>1
				       },
  dimmer            =>{transmitTryMax  =>1,statusInfoMinDly=>1,statusInfoRandom=>1,powerUpAction   =>1,
                       OnDly           =>1,OnTime          =>1,OffDly          =>1,OffTime         =>1,
                       OffDlyBlink     =>1,OnLvlPrio       =>1,OnDlyMode       =>1,
		               ActionTypeDim   =>1,OnTimeMode      =>1,OffTimeMode     =>1,
		               OffLevel        =>1,OnMinLevel      =>1,OnLevel         =>1,               
                       RampSstep       =>1,RampOnTime      =>1,RampOffTime     =>1,
		               DimMinLvl       =>1,DimMaxLvl       =>1,DimStep         =>1,
                       DimJtOn         =>1,DimJtOff        =>1,DimJtDlyOn      =>1,
                       DimJtDlyOff     =>1,DimJtRampOn     =>1,DimJtRampOff    =>1,
                       CtValLo         =>1,CtValHi         =>1,
                       CtOn            =>1,CtDlyOn         =>1,CtRampOn        =>1,
                       CtOff           =>1,CtDlyOff        =>1,CtRampOff       =>1,
		               OffDlyNewTime   =>1,OffDlyOldTime   =>1,
		               lgMultiExec     =>1
		               },
);
#clones - - - - - - - - - - - - - - -   
$culHmRegType{pushButton}     = $culHmRegType{remote};

my %culHmRegModel = (
  "HM-RC-12"        =>{backAtKey       =>1, backAtMotion   =>1, backOnTime     =>1},
  );
#clones - - - - - - - - - - - - - - -   

$culHmRegModel{"ASH550I"}          = $culHmRegModel{"HM-WDS10-TH-O"};
$culHmRegModel{"ASH550"}           = $culHmRegModel{"HM-WDS10-TH-O"};

my %culHmRegChan = (# if channelspecific then enter them here 
  "HM-CC-TC02"        =>{displayMode     =>1,displayTemp     =>1,displayTempUnit =>1,
                         controlMode     =>1,decalcDay       =>1,
                         "day-temp"      =>1,"night-temp"    =>1,"party-temp"    =>1,
			             mdTempValve     =>1,partyEndDay     =>1,
			             partyEndMin     =>1,partyEndHr      =>1,
			             decalHr         =>1,decalMin        =>1
                      },    
					  );
#clones - - - - - - - - - - - - - - -   
$culHmRegChan{"HM-RC-19-B12"}     = $culHmRegChan{"HM-RC-1912"};

sub HMConfig_getHash($){
  my $hn = shift;
  return %culHmRegDefShLg       if($hn eq "culHmRegDefShLg"      );
  return %culHmRegDefine        if($hn eq "culHmRegDefine"       );
#  return %culHmRegGeneral       if($hn eq "culHmRegGeneral"      );
#  return %culHmRegType          if($hn eq "culHmRegType"         );
#  return %culHmRegModel         if($hn eq "culHmRegModel"        );
#  return %culHmRegChan          if($hn eq "culHmRegChan"         );
  
}
1;
