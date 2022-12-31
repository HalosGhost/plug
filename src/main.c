#include "main.h"

signed
main (signed argc, char * argv []) {

    if ( !argc ) { return EXIT_FAILURE; }

    signed status = EXIT_SUCCESS;

    bool color_flag = false;
    char color_arg [7] = "auto";
    enum sink sink_switch = X11_ROOT;

    char * prognm = basename(argv[0]);
    char * modpath = 0;
    Display * dpy = 0;

    const char * vos = "hls:c:";
    for ( signed oi = 0, c = getopt_long(argc, argv, vos, os, &oi);
          c != -1; c = getopt_long(argc, argv, vos, os, &oi) ) {
        switch ( c ) {
            case 'c': snprintf(color_arg, 7, "%s", optarg); break;
            case 'h': puts(usage_str); goto cleanup;
            case 'l': list_modules(prognm); goto cleanup;
            case 's': sink_switch = *optarg != 'x'; break;
        }
    }

    color_flag = !strncmp(color_arg, "auto", 4) ? sink_switch != STDOUT :
                 !strncmp(color_arg, "always", 7);

    (void )color_flag; // todo: expose this to modules
    modpath = module_path(prognm);
    if ( !modpath ) {
        fputs("No suitable module path found\n", stderr);
        return EXIT_FAILURE;
    }

    openlog(NULL, LOG_CONS, LOG_USER);
    if ( sink_switch == X11_ROOT ) {
        dpy = XOpenDisplay(NULL);
        if ( !dpy ) {
            syslog(LOG_ERR, "Could not open display\n");
            return EXIT_FAILURE;
        }
    } else {
        fputs("\x1b[?25l", stdout);
    }

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
        if ( !plugins[i].interval ) { continue; }

        if ( plugins[i].setup ) {
            if ( !plugins[i].setup() ) { plugins[i].interval = 0; continue; }
        }
    }

    static signed amount_written = 1;
    for ( time_t c_time = 0; ; c_time = time(NULL) ) {
        if ( !!caught_signum ) {
            syslog(LOG_INFO, "Caught %s\n", strsignal(caught_signum));
            goto teardown;
        }

        *out_buf = ' ';
        amount_written = 1;

        for ( size_t i = 0; i < modcount; ++ i ) {
            if ( !plugins[i].interval ) { continue; }

            if ( !(c_time % *(plugins[i].interval)) ) {
                 if ( !plugins[i].play(&plugins[i].buffer) ) {
                    plugins[i].interval = 0; // disable module if play() fails
                    continue;
                }
            }

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

        if ( dpy ) {
            XStoreName(dpy, DefaultRootWindow(dpy), out_buf);
            XSync(dpy, False);
        } else {
            fputs(out_buf, stdout);
            putchar('\r');
            fflush(stdout);
        }

        sleep(PAINT_INTERVAL);
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
        if ( dpy ) {
            XCloseDisplay(dpy);
        } else {
            printf("\x1b[?25h");
        }
        if ( modpath ) { free(modpath); }
        return status;
}

void
signal_handler (signed signum) {

    caught_signum = (sig_atomic_t )signum;
}

char *
module_path (const char * name) {

    if ( !name ) {
        return NULL;
    }

    char modpath [PATH_MAX + 1] = "";

    char * xdg = getenv("XDG_CONFIG_HOME");
    if ( xdg ) {
        snprintf(modpath, PATH_MAX - 1, "%s/%s/modules.d", xdg, name);
        DIR * d = opendir(modpath);
        if ( d ) {
            closedir(d);
            return strdup(modpath);
        }
    }

    char * home = getenv("HOME");
    if ( home ) {
        snprintf(modpath, PATH_MAX - 1, "%s/.%s/modules.d", home, name);
        DIR * d = opendir(modpath);
        if ( d ) {
            closedir(d);
            return strdup(modpath);
        }
    }

    snprintf(modpath, PATH_MAX - 1, "/etc/%s/modules.d", name);
    DIR * d = opendir(modpath);
    if ( d ) {
        closedir(d);
        return strdup(modpath);
    }

    return NULL;
}

void
list_modules (const char * name) {

    char path [PATH_MAX + 1] = "";
    snprintf(path, PATH_MAX - 1, "%s/lib/%s/modules", PREFIX, name);
    char ** plugins = discover_plugins(path);
    if ( !plugins ) {
        fputs("No plugins found.\n", stderr);
        return;
    }

    printf("Known modules:\n");
    for ( char ** p = plugins; *p; ++p ) {
        printf("\t%s/%s\n", path, *p);
        free(*p);
    }

    free(plugins);
}
