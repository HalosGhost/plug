#include "module.h"

#include <X11/Xlib.h>
#include <X11/extensions/dpms.h>
#include <X11/extensions/scrnsaver.h>
#include <dirent.h>
#include <sys/types.h>
#include <errno.h>

#define DEFVALUE "⏾: 10000/10000/10000"

size_t size = sizeof DEFVALUE;
signed interval = 5;

size_t
play (char ** buf) {

    if ( !buf || !*buf ) { return 0; }

    Display * dpy = XOpenDisplay(NULL);
    XScreenSaverInfo * info = XScreenSaverAllocInfo();
    signed res = 0;

    if ( !dpy || !info ) {
        return snprintf(*buf, size, "DPMS: No Display");
    }

    if ( !DPMSCapable(dpy) ) {
        res = snprintf(*buf, size, "DPMS: N/A");
        goto cleanup;
    }

    CARD16 level = DPMSModeOn;
    BOOL stat = True;

    if ( !DPMSInfo(dpy, &level, &stat) || !stat ) {
        res = snprintf(*buf, size, "☕");
        goto cleanup;
    }

    CARD16 stnd, spnd, ff;
    if ( !DPMSGetTimeouts(dpy, &stnd, &spnd, &ff) ) {
        res = snprintf(*buf, size, "DPMS: No Timeouts");
        goto cleanup;
    }

    signed ev_base, err_base;
    if ( !XScreenSaverQueryExtension(dpy, &ev_base, &err_base) 
      || !XScreenSaverQueryInfo(dpy, DefaultRootWindow(dpy), info) ) {
        res = snprintf(*buf, size, "⏾: %u/%u/%u",
            stnd % 100000, spnd % 100000, ff % 100000
        );
        goto cleanup;
    }

    unsigned minutes = (stnd - (info->idle / 1000)) / 60;
    res = snprintf(*buf, size, "⏾: %u min", minutes);

cleanup:
    XFree(info);
    XCloseDisplay(dpy);
    return res > 0 ? (size_t )res : 0;
}

