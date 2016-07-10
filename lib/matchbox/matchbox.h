/*
 * matchbox.h
 *
 *  Created on: Jul 10, 2016
 *      Author: jmiller
 */

#ifndef MATCHBOX_H_
#define MATCHBOX_H_

class MatchBox {
    public:
        MatchBox();
        ~MatchBox();
        SPI_HandleTypeDef* getSpi1() { return &hspi1; }
    private:
        void gpioInit(void);
        void systemClockConfig(void);
        void spi1Init(void);
        SPI_HandleTypeDef hspi1;
};

#endif /* MATCHBOX_H_ */
