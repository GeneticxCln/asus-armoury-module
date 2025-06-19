# Universal Laptop Armoury Kernel Module Makefile

# Module name
obj-m := universal-armoury.o
universal-armoury-objs := asus-armoury.o

# Kernel build directory
KERNEL_DIR := /lib/modules/$(shell uname -r)/build

# Current directory
PWD := $(shell pwd)

# Default target
all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

# Clean target
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

# Install target
install: all
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules_install
	depmod -a

# Uninstall target
uninstall:
	rm -f /lib/modules/$(shell uname -r)/extra/universal-armoury.ko
	depmod -a

# Load module
load:
	sudo insmod universal-armoury.ko

# Unload module
unload:
	sudo rmmod universal-armoury

# Show module info
info:
	modinfo universal-armoury.ko

# Check module status
status:
	lsmod | grep universal-armoury

# Help target
help:
	@echo "Available targets:"
	@echo "  all      - Build the kernel module"
	@echo "  clean    - Clean build files"
	@echo "  install  - Install the module to system"
	@echo "  uninstall- Remove the module from system"
	@echo "  load     - Load the module"
	@echo "  unload   - Unload the module"
	@echo "  info     - Show module information"
	@echo "  status   - Check if module is loaded"
	@echo "  help     - Show this help message"

.PHONY: all clean install uninstall load unload info status help

