PROGNM =  dyload
PREFIX ?= /usr/local
BINDIR ?= $(DESTDIR)$(PREFIX)/bin
MODULES = $(patsubst src/mod%.c, dist/modules/%.so, $(wildcard src/mod*.c))

include Makerules
CFLAGS += -Wno-disabled-macro-expansion

.PHONY: all bin clean scan-build cov-build install uninstall

all: dist bin $(MODULES)

bin: dist
	@$(CC) $(CFLAGS) -pie $(LDFLAGS) src/loader.c -o dist/$(PROGNM)

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

install:
	@install -Dm755 dist/$(PROGNM) $(BINDIR)/$(PROGNM)

uninstall:
	@rm -f -- $(BINDIR)/$(PROGNM)
