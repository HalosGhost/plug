#include "module.h"

#include <dirent.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>

#define DEFVALUE "B: 100% (+100W) 999:59 till replenished"
#define MODFORMAT "B: %hhu%% (%+.2lgW)%s"

#define bat_device "BAT0"
#define bat_path "/sys/class/power_supply/" bat_device

#define FAIL_OPEN(x) "Failed to open " x

size_t size = sizeof DEFVALUE;
signed priority = 60;

static unsigned char capacity;
static double power;
static char time_estimate [25] = "";

static long running;
static long samples;

size_t
play (char ** buf) {

    if ( !buf || !*buf ) { return 0; }

    signed errsv = 0;
    errno = 0;
    FILE * in = fopen(bat_path "/uevent", "r");
    if ( !in ) {
        errsv = errno;
        fprintf(stderr, FAIL_OPEN(bat_path) "/uevent: %s\n", strerror(errsv));
        return EXIT_FAILURE;
    }

    enum { DISCHARGING, EMPTY, CHARGING, FULL } status = EMPTY;
    long power_now = 0,
         current_now = 0;
    unsigned long voltage_now = 0,
                  energy_full_design = 0,
                  energy_full = 0,
                  charge_full = 0,
                  charge_full_design = 0,
                  energy_now = 0,
                  charge_now = 0;

    char keybuf [64] = "", // pretty sure it should never be larger than 32
         val    [64] = "";

    while ( fscanf(in, "%[^=]=%s\n", keybuf, val) != EOF ) {
        char * key = keybuf + sizeof("POWER_SUPPLY_") - 1; // ignore nul byte

        if ( !strncmp(key, "STATUS", 6) ) {
            switch ( val[0] ) {
                case 'D': status = DISCHARGING; break;
                case 'C': status = CHARGING; break;
                case 'F': status = FULL; break;
            }
        } else if ( !strncmp(key, "POWER_NOW", 9) ) {
            sscanf(val, "%ld", &power_now);
        } else if ( !strncmp(key, "CURRENT_NOW", 11) ) {
            sscanf(val, "%ld", &current_now);
        } else if ( !strncmp(key, "VOLTAGE_NOW", 11) ) {
            sscanf(val, "%lu", &voltage_now);
        } else if ( !strncmp(key, "ENERGY_FULL_DESIGN", 18) ) {
            sscanf(val, "%lu", &energy_full_design);
        } else if ( !strncmp(key, "ENERGY_FULL", 11) ) {
            sscanf(val, "%lu", &energy_full);
        } else if ( !strncmp(key, "CHARGE_FULL_DESIGN", 18) ) {
            sscanf(val, "%lu", &charge_full_design);
        } else if ( !strncmp(key, "CHARGE_FULL", 11) ) {
            sscanf(val, "%lu", &charge_full);
        } else if ( !strncmp(key, "ENERGY_NOW", 10) ) {
            sscanf(val, "%lu", &energy_now);
        } else if ( !strncmp(key, "CHARGE_NOW", 10) ) {
            sscanf(val, "%lu", &charge_now);
        } else if ( !strncmp(key, "CAPACITY", 8) ) {
            sscanf(val, "%hhu", &capacity);
        }
    }

    fclose(in);

    voltage_now /= 1000;
    voltage_now = voltage_now ? voltage_now : 1;

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

    unsigned long max_capacity = charge_full        ? charge_full        :
                                 energy_full        ? energy_full        :
                                 charge_full_design ? charge_full_design :
                                 energy_full_design ? energy_full_design : 0;
    unsigned long cur_capacity = charge_now ? charge_now : energy_now;

    unsigned long target = cur_capacity;
    double power_old = power;

    power = (signed long )-voltage_now * rate / 1000000.;
    if ( status == CHARGING ) {
        target = max_capacity - cur_capacity;
        power *= -1;
    }

    if ( samples == LONG_MAX || running > INT_MAX - rate || power_old < 0 != power < 0) {
        samples = 1;
        running = rate;
    } else {
        ++samples;
        running += rate;
    }

    unsigned long seconds = 3600 * target / (unsigned long )(running / samples);
    unsigned long hours = (seconds / 3600 > 999 ? 999 : seconds / 3600) % 99;
    unsigned long minutes = ((seconds - hours * 3600) / 60) % 60;

    const char * when = status == CHARGING ? "replenished" : "depleted";

    signed res = 0;
    if ( hours || minutes || seconds ) {
        res = snprintf(time_estimate, 25, " %.2lu:%.2lu till %s", hours, minutes, when);
    } else {
        res = snprintf(time_estimate, 2, "");
    }

    res = snprintf(*buf, size, MODFORMAT, capacity, power, time_estimate);

    return res > 0 ? (size_t )res : 0;
}

