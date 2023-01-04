#include "module.h"

#include <stdbool.h>
#include <curl/curl.h>
#include <jansson.h>

#define DEFVALUE "AIR: Moderate Freezing Drizzle (+100°F) +100°F"

#include "pirate_weather.apikey"

#define PROVIDER "https://api.pirateweather.net/forecast"
#define AIRPORT "MSP"
#define LAT "44.881944"
#define LON "-93.221667"
#define EXCLUDE "minutely,hourly,daily"

#define RETRIES 3

#define min(n, m) (((n) < (m)) ? (n) : (m))

size_t size = sizeof DEFVALUE;
signed interval = 60 * 60 * 3; // 3 hours

static bool first_run = true;
static CURL * handle;

struct response {
    char * body;
    size_t size;
};

_Thread_local struct response resp;

static size_t
cb (void * data, size_t sz, size_t nmemb, void * userp) {

    size_t realsize = sz * nmemb;
    struct response * resp = userp;

    char * ptr = realloc(resp->body, resp->size + realsize + 1);
    if ( !ptr ) {
        return 0;  /* out of memory! */
    }

    resp->body = ptr;
    memcpy(&(resp->body[resp->size]), data, realsize);
    resp->size += realsize;
    resp->body[resp->size] = 0;

    return realsize;
}

size_t
parse (char * out, const char * json) {

    json_error_t err;
    json_t * root = json_loads(json, 0, &err);
    if ( !root ) { return 0; }

    json_t * current = json_object_get(root, "currently");
    if ( !json_is_object(current) ) {
        MODLOG(LOG_ERR, "error: %s", ".currently is not an object");
        json_decref(current);
        return 0;
    }

    json_t * s = json_object_get(current, "summary");
    json_t * t = json_object_get(current, "temperature");
    json_t * a = json_object_get(current, "apparentTemperature");

    size_t written = snprintf(
        out, sizeof DEFVALUE, "%s: %s %+.1f°F (%+.1f°F)", AIRPORT,
        json_string_value(s),
        json_real_value(t),
        json_real_value(a)
    );

    json_decref(current);

    return written;
}

signed
setup (void) {

    handle = curl_easy_init();

    char * url = PROVIDER "/" APIKEY "/" LAT "," LON "\?exclude=" EXCLUDE;

    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "plug/rolling");
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 1L);
    curl_easy_setopt(handle, CURLOPT_TCP_KEEPALIVE, 0L);
    curl_easy_setopt(handle, CURLOPT_HTTP_VERSION, (signed long )CURL_HTTP_VERSION_1_1);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, cb);

    return !!handle;
}

size_t
play (char ** buf) {

    if ( !buf || !*buf ) {
        return 0;
    }

    if ( first_run ) {
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void * )(&resp));
        first_run = false;
    }

    CURLcode ret = !CURLE_OK;
    for ( size_t i = 0, delay = 300; i < RETRIES && ret != CURLE_OK; ++i ) {
        ret = curl_easy_perform(handle);
        if ( ret != CURLE_OK ) {
            sleep(delay);
            delay *= 2;
        }

        size_t written = parse(*buf, resp.body);
        free(resp.body);
        resp.size = 0;
        return written;
    }

    if ( ret != CURLE_OK ) {
        MODLOG(LOG_ERR, "%s", curl_easy_strerror(ret));
        return 0;
    }

    // unreachable
    return sizeof DEFVALUE;
}

void
teardown (void) {

    curl_easy_cleanup(handle);
    handle = NULL;
}
