## -- definitions -------------------------------------------------------------------------------------------
my %regList;
 

## -- Sub Type ID information -------------------------------------------------------------------------------
##  "0x01" => "AlarmControl"      "0x41" => "sensor"          "0x70" => "THSensor"
##  "0x10" => "switch"            "0x42" => "swi"             "0x80" => "threeStateSensor"
##  "0x12" => "outputUnit"        "0x43" => "pushButton"      "0x81" => "motionDetector"
##  "0x20" => "dimmer"            "0x44" => "singleButton"    "0xC0" => "keyMatic"
##  "0x30" => "blindActuator"     "0x51" => "powerMeter"      "0xC1" => "winMatic"
##  "0x39" => "ClimateControl"    "0x58" => "thermostat"      "0xC3" => "tipTronic"
##  "0x40" => "remote"            "0x60" => "KFM100"          "0xCD" => "smokeDetector"

my $base_config = {
    "serial"=>       "HBremote01",
    "hmID"=>         "",
    "hmKEY"=>        "0102030405060708090a0b0c0d0e0f10",
	
    "modelID"=>      "00A9",
    "deviceInfo"=>   "060000",

    "firmwareVer"=>  "11",


};

my $extended_config = {
    "name"=>         "Test128", 
    "description"=>  "das ist ein test",

    "subtypeID"=>    "40",
	
    "localResDis"=>  1,
    "intKeysVis"=>   1,

    "confKeyMode"=>  1,

    "statusLED"=>    2,

    "battValue"=>    30,
    "battVisib"=>    0,
    "battChkDura"=>  3600000,

    "powerMode"=>    0,
    "burstRx"=>      1,
};


## -- device config -----------------------------------------------------------------------------------------
my %confType = (
    ## some basic information to create a new device, serial number and hm id has to be unique in your network
    ## on modelID you could choose between an already existing configuration or you can create a new device
	
    serial      => 'HBremote01',                           # 0 to get it automatically generated - otherwise 10 byte ASCII format
    hmID        => '',                                     # empty to get it automatically generated - otherwise 6 HEX digits (3 byte)
    hmKEY       => '0102030405060708090a0b0c0d0e0f10',     # 32 HEX digits (16 byte) HM AES Key 
	
    modelID     => '00A9',                                 # if model id is known, details will taken from HM config xml files, 4 HEX digits
    firmwareVer => '11',                                   # firmware version, 2 HEX digits - important if you took a model id where more then one device exists


    ## no input needed if model id is valid and device already exists in HM config software,
    ## otherwise fill accordingly to create a proper register.h and xml config file 

    name        => 'Test128',                              # name of the device, ascii [A-Z, a-z, 0-9, '-'], no blanks
    description => 'das ist ein test',                     # short description of the device

    subtypeID   => '40',                                   # depending on type of device / see above
    deviceInfo  => '060000',                               # not complete clear yes, but 3 bytes HEX needed - referer to count_from_sysinfo="23.0:1.0"
	
    burstRx     => 1,                                      # device needs a burst signal to wakeup
    localResDis => 1,                                      # local reset disable 
    intKeysVis  => 1,                                      # internal keys visible

    confKeyMode => 1,                                      # config key mode; 
	                                                         # 0 = no config key; 
	                                                         # 1 = pair on short press, reset device on double long key press
                                                           # 2 = short press to toogle channel one, long press to pair, and double long to reset device
    
    statusLED   => 2,                                      # amount of available status leds, possible values 0, 1 ,2
                                                           
    battValue   => 30,                                     # one byte default value in milli volt, 0 if not a battery device 
    battVisib   => 0,                                      # battery flag visible in registers of channel 0
    battChkDura => 3600000,	                               # the time between two measurements, value in milli seconds

	powerMode   => 0,                                      # there are 5 power modes available, which could be choosed to get the best ratio between power consumption and availablity
                                                           # 0, now power saving - 19.9ma
                                                           # 1, wake up every 250ms, check for wakeup signal on air and stay awake accordingly, timer gets updated every 256ms
                                                           # 2, deep sleep, wakeup every 250ms, not able to receive anything while sleeping, timer gets updated every 256ms
                                                           # 3, deep sleep, wakeup every 8 seconds, not able to receive anything while sleeping, timer gets updated every 8192ms - 0.04ma
                                                           # 4, deep sleep, wakeup only on interrupt - 0.00ma

);

## -- channel config ----------------------------------------------------------------------------------------
## predefined type in linkset.xml choose xmlDimmer, xmlSwitch, xmlKey, xmlWeather - more to come
## peers reflects the amount of possible peers, hidden makes a channel hidden for the config software, but will still work
## with linked you can link channels together, e.g. key to dimmer
## todo: linked

#$regList{1}     = {type => "xmlBlind",  peers => 6, hidden => 0, linked => 0     };
#$regList{2}     = {type => "xmlDimmer", peers => 1, hidden => 0, linked => 0     };
#$regList{3}     = {type => "xmlDimmer", peers => 1, hidden => 0, linked => 0     };
#$regList{3}     = {type => "xmlRemote", peers => 6, hidden => 0, linked => 2     };
$regList{1}     = {type => "xmlRemote", peers => 6, hidden => 0, linked => 0     };
$regList{2}     = {type => "xmlRemote", peers => 6, hidden => 0, linked => 0     };
$regList{3}     = {type => "xmlRemote", peers => 6, hidden => 0, linked => 0     };
$regList{4}     = {type => "xmlRemote", peers => 6, hidden => 0, linked => 0     };
$regList{5}     = {type => "xmlRemote", peers => 6, hidden => 0, linked => 0     };
$regList{6}     = {type => "xmlRemote", peers => 6, hidden => 0, linked => 0     };

#$regList{1}     = {type => "xmlSwitch", peers => 6, hidden => 0, linked => 0     };
#$regList{2}     = {type => "xmlSwitch", peers => 6, hidden => 0, linked => 0     };
#$regList{3}     = {type => "xmlSwitch", peers => 6, hidden => 0, linked => 0     };
#$regList{4}     = {type => "xmlSwitch", peers => 6, hidden => 0, linked => 0     };
#$regList{2}     = {type => "xmlDimmer", peers => 6, hidden => 0, linked => 0     };
#$regList{3}     = {type => "xmlSwitch", peers => 6, hidden => 0, linked => {3,4} };
#$regList{4}     = {type => "xmlRemote", peers => 6, hidden => 0, linked => 2     };
#$regList{3}     = {type => "xmlSwitch", peers => 6, hidden => 0, linked => 2     };
#$regList{4}     = {type => "xmlSwitch", peers => 6, hidden => 0, linked => 2     };
	





## -- helpers -----------------------------------------------------------------------------------------------
package usrRegs;

sub get_perl_config {
  #my $x = %base_conf;
  return my $x = {base_config=>$base_config,extended_config=>$extended_config};
}

sub usr_getHash($){
  my $hn = shift;
  return %regList    if($hn eq "regList"      );
  return %confType   if($hn eq "configType"       );
}
