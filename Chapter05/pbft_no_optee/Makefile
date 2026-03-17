CC := gcc
CFLAGS := -Wall -Wextra -O2 -Iinclude -g
LDFLAGS := -lssl -lcrypto -lpthread -lcjson -lcurl

TARGET := pbft
SRCS := main.c crypto.c json.c message.c network.c nonce.c utils.c
OBJS := $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

cleanobj:
	rm -f $(OBJS)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
