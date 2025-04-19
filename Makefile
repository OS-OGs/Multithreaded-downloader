CC := gcc

# Directories
SRC_DIR := src
INCLUDE_DIR := include

# Compiler flags
CFLAGS := -I$(INCLUDE_DIR) -pthread -Wall -Wextra -Werror

# Linker flags
LDFLAGS := -lcurl -lpthread

# Target executable
TARGET := file_download_manager

# Source and object files
SRCS := $(wildcard $(SRC_DIR)/*.c)
HEADERS := $(wildcard $(INCLUDE_DIR)/*.h)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(SRC_DIR)/%.o)

.PHONY: all clean

# Default target
all: $(TARGET)

# Link objects into the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Compile source files into object files
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET)
