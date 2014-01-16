SRCS := $(wildcard js*.c)
HDRS := $(wildcard js*.h)
OBJS := $(SRCS:%.c=build/%.o)

CFLAGS = -Wall -g

default: build js

build:
	mkdir -p build

build/%.o : %.c $(HDRS)
	$(CC) -c $< -o $@ $(CFLAGS)

build/libjs.a: $(OBJS)
	ar cru $@ $^

opnames.h : jscompile.h
	grep 'OP_' jscompile.h | sed 's/OP_/"/;s/,.*/",/' | tr A-Z a-z > opnames.h

jsdump.c : opnames.h

js: build/main.o build/libjs.a
	$(CC) -o $@ $^ -lm

tags: $(SRCS) main.c $(HDRS)
	ctags $^

clean:
	rm -f build/* js
