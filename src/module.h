#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define READ_ENV(cvar, def) \
const char * cvar = 0; \
do { \
    (cvar) = getenv(#def); \
    (cvar) = (cvar) ? (cvar) : (def); \
} while ( false )

extern size_t size;
extern signed priority;

signed
setup (void);

size_t
play (char **);

void
teardown (void);

