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

    fputs("\x1b[?25l", stdout);

    char * basepath = argc > 1 && argv[1] ? argv[1] : dirname(argv[0]);

    size_t modpathlen = strlen(basepath) + sizeof "/modules";
    char * modpath = malloc(modpathlen);

    snprintf(modpath, modpathlen, "%s/modules", basepath);

    static void ** handles = 0;
    static struct plugin * plugins = 0;
    static size_t modcount = 0;
    static char * out_buf = 0;
    static size_t out_len = 0;

    setup:
    signal(SIGHUP,  signal_handler);
    signal(SIGINT,  signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);

    caught_signum = 0;
    handles = 0;
    plugins = 0;
    modcount = load_plugins(modpath, &handles, &plugins);
    if ( !modcount ) {
        goto cleanup;
    }

    for ( size_t i = 0; i < modcount; ++ i ) {
        out_len += *(plugins[i].size);

        if ( i + 1 != modcount ) {
            out_len += sizeof MODSEP;
        }
    }

    out_buf = malloc(out_len + 2);
    memset(out_buf, 0, out_len);

    for ( size_t i = 0; i < modcount; ++ i ) {
        if ( !plugins[i].priority ) { continue; }

        if ( plugins[i].setup ) {
            if ( !plugins[i].setup() ) { plugins[i].priority = 0; continue; }
        }
    }

    static signed amount_written = 1;
    for (;; sleep(1)) {
        if ( !!caught_signum ) {
            fprintf(stderr, "\nCaught %s\n", strsignal(caught_signum));
            goto teardown;
        }

        fputs("\x1b[2K", stdout);
        *out_buf = ' ';
        amount_written = 1;

        for ( size_t i = 0; i < modcount; ++ i ) {
            if ( !plugins[i].priority ) { continue; }

            if ( !plugins[i].play(&plugins[i].buffer) ) { continue; }
            amount_written += snprintf(
                out_buf + amount_written,
                *(plugins[i].size),
                "%s",
                plugins[i].buffer
            );
            if ( i + 1 != modcount ) {
                amount_written += snprintf(
                    out_buf + amount_written,
                    sizeof MODSEP,
                    "%s",
                    MODSEP
                );
            }
        }

        fputs(out_buf, stdout);
        putchar('\r');
        fflush(stdout);
    }

    teardown:
    for ( size_t i = 0; i < modcount; ++ i ) {
        if ( plugins[i].teardown ) { plugins[i].teardown(); }
        free(plugins[i].buffer);
    }
    free(plugins);

    for ( size_t i = 0; i < modcount; ++ i ) {
        if ( handles[i] ) { dlclose(handles[i]); }
    }
    free(handles);

    if ( out_buf ) { free(out_buf); }

    if ( caught_signum == SIGHUP ) {
        fputs("Reloading\n", stderr);
        goto setup; // JUMPS BACKWARDS
    }

    cleanup:
        fputs("\x1b[?25h", stdout);
        if ( modpath ) { free(modpath); }
        return status;
}

void
signal_handler (signed signum) {

    caught_signum = (sig_atomic_t )signum;
}

