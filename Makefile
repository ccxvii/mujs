# Build type and install directories:

build ?= release

prefix ?= /usr/local
bindir ?= $(prefix)/bin
incdir ?= $(prefix)/include
libdir ?= $(prefix)/lib

VERSION = $(shell git describe --tags --always)

# Compiler flags for various configurations:

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

# You shouldn't need to edit anything below here.

OUT := build/$(build)

SRCS := $(wildcard js*.c utf*.c regexp.c)
HDRS := $(wildcard js*.h mujs.h utf.h regexp.h)

default: static
static: $(OUT) $(OUT)/mujs $(OUT)/libmujs.a $(OUT)/mujs.pc
shared: static $(OUT)/libmujs.so

astnames.h: jsparse.h
	grep -E '(AST|EXP|STM)_' jsparse.h | sed 's/^[^A-Z]*\(AST_\)*/"/;s/,.*/",/' | tr A-Z a-z > $@

opnames.h: jscompile.h
	grep -E 'OP_' jscompile.h | sed 's/^[^A-Z]*OP_/"/;s/,.*/",/' | tr A-Z a-z > $@

one.c: $(SRCS)
	ls $(SRCS) | awk '{print "#include \""$$1"\""}' > $@

jsdump.c: astnames.h opnames.h

$(OUT):
	mkdir -p $(OUT)

$(OUT)/main.o: main.c $(HDRS)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OUT)/libmujs.o: one.c $(HDRS)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OUT)/libmujs.a: $(OUT)/libmujs.o
	$(AR) cru $@ $^

$(OUT)/libmujs.so: one.c $(HDRS)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $< -lm

$(OUT)/mujs: $(OUT)/libmujs.o $(OUT)/main.o
	$(CC) $(LDFLAGS) -o $@ $^ -lm

$(OUT)/mujs.pc:
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
	install -m 644 build/release/mujs.pc $(DESTDIR)$(libdir)/pkgconfig
	install -m 755 build/release/mujs $(DESTDIR)$(bindir)

install-static: install-common
	install -m 644 build/release/libmujs.a $(DESTDIR)$(libdir)

install-shared: install-common
	install -m 755 build/release/libmujs.so $(DESTDIR)$(libdir)

install: install-static

tarball:
	git archive --format=zip --prefix=mujs-$(VERSION)/ HEAD > mujs-$(VERSION).zip
	git archive --format=tar --prefix=mujs-$(VERSION)/ HEAD | gzip > mujs-$(VERSION).tar.gz
	git archive --format=tar --prefix=mujs-$(VERSION)/ HEAD | xz > mujs-$(VERSION).tar.xz

tags: $(SRCS) main.c $(HDRS)
	ctags $^

clean:
	rm -rf build

nuke: clean
	rm -f astnames.h opnames.h one.c

debug:
	$(MAKE) build=debug

sanitize:
	$(MAKE) build=sanitize

release:
	$(MAKE) build=release shared

.PHONY: default static shared clean nuke
.PHONY: install install-common install-shared install-static
.PHONY: debug sanitize release
