#!/bin/sh
# Set pin 4 to input, to release it from the PWM.
cd /sys/class/gpio
echo 4 > export
echo in > gpio4/direction
echo 4 > unexport

