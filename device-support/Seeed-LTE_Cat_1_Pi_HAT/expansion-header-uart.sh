#!/bin/bash -e

# reference on initializing u-blox LARA-R203 on Seeed Raspberry PI LTE Hat
# https://github.com/Seeed-Studio/ublox_lara_r2_pi_hat/blob/master/test/test01.py
# https://github.com/Seeed-Studio/ublox_lara_r2_pi_hat/blob/master/ublox_lara_r2/ublox_lara_r2.py

# configure hardware flow control for ttyAMA0 port
rpirtscts on
