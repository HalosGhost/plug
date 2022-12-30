#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#define MODSEP " | "
#define PAINT_INTERVAL 1

struct plugin {
    signed * interval;
    size_t * size;
    signed (* setup)(void);
    size_t (* play)(char **);
    void (* teardown)(void);
    char * buffer;
};

struct plugin
load_plugin (void *);

signed
has_valid_name (const struct dirent *);

char**
discover_plugins (const char *);

size_t
load_plugins (const char *, void ***, struct plugin **);
