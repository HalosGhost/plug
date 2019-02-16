#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <errno.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/types.h>

#define MAXMODS 100

signed
main (signed argc, char * argv []) {

    if ( !argc ) { return EXIT_FAILURE; }

    signed status = EXIT_SUCCESS;

    void * handles [MAXMODS] = { 0 };
    const char * dlerr = 0;
    size_t modcount = 0;

    char cwd [PATH_MAX] = "";
    errno = 0;
    char * res = getcwd(cwd, PATH_MAX - 1);
    if ( !res ) {
        fprintf(stderr, "Failed to get cwd: %s\n", strerror(errno));
    }

    size_t modpathlen = strlen(argv[0]) + sizeof "/modules";
    char * modpath = malloc(modpathlen);

    snprintf(modpath, modpathlen, "%s/modules", dirname(argv[0]));

    errno = 0;
    signed res1 = chdir(modpath);
    if ( res1 ) {
        fprintf(stderr, "Failed to cd to %s: %s\n", modpath, strerror(errno));
    }

    DIR * modules = opendir(".");
    if ( !modules ) {
        fprintf(stderr, "Failed to open modules directory: %s\n", strerror(errno));
        status = EXIT_FAILURE;
        goto cleanup;
    }

    for ( struct dirent * p = readdir(modules); p; p = readdir(modules) ) {
        if ( modcount == MAXMODS ) { break; }

        size_t len = strlen(p->d_name);
        if ( !strncmp(".so", p->d_name + len - 3, 3) ) {
            size_t pathlen = len + sizeof "./" + 1;
            char * path = malloc(pathlen);
            snprintf(path, pathlen, "./%s", p->d_name);
            handles[modcount] = dlopen(path, RTLD_LAZY);
            if ( !handles[modcount] ) {
                fprintf(stderr, "Failed to load module: %s\n", dlerror());
            } else {
                ++modcount;
            }

            free(path);
        }
    }

    fprintf(stderr, "Loaded %zu module(s)\n", modcount);

    for ( size_t i = 0; i < modcount; ++ i ) {

        size_t (*modstep)(char **, bool);
        dlerror();
        *(void **)(&modstep) = dlsym(handles[i], "modstep");
        dlerr = dlerror();

        if ( dlerr ) {
            fprintf(stderr, "Failed to find modstep: %s\n", dlerr);
            status = EXIT_FAILURE;
            goto cleanup;
        }

        size_t * modsize = 0;
        dlerror();
        *(void **)(&modsize) = dlsym(handles[i], "modsize");
        dlerr = dlerror();
        if ( dlerr ) {
            fprintf(stderr, "Failed to find modsize: %s\n", dlerr);
            status = EXIT_FAILURE;
            goto cleanup;
        }

        char * buf = malloc(*modsize);
        modstep(&buf, false);
        printf("%s\n", buf);
        free(buf);
    }

    cleanup:
        (void)chdir(cwd);
        if ( modules ) { closedir(modules); }

        for ( size_t i = 0; i < modcount; ++ i ) {
            if ( handles[i] ) { dlclose(handles[i]); }
        }

        return status;
}

