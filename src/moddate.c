#include "module.h"

#include <time.h>

#define DEFVALUE "Wednesday, 01 September 00001"
#define MODFORMAT "%A, %d %B %Y"

size_t modsize = sizeof DEFVALUE;

size_t
modstep (char ** buf) {

    if ( !buf || !*buf ) {
        return 0;
    }

    static time_t current;
    time(&current);
    return strftime(*buf, modsize, MODFORMAT, localtime(&current));
}

