#include "module.h"

#include <time.h>

#define DEFVALUE "00.00 (UTC)"
#define MODFORMAT "%H.%M (%Z)"

size_t size = sizeof DEFVALUE;
signed interval = 15;

size_t
play (char ** buf) {

    if ( !buf || !*buf ) {
        return 0;
    }

    static time_t current;
    time(&current);
    return strftime(*buf, size, MODFORMAT, localtime(&current));
}

