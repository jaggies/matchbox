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
    private:
        void gpioInit(void);
        void systemClockConfig(void);
};

#endif /* MATCHBOX_H_ */
