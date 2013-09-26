TOPDIR=$(shell pwd)

INSTALL=install
ifndef PREFIX
	PREFIX=/usr
endif

INSULTERR_CFLAGS  = -std=gnu99
INSULTERR_CFLAGS += -Wall
INSULTERR_CFLAGS += -Wunused-value
INSULTERR_CFLAGS += -fPIC

INSULTERR_LDFLAGS  = -shared
INSULTERR_LDFLAGS += -ldl
INSULTERR_LDFLAGS += -Wl,-init,init

V ?= 0
ifeq ($(V),0)
.SILENT:
endif

.PHONY: install clean dist distclean

all:
	$(CC) $(INSULTERR_CFLAGS) $(INSULTERR_LDFLAGS) $(CFLAGS) $(LDFLAGS) -o insulterr.so main.c

clean:
	rm insulterr.so

distclean: clean
