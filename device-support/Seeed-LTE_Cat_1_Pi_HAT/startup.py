#!/usr/bin/env python

import RPi.GPIO as GPIO
import time

# reference on initializing u-blox LARA-R203 on Seeed Raspberry PI LTE Hat
# https://github.com/Seeed-Studio/ublox_lara_r2_pi_hat/blob/master/test/test01.py
# https://github.com/Seeed-Studio/ublox_lara_r2_pi_hat/blob/master/ublox_lara_r2/ublox_lara_r2.py

POWER_PIN=29 # GPIO 5
RESET_PIN=31 # GPIO 6

GPIO.setwarnings(False)

if GPIO.getmode() == None:
    GPIO.setmode(GPIO.BOARD)

if GPIO.getmode() == GPIO.BCM:
    POWER_PIN=5
    RESET_PIN=6

GPIO.setup([POWER_PIN, RESET_PIN], GPIO.OUT)

GPIO.output([POWER_PIN, RESET_PIN], GPIO.LOW)

GPIO.output([POWER_PIN, RESET_PIN], GPIO.HIGH)

time.sleep(1)

GPIO.output([POWER_PIN, RESET_PIN], GPIO.LOW)
