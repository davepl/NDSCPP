SHELL:=/bin/bash

CC=clang++
CFLAGS=-std=c++20 -g3 -O3 -Ieffects
LDFLAGS=
LIBS=-lpthread -lz -lavformat -lavcodec -lavutil -lswscale -lswresample -lfmt

DEPFLAGS=-MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

SOURCES=main.cpp
EXECUTABLE=ndscpp
DEPDIR=.deps
OBJECTS:=$(SOURCES:.cpp=.o)
DEPFILES:=$(SOURCES:%.cpp=$(DEPDIR)/%.d)

define helptext =
Makefile for ndscpp

Usage:
	all	Build the ndscpp application (default)
	clean	Remove all build artifacts
	help	Show this help text

Examples:
	$$ make all

endef

# Detect platform
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Darwin)
    CFLAGS += -I$(shell brew --prefix)/include/ -I$(shell brew --prefix spdlog)/include
    LDFLAGS += -L$(shell brew --prefix)/lib/ -L$(shell brew --prefix fmt)/lib
endif

all: $(EXECUTABLE)

help:; @ $(info $(helptext)) :

$(EXECUTABLE): $(OBJECTS)
	@echo Linking $@...
	@$(CC) $(LDFLAGS) $^ -o $@ $(LIBS)

%.o: %.cpp $(DEPDIR)/%.d | $(DEPDIR)
	@echo Compiling $<...
	@$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(DEPDIR): ; @mkdir -p $@

$(DEPFILES):

clean:
	@echo Cleaning build files...
	@rm -f $(OBJECTS) $(EXECUTABLE) $(DEPFILES)

.PHONY: all clean help

include $(wildcard $(DEPFILES))
