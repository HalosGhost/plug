#include "plug.h"

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

