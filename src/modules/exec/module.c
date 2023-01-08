#include "module.h"

#define COMMAND "/usr/bin/printf 'Hello, World!\n'"

size_t size = 512;
signed interval = 30;

size_t
play (char ** buf) {

    if ( !buf || !*buf ) {
        return 0;
    }

    memset(*buf, 0, size);

    errno = 0;
    FILE * p = popen(COMMAND, "r");
    if ( !p ) {
        MODLOG(LOG_ERR, "Failed to execute `%s`: %s\n", COMMAND, strerror(errno));
    }

    if ( fscanf(p, "%511[^\n]", *buf) != 1 ) {
        MODLOG(LOG_ERR, "%s\n", "Failed to read first line of output");
    }

    fclose(p);

    return strlen(*buf) + 1;
}

