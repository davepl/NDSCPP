# Compiler settings
CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -Werror -O2
INCLUDES = -I.
LDFLAGS =

# Libraries needed
LIBS = -lpthread -lcurl -lcpr -lgtest_main -lgtest  

# Binary name
TARGET = tests

# Source files
SOURCES = tests.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Detect platform
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Darwin)
    # macOS-specific settings using Homebrew
    INCLUDES += -I$(shell brew --prefix)/include/
    LDFLAGS += -L$(shell brew --prefix)/lib/
endif

# Default target
all: $(TARGET)

# Link the target binary
$(TARGET): $(OBJECTS)
	@echo "Linking $@..."
	@$(CXX) $(LDFLAGS) $(LIBS) -o $(TARGET) $(OBJECTS)

# Compile source files
%.o: %.cpp
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean build files
clean:
	@echo "Cleaning build files..."
	@rm -f $(OBJECTS) $(TARGET)

# Run the tests
test: $(TARGET)
	@echo "Running tests..."
	@./$(TARGET)

# Install dependencies on macOS
install-deps-mac:
	@echo "Installing dependencies via Homebrew..."
	@brew install googletest cpr

.PHONY: all clean test install-deps-mac