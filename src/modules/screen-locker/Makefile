NAME := $(shell basename "$$PWD")

LIBS := x11 xext xscrnsaver

CFLAGS += $(_CFLAGS) -I$(HDRDIR) $(shell pkg-config --libs-only-L $(LIBS))
LINKFLAGS := $(shell pkg-config --libs-only-l $(LIBS))

$(BLDDIR)/$(NAME).so: module.c
	$(CC) $(CFLAGS) -DMODNAME='"$(NAME)"' -fPIC -shared $< -o $@ $(LINKFLAGS)
