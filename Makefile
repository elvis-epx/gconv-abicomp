CC      ?= gcc
CFLAGS  ?= -O2 -g -Wall -Wextra
GCONVDIR ?= $(shell gcc -print-multiarch >/dev/null 2>&1 && echo /usr/lib/$$(gcc -print-multiarch)/gconv || echo /usr/lib/gconv)

all: ABICOMP.so

ABICOMP.so: abicomp.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $<

test: ABICOMP.so test-api
	./test.sh
	GCONV_PATH=$(CURDIR) ./test-api

test-api: test-api.c
	$(CC) $(CFLAGS) -fsanitize=address,undefined -o $@ $<

install: ABICOMP.so
	install -m 644 ABICOMP.so $(DESTDIR)$(GCONVDIR)/ABICOMP.so
	install -m 644 gconv-modules $(DESTDIR)$(GCONVDIR)/gconv-modules.d/abicomp.conf
	iconvconfig $(if $(DESTDIR),--prefix=$(DESTDIR))

uninstall:
	rm -f $(GCONVDIR)/ABICOMP.so $(GCONVDIR)/gconv-modules.d/abicomp.conf
	iconvconfig $(if $(DESTDIR),--prefix=$(DESTDIR))

clean:
	rm -f ABICOMP.so test-api

.PHONY: all test install uninstall clean
