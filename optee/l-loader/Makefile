CROSS_COMPILE?=arm-linux-gnueabihf-
CC=$(CROSS_COMPILE)gcc
CFLAGS=-march=armv7-a
LD=$(CROSS_COMPILE)ld
OBJCOPY=$(CROSS_COMPILE)objcopy
BL1=bl1.bin
PTABLE_LST:=aosp-4g aosp-8g linux-4g linux-8g

all: l-loader.bin ptable.img

%.o: %.S $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

l-loader.bin: start.o $(BL1)
	$(LD) -Bstatic -Tl-loader.lds -Ttext 0xf9800800 start.o -o loader
	$(OBJCOPY) -O binary loader temp
	python gen_loader.py -o $@ --img_loader=temp --img_bl1=$(BL1)
	rm -f loader temp

ptable.img:
	for ptable in $(PTABLE_LST); do \
		PTABLE=$${ptable} bash -x generate_ptable.sh;\
		python gen_loader.py -o ptable-$${ptable}.img --img_prm_ptable=prm_ptable.img;\
	done

clean:
	rm -f *.o *.img l-loader.bin
