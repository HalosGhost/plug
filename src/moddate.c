#include "module.h"

#include <time.h>

#define DEFVALUE "Wednesday, 01 September 00001"
#define MODFORMAT "%A, %d %B %Y"

size_t size = sizeof DEFVALUE;
signed priority = 90;

size_t
step (char ** buf) {

    if ( !buf || !*buf ) {
        return 0;
    }

    static time_t current;
    time(&current);
    return strftime(*buf, size, MODFORMAT, localtime(&current));
}

