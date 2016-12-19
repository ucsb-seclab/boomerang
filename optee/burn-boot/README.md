hisi-idt
========

A tool for downloading binaries to soc ram and ddr through serial port.

1. Command
----------

sudo python hisi-idt.py -d device --img1 <fastboot1> --img2 <fastboot2>
or
sudo python hisi-idt.py -d device --img1 <l-loader.bin>

2. Download Steps
-----------------

a.  Insert USB cable and connect with PC;
b.  Enter force downloand mode:

    For LCB board: short J15 / Pin 3-4;

    For EVB board: press “Force Download” key + “Reset” key, and then
    release “Reset” key then will enter into “force download” mode;

c.  Check if there have the device node “/dev/ttyUSBx”, if there have
    device node that means the PC has detected the target board;
d.  Use command "sudo python hisi-idt.py" to run the script; after idt
    download binaries successfully, it will print out below log:

    +----------------------+
     Serial:  /dev/ttyUSB1
     Image1:  fastboot1.img
     Image2:  fastboot2.img
    +----------------------+

    Sending fastboot1.img ...
    Done

    Sending fastboot2.img ...
    Done

3. Burn Images
--------------

After download fastboot1.img and fastboot2.img on the board, then can use
fastboot command to burn images:

sudo fastboot flash fastboot1 fastboot1.img
sudo fastboot flash fastboot fastboot2.img

4. Troubleshooting
------------------

a.  After enter the force download mode, if Ubuntu PC cannot recognize the
    device ttyUSBx; this issue can be fixed by input below commands:
    sudo echo 12D1 3609 > /sys/bus/usb-serial/drivers/option1/new_id
    sudo makenod /dev/ttyUSB0 c 188 0

b.  Need supervisor permission for hisi-idt.py: "sudo python hisi-idt.py"

c.  Need supervisor permission for fastboot: "sudo fastboot"

d.  If download binaries failed with below message:

    Sending fastboot1.img ...
    failed
    failed

    Usually this means you are using the wrong ttyUSBx device; the reason
    is when connect board with the uart cable and usb cable, then PC will
    create two device nodes /dev/ttyUSB0 and /dev/ttyUSB1;

    But the nodes which are randomly binding to uart and usb, so sometimes
    /dev/ttyUSB0 is created for the uart and /dev/ttyUSB1 is for the usb
    port, in this case should use /dev/ttyUSB1 for the idt; if PC exchanges
    the nodes then should use /dev/ttyUSB0.
