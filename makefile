BUILD_DIR=build-$(shell uname -r)
NOPWD = --no-print-directory
CFLAGS ?= -O2 -g -Wall -pedantic -Wno-unknown-pragmas
CC=gcc $(CFLAGS) -std=gnu99 -D_LITTLE_ENDIAN -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64
MACH=$(shell uname -m)
DTO=-D_ILP64
COMPILE.cpp= $(CC) -E -C

export CC
export CFLAGS
export BUILD_DIR
export DTO
export COMPILE.cpp

all:
	$(MAKE) archcheck
	$(MAKE) all1
	cd libctf; $(MAKE) $(NOPWD)
	cd libdtrace ; $(MAKE) $(NOPWD) all
	cd libport ; $(MAKE) $(NOPWD)
	cd libproc/common ; $(MAKE) $(NOPWD)
	cd cmd ; $(MAKE) $(NOPWD)
all1:
	if [ ! -d $(BUILD_DIR) ] ; then \
		mkdir $(BUILD_DIR); \
	fi
archcheck:
	if [ "$(MACH)" != "x86_64" ]; then \
		echo "Dtrace for Linux only supports x86_64"; \
		exit 1; \
	fi
install:
	cd libdtrace ; $(MAKE) $(NOPWD) install
uninstall:
	cd libdtrace ; $(MAKE) $(NOPWD) uninstall
clean:
	-rm -rf $(BUILD_DIR)
	cd libctf; $(MAKE) $(NOPWD) clean
	cd libdtrace ; $(MAKE) $(NOPWD) clean
	cd libport ; $(MAKE) $(NOPWD) clean
	cd libproc/common ; $(MAKE) $(NOPWD) clean 
