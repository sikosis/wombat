CXX ?= c++
CC ?= cc
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
CFLAGS ?= -std=c11 -O2
PREFIX ?= /boot/system/non-packaged
BINDIR ?= $(PREFIX)/bin
DESTDIR ?=

TARGET := build/wombat
SOURCES := src/main.cpp src/Formatter.cpp src/BashParser.cpp
OBJECTS := $(SOURCES:src/%.cpp=build/obj/%.o)
DEPENDENCIES := $(OBJECTS:.o=.d)
TREE_SITTER_OBJECTS := build/obj/tree-sitter.o build/obj/tree-sitter-bash-parser.o build/obj/tree-sitter-bash-scanner.o
TREE_SITTER_INCLUDES := -Ithird_party/tree-sitter/lib/include -Ithird_party/tree-sitter/lib/src -Ithird_party/tree-sitter-bash/src -Ithird_party/tree-sitter-bash/bindings/c

.PHONY: all clean install test

all: $(TARGET)

$(TARGET): $(OBJECTS) $(TREE_SITTER_OBJECTS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(TREE_SITTER_OBJECTS) -o $(TARGET)

build/obj/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -Isrc $(TREE_SITTER_INCLUDES) -c $< -o $@

build/obj/tree-sitter.o: third_party/tree-sitter/lib/src/lib.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP $(TREE_SITTER_INCLUDES) -c $< -o $@

build/obj/tree-sitter-bash-parser.o: third_party/tree-sitter-bash/src/parser.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP $(TREE_SITTER_INCLUDES) -c $< -o $@

build/obj/tree-sitter-bash-scanner.o: third_party/tree-sitter-bash/src/scanner.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP $(TREE_SITTER_INCLUDES) -c $< -o $@

test: $(TARGET)
	sh tests/test.sh

install: $(TARGET)
	mkdir -p "$(DESTDIR)$(BINDIR)"
	install -m 755 $(TARGET) "$(DESTDIR)$(BINDIR)/wombat"

clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPENDENCIES) $(TREE_SITTER_OBJECTS) $(TREE_SITTER_OBJECTS:.o=.d)

-include $(DEPENDENCIES)

