SRCS := js-state.c js-string.c js-load.c js-lex.c js-parse.c
HDRS := js.h js-parse.h
OBJS := $(SRCS:%.c=build/%.o)

CFLAGS = -Wall -g

default: build js

build:
	mkdir -p build

build/%.o : %.c $(HDRS)
	$(CC) -c $< -o $@ $(CFLAGS)

build/libjs.a: $(OBJS)
	ar cru $@ $^

js: build/main.o build/libjs.a
	$(CC) -o $@ $^ -lm

tags:
	ctags *.c *.h

clean:
	rm -f build/* js
