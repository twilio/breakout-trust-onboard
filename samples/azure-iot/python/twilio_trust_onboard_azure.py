"""Twilio Telemetry/Sensor Demo For Quickstart and Quest."""
import time
import sys
import os
import subprocess
import json
from grove.i2c import Bus
import iothub_client
from iothub_client import IoTHubClient, IoTHubClientError, \
    IoTHubTransportProvider, IoTHubClientResult
from iothub_client import IoTHubMessage, IoTHubMessageDispositionResult, \
    IoTHubError
from iothub_client_args import get_iothub_opt
from luma.core.interface.serial import i2c
from luma.core.render import canvas
from luma.oled.device import sh1106


##############################################
# Global options and variables               #
##############################################
# Behavior Options
TIMEOUT = 241000  # 25 Minutes
MINIMUM_POLLING_TIME = 2  # X+1 = Seconds between readings
MESSAGE_TIMEOUT = 10000
RECEIVE_CONTEXT = 0
RECEIVED_COUNT = 0
UNITS = 'celsius'  # Default temperature unit

# Global counters
RECEIVE_CALLBACKS = 0
SEND_CALLBACKS = 0
PROTOCOL = IoTHubTransportProvider.HTTP

# Display settings
serial = i2c(port=1, address=0x3C)
device = sh1106(serial, width=128, height=128, rotate=1)

# Trust Onboard Options
TOB_DEVICE = '/dev/ttyACM3'
TOB_BAUDRATE = '115200'
TOB_PIN = '0000'

# Run Cloud
RUN_CLOUD = True

##############################################
# Splash Screen                              #
##############################################
with canvas(device) as draw:
    draw.rectangle(device.bounding_box, outline="white", fill="black")
    draw.text((5, 10), "Twilio Trust Onboard", fill="white")
    draw.text((5, 20), "and Microsoft Azure", fill="white")
    draw.text((42, 70), "present", fill="white")
    draw.text((12, 80), "T E L E M E T R Y", fill="white")

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
if RUN_CLOUD:
    id_scope = get_id_scope()

    if not id_scope:
        print ('Could not get DPS ID scope. Either set AZURE_ID_SCOPE environment variable or put your ID scope into ~/azure_id_scope.txt')
        sys.exit(1)

    registerer_command = ['azure_dps_registerer', '-d', TOB_DEVICE, '-p', TOB_PIN, '-b', TOB_BAUDRATE, '-k', 'available', '-a', id_scope]

    return_json = subprocess.check_output(registerer_command)
    try:
        register_data = json.loads(return_json.decode('utf-8'))
    except json.JSONDecodeError as e:
        print("Error parsing azure_dps_registerer output: ", str(e))

    if not 'status' in register_data or register_data['status'] != 'SUCCESS':
        print ('Registering to Azure DPS failed: ', register_data.get('message') or 'reason unknown')
        sys.exit(3)

    if not 'iothub_uri' in register_data:
        print ('IoT Hub URI not present in registerer output, can''t proceed')
        sys.exit(3)

    if not 'device_id' in register_data:
        print ('Device ID not present in registerer output, can''t proceed')
        sys.exit(3)

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

    X509_CERTIFICATE = tob_data['available_certificate']
    X509_PRIVATEKEY =  tob_data['available_pkey']

    CONNECTION_STRING = "HostName=" + register_data['iothub_uri'] + \
        ";DeviceId=" + register_data['device_id'] + ";x509=true"
else:
    register_data = ""
    tob_data = ""
    X509_CERTIFICATE = ""
    X509_PRIVATEKEY = ""


##############################################
# Grove Temperature / Humidity Sensor SHT35  #
##############################################
class GroveTemperatureHumiditySensorSHT3x(object):
    def __init__(self, address=0x45, bus=None):
        self.address = address

        # I2C bus
        self.bus = Bus(bus)

    def CRC(self, data):
        crc = 0xff
        for s in data:
            crc ^= s
            for i in range(8):
                if crc & 0x80:
                    crc <<= 1
                    crc ^= 0x131
                else:
                    crc <<= 1
        return crc

    def read(self):
        # High repeatability, clock stretching disabled
        self.bus.write_i2c_block_data(self.address, 0x24, [0x00])

        # Measurement duration < 16 ms
        time.sleep(0.016)

        # Read 6 bytes back:
        # Temp MSB, Temp LSB, Temp CRC,
        # Humididty MSB, Humidity LSB, Humidity CRC
        data = self.bus.read_i2c_block_data(0x45, 0x00, 6)
        temperature = data[0] * 256 + data[1]
        celsius = -45 + (175 * temperature / 65535.0)
        humidity = 100 * (data[3] * 256 + data[4]) / 65535.0
        if data[2] != self.CRC(data[:2]):
            raise RuntimeError("Temperature CRC mismatch")
        if data[5] != self.CRC(data[3:5]):
            raise RuntimeError("Humidity CRC mismatch")
        return celsius, humidity


