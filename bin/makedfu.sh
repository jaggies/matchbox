#!/bin/bash
#pip install intelhex
#sudo pip install intelhex
arm-none-eabi-objcopy -O ihex build/Users/jmiller/trees/open-source/gcc-arm-none-eabi-4_9-2015q3/bin/arm-none-eabi/main /tmp/main.hex
python ~/trees/workspace/alice/alice3/io_board/stm32f4/dfu-convert -i /tmp/main.hex /tmp/main.dfu
python ~/trees/workspace/alice/alice3/io_board/stm32f4/dfu-convert -i /tmp/main.hex /tmp/main.dfu
dfu-util -d "0483:df11" -a0 -D /tmp/main.dfu
#system_profiler SPUSBDataType
