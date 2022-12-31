#ifndef MAIN_H
#define MAIN_H

#pragma once

#include <signal.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <syslog.h>
#include <time.h>
#include <stdbool.h>
#include <getopt.h>
#include <limits.h>

#include "plug.h"

#define MODSEP " | "
#define PAINT_INTERVAL 1

#ifndef PREFIX
#define PREFIX "/usr/local"
#endif

static volatile sig_atomic_t caught_signum;

void
signal_handler (signed);

char *
module_path (const char *);

void
list_modules (const char *);

enum sink { X11_ROOT, STDOUT };

static struct option os [] = {
    { "help",  0, 0, 'h' },
    { "list",  0, 0, 'l' },
    { "sink",  1, 0, 's' },
    { "color", 1, 0, 'c' },
    { 0,       0, 0, 0   }
};

static const char usage_str [] =
    "Usage: %s [option ...]\n"
    "%s -- a dynamic, modular status program\n\n"
    "Options:\n"
    "  -c, --color=<when>   Colorize output; <when>\n"
    "                       can be \"auto\" (default),\n"
    "                       \"always\", or \"never\"\n"
    "  -h, --help           Print this help and exit\n"
    "  -l, --list           List available modules\n"
    "  -s, --sink=<sink>    Where to write output; <sink>\n"
    "                       can be \"x11-root\" (default)\n"
    "                       or \"stdout\"";

#endif
