SRCS := $(wildcard js*.c utf*.c regex.c)
HDRS := $(wildcard js*.h utf.h regex.h)
OBJS := $(SRCS:%.c=build/%.o)

CC := clang
CFLAGS := -std=c99 -pedantic -Wall -Wextra -Wunreachable-code -Wno-unused-parameter -g

default: build build/js build/jsone build/re

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

build/libjs.a: $(OBJS)
	ar cru $@ $^

build/js: build/main.o build/libjs.a
	$(CC) -o $@ $^ -lm

build/jsone: build/main.o build/one.o
	$(CC) -o $@ $^ -lm

build/re: regex.c utf.c utftype.c
	$(CC) $(CFLAGS) -DTEST -o $@ $^

tags: $(SRCS) main.c $(HDRS)
	ctags $^

test: build/js
	python tests/sputniktests/tools/sputnik.py --tests=tests/sputniktests --command ./build/js --summary

clean:
	rm -f astnames.h opnames.h one.c build/*

.PHONY: default test clean
