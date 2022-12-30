PROGNM =  plug

PREFIX ?= /usr/local
BINDIR ?= $(DESTDIR)$(PREFIX)/bin
LIBDIR ?= $(DESTDIR)$(PREFIX)/lib
INCDIR ?= $(DESTDIR)$(PREFIX)/include

SRCDIR = $(PWD)/src
INCDIR = $(PWD)/inc
BLDDIR = $(PWD)/bld

SOURCES = $(wildcard $(SRCDIR)/modules/*/module.c)
MODULES = $(patsubst $(SRCDIR)%,$(BLDDIR)%,$(patsubst %/module.c,%.so,$(SOURCES)))

MKDIR ?= mkdir -p --
RM := rm -rf --

include Makerules

.PHONY: all bin clean lib run install uninstall

all: $(MODULES) bin

$(BLDDIR)/modules/%.so: $(SRCDIR)/modules/%
	$(MKDIR) $(@D)
	(cd $<; $(MAKE) BLDDIR=$(@D) INCDIR=$(INCDIR))

lib: $(BLDDIR)/lib$(PROGNM).so

$(BLDDIR)/lib$(PROGNM).so: $(SRCDIR)/library.c
	$(MKDIR) $(@D)
	$(CC) $(CFLAGS) -fPIC -shared $(LDFLAGS) $< -o $@

bin: $(BLDDIR)/$(PROGNM)

$(BLDDIR)/$(PROGNM): $(SRCDIR)/main.c | $(BLDDIR)/lib$(PROGNM).so
	$(CC) $(CFLAGS) -fPIE $(LDFLAGS) $< -L$(BLDDIR) -l$(PROGNM) -o $@

clean:
	$(RM) $(BLDDIR)

run: all
	(pushd $(BLDDIR); LD_LIBRARY_PATH=. ./$(PROGNM))

install:
	install -Dm755 $(BLDDIR)/$(PROGNM) $(BINDIR)/$(PROGNM)
	install -Dm755 $(BLDDIR)/lib$(PROGNM).so $(LIBDIR)/lib$(PROGNM).so
	install -Dm755 $(BLDDIR)/$(PROGNM).h $(INCDIR)/$(PROGNM).h

uninstall:
	rm -f -- $(BINDIR)/$(PROGNM) $(LIBDIR)/lib$(PROGNM).so $(INCDIR)/$(PROGNM).h
