# Simple Makefile — alternative to CMake.
# Usage:
#   make          # build ./cvm
#   make run      # run sample programs
#   make clean

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude
SRC      := src/lexer.cpp src/parser.cpp src/compiler.cpp src/vm.cpp src/main.cpp
TARGET   := cvm

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $@

run: $(TARGET)
	@for f in tests/*.cvm; do \
	    echo "======== $$f ========"; \
	    ./$(TARGET) $$f; \
	    echo; \
	done

clean:
	rm -f $(TARGET)

.PHONY: all run clean
