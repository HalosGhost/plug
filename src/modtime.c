#include "module.h"

#include <time.h>

#define DEFVALUE "00.00 (UTC)" /* "Wednesday, 01 September 00001" */
#define MODFORMAT "%H.%M (%Z)"

size_t size = sizeof DEFVALUE;
signed priority = 80;

size_t
play (char ** buf) {

    if ( !buf || !*buf ) {
        return 0;
    }

    static time_t current;
    time(&current);
    return strftime(*buf, size, MODFORMAT, localtime(&current));
}

