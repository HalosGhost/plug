#include "plug.h"

char **
discover_plugins (const char * path) {

    errno = 0;

    struct dirent ** plugins = 0;
    signed modcount = scandir(path, &plugins, has_valid_name, alphasort);
    if ( modcount == -1 ) {
        fprintf(stderr, "Failed to read from %s: %s\n", path, strerror(errno));
        return 0;
    }

    char ** paths = malloc(sizeof(char *) * (modcount + 1));
    memset(paths, 0, sizeof(char *) * (modcount + 1));

    for ( size_t i = 0; i < (size_t )modcount; ++ i ) {
        size_t len = strlen(plugins[i]->d_name);
        paths[i] = malloc(len + 2);
        snprintf(paths[i], len + 1, "%s", plugins[i]->d_name);
        free(plugins[i]);
        continue;
    }

    free(plugins);
    return paths;
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

    *(void **)(&p.teardown) = dlsym(handle, "teardown");

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
has_valid_name (const struct dirent * e) {

    size_t len = strlen(e->d_name);
    if ( len > 3 && !strncmp(".so", e->d_name + len - 3, 3)) {
        return 1;
    }

    return 0;
}

size_t
load_plugins (const char * modpath, void *** handles, struct plugin ** plugins) {

    char ** paths = discover_plugins(modpath);

    size_t modcount = 0;
    for ( char ** p = paths; *p; p++, modcount++ );

    if ( !modcount ) { goto cleanup; }

    *handles = malloc(sizeof(void *) * modcount);
    memset(*handles, 0, sizeof(void *) * modcount);

    *plugins = malloc(sizeof(struct plugin) * modcount);
    memset(*plugins, 0, sizeof(struct plugin) * modcount);

    size_t modpathlen = strlen(modpath) + 1;
    for ( size_t i = 0; i < modcount; ++i ) {
        size_t len = modpathlen + strlen(paths[i]) + 2;
        char * fullpath = malloc(len);
        snprintf(fullpath, len, "%s/%s", modpath, paths[i]);
        (*handles)[i] = dlopen(fullpath, RTLD_LAZY);

        free(fullpath);
        free(paths[i]);
    }

    fprintf(stderr, "Loaded %zu module(s)\n", modcount);

    for ( size_t i = 0; i < modcount; ++ i ) {
        (*plugins)[i] = load_plugin((*handles)[i]);
    }

    cleanup:
        free(paths);
        return modcount;
}

