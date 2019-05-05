#include <signal.h>
#include <unistd.h>

#include "plug.h"

static volatile sig_atomic_t caught_signum;

void
signal_handler (signed);

signed
main (signed argc, char * argv []) {

    if ( !argc ) { return EXIT_FAILURE; }

    signed status = EXIT_SUCCESS;

    signal(SIGINT,  signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);
    fputs("\x1b[?25l", stdout);

    char * basepath = argc > 1 && argv[1] ? argv[1] : dirname(argv[0]);

    size_t modpathlen = strlen(basepath) + sizeof "/modules";
    char * modpath = malloc(modpathlen);

    snprintf(modpath, modpathlen, "%s/modules", basepath);

    char ** paths = discover_plugins(modpath);

    void ** handles = 0;
    struct plugin * plugins = 0;

    size_t modcount = 0;
    for ( char ** p = paths; *p; p++, modcount++ );

    if ( !modcount ) { goto cleanup; }

    handles = malloc(sizeof(void *) * modcount);
    memset(handles, 0, sizeof(void *) * modcount);

    plugins = malloc(sizeof(struct plugin) * modcount);
    memset(plugins, 0, sizeof(struct plugin) * modcount);

    for ( size_t i = 0; i < modcount; ++i ) {
        size_t len = modpathlen + strlen(paths[i]) + 2;
        char * fullpath = malloc(len);
        snprintf(fullpath, len, "%s/%s", modpath, paths[i]);
        handles[i] = dlopen(fullpath, RTLD_LAZY);

        free(fullpath);
        free(paths[i]);
    }

    free(paths);

    fprintf(stderr, "Loaded %zu module(s)\n", modcount);

    for ( size_t i = 0; i < modcount; ++ i ) {
        plugins[i] = load_plugin(handles[i]);
    }

    qsort(plugins, modcount, sizeof (struct plugin), compare_plugins);

    for ( size_t i = 0; i < modcount; ++ i ) {
        if ( !plugins[i].priority ) { continue; }

        if ( plugins[i].setup ) {
            if ( !plugins[i].setup() ) { plugins[i].priority = 0; continue; }
        }
    }

    for (;; sleep(1)) {
        if ( !!caught_signum ) {
            goto teardown;
        }

        for ( size_t i = 0; i < modcount; ++ i ) {
            if ( !plugins[i].priority ) { continue; }

            if ( !plugins[i].play(&plugins[i].buffer) ) { continue; }
            printf("%s%s", plugins[i].buffer, i + 1 != modcount ? MODSEP : "");
        }

        putchar('\r');
        fflush(stdout);
    }

    teardown:
    for ( size_t i = 0; i < modcount; ++ i ) {
        if ( plugins[i].teardown ) { plugins[i].teardown(); }
        free(plugins[i].buffer);
    }

    cleanup:
        fputs("\x1b[?25h", stdout);
        if ( modpath ) { free(modpath); }

        if ( handles ) {
            for ( size_t i = 0; i < modcount; ++ i ) {
                if ( handles[i] ) { dlclose(handles[i]); }
            }
        }

        if ( plugins ) { free(plugins); }

        return status;
}

void
signal_handler (signed signum) {

    caught_signum = (sig_atomic_t )signum;
}

