#!/usr/bin/python
#-*- coding: utf-8 -*-

import os
import os.path
import serial, time
import array
import sys, getopt

class bootdownload(object):
    '''
    Hisilicon boot downloader

    >>> downloader = bootdownload()
    >>> downloader.download(filename)

    '''

    # crctab calculated by Mark G. Mendel, Network Systems Corporation
    crctable = [
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
        0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
        0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
        0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
        0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
        0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
        0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
        0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
        0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
        0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
        0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
        0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
        0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
        0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
        0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
        0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
        0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
        0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
        0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
        0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
        0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
        0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
        0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
        0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
        0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
        0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
        0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
        0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
        0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
        0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
        0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
        0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
    ]

    startframe = {
        'hi3716cv200':[0xFE,0x00,0xFF,0x01,0x00,0x00,0x00,0x04,0x00,0x00,0x02,0x01]
    }

    headframe = {
        'hi3716cv200':[0xFE,0x00,0xFF,0x01,0x00,0x00,0x00,0x04,0x00,0x00,0x02,0x01]
    }

    bootheadaddress = {
        'hi3716cv200':0xF9800800
    }

    bootdownloadaddress = {
        'hi3716cv200':0x07000000
    }

    BOOT_HEAD_LEN = 0x4F00
    MAX_DATA_LEN  = 0x400

    def __init__(self,chiptype,serialport):
        try:
            self.s = serial.Serial(port=serialport, baudrate=115200, timeout=1)
        except serial.serialutil.SerialException:
            #no serial connection
            self.s = None
            print("\nFailed to open serial!", serialport)
            sys.exit(2)

        self.chip = chiptype

    def __del__(self):
        if self.s != None:
            self.s.close()

    def calc_crc(self, data, crc=0):
        for char in data:
            crc = ((crc << 8) | ord(char)) ^ self.crctable[(crc >> 8) & 0xff]
        for i in range(0,2):
            crc = ((crc << 8) | 0) ^ self.crctable[(crc >> 8) & 0xff]
        return crc & 0xffff

    def getsize(self, filename):
        st = os.stat(filename)
        return st.st_size

    def sendframe(self, data, loop):
        for i in range(1, loop):
            self.s.flushOutput()
            self.s.write(data)
            self.s.flushInput()
            try:
                ack = self.s.read()
                if len(ack) == 1:
                    if ack == chr(0xaa):
                        return None
            except:
                return None

        print('failed')

    def sendstartframe(self):
        self.s.timeout =0.01
        data = array.array('B', self.startframe[self.chip]).tostring()
        crc = self.calc_crc(data)
        data += chr((crc >> 8)&0xff)
        data += chr(crc&0xff)
        self.sendframe(data,10000)

    def sendheadframe(self,length,address):
        self.s.timeout = 0.03
        self.headframe[self.chip][4] = (length>>24)&0xff
        self.headframe[self.chip][5] = (length>>16)&0xff
        self.headframe[self.chip][6] = (length>>8)&0xff
        self.headframe[self.chip][7] = (length)&0xff
        self.headframe[self.chip][8] = (address>>24)&0xff
        self.headframe[self.chip][9] = (address>>16)&0xff
        self.headframe[self.chip][10] = (address>>8)&0xff
        self.headframe[self.chip][11] = (address)&0xff

        data = array.array('B', self.headframe[self.chip]).tostring()
        crc = self.calc_crc(data)

        data += chr((crc >> 8)&0xff)
        data += chr(crc&0xff)

        self.sendframe(data,16)


    def senddataframe(self,seq,data):
        self.s.timeout = 0.15
        head = chr(0xDA)
        head += chr(seq&0xFF)
        head += chr((~seq)&0xFF)

        data = head + data

        crc = self.calc_crc(data)
        data += chr((crc >> 8)&0xff)
        data += chr(crc&0xff)

        self.sendframe(data,32)

    def sendtailframe(self,seq):
        data = chr(0xED)
        data += chr(seq&0xFF)
        data += chr((~seq)&0xFF)
        crc = self.calc_crc(data)
        data += chr((crc >> 8)&0xff)
        data += chr(crc&0xff)

        self.sendframe(data,16)

    def senddata(self, data, address):
        length=len(data)
        self.sendheadframe(length,address)
        seq=1
        while length > self.MAX_DATA_LEN:
            self.senddataframe(seq,data[(seq-1)*self.MAX_DATA_LEN:seq*self.MAX_DATA_LEN])
            seq = seq+1
            length = length-self.MAX_DATA_LEN
        self.senddataframe(seq,data[(seq-1)*self.MAX_DATA_LEN:])
        self.sendtailframe(seq+1)


    def download(self, filename1, filename2):

        f=open(filename1,"rb")
        data = f.read()
        f.close()

        print('Sending', filename1, '...')
        self.senddata(data,self.bootheadaddress[self.chip])
        print('Done\n')

        if filename2:
            f=open(filename2,"rb")
            data = f.read()
            f.close()

            print('Sending', filename2, '...')
            self.senddata(data,self.bootdownloadaddress[self.chip])
            print('Done\n')


def burnboot(chiptype, serialport, filename1, filename2=''):
    downloader = bootdownload(chiptype, serialport)
    downloader.download(filename1, filename2)

def startterm(serialport=0):
    try:
        miniterm = Miniterm(
            serialport,
            115200,
            'N',
            rtscts=False,
            xonxoff=False,
            echo=False,
            convert_outgoing=2,
            repr_mode=0,
        )
    except serial.SerialException as e:
        sys.stderr.write("could not open port %r: %s\n" % (port, e))
        sys.exit(1)
    miniterm.start()
    miniterm.join(True)
    miniterm.join()

def main(argv):
    '''
    img2 = 'fastboot2.img'
    '''
    img1 = 'fastboot1.img'
    img2 = ''
    dev  = '';
    dev1 = '/dev/serial/by-id/usb-䕇䕎䥎_㄰㌲㔴㜶㤸-if00-port0'
    dev2 = '/dev/serial/by-id/pci-䕇䕎䥎_㄰㌲㔴㜶㤸-if00-port0'
    try:
        opts, args = getopt.getopt(argv,"hd:",["img1=","img2="])
    except getopt.GetoptError:
        print('hisi-idt.py -d device --img1 <fastboot1> --img2 <fastboot2>')
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print('hisi-idt.py -d device --img1 <fastboot1> --img2 <fastboot2>')
            sys.exit()
        elif opt in ("-d"):
            dev = arg
        elif opt in ("--img1"):
            img1 = arg
        elif opt in ("--img2"):
            img2 = arg
    if dev == '':
        if os.path.exists(dev1):
            dev = dev1
        elif os.path.exists(dev2):
            dev = dev2
        else:
            print('Device not detected under /dev/serial/by-id/. Please use -d.')
            sys.exit(3)
    print('+----------------------+')
    print(' Serial: ', dev)
    print(' Image1: ', img1)
    print(' Image2: ', img2)
    print('+----------------------+\n')

    if not os.path.isfile(img1):
        print("Image don't exists:", img1)
        sys.exit(1)

    if (img2):
        if not os.path.isfile(img2):
            print("Image don't exists:", img2)
            sys.exit(1)

    burnboot('hi3716cv200', dev, img1, img2)

if __name__ == "__main__":
    main(sys.argv[1:])
