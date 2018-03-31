/*
 * Copyright 2012-2013 BrewPi/Elco Jacobs.
 * Copyright 2013 Matthew McGowan.
 *
 * This file is part of BrewPi.
 * 
 * BrewPi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * BrewPi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with BrewPi.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Brewpi.h"
#include "TemperatureFormats.h"
#include "OneWireTempSensor.h"
#include "DallasTemperature.h"
#include "OneWire.h"
#include "OneWireDevices.h"
#include "PiLink.h"
#include "Ticks.h"


OneWireTempSensor::~OneWireTempSensor() {
    delete sensor;
};

/**
 * Initializes the temperature sensor.
 * This method is called when the sensor is first created and also any time the sensor reports it's disconnected.
 * If the result is TEMP_SENSOR_DISCONNECTED then subsequent calls to read() will also return TEMP_SENSOR_DISCONNECTED.
 * Clients should attempt to re-initialize the sensor by calling init() again. 
 */
bool OneWireTempSensor::init() {

    // save address and pinNr for log messages
    char addressString[17];
    printBytes(sensorAddress, 8, addressString);
#if BREWPI_DEBUG
    uint8_t pinNr = oneWire->pinNr();
#endif

    bool success = false;

    if (sensor == NULL) {
        sensor = new DallasTemperature(oneWire);
        if (sensor == NULL) {
            logErrorString(ERROR_SRAM_SENSOR, addressString);
        }
    }

    logDebug("init onewire sensor");
    // This quickly tests if the sensor is connected and initializes the reset detection if necessary.
    if (sensor){
        // If this is the first conversion after power on, the device will return DEVICE_DISCONNECTED
        // Because HIGH_ALARM_TEMP will be copied from EEPROM
        temperature temp = sensor->getTempRaw(sensorAddress);
        if(temp == DEVICE_DISCONNECTED){
            // Device was just powered on and should be initialized
            if(sensor->initConnection(sensorAddress)){
                requestConversion();
                waitForConversion();
                temp = sensor->getTempRaw(sensorAddress);            
            }
        }        
        DEBUG_ONLY(logInfoIntStringTemp(INFO_TEMP_SENSOR_INITIALIZED, pinNr, addressString, temp));
        success = temp != DEVICE_DISCONNECTED;
        if(success){
            requestConversion(); // piggyback request for a new conversion
        }
    }
    setConnected(success);
    logDebug("init onewire sensor complete %d", success);
    return success;
}

void OneWireTempSensor::requestConversion() {
    sensor->requestTemperaturesByAddress(sensorAddress);
}

void OneWireTempSensor::setConnected(bool connected) {
    if (this->connected == connected)
        return; // state is stays the same

    char addressString[17];
    printBytes(sensorAddress, 8, addressString);
    this->connected = connected;
    if (connected) {
        logInfoIntString(INFO_TEMP_SENSOR_CONNECTED, this->oneWire->pinNr(), addressString);
    } else {
        logWarningIntString(WARNING_TEMP_SENSOR_DISCONNECTED, this->oneWire->pinNr(), addressString);
    }
}

temperature OneWireTempSensor::read() {

    if (!connected)
        return TEMP_SENSOR_DISCONNECTED;

    temperature temp = readAndConstrainTemp();
    requestConversion();
    return temp;
}

temperature OneWireTempSensor::readAndConstrainTemp() {
    temperature temp = sensor->getTempRaw(sensorAddress);
    if (temp == DEVICE_DISCONNECTED) {
        setConnected(false);
        return TEMP_SENSOR_DISCONNECTED;
    }

    const uint8_t shift = TEMP_FIXED_POINT_BITS - ONEWIRE_TEMP_SENSOR_PRECISION; // difference in precision between DS18B20 format and temperature adt
    temp = constrainTemp(temp + calibrationOffset + (C_OFFSET >> shift), ((int) MIN_TEMP) >> shift, ((int) MAX_TEMP) >> shift) << shift;
    return temp;
}
