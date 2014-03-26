SRCS := $(wildcard js*.c utf*.c regex.c)
HDRS := $(wildcard js*.h mujs.h utf.h regex.h)
OBJS := $(SRCS:%.c=build/%.o)

prefix ?= /usr/local
bindir ?= $(prefix)/bin
incdir ?= $(prefix)/include
libdir ?= $(prefix)/lib

CC := clang
CFLAGS := -std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter -Wunreachable-code

ifeq "$(build)" "debug"
CFLAGS += -g
else
CFLAGS += -O3
endif

default: build build/mujs build/mujsone

debug:
	$(MAKE) build=debug

release:
	$(MAKE) build=release

astnames.h: jsparse.h
	grep '\(AST\|EXP\|STM\)_' jsparse.h | sed 's/^[ \t]*\(AST_\)\?/"/;s/,.*/",/' | tr A-Z a-z > $@

opnames.h: jscompile.h
	grep 'OP_' jscompile.h | sed 's/^[ \t]*OP_/"/;s/,.*/",/' | tr A-Z a-z > $@

one.c: $(SRCS)
	ls $(SRCS) | awk '{print "#include \""$$1"\""}' > $@

jsdump.c: astnames.h opnames.h

build:
	mkdir -p build

build/%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -o $@ -c $<

build/libmujs.a: $(OBJS)
	ar cru $@ $^

build/mujs: build/main.o build/libmujs.a
	$(CC) -o $@ $^ -lm

build/mujsone: build/main.o build/one.o
	$(CC) -o $@ $^ -lm

install: release
	install -d $(DESTDIR)$(incdir)
	install -d $(DESTDIR)$(libdir)
	install -d $(DESTDIR)$(bindir)
	install -t $(DESTDIR)$(incdir) mujs.h
	install -t $(DESTDIR)$(libdir) build/libmujs.a
	install -t $(DESTDIR)$(bindir) build/mujs

tags: $(SRCS) main.c $(HDRS)
	ctags $^

test: build/js
	python tests/sputniktests/tools/sputnik.py --tests=tests/sputniktests --command ./build/js --summary

clean:
	rm -f astnames.h opnames.h one.c build/*

.PHONY: default test clean install debug release
