#!/bin/sh

idf.py build && idf.py -p /dev/ttyUSB0 flash && stty -F /dev/ttyUSB0 115200 && tail -f /dev/ttyUSB0

