NAME := $(shell basename "$$PWD")

CFLAGS += -I$(INCDIR)
LINKFLAGS := #$(shell pkg-config --libs-only-l)

$(BLDDIR)/$(NAME).so: module.c
	$(CC) $(CFLAGS) -fPIC -shared $< -o $@ $(LINKFLAGS)
