obj-m += crc-test-mod.o

all:
	$(MAKE) -C $(LINUX_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(LINUX_DIR) M=$(PWD) clean
