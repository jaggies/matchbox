/*
 * eeprom.h
 *
 *  Created on: Jul 20, 2016
 *      Author: jmiller
 *
 *  Emulation layer for implementing STM32 backup SRAM memory.
 */

#ifndef LIB_MATCHBOX_EEPROM_H_
#define LIB_MATCHBOX_EEPROM_H_

class Eeprom {
    public:
        Eeprom();
        ~Eeprom();

        uint8_t read(uint16_t addr) const;
        void write(uint16_t addr, uint8_t value) const;
};

#endif /* LIB_MATCHBOX_EEPROM_H_ */