##############################################
# Utilities                                  #
##############################################
def iothub_client_init():
    client = IoTHubClient(CONNECTION_STRING, PROTOCOL)
    if client.protocol == IoTHubTransportProvider.HTTP:
        client.set_option("timeout", TIMEOUT)
        client.set_option("MinimumPollingTime", MINIMUM_POLLING_TIME)

    client.set_option("messageTimeout", MESSAGE_TIMEOUT)
    client.set_option("x509certificate", X509_CERTIFICATE)
    client.set_option("x509privatekey", X509_PRIVATEKEY)

    if client.protocol == IoTHubTransportProvider.MQTT:
        client.set_option("logtrace", 0)

    client.set_message_callback(receive_message_callback, RECEIVE_CONTEXT)
    return client


def receive_message_callback(message, counter):
    global RECEIVE_CALLBACKS
    global UNITS
    message_buffer = message.get_bytearray()
    size = len(message_buffer)

    print ("Received Message [%d]:" % counter)
    print (
        "    Data: <<<%s>>> & Size=%d" %
        (message_buffer[:size].decode('utf-8'), size)
    )

    map_properties = message.properties()
    key_value_pair = map_properties.get_internals()

    print ("    Properties: %s" % key_value_pair)
    if 'units' in key_value_pair and \
            key_value_pair['units'] in ('fahrenheit', 'celsius'):
        UNITS = key_value_pair['units']

    counter += 1
    RECEIVE_CALLBACKS += 1

    print ("    Total calls received: %d" % RECEIVE_CALLBACKS)

    return IoTHubMessageDispositionResult.ACCEPTED


def send_confirmation_callback(message, result, user_context):
    global SEND_CALLBACKS
    print (
        "Confirmation[%d] received for message with result = %s" %
        (user_context, result)
    )

    map_properties = message.properties()

    print ("    message_id: %s" % message.message_id)
    print ("    correlation_id: %s" % message.correlation_id)

    key_value_pair = map_properties.get_internals()

    print ("    Properties: %s" % key_value_pair)

    SEND_CALLBACKS += 1

    print ("    Total calls confirmed: %d" % SEND_CALLBACKS)


def print_last_message_time(client):
    try:
        last_message = client.get_last_message_receive_time()
        print(
            "Last Message: %s" % time.asctime(time.localtime(last_message))
        )
        print("Actual time : %s" % time.asctime())

    except IoTHubClientError as iothub_client_error:
        if iothub_client_error.args[0].result == \
                IoTHubClientResult.INDEFINITE_TIME:
            print ("No message received")
        else:
            print (iothub_client_error)


##############################################
# Main sensor, display, and telemetry loop   #
##############################################
def twilio_telemetry_loop():
    global UNITS

    try:
        sensor = GroveTemperatureHumiditySensorSHT3x()
        client = iothub_client_init()

        while RUN_CLOUD:
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
            device.clear()
            with canvas(device) as draw:
                draw.text((5, 30), long_temp, fill="white")
                draw.text((5, 60), long_humid, fill="white")

            # Craft a message to the cloud
            msg_txt_formatted = json.dumps({
                'deviceId': register_data['device_id'],
                'temperatureFahrenheit': fahrenheit_temperature,
                'temperatureCelsius': celsius_temperature,
                'displayedTemperatureUnits': UNITS,
                'humidity': humidity,
                'shenanigans': "none",
            })

            message = IoTHubMessage(msg_txt_formatted)

            # Optional: assign ids
            message.message_id = "message_1"
            message.correlation_id = "correlation_1"

            # Optional: assign properties
            prop_map = message.properties()

            # Optional: add an alert
            prop_map.add("temperatureAlert",
                         'true' if celsius_temperature > 28 else 'false')

            client.send_event_async(
                message,
                send_confirmation_callback,
                1
            )

            print ("IoTHubClient.send_event_async accepted a message "
                   "for transmission to IoT Hub.")

            # Wait for Commands or exit
            print ("IoTHubClient waiting for commands, press Ctrl-C to exit")

            status_counter = 0
            while status_counter <= 1:
                status = client.get_send_status()
                print ("Send status: %s" % status)
                time.sleep(10)
                status_counter += 1

    except IoTHubError as iothub_error:
        print ("Unexpected error %s from IoTHub" % iothub_error)
        return
    except KeyboardInterrupt:
        print ("IoTHubClient sample stopped")

    print_last_message_time(client)


##############################################
# Called from the command line               #
##############################################
if __name__ == '__main__':
    print ("\nPython %s" % sys.version)
#    print ("IoT Hub for Python SDK Version: %s" % iothub_client.__version__)

    if not RUN_CLOUD:
        exit(0)

    try:
        (CONNECTION_STRING, PROTOCOL) = \
            get_iothub_opt(sys.argv[1:], CONNECTION_STRING, PROTOCOL)
    except:
        sys.exit(1)

    print ("Starting the Twilio Telemetry/Sensor Demo...")
    print ("    Protocol %s" % PROTOCOL)
    print ("    Connection string=%s" % CONNECTION_STRING)

    twilio_telemetry_loop()
