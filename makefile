CC = gcc
CFLAGS = -Wall -Wextra -pedantic
LDFLAGS = -L./libs -lssl -lcrypto -lcyaml -lyaml -Wl,-rpath=./libs
BUILD_DIR = build

# Source files
SRCS = imap.c yaml_conf.c imap_client.c imap_utility.c
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

# Output executable
TARGET = $(BUILD_DIR)/imap

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean







