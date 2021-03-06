#include "plug.h"

char **
discover_plugins (const char * path) {

    errno = 0;
    DIR * dir = opendir(path);
    if ( !dir ) {
        fprintf(stderr, "Failed to read from %s: %s\n", path, strerror(errno));
        return 0;
    }

    size_t modcount = 0;
    for ( struct dirent * p = readdir(dir); p; p = readdir(dir) ) {
        size_t len = strlen(p->d_name);
        if ( len > 3 && !strncmp(".so", p->d_name + len - 3, 3)) {
            ++modcount;
        }
    }

    char ** paths = malloc(sizeof(char *) * (modcount + 1));
    memset(paths, 0, sizeof(char *) * (modcount + 1));

    rewinddir(dir);

    while ( modcount-- > 0 ) {
        struct dirent * p = readdir(dir);
        size_t len = strlen(p->d_name);
        if ( !strncmp(".so", p->d_name + len - 3, 3) ) {
            paths[modcount] = malloc(len + 1);
            strncpy(paths[modcount], p->d_name, len);
            continue;
        }

        ++modcount;
    }

    closedir(dir);

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
compare_plugins (const void * left, const void * right) {

    signed l = *(((const struct plugin * )left)->priority);
    signed r = *(((const struct plugin * )right)->priority);

    return l < r ? -1 :
           l > r ?  1 : 0;
}

