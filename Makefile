PROGNM =  plug
PREFIX ?= /usr/local
BINDIR ?= $(DESTDIR)$(PREFIX)/bin
LIBDIR ?= $(DESTDIR)$(PREFIX)/lib
INCDIR ?= $(DESTDIR)$(PREFIX)/include
MODULES = $(patsubst src/mod%.c, dist/modules/%.so, $(wildcard src/mod*.c))

include Makerules
CFLAGS += -Wno-disabled-macro-expansion

.PHONY: all bin clean scan-build lib cov-build test install uninstall

all: dist bin $(MODULES)

lib: dist
	@$(CC) $(CFLAGS) -fPIC -shared $(LDFLAGS) src/loader.c -o dist/lib$(PROGNM).so
	@install -Dm644 src/$(PROGNM).h dist/$(PROGNM).h

bin: lib
	@$(CC) $(CFLAGS) -fPIE $(LDFLAGS) src/main.c -Ldist -l$(PROGNM) -o dist/$(PROGNM)

$(MODULES): dist/modules/%.so: src/mod%.c
	@$(CC) $(CFLAGS) `pkg-config --libs-only-l alsa` -fPIC -shared $< -o $@

clean:
	@rm -rf -- dist cov-int $(PROGNM).tgz make.sh ./src/*.plist

dist:
	@mkdir -p ./dist/modules

cov-build: dist
	@cov-build --dir cov-int ./make.sh
	@tar czvf $(PROGNM).tgz cov-int

scan-build:
	@scan-build --use-cc=$(CC) make all

test: all
	@(pushd dist; LD_LIBRARY_PATH=. ./$(PROGNM))

install:
	@install -Dm755 dist/$(PROGNM) $(BINDIR)/$(PROGNM)
	@install -Dm755 dist/lib$(PROGNM).so $(LIBDIR)/lib$(PROGNM).so
	@install -Dm755 dist/$(PROGNM).h $(INCDIR)/$(PROGNM).h

uninstall:
	@rm -f -- $(BINDIR)/$(PROGNM) $(LIBDIR)/lib$(PROGNM).so $(INCDIR)/$(PROGNM).h
