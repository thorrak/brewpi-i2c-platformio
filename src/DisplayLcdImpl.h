/*
 * DisplayLcdImpl.h
 *
 * Created: 07/01/2015 06:01:02
 *  Author: mat
 */ 

#pragma once

#if BREWPI_EMULATE || BREWPI_LCD_TYPE==BREWPI_DISPLAY_NONE || !ARDUINO
#include "NullLcdDriver.h"
typedef NullLcdDriver LcdDriver;
#elif BREWPI_LCD_TYPE==BREWPI_DISPLAY_SHIFT_LCD
#include "SpiLcd.h"
typedef SpiLcd		LcdDriver;
#elif BREWPI_LCD_TYPE==BREWPI_DISPLAY_TWI_LCD
#if BREWPI_BOARD != BREWPI_BOARD_STANDARD
#error "I2C/TWI and pinouts verified for Uno only"
#endif
#include "TwiLcdDriver.h"
typedef TwiLcdDriver LcdDriver;
#elif  BREWPI_LCD_TYPE==BREWPI_DISPLAY_OLED
#include "OLEDFourBit.h"
typedef OLEDFourBit LcdDriver;
#endif

