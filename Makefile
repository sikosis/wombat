CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
PREFIX ?= /boot/system/non-packaged
BINDIR ?= $(PREFIX)/bin
DESTDIR ?=

TARGET := build/wombat
SOURCES := src/main.cpp src/Formatter.cpp
OBJECTS := $(SOURCES:src/%.cpp=build/obj/%.o)
DEPENDENCIES := $(OBJECTS:.o=.d)

.PHONY: all clean install test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)

build/obj/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -Isrc -c $< -o $@

test: $(TARGET)
	sh tests/test.sh

install: $(TARGET)
	mkdir -p "$(DESTDIR)$(BINDIR)"
	install -m 755 $(TARGET) "$(DESTDIR)$(BINDIR)/wombat"

clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPENDENCIES)

-include $(DEPENDENCIES)

