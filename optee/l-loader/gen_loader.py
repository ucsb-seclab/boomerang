#!/usr/bin/env python

import os
import os.path
import sys, getopt
import binascii
import struct
import string

class generator(object):
    #
    # struct l_loader_head {
    #      unsigned int	first_instr;
    #      unsigned char	magic[16];	@ BOOTMAGICNUMBER!
    #      unsigned int	l_loader_start;
    #      unsigned int	l_loader_end;
    # };
    file_header = [0, 0, 0, 0, 0, 0, 0]

    #
    # struct entry_head {
    #       unsigned char   magic[8];           @ ENTY
    #       unsigned char   name[8];            @ loader/bl1
    #       unsigned int    start_lba;
    #       unsigned int    count_lba;
    #       unsigned int    flag;               @ boot partition or not
    # };

    s1_entry_name = ['loader', 'bl1']
    s2_entry_name = ['primary', 'second']

    block_size = 512

    stage = 0

    # set in self.add()
    idx = 0

    # set in self.parse()
    ptable_lba = 0
    stable_lba = 0

    # file pointer
    p_entry = 0
    p_file = 0

    def __init__(self, out_img):
        try:
            self.fp = open(out_img, "wb+")
        except IOError, e:
            print "*** file open error:", e
            sys.exit(3)
        else:
            self.entry_hd = [[0 for col in range(7)] for row in range(5)]

    def __del__(self):
        self.fp.close()

    # parse partition from the primary ptable
    def parse(self, fname):
        try:
            fptable = open(fname, "rb")
        except IOError, e:
            print "*** file open error:", e
            sys.exit(3)
        else:
            # skip the first block in primary partition table
            # that is MBR protection information
            fptable.read(self.block_size)
            # check whether it's a primary paritition table
            data = struct.unpack("8s", fptable.read(8))
            efi_magic = 'EFI PART'
            if cmp("EFI PART", data[0]):
                print "It's not partition table image."
                fptable.close()
                sys.exit(4)
            # skip 16 bytes
            fptable.read(16)
            # get lba of both primary partition table and secondary partition table
            data = struct.unpack("QQQQ", fptable.read(32))
            self.ptable_lba = data[0] - 1
            self.stable_lba = data[3] + 1
            # skip 24 bytes
            fptable.read(24)
            data = struct.unpack("i", fptable.read(4))
            pentries = data[0]
            # skip the reset in this block
            fptable.read(self.block_size - 84)

            for i in range(1, pentries):
                # name is encoded as UTF-16
                d0,lba,d2,name = struct.unpack("32sQ16s72s", fptable.read(128))
                plainname = unicode(name, "utf-16")
                if (not cmp(plainname[0:7], 'l-loader'[0:7])):
                    print 'bl1_lba: ', lba
                    self.bl1_lba = lba
                    sys.exit(1)

            fptable.close()

    def add(self, lba, fname):
        try:
            fsize = os.path.getsize(fname)
        except IOError, e:
            print "*** file open error:", e
            sys.exit(4)
        else:
            blocks = (fsize + self.block_size - 1) / self.block_size
            if (self.stage == 1):
                # Boot Area1 in eMMC
                bootp = 1
                if self.idx == 0:
                    self.p_entry = 28
            elif (self.stage == 2):
                # User Data Area in eMMC
                bootp = 0
                # create an empty block only for stage2
                # This empty block is used to store entry head
                print 'p_file: ', self.p_file, 'p_entry: ', self.p_entry
                if self.idx == 0:
                    self.fp.seek(self.p_file)
                    for i in range (0, self.block_size):
                        zero = struct.pack('x')
                        self.fp.write(zero)
                    self.p_file += self.block_size
                    self.p_entry = 0
            else:
                print "wrong stage ", stage, "is specified"
                sys.exit(4)
            # Maybe the file size isn't aligned. So pad it.
            if (self.idx == 0) and (self.stage == 1):
                if fsize > 2048:
                    print 'loader size exceeds 2KB. file size: ', fsize
                    sys.exit(4)
                else:
                    left_bytes = 2048 - fsize
            else:
                left_bytes = fsize % self.block_size
                if left_bytes:
                    left_bytes = self.block_size - left_bytes
            print 'lba: ', lba, 'blocks: ', blocks, 'bootp: ', bootp, 'fname: ', fname
            # write images
            fimg = open(fname, "rb")
            for i in range (0, blocks):
                buf = fimg.read(self.block_size)
                self.fp.seek(self.p_file)
                self.fp.write(buf)
                # p_file is the file pointer of the new binary file
                # At last, it means the total block size of the new binary file
                self.p_file += self.block_size

            if (self.idx == 0) and (self.stage == 1):
                self.p_file = 2048
            print 'p_file: ', self.p_file, 'last block is ', fsize % self.block_size, 'bytes', '  tell: ', self.fp.tell(), 'left_bytes: ', left_bytes
            if left_bytes:
                for i in range (0, left_bytes):
                    zero = struct.pack('x')
                    self.fp.write(zero)
                print 'p_file: ', self.p_file, '  pad to: ', self.fp.tell()

            # write entry information at the header
            if self.stage == 1:
                byte = struct.pack('8s8siii', 'ENTRYHDR', self.s1_entry_name[self.idx], lba, blocks, bootp)
            elif self.stage == 2:
                byte = struct.pack('8s8siii', 'ENTRYHDR', self.s2_entry_name[self.idx], lba, blocks, bootp)
            self.fp.seek(self.p_entry)
            self.fp.write(byte)
            self.p_entry += 28
            self.idx += 1

            fimg.close()

    def hex2(self, data):
        return data > 0 and hex(data) or hex(data & 0xffffffff)

    def end(self):
        if self.stage == 1:
            self.fp.seek(20)
            start,end = struct.unpack("ii", self.fp.read(8))
            print "start: ", self.hex2(start), 'end: ', self.hex2(end)
            end = start + self.p_file
            print "start: ", self.hex2(start), 'end: ', self.hex2(end)
            self.fp.seek(24)
            byte = struct.pack('i', end)
            self.fp.write(byte)
        self.fp.close()

    def create_stage1(self, img_loader, img_bl1, output_img):
        print '+-----------------------------------------------------------+'
        print ' Input Images:'
        print '     loader:                       ', img_loader
        print '     bl1:                          ', img_bl1
        print ' Ouput Image:                      ', output_img
        print '+-----------------------------------------------------------+\n'

        self.stage = 1

        # The first 2KB is reserved
        # The next 2KB is for loader image
        self.add(4, img_loader)    # img_loader doesn't exist in partition table
        print 'self.idx: ', self.idx
        # bl1.bin starts from 4KB
        self.add(8, img_bl1)      # img_bl1 doesn't exist in partition table

    def create_stage2(self, img_prm_ptable, img_sec_ptable, output_img):
        print '+-----------------------------------------------------------+'
        print ' Input Images:'
        print '     primary partition table:      ', img_prm_ptable
        print '     secondary partition table:    ', img_sec_ptable
        print ' Ouput Image:                      ', output_img
        print '+-----------------------------------------------------------+\n'

        self.stage = 2
        self.parse(img_prm_ptable)
        self.add(self.ptable_lba, img_prm_ptable)
        if (cmp(img_sec_ptable, 'secondary partition table')):
            # Doesn't match. It means that secondary ptable is specified.
            self.add(self.stable_lba, img_sec_ptable)
        else:
            print 'Don\'t need secondary partition table'

