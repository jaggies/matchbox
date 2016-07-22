#!/bin/bash
#pip install intelhex
#sudo pip install intelhex
#system_profiler SPUSBDataType
name=`basename $1`
echo $name
arm-none-eabi-objcopy -O ihex $1 /tmp/$name.hex
python bin/dfu-convert -i /tmp/$name.hex /tmp/$name.dfu
dfu-util -d "0483:df11" -a0 -D /tmp/$name.dfu
