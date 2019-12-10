#!/usr/bin/env python3

"""Twilio Telemetry/Sensor Demo For Quickstart and Quest."""
import asyncio
import time
import sys
import os
import subprocess
import tempfile
import json
from grove.grove_temperature_humidity_sensor_sht3x import GroveTemperatureHumiditySensorSHT3x

from luma.core.interface.serial import i2c
from luma.core.render import canvas
from luma.oled.device import sh1106

from azure.iot.device.aio import IoTHubDeviceClient
from azure.iot.device.aio import ProvisioningDeviceClient
from azure.iot.device import Message, X509

##############################################
# Global options and variables               #
##############################################
# Cloud options
AZURE_PROV_URI = 'global.azure-devices-provisioning.net'

# Behavior Options
UNITS = 'celsius'  # Default temperature unit

# Display settings
DISPLAY_WIDTH = 128
DISPLAY_HEIGHT = 128
# DISPLAY_ROTATE values:
#  0 - no rotation
#  1 - 90 degrees CW
#  2 - 180 degrees
#  3 - 270 degrees CW / 90 degrees CCW
DISPLAY_ROTATE = 1

# Trust Onboard Options
TOB_DEVICE = '/dev/ttyACM1'
TOB_BAUDRATE = '115200'
TOB_PIN = '0000'

def get_id_scope():
    scope = os.environ.get('AZURE_ID_SCOPE') or None

    if not scope:
        try:
            with open(os.path.expanduser('~/azure_id_scope.txt'), 'r') as f:
                scope = f.read().replace('\n', '')
        except:
            pass

    return scope

##############################################
# Get Certificate and Key Data from ToB Tool #
##############################################
async def iothub_client_init():
    id_scope = get_id_scope()

    if not id_scope:
        print ('Could not get DPS ID scope. Either set AZURE_ID_SCOPE environment variable or put your ID scope into ~/azure_id_scope.txt')
        sys.exit(1)

    print('Running trust_onboard_tool')
    tool_command = ['trust_onboard_tool', '-d', TOB_DEVICE, '-p', TOB_PIN, '-b', TOB_BAUDRATE, '-j']

    return_json = subprocess.check_output(tool_command)
    try:
        tob_data = json.loads(return_json.decode('utf-8'))
    except json.JSONDecodeError as e:
        print("Error parsing trust_onboard_tool output: ", str(e))

    if 'available_certificate' not in tob_data:
        print ('Could not get available certificate')
        sys.exit(5)

    if 'available_pkey' not in tob_data:
        print ('Could not get available private key')
        sys.exit(5)

    if 'available_cn' not in tob_data:
        print ('Could not get available certificate common name')
        sys.exit(5)

    cf = tempfile.NamedTemporaryFile()
    with open(cf.name, 'w') as file:
        file.write(tob_data['available_certificate'])

    kf = tempfile.NamedTemporaryFile()
    with open(kf.name, 'w') as file:
        file.write(tob_data['available_pkey'])

    x509 = X509 (
        cert_file = cf.name,
        key_file = kf.name
    )

    provisioning_client = ProvisioningDeviceClient.create_from_x509_certificate(
            provisioning_host = AZURE_PROV_URI,
            registration_id=tob_data['available_cn'],
            id_scope = id_scope,
            x509=x509)

    provisioning_result = await provisioning_client.register()

    if provisioning_result.status != 'assigned':
        print ('Provisioning to IoT Hub failed: ', provisioning_result)
        sys.exit(1)

    client = IoTHubDeviceClient.create_from_x509_certificate(
            hostname=provisioning_result.registration_state.assigned_hub,
            device_id=provisioning_result.registration_state.device_id,
            x509=x509)

    await client.connect()

    return client, provisioning_result.registration_state.device_id

async def message_receiver(client):
    global UNITS

    counter = 0
    while True:
        message = await client.receive_message()

        print ("Received Message [%d]:" % counter)
        print (
            "    Data: <<<%s>>> & Size=%d" %
            (message.data.decode(message.content_encoding), len(message.data))
        )

        print ("    Properties: %s" % message.custom_properties)
        if message.custom_properties.get('units') in ('fahrenheit', 'celsius'):
            UNITS = message.custom_properties['units']

        counter += 1

