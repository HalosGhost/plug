#include "module.h"

#include <stdbool.h>
#include <curl/curl.h>

#define DEFVALUE "AIR: Partly cloudy ⛅️ (+100°F) +100°F"

#define DOMAIN "https://wttr.in"
#define AIRPORT "MSP"
#define UNIT "u" /* "m" for metric */
#define OUTFORMAT "%C+%c(%t)+%f"

#define min(n, m) (((n) < (m)) ? (n) : (m))

size_t size = sizeof DEFVALUE;
signed interval = 60 * 60 * 3; // 3 hours

static bool first_run = true;
static CURL * handle;

static size_t
cb (void * buf, size_t sz, size_t nmemb, void * userp) {

    memcpy(userp, buf, sizeof DEFVALUE);

    return min(sz * nmemb, sizeof DEFVALUE);
}

signed
setup (void) {

    handle = curl_easy_init();

    char * url = DOMAIN "/" AIRPORT "\?" UNIT "&format=" OUTFORMAT;

    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "plug/rolling");
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 1L);
    curl_easy_setopt(handle, CURLOPT_HTTP_VERSION, (signed long )CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(handle, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, cb);

    return !!handle;
}

size_t
play (char ** buf) {

    if ( !buf || !*buf ) {
        return 0;
    }

    if ( first_run ) {
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void * )(*buf));
        first_run = false;
    }

    CURLcode ret = curl_easy_perform(handle);
    if ( ret == CURLE_OK ) {
        return sizeof DEFVALUE;
    }

    return 0;
}

void
teardown (void) {

    curl_easy_cleanup(handle);
    handle = NULL;
}
