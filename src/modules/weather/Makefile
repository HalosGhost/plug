NAME := $(shell basename "$$PWD")

CFLAGS += $(_CFLAGS) -I$(HDRDIR) -I$(RTDIR)/.secrets
LINKFLAGS := $(shell pkg-config --libs-only-l libcurl jansson)

$(BLDDIR)/$(NAME).so: module.c
	$(CC) $(CFLAGS) -DMODNAME='"$(NAME)"' -fPIC -shared $< -o $@ $(LINKFLAGS)
