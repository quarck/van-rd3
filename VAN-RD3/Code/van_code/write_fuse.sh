#!/bin/bash

echo "This script will turn ATmega8/8a chip to use external quartz oscillator running at 16MHz, continue (y/n)?"

read X

if [ "$X" == 'y' ] 
then 
	avrdude   -c stk200 -p m8 -U hfuse:w:0xc9:m
	avrdude   -c stk200 -p m8 -U lfuse:w:0xff:m
fi
