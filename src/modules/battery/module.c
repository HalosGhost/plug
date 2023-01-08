#include "module.h"

#include <dirent.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <math.h>

// note the extra spaces (for colorization)
#define DEFVALUE " B: 100% (+100W) 99:59 till replenished "
#define MODFORMAT "B: %hhu%% (%+.2lgW)%s"

#define bat_device "cw2015-battery"
#define bat_path "/sys/class/power_supply/" bat_device

#define FAIL_OPEN(x) "Failed to open " x

size_t size = sizeof DEFVALUE;
signed interval = 30;

static double power;
static char time_estimate [25] = "";

static long running;
static long samples;

#define FOR_EACH_UEVENT_PROPERTY \
    X(char, "STATUS", "%c", stat_char) \
    X(long, "POWER_NOW", "%ld", power_now) \
    X(long, "CURRENT_NOW", "%ld", current_now) \
    X(unsigned long, "VOLTAGE_NOW", "%lu", voltage_now) \
    X(unsigned long, "CHARGE_NOW", "%lu", charge_now) \
    X(unsigned long, "CHARGE_FULL", "%lu", charge_full) \
    X(unsigned long, "CHARGE_FULL_DESIGN", "%lu", charge_full_design) \
    X(unsigned long, "ENERGY_NOW", "%lu", energy_now) \
    X(unsigned long, "ENERGY_FULL", "%lu", energy_full) \
    X(unsigned long, "ENERGY_FULL_DESIGN", "%lu", energy_full_design) \
    X(unsigned long, "TIME_TO_EMPTY_NOW", "%lu", time_to_empty) \
    X(unsigned long, "TIME_TO_FULL_NOW", "%lu", time_to_full) \
    X(unsigned char, "CAPACITY", "%hhu", capacity)

size_t
play (char ** buf) {

    if ( !buf || !*buf ) { return 0; }

    signed errsv = 0;
    errno = 0;
    FILE * in = fopen(bat_path "/uevent", "r");
    if ( !in ) {
        errsv = errno;
        MODLOG(LOG_ERR, FAIL_OPEN(bat_path) "/uevent: %s\n", strerror(errsv));
        return 0;
    }

    #define X(t, k, f, v) t v = 0;
    FOR_EACH_UEVENT_PROPERTY
    #undef X

    char key [sizeof "CONSTANT_CHARGE_CURRENT_MAX"] = "", // largest defined key
         val [24] = "";

    #define X(t, k, f, v) \
        if ( !strncmp(key, (k), ((sizeof (k) - 1))) ) { \
            sscanf(val, f, &v); \
        } else

    while ( fscanf(in, "POWER_SUPPLY_%[^=]=%s\n", key, val) != EOF ) {
        FOR_EACH_UEVENT_PROPERTY {
            continue;
        }
    }
    #undef X

    fclose(in);

    enum battery_status {
        CHARGING = 'C',
        DISCHARGING = 'D',
        EMPTY = 'E',
        FULL = 'F',
        UNKNOWN = 'U'
    };

    enum battery_status status = strchr("CDEFU", stat_char) ? (enum battery_status )stat_char : UNKNOWN;

    #define max(l, r) (((l) > (r)) ? (l) : (r))
    voltage_now = max(voltage_now / 1000, 1);
    #undef max

    power_now   /= 1000;
    current_now /= 1000;

    energy_now         /= voltage_now;
    energy_full        /= voltage_now;
    energy_full_design /= voltage_now;
    charge_now         /= 1000;
    charge_full        /= 1000;
    charge_full_design /= 1000;

    long rate = power_now ? power_now : current_now;
    if ( !rate ) {
        rate = 1;
    } else if ( rate == LONG_MIN ) {
        rate = LONG_MAX;
    } else if ( rate < 0 ) {
        rate = -rate;
    }

    for ( ; rate >= 10000; rate /= 10 );
    if ( status == CHARGING ) {
        rate *= 1000;
    }

    unsigned long max_capacity = charge_full        ? charge_full        :
                                 energy_full        ? energy_full        :
                                 charge_full_design ? charge_full_design :
                                 energy_full_design ? energy_full_design : 0;
    unsigned long cur_capacity = charge_now ? charge_now : energy_now;

    if ( !cur_capacity ) {
        cur_capacity = max_capacity * capacity / 100;
    }

    unsigned long target = cur_capacity;
    double power_old = power;

    power = round(((signed long )-voltage_now * rate / 100000.)) / 10;
    if ( status == CHARGING ) {
        target = max_capacity - cur_capacity;
        power *= -1;
    }

    if ( samples == LONG_MAX || running > INT_MAX - rate || ((power_old < 0) != (power < 0)) ) {
        samples = 1;
        running = rate;
    } else {
        ++samples;
        running += rate;
    }

    unsigned long seconds = 3600 * target / (unsigned long )(running / samples);
    if ( !seconds && (time_to_empty || time_to_full) ) {
        seconds = status == CHARGING ? time_to_full : time_to_empty;
    }

    #define min(l, r) (((l) < (r)) ? (l) : (r))
    unsigned long hours = min(seconds / 3600, 99);
    unsigned long minutes = min((seconds - hours * 3600) / 60, 59);
    #undef min

    const char * when = status == CHARGING ? "replenished" : "depleted";
    signed res = 0;
    if ( (hours || minutes) && (status == CHARGING || status == DISCHARGING) ) {
        res = snprintf(time_estimate, 25, " %.2lu:%.2lu till %s", hours, minutes, when);
    } else {
        res = snprintf(time_estimate, 2, "%s", "");
    }

    res = 0;
    if ( capacity <= 15 ) {
        res = snprintf(*buf, 2, "%1c", 3 + !(capacity > 5));
    }
    res += snprintf(*buf + res, size, MODFORMAT, capacity, power, time_estimate);
    if ( capacity <= 15 ) {
        res += snprintf(*buf + res, 2, "%1c", 1);
    }

    return res > 0 ? (size_t )res : 0;
}