##############################################
# Main sensor, display, and telemetry loop   #
##############################################
async def twilio_telemetry_loop():
    global UNITS

    res = 0
    client = None
    try:
        display_device = sh1106(i2c(port=1, address=0x3C), width=DISPLAY_WIDTH, height=DISPLAY_HEIGHT, rotate=DISPLAY_ROTATE)

        # Draw splash screen
        display_device.clear()
        with canvas(display_device) as draw:
            draw.rectangle(display_device.bounding_box, outline="white", fill="black")
            draw.text((5, 10), "Twilio Trust Onboard", fill="white")
            draw.text((5, 20), "and Microsoft Azure", fill="white")
            draw.text((42, 70), "present", fill="white")
            draw.text((12, 80), "T E L E M E T R Y", fill="white")


        client, device_id = await iothub_client_init()

        sensor = GroveTemperatureHumiditySensorSHT3x(address=0x45)

        if sys.version_info[1] >= 7:
            asyncio.create_task(message_receiver(client))
        else:
            loop = asyncio.get_event_loop()
            loop.create_task(message_receiver(client))

        while True:
            celsius_temperature, humidity = sensor.read()
            fahrenheit_temperature = (celsius_temperature * 1.8) + 32
            celsius_temperature = float("{0:.2f}".format(celsius_temperature))
            fahrenheit_temperature = float("{0:.2f}"
                                           .format(fahrenheit_temperature))
            humidity = float("{0:.2f}".format(humidity))

            # Format a temperature reading message to the cloud
            if UNITS == "fahrenheit":
                long_temp = ('Temperature in\nFahrenheit: {:.2f} F'
                             .format(fahrenheit_temperature))
            else:
                long_temp = ('Temperature in\nCelsius: {:.2f} C'
                             .format(celsius_temperature))

            # Format humidity
            long_humid = ('Relative Humidity:\n {:.2f} %'
                          .format(humidity))

            print(long_temp)
            print(long_humid)

            # Print on the display
            display_device.clear()
            with canvas(display_device) as draw:
                draw.text((5, 30), long_temp, fill="white")
                draw.text((5, 60), long_humid, fill="white")

            # Craft a message to the cloud
            msg_txt_formatted = json.dumps({
                'deviceId': device_id,
                'temperatureFahrenheit': fahrenheit_temperature,
                'temperatureCelsius': celsius_temperature,
                'displayedTemperatureUnits': UNITS,
                'humidity': humidity,
                'shenanigans': "none",
            })

            message = Message(msg_txt_formatted)

            # Optional: assign ids
            message.message_id = "message_1"
            message.correlation_id = "correlation_1"

            # Optional: assign properties
            message.custom_properties["temperatureAlert"] = 'true' if celsius_temperature > 28 else 'false'

            if sys.version_info[1] >= 7:
                asyncio.create_task(client.send_message(message))
            else:
                loop = asyncio.get_event_loop()
                loop.create_task(client.send_message(message))

            print ("Asynchronously sending a message to IoT Hub")

            await asyncio.sleep(10)

    except Exception as iothub_error:
        print ("Unexpected error %s from IoTHub" % iothub_error)
        res = 1
    except KeyboardInterrupt:
        print ("IoTHubClient sample stopped")
    finally:
        if client:
            await client.disconnect()
        return res

##############################################
# Called from the command line               #
##############################################
if __name__ == '__main__':
    print ("\nPython %s" % sys.version)

    print ("Starting the Twilio Telemetry/Sensor Demo...")

    if sys.version_info[0] < 3:
        print ('Python >= 3 is required to run this demo')
        exit(1)

    res = 0
    if sys.version_info[1] >= 7:
        res = asyncio.run(twilio_telemetry_loop())
    else:
        loop = asyncio.get_event_loop()
        res = loop.run_until_complete(twilio_telemetry_loop())
        loop.close()

    exit(res)
