#include "module.h"

#include <dirent.h>
#include <sys/types.h>
#include <errno.h>

#define DEFVALUE "C: -100°C"
#define MODFORMAT "C: %hhd°C"

#define TARGET "x86_pkg_temp"

size_t modsize = sizeof DEFVALUE;

size_t
modstep (char ** buf) {

    if ( !buf || !*buf ) { return 0; }

    #define thermal_dir "/sys/class/thermal/"
    #define thermal_dev_size (sizeof thermal_dir + sizeof "cooling_device100/temp")
    static char temp_file [thermal_dev_size] = "";
    static signed long temp = 0;

    if ( !temp_file[0] ) {
        DIR * outer = opendir(thermal_dir);
        if ( !outer ) { return 0; }

        char path [thermal_dev_size] = thermal_dir;

        for ( struct dirent * p = readdir(outer); p; p = readdir(outer) ) {
            snprintf(path, thermal_dev_size, thermal_dir "%s", p->d_name);

            DIR * inner = opendir(path);
            if ( !inner ) { continue; }

            for ( struct dirent * pi = readdir(inner); pi; pi = readdir(inner) ) {
                if ( strcmp(pi->d_name, "type") ) { continue; }

                snprintf(path, thermal_dev_size, thermal_dir "%s/type", p->d_name);
                FILE * f = fopen(path, "r");
                char contents [sizeof TARGET] = "";
                errno = 0;
                signed ret = fscanf(f, "%[^ ]", contents);
                if ( ret == EOF && errno ) { continue; }
                fclose(f);
                if ( !strncmp(TARGET, contents, sizeof TARGET - 1) ) {
                    snprintf(temp_file, thermal_dev_size, thermal_dir "%s/temp", p->d_name);
                    break;
                }
            }

            if ( temp_file[0] ) { break; }
        }
    }

    FILE * f = fopen(temp_file, "r");
    signed ret = fscanf(f, "%ld", &temp);
    fclose(f);
    temp = ret == EOF ? 0 : temp;

    int res = snprintf(*buf, modsize, MODFORMAT, (signed char )(temp / 1000));

    return res > 0 ? (size_t )res : 0;
}

