KERNEL_RELEASE ?= `uname -r`
KERNEL_DIR ?= /usr/src/linux-headers-$(KERNEL_RELEASE)
# CFLAGS= -std=c99

default:
	$(MAKE) -C $(KERNEL_DIR) M=$$PWD

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$$PWD clean
