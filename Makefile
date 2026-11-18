CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2

TARGET := build/wombat
SOURCES := src/main.cpp src/Formatter.cpp

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SOURCES) src/Formatter.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc $(SOURCES) -o $(TARGET)

test: $(TARGET)
	sh tests/test.sh

clean:
	rm -f $(TARGET)

