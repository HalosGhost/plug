#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

extern size_t size;
extern signed priority;

signed
init (void);

size_t
step (char **);
