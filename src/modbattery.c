#include "module.h"

#include <dirent.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <math.h>

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

    long power_now = 0,
         current_now = 0;
    unsigned long voltage_now = 0,
                  energy_full_design = 0,
                  energy_full = 0,
                  charge_full = 0,
                  charge_full_design = 0,
                  energy_now = 0,
                  charge_now = 0;

    char key [sizeof "CONSTANT_CHARGE_CURRENT_MAX"] = "", // largest defined key
         val [24] = "";

    char stat_char = 'U';

    #define key_eq(str) (!strncmp(key, (str), ((sizeof str) - 1)))

    while ( fscanf(in, "POWER_SUPPLY_%[^=]=%s\n", key, val) != EOF ) {
        if ( key_eq("STATUS") ) {
            sscanf(val, "%c", &stat_char);
        } else if ( key_eq("POWER_NOW") ) {
            sscanf(val, "%ld", &power_now);
        } else if ( key_eq("CURRENT_NOW") ) {
            sscanf(val, "%ld", &current_now);
        } else if ( key_eq("VOLTAGE_NOW") ) {
            sscanf(val, "%lu", &voltage_now);
        } else if ( key_eq("ENERGY_FULL_DESIGN") ) {
            sscanf(val, "%lu", &energy_full_design);
        } else if ( key_eq("ENERGY_FULL") ) {
            sscanf(val, "%lu", &energy_full);
        } else if ( key_eq("CHARGE_FULL_DESIGN") ) {
            sscanf(val, "%lu", &charge_full_design);
        } else if ( key_eq("CHARGE_FULL") ) {
            sscanf(val, "%lu", &charge_full);
        } else if ( key_eq("ENERGY_NOW") ) {
            sscanf(val, "%lu", &energy_now);
        } else if ( key_eq("CHARGE_NOW") ) {
            sscanf(val, "%lu", &charge_now);
        } else if ( key_eq("CAPACITY") ) {
            sscanf(val, "%hhu", &capacity);
        }
    }

    fclose(in);

    enum battery_status {
        CHARGING = 'C',
        DISCHARGING = 'D',
        EMPTY = 'E',
        FULL = 'F',
        UNKNOWN = 'U'
    };

    enum battery_status status = strchr("CDEFU", stat_char) ? (enum battery_status )stat_char : UNKNOWN;

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

    if ( samples == LONG_MAX || running > INT_MAX - rate || power_old < 0 != power < 0) {
        samples = 1;
        running = rate;
    } else {
        ++samples;
        running += rate;
    }

    unsigned long seconds = 3600 * target / (unsigned long )(running / samples);

    #define min(l, r) (((l) < (r)) ? (l) : (r))
    unsigned long hours = min(seconds / 3600, 999);
    unsigned long minutes = min((seconds - hours * 3600) / 60, 59);
    #undef min

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

