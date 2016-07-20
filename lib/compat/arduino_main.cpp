#include <stdio.h>
#include "arduino.h"

// Instantiations of static classes
Eeprom EEPROM;
ArduinoSerial Serial;
SPIClass SPI;

__attribute((weak))
void setup() {
	// do nothing
}

__attribute((weak))
void loop() {
	// do nothing
}

__attribute ((weak))
int main(int argc, char** argv)
{
	setup();
	loop();
}

