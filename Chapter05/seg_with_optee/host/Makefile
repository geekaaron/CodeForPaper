CC      ?= $(CROSS_COMPILE)gcc
LD      ?= $(CROSS_COMPILE)ld
AR      ?= $(CROSS_COMPILE)ar
NM      ?= $(CROSS_COMPILE)nm
OBJCOPY ?= $(CROSS_COMPILE)objcopy
OBJDUMP ?= $(CROSS_COMPILE)objdump
READELF ?= $(CROSS_COMPILE)readelf

OBJS = main.o

CFLAGS += -Wall \
          -I../ta/include \
          -I$(TEEC_EXPORT)/include \
          -I./include \
          -I/home/q/projects/rpi3/buildroot/output/staging/usr/include

LDADD += -L$(TEEC_EXPORT)/lib \
         -L/home/q/projects/rpi3/buildroot/output/staging/usr/lib \
         -lteec -lcurl -lssl -lcrypto -lz

BINARY = optee_example_seg_with_optee

.PHONY: all
all: $(BINARY)

$(BINARY): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $< $(LDADD)

.PHONY: clean
clean:
	rm -f $(OBJS) $(BINARY)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
