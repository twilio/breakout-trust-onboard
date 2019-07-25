#!/usr/bin/env python3

import csv
import sys
import smartcard
import serial
import glob
import time
import subprocess
import os
import signal

class ModemsEnumerator:
    """Takes a CSV file and lists available and unavailable modems"""

    baudrates = [9600, 115200]
    atmodem_globs = ['/dev/ttyACM*', '/dev/ttyUSB*']

    def __init__(self, csv_path):
        self.modems = {}           # {modem_name -> interface}, interface is one of ('AT', 'PCSC')
        self.modems_args = []       # [(modem_name, device_string, baudrate, pin)]
        self.modems_missing = []   # [(modem_name, interface)]

        with open(csv_path) as modemcsv:
            modemreader = csv.reader(modemcsv, delimiter=";")
            for row in modemreader:
                self.modems[row[2] + '@' + row[1]] = row[0]

    def doATCommand(self, port, command):
        port.write(str.encode(command))
        ts = time.time()

        status = 'TIMEOUT'
        resp = ''
        while True:
            line = str(port.readline(), 'utf-8').strip()
            if line.startswith('OK'):
                status = 'OK'
                break
            elif line.startswith('ERROR'):
                status = 'ERROR'
                break
            elif line != '':
                resp = line

            if time.time() - ts > 2.0:
                break

        return (status, resp)

    def probe(self):
        modems_found = {} # {modem_name -> (interface, (device_string, baudrate, pin))}
        # first probe all serial interfaces
        ifaces = []
        for px in self.atmodem_globs:
            ifaces += glob.glob(px)

        for dev in ifaces:
            try:
                modem_name = ""
                skip_modem = False
                for baud in self.baudrates:
                    if skip_modem:
                        break

                    with serial.Serial(dev, baud, timeout=1) as port:
                        status, resp = self.doATCommand(port, 'AT+CGMM\r\n');

                        if status == 'ERROR':
                            skip_modem = True
                            break
                        elif status == 'TIMEOUT':
                            break

                        model = str(resp)
                        skip_modem = True

                        status, resp = self.doATCommand(port, 'AT+CGMI\r\n');

                        if status != 'OK':
                            break

                        name = model + '@' + resp

                        if name in modems_found:
                            # same modem on a different interface, skip
                            break;
                        else:
                            modems_found[name] = ('AT', (dev, str(baud), '0000'))
            except:
                print('Error with serial device' + dev + ':', sys.exc_info()[0], file=sys.stderr)

        # then probe pc/sc readers
        readers = smartcard.System.readers()
        pcsc_idx = 0
        for r in readers:
            try:
                conn = r.createConnection()
                conn.connect()
                SELECT = [0x00, 0xA4, 0x00, 0x04]
                MFFILE = [0x3F, 0x00]
                data, sw1, sw2 = conn.transmit (SELECT + [len(MFFILE)] + MFFILE)
                if sw1 != 0x61:
                    continue
                ICCIDFILE = [0x2F, 0xE2]
                data, sw1, sw2 = conn.transmit (SELECT + [len(ICCIDFILE)] + ICCIDFILE)
                if sw1 != 0x61:
                    continue
                resplen = sw2
                GETRESPONSE = [0x00, 0xC0, 0x00, 0x00]
                data, sw1, sw2 = conn.transmit (GETRESPONSE + [resplen])
                if sw1 != 0x90 or sw2 != 0x00:
                    continue

                iccid_size = 0
                i = 2
                while i < len(data):
                    t = data[i]
                    l = data[i+1]

                    if (t == 0x80): # File size
                        i += 2 # skip t,l
                        for j in range(l):
                            iccid_size *= 256
                            iccid_size += data[i]
                            i += 1
                        break
                    else:
                        i += 2 + l # Skip the TLV pair

                READBINARY = [0x00, 0xB0, 0x00, 0x00]
                data, sw1, sw2 = conn.transmit (READBINARY + [iccid_size])

                if sw1 != 0x90 or sw2 != 0x00:
                    continue
                iccid = data # ignored for now, may be used to determine PIN in future

                modems_found[str(r)] = ('PCSC', ('pcsc:' + str(pcsc_idx), '0', '0000'))
                pcsc_idx += 1
            except:
                print('Error with card reader' + str(r) + ':', sys.exc_info()[0], file=sys.stderr)

        # finally, return found and missing modems
        for (name, iface) in self.modems.items():
            found = False
            if name == '*@*':
                # Will use the first modem found, impossible to combine with named interfaces
                for (k, v) in modems_found.items():
                    if v[0] == iface:
                        found = True
                        self.modems_args += [(k,) + v[1]]
                        del modems_found[k]
                        break

            else:
                if name in modems_found:
                    found = True
                    self.modems_args += [(name,) + modems_found[name][1]] # interface is ignored
                    del modems_found[name]

            if not found:
                self.modems_missing += [(name, iface)]

        return (self.modems_args, self.modems_missing)

def main():
    if len(sys.argv) < 2:
        print('Usage:' + sys.argv[0] + ' modems.csv [test1 [test2 [test3...]]]', file=sys.stderr)

    success = True

    me = ModemsEnumerator(sys.argv[1])
    test_args, missing = me.probe()

    for test in sys.argv[2:]:
        for args in test_args:
            print('\n\n===== Running test', test, 'for', args[0], '=====');
            command_line = [test, '--device', args[1], '--baudrate', args[2], '--pin', args[3]]
            print('===== Command line:', ' '.join(command_line), '=====')


            p = subprocess.Popen(command_line,  stdout=subprocess.PIPE, stderr=subprocess.STDOUT, preexec_fn=os.setsid)

            try:
              outs, errs = p.communicate(timeout=600)
            except subprocess.TimeoutExpired:
              print('===== Test timed out =====')
              os.killpg(os.getpgid(p.pid), signal.SIGTERM)
              outs, _ = p.communicate()

            print('===== Output =====')
            print(str(outs, 'utf-8'))

            if (p.returncode == 0):
                print('===== Test above succeeded =====')
            else:
                print('===== Test above failed with exit code', str(p.returncode), '=====')
                success = False

    if len(missing) != 0:
        success = False
        print('\n===== Could not find the following modems =====')
        for m in missing:
            print(' *', m[0], 'on', m[1], 'interface')

    if success:
        print('\n===== Test suite succeeded =====')
        sys.exit(0)
    else:
        print('\n===== Test suite failed =====')
        sys.exit(1)


if __name__ == "__main__":
    main()
