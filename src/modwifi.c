#include "module.h"

#include <linux/if.h>
#include <linux/wireless.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stropts.h>

#define DEFVALUE "Long Wireless Network Name: ▂▃▄▅▆▇█"
#define MODFORMAT "%s: %s"

size_t size = sizeof DEFVALUE;
signed priority = 50;

#define wl_iface "wlp3s0"
#define wl_path "/proc/net/wireless"

#define FAIL_OPEN(x) "Failed to open " x
#define FAIL_READ(x) "Failed to read from " x
#define FAIL_IOCTL(x) "Failed to execute " x " ioctl"

static char ssid [IW_ESSID_MAX_SIZE + 1] = "W";
static unsigned char strength = 0;

static const char signal_bars [][22] = {
    "No Signal", "▂", "▂▃", "▂▃▄", "▂▃▄▅", "▂▃▄▅▆", "▂▃▄▅▆▇", "▂▃▄▅▆▇█"
};

signed
init (void) {

    return 1;
}

size_t
step (char ** buf) {

    if ( !buf || !*buf ) { return 0; }

    memset(ssid, 0, IW_ESSID_MAX_SIZE);
    *ssid = 'W';

    signed sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct iwreq w;
    struct iw_range range;
    FILE * in = 0;

    strncpy(w.ifr_ifrn.ifrn_name, wl_iface, IFNAMSIZ);

    w.u.essid.pointer = ssid;
    w.u.data.length = IW_ESSID_MAX_SIZE;
    w.u.data.flags = 0;

    signed errsv = 0;
    errno = 0;
    if ( ioctl(sock, SIOCGIWESSID, &w) == -1 ) {
        errsv = errno;
        fprintf(stderr, FAIL_IOCTL("SIOCGIWESSID") ": %s\n", strerror(errsv));
        goto cleanup;
    }

    w.u.data.pointer = &range;
    w.u.data.length = sizeof (struct iw_range);
    w.u.data.flags = 0;

    errno = 0;
    if ( ioctl(sock, SIOCGIWRANGE, &w) == -1 ) {
        errsv = errno;
        fprintf(stderr, FAIL_IOCTL("SIOCGIWRANGE") ": %s\n", strerror(errsv));
        goto cleanup;
    }

    errno = 0;
    in = fopen(wl_path, "r");
    if ( !in ) {
        errsv = errno;
        fprintf(stderr, FAIL_OPEN(wl_path) ": %s\n", strerror(errsv));
        strength = 0;
        goto cleanup;
    }

    errno = 0;
    unsigned char n = 0;
    if ( fscanf(in, "%*[^\n]\n%*[^\n]\n" wl_iface ": %*d %hhu.", &n) != 1 ) {
        errsv = errno;
        fprintf(stderr, FAIL_READ(wl_path) ": %s\n", strerror(errsv));
        strength = 0;
        goto cleanup;
    }

    strength = n * 7 / range.max_qual.qual;

    cleanup:
        if ( sock ) { close(sock); }
        if ( in ) { fclose(in); }
        if ( errsv ) { return 0; }

        signed res = snprintf(*buf, size, MODFORMAT, ssid, signal_bars[strength]);
        return res > 0 ? (size_t )res : 0;
}

