#!/bin/bash
#pip install intelhex
#sudo pip install intelhex
#system_profiler SPUSBDataType
arm-none-eabi-objcopy -O ihex build/arm-none-eabi/main /tmp/main.hex
python bin/dfu-convert -i /tmp/main.hex /tmp/main.dfu
dfu-util -d "0483:df11" -a0 -D /tmp/main.dfu
