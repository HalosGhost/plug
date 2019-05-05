#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/types.h>

#define MODSEP " | "

struct plugin {
    signed * priority;
    size_t * size;
    signed (* setup)(void);
    size_t (* play)(char **);
    void (* teardown)(void);
    char * buffer;
};

struct plugin
load_plugin (void *);

int
compare_plugins (const void *, const void *);

char**
discover_plugins (const char *);

