CROSS_COMPILE = arm-linux-
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm

STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump

export AS LD CC CPP AR NM
export STRIP OBJCOPY OBJDUMP

CFLAGS := -Wall -Werror -O2
#调试时最好取消优化，不然难以跟踪
#CFLAGS := -Wall -Werror -g
CFLAGS += -I $(shell pwd)/include

LDFLAGS := -lrt -lconfig -L$(shell pwd)/lib

export CFLAGS LDFLAGS LDFLAGS1

TOPDIR := $(shell pwd)
export TOPDIR

TARGET := f806A

obj-y += main.o
obj-y += connect/
obj-y += debug/
obj-y += frame/
obj-y += command/
obj-y += parameter/
obj-y += rf_ctrl/

all : 
	make -C ./ -f $(TOPDIR)/Makefile.build
	$(CC) $(LDFLAGS) -o $(TARGET) built-in.o
	cp $(TARGET) /var/lib/tftpboot

clean:
	rm -f $(shell find -name "*.o")
	rm -f $(TARGET)

distclean:
	rm -f $(shell find -name "*.o")
	rm -f $(shell find -name "*.d")
	rm -f $(TARGET)
	
