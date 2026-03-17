CC := gcc
CFLAGS := -Wall -Wextra -O2 -Iinclude -g
LDFLAGS := -lcjson

TARGET := client
SRCS := main.c json.c
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