def main(argv):
    stage1 = 0
    stage2 = 0
    img_prm_ptable = "primary partition table"
    img_sec_ptable = "secondary partition table"
    try:
        opts, args = getopt.getopt(argv,"ho:",["img_loader=","img_bl1=","img_prm_ptable=","img_sec_ptable="])
    except getopt.GetoptError:
        print 'gen_loader.py -o <l-loader.bin> --img_loader <l-loader> --img_bl1 <bl1.bin> --img_prm_ptable <prm_ptable.img> --img_sec_ptable <sec_ptable.img>'
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print 'gen_loader.py -o <l-loader.bin> --img_loader <l-loader> --img_bl1 <bl1.bin> --img_prm_ptable <prm_ptable.img> --img_sec_ptable <sec_ptable.img>'
            sys.exit(1)
        elif opt == '-o':
            output_img = arg
        elif opt in ("--img_loader"):
            img_loader = arg
            stage1 = 1
        elif opt in ("--img_bl1"):
            img_bl1 = arg
            stage1 = 1
        elif opt in ("--img_prm_ptable"):
            img_prm_ptable = arg
            stage2 = 1
        elif opt in ("--img_sec_ptable"):
            img_sec_ptable = arg

    loader = generator(output_img)
    loader.idx = 0

    if (stage1 == 1) and (stage2 == 1):
        print 'There are only loader & BL1 in stage1.'
        print 'And there are primary partition table, secondary partition table and FIP in stage2.'
        sys.exit(1)
    elif (stage1 == 0) and (stage2 == 0):
        print 'No input images are specified.'
        sys.exit(1)
    elif stage1 == 1:
        loader.create_stage1(img_loader, img_bl1, output_img)
    elif stage2 == 1:
        loader.create_stage2(img_prm_ptable, img_sec_ptable, output_img)

    loader.end()

if __name__ == "__main__":
    main(sys.argv[1:])
