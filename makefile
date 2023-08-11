# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -O3

# Library name
LIBRARY := libcwebsockets.a

# Source files
SRCS := CWebSockets.c

# Object files
OBJS := $(SRCS:.c=.o)

# Include directory
INCLUDE_DIR := .

# Installation directory
INSTALL_DIR := /usr/local

# Targets
.PHONY: all clean install

all: $(LIBRARY)

$(LIBRARY): $(OBJS)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE_DIR)

clean:
	rm -f $(OBJS) $(LIBRARY)

install: $(LIBRARY)
	install -m 755 $(LIBRARY) $(INSTALL_DIR)/lib
	install -m 644 $(INCLUDE_DIR)/*.h $(INSTALL_DIR)/include

uninstall:
	rm -f $(INSTALL_DIR)/lib/$(LIBRARY)
	rm -f $(patsubst $(INCLUDE_DIR)/%.h,$(INSTALL_DIR)/include/%.h,$(wildcard $(INCLUDE_DIR)/*.h))
