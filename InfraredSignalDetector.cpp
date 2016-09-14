/*
 * infraredSignalDetector.c
 *
 *  Created on: 16 Jan 2016
 *      Author: raimund
 */

#include <InfraredSignalDetector.h>


uint8_t reportedHigh = 0;
uint16_t limit = 0;
const int sensorPin = 5;
unsigned long lastReportedTs = 0;



InfraRedSignalDetector::InfraRedSignalDetector(){
	//limit = 1023 / 2 ;
}

void InfraRedSignalDetector::setLimit(uint8_t sensitivity){
	/* sensitivity is 1-199, analog reading 0-1023
	 * cover range from 6-1020
	*/
	limit = (sensitivity + 5) * 5;
	Serial << F("setLimit input: ") << sensitivity << F(" limit: ") << limit << '\n';
}

uint8_t InfraRedSignalDetector::powerInputChanged(){
	unsigned long now = getMillis();

	//return (random(0, 100001) > 90001) ? 1 : 0;

	//pinMode(A0, INPUT);
	/*power_adc_enable();
	power_all_enable();
*/
	int reading = analogRead(sensorPin);
/*	reading = analogRead(sensorPin);
	reading = analogRead(sensorPin);*/
	uint8_t currentHigh = reading > limit ? 1 : 0;


	if(now > lastReportedTs + 1200){
		Serial << F("Sensor reading: ") << analogRead(sensorPin) << F(" limit: ") << limit << F(" currentHigh: ") << currentHigh << '\n';
		lastReportedTs = now;
	}

	if(currentHigh && !reportedHigh){
		reportedHigh = 1;
		return 1;
	} else {
		led.set(key_long);
		reportedHigh = currentHigh;
	}
	return 0;

}
