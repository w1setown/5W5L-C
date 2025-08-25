# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O3 -std=c11 -Iinclude
LDFLAGS = -lpthread

# Directories
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

# Source files and objects
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TARGET = $(BINDIR)/word_combo_finder

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(OBJDIR) $(BINDIR)

# Link the executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJDIR)/*.o $(TARGET)

# Rebuild everything
rebuild: clean all

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: all

# Show help
help:
	@echo "Available targets:"
	@echo "  all     - Build the program (default)"
	@echo "  clean   - Remove build files"
	@echo "  rebuild - Clean and build"
	@echo "  debug   - Build with debug symbols"
	@echo "  help    - Show this help"

.PHONY: all clean rebuild debug help