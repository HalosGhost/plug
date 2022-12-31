PROGNM =  plug

PREFIX ?= /usr/local
BINDIR ?= $(DESTDIR)$(PREFIX)/bin
LIBDIR ?= $(DESTDIR)$(PREFIX)/lib
INCDIR ?= $(DESTDIR)$(PREFIX)/include

SRCDIR = $(PWD)/src
HDRDIR = $(PWD)/inc
BLDDIR = $(PWD)/bld

SOURCES = $(wildcard $(SRCDIR)/modules/*/module.c)
MODULES = $(patsubst $(SRCDIR)%,$(BLDDIR)%,$(patsubst %/module.c,%.so,$(SOURCES)))

MKDIR ?= mkdir -p --
RM := rm -rf --

include Makerules

.PHONY: all bin clean lib run debug install uninstall

all: $(MODULES) bin

$(BLDDIR)/modules/%.so: $(SRCDIR)/modules/% $(HDRDIR)/plug.h
	$(MKDIR) $(@D)
	(cd $<; $(MAKE) BLDDIR=$(@D) HDRDIR=$(HDRDIR) CFLAGS="$(CFLAGS)")

lib: $(BLDDIR)/lib$(PROGNM).so

$(BLDDIR)/lib$(PROGNM).so: $(SRCDIR)/library.c $(HDRDIR)/$(PROGNM).h
	$(MKDIR) $(@D)
	$(CC) $(CFLAGS) -fPIC -shared $(LDFLAGS) $< -o $@

bin: $(BLDDIR)/$(PROGNM)

$(BLDDIR)/$(PROGNM): $(SRCDIR)/main.c $(HDRDIR)/main.h $(HDRDIR)/$(PROGNM).h | $(BLDDIR)/lib$(PROGNM).so
	$(CC) $(CFLAGS) -DPREFIX='"$(PREFIX)"' -fPIE $(LDFLAGS) $< -L$(BLDDIR) -l$(PROGNM) -o $@

clean:
	$(RM) $(BLDDIR)

run: all
	(pushd $(BLDDIR); LD_LIBRARY_PATH=. ./$(PROGNM))

debug: all
	(pushd $(BLDDIR); LD_LIBRARY_PATH=. gdb ./$(PROGNM))

install:
	install -Dm755 $(BLDDIR)/$(PROGNM) $(BINDIR)/$(PROGNM)
	install -Dm755 $(BLDDIR)/lib$(PROGNM).so $(LIBDIR)/lib$(PROGNM).so
	install -Dm644 $(HDRDIR)/$(PROGNM).h $(INCDIR)/$(PROGNM)/$(PROGNM).h
	install -Dm644 $(HDRDIR)/module.h $(INCDIR)/$(PROGNM)/module.h
	install -d -m755 $(LIBDIR)/$(PROGNM)/modules
	install -m755 -t $(LIBDIR)/$(PROGNM)/modules $(BLDDIR)/modules/*

uninstall:
	$(RM) -- $(BINDIR)/$(PROGNM) $(LIBDIR)/lib$(PROGNM).so $(INCDIR)/$(PROGNM).h $(LIBDIR)/$(PROGNM)
