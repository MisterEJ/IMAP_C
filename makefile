CC = gcc
CFLAGS = -Wall -Wextra -pedantic
LDFLAGS = -L./libcyaml/build/release -lssl -lcrypto -lyaml ./libcyaml/build/release/libcyaml.a -Wl,-rpath=./libcyaml/build/release
BUILD_DIR = build

# Source files
SRCS = imap.c yaml_conf.c imap_client.c imap_utility.c
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

# Output executable
TARGET = $(BUILD_DIR)/imap

# Libcyaml library
LIBCYAML_DIR = libcyaml
LIBCYAML_TARGET = $(LIBCYAML_DIR)/build/release/libcyaml.so.1

.PHONY: all clean libcyaml

all: libcyaml $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

libcyaml:
	$(MAKE) -C $(LIBCYAML_DIR) VARIANT=release

clean:
	$(MAKE) -C $(LIBCYAML_DIR) clean
	rm -rf $(BUILD_DIR)

.PHONY: all clean libcyaml






