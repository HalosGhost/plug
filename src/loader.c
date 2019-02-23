#include "loader.h"

static struct plugin plugins [MAXMODS];

signed
main (signed argc, char * argv []) {

    if ( !argc ) { return EXIT_FAILURE; }

    signed status = EXIT_SUCCESS;

    void * handles [MAXMODS] = { 0 };
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

    free(modpath);

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
        plugins[i] = load_plugin(handles[i]);
    }

    qsort(plugins, modcount, sizeof (struct plugin), compare_plugins);

    for ( size_t i = 0; i < modcount; ++ i ) {
        if ( !plugins[i].priority ) { continue; }

        if ( plugins[i].setup ) { plugins[i].setup(); }
    }

    for ( size_t i = 0; i < modcount; ++ i ) {
        if ( !plugins[i].priority ) { continue; }

        plugins[i].play(&plugins[i].buffer);
        printf("%s%s", plugins[i].buffer, i + 1 != modcount ? MODSEP : "");
    }

    printf("\n");

    for ( size_t i = 0; i < modcount; ++ i ) {
        if ( !plugins[i].priority ) { continue; }

        free(plugins[i].buffer);
    }

    cleanup:
        (void)chdir(cwd);
        if ( modules ) { closedir(modules); }

        for ( size_t i = 0; i < modcount; ++ i ) {
            if ( handles[i] ) { dlclose(handles[i]); }
        }

        return status;
}

struct plugin
load_plugin (void * handle) {

    struct plugin p;

    *(void **)(&p.setup) = dlsym(handle, "setup");

    *(void **)(&p.play) = dlsym(handle, "play");
    const char * dlerr = dlerror();

    if ( dlerr ) {
        fprintf(stderr, "Failed to find play: %s\n", dlerr);
        p.priority = 0;
    }

    dlerror();
    *(void **)(&p.size) = dlsym(handle, "size");
    dlerr = dlerror();
    if ( dlerr ) {
        fprintf(stderr, "Failed to find size: %s\n", dlerr);
        p.priority = 0;
    }

    dlerror();
    *(void **)(&p.priority) = dlsym(handle, "priority");
    dlerr = dlerror();
    if ( dlerr ) {
        fprintf(stderr, "Failed to find priority: %s\n", dlerr);
        p.priority = 0;
    }

    if ( !p.priority ) { return p; }

    p.buffer = malloc(*p.size);

    return p;
}

signed
compare_plugins (const void * left, const void * right) {

    signed l = *(((const struct plugin * )left)->priority);
    signed r = *(((const struct plugin * )right)->priority);

    return l < r ? -1 :
           l > r ?  1 : 0;
}

