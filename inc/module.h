#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>

#define READ_ENV(cvar, def) \
do { \
    (cvar) = getenv(#def); \
    (cvar) = (cvar) ? (cvar) : (def); \
} while ( false )

#define MODLOG(lvl, fmt, ...) syslog(lvl, "[" MODNAME "] " fmt, __VA_ARGS__)

extern size_t size;
extern signed interval;

signed
setup (void);

size_t
play (char **);

void
teardown (void);

