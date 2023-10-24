NAME := $(shell basename "$$PWD")

CFLAGS += $(_CFLAGS) -I$(HDRDIR)
LINKFLAGS := $(shell pkg-config --libs-only-l libcurl)

$(BLDDIR)/$(NAME).so: module.c
	$(CC) $(CFLAGS) -DMODNAME='"$(NAME)"' -fPIC -shared $< -o $@ $(LINKFLAGS)
