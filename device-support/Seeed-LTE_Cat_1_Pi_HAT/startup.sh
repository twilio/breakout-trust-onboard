#!/bin/bash -e

GPIO_PATH=/sys/class/gpio
TRUE=1
FALSE=0

function gpioExport {
  PIN=$1
  if [ ! -e ${GPIO_PATH}/gpio${PIN} ]; then
    echo ${PIN} > ${GPIO_PATH}/export
  fi
}

function gpioSetDirection {
  PIN=$1
  DIRECTION=$2
  echo ${DIRECTION} > ${GPIO_PATH}/gpio${PIN}/direction
}

function gpioSet {
  PIN=$1
  VALUE=$2
  echo ${VALUE} > ${GPIO_PATH}/gpio${PIN}/value
}

# reference on initializing u-blox LARA-R203 on Seeed Raspberry PI LTE Hat
# https://github.com/Seeed-Studio/ublox_lara_r2_pi_hat/blob/master/test/test01.py
# https://github.com/Seeed-Studio/ublox_lara_r2_pi_hat/blob/master/ublox_lara_r2/ublox_lara_r2.py

# GPIO 5 / pin 29
POWER_PIN=5
# GPIO 6 / pin 31
RESET_PIN=6

# configure gpio pins to be visible, if needed
gpioExport ${POWER_PIN}
gpioExport ${RESET_PIN}

# configure power and reset pins as output
gpioSetDirection ${POWER_PIN} out
gpioSetDirection ${RESET_PIN} out

# bring power and reset pins low
gpioSet ${POWER_PIN} ${FALSE}
gpioSet ${RESET_PIN} ${FALSE}

# configure hardware flow control for ttyAMA0 port
rpirtscts on

# cycle power
# bring power and reset pins high
gpioSet ${POWER_PIN} ${TRUE}
gpioSet ${RESET_PIN} ${TRUE}

# sleep 1 second
sleep 1

# bring power and reset pins low
gpioSet ${POWER_PIN} ${FALSE}
gpioSet ${RESET_PIN} ${FALSE}
