SRCS := $(wildcard js*.c utf*.c regexp.c)
HDRS := $(wildcard js*.h mujs.h utf.h regexp.h)

VERSION = $(shell git describe --tags --always)

prefix ?= /usr/local
bindir ?= $(prefix)/bin
incdir ?= $(prefix)/include
libdir ?= $(prefix)/lib

CFLAGS := -std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter

ifeq "$(CC)" "clang"
CFLAGS += -Wunreachable-code
endif

ifeq "$(shell uname)" "Linux"
CFLAGS += -ffunction-sections -fdata-sections
LDFLAGS += -Wl,--gc-sections -Wl,-s
endif

ifeq "$(build)" "debug"
CFLAGS += -g
else ifeq "$(build)" "sanitize"
CFLAGS += -pipe -g -fsanitize=address -fno-omit-frame-pointer
LDFLAGS += -fsanitize=address
else
CFLAGS += -Os
endif

default: build build/mujs build/libmujs.a build/libmujs.so build/mujs.pc

astnames.h: jsparse.h
	grep -E '(AST|EXP|STM)_' jsparse.h | sed 's/^[^A-Z]*\(AST_\)*/"/;s/,.*/",/' | tr A-Z a-z > $@

opnames.h: jscompile.h
	grep -E 'OP_' jscompile.h | sed 's/^[^A-Z]*OP_/"/;s/,.*/",/' | tr A-Z a-z > $@

one.c: $(SRCS)
	ls $(SRCS) | awk '{print "#include \""$$1"\""}' > $@

jsdump.c: astnames.h opnames.h

build:
	mkdir -p build

build/main.o: main.c $(HDRS)
	$(CC) $(CFLAGS) -o $@ -c $<

build/libmujs.o: one.c $(HDRS)
	$(CC) $(CFLAGS) -o $@ -c $<

build/libmujs.a: build/libmujs.o
	$(AR) cru $@ $^

build/libmujs.so: one.c $(HDRS)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $< -lm

build/mujs: build/libmujs.o build/main.o
	$(CC) $(LDFLAGS) -o $@ $^ -lm

build/mujs.pc:
	@ echo Creating $@
	@ echo > $@ Name: mujs
	@ echo >> $@ Description: MuJS embeddable Javascript interpreter
	@ echo >> $@ Version: $(VERSION)
	@ echo >> $@ URL: http://dev.mujs.com/
	@ echo >> $@ Cflags: -I$(incdir)
	@ echo >> $@ Libs: -L$(libdir) -lmujs
	@ echo >> $@ Libs.private: -lm

install-common: release
	install -d $(DESTDIR)$(incdir)
	install -d $(DESTDIR)$(libdir)
	install -d $(DESTDIR)$(libdir)/pkgconfig
	install -d $(DESTDIR)$(bindir)
	install -m 644 mujs.h $(DESTDIR)$(incdir)
	install -m 644 build/mujs.pc $(DESTDIR)$(libdir)/pkgconfig
	install -m 755 build/mujs $(DESTDIR)$(bindir)

install-static: install-common
	install -m 644 build/libmujs.a $(DESTDIR)$(libdir)

install-shared: install-common
	install -m 755 build/libmujs.so $(DESTDIR)$(libdir)

install: install-static install-shared

tarball:
	git archive --format=zip --prefix=mujs-$(VERSION)/ HEAD > mujs-$(VERSION).zip
	git archive --format=tar --prefix=mujs-$(VERSION)/ HEAD | gzip > mujs-$(VERSION).tar.gz
	git archive --format=tar --prefix=mujs-$(VERSION)/ HEAD | xz > mujs-$(VERSION).tar.xz

tags: $(SRCS) main.c $(HDRS)
	ctags $^

clean:
	rm -f astnames.h opnames.h one.c build/*

debug:
	$(MAKE) build=debug clean default

sanitize:
	$(MAKE) build=sanitize clean default

release:
	$(MAKE) build=release clean default

.PHONY: default clean debug sanitize release install install-common install-shared install-static
