KERNELRELEASE ?= $(shell uname -r)
#INSTALL_MOD_DIR ?= updates
KERNEL_DIR  ?= /lib/modules/${KERNELRELEASE}/build
INSTALL_DIR ?= /lib/modules/${KERNELRELEASE}/updates #${INSTALL_MOD_DIR}
EXTRA_CFLAGS += -I`pwd` -Wno-declaration-after-statement
#obj-m += hello.o

kernel: CFLAGS += -O3 -std=c99

kernel:
	make -C ${KERNEL_DIR} M=`pwd` modules

clean-kernel:
	make -C ${KERNEL_DIR} M=`pwd` clean

install:
	$(shell mkdir -p ${INSTALL_DIR})
	make -C ${KERNEL_DIR} M=`pwd` modules_install

all: kernel
clean: clean-kernel