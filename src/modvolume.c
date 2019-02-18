#include "module.h"

#include <alsa/asoundlib.h>
#include <alsa/control.h>

#define DEFVALUE "A: 100%"
#define MODFORMAT "A: %hhd%c"

size_t size = sizeof DEFVALUE;
signed priority = 60;

#define audio_dev "default"

static snd_hctl_t * alsa_handle;
static snd_ctl_elem_id_t * alsa_sid;
static snd_ctl_elem_value_t * alsa_control;
static long volume;
static char mute;

signed
init (void) {

    snd_ctl_elem_id_malloc(&alsa_sid);
    if ( !alsa_sid ) {
        fprintf(stderr, "Could not allocate ALSA element\n");
        return 0;
    }

    snd_ctl_elem_value_malloc(&alsa_control);
    if ( !alsa_control ) {
        fprintf(stderr, "Could not allocate ALSA control\n");
        return 0;
    }

    snd_hctl_open(&alsa_handle, audio_dev, 0);
    snd_hctl_load(alsa_handle);

    snd_ctl_elem_id_set_interface(alsa_sid, SND_CTL_ELEM_IFACE_MIXER);

    return 1;
}

size_t
step (char ** buf) {

    if ( !buf || !*buf ) { return 0; }

    snd_ctl_elem_id_set_name(alsa_sid, "Master Playback Volume");
    snd_hctl_elem_t * alsa_element = snd_hctl_find_elem(alsa_handle, alsa_sid);

    snd_ctl_elem_value_set_id(alsa_control, alsa_sid);

    snd_hctl_elem_read(alsa_element, alsa_control);
    volume = snd_ctl_elem_value_get_integer(alsa_control, 0) * 100 / ((1 << 16) - 1);

    snd_ctl_elem_id_set_name(alsa_sid, "Master Playback Switch");
    alsa_element = snd_hctl_find_elem(alsa_handle, alsa_sid);

    snd_ctl_elem_value_set_id(alsa_control, alsa_sid);

    snd_hctl_elem_read(alsa_element, alsa_control);
    mute = snd_ctl_elem_value_get_boolean(alsa_control, 0) ? '%' : 'M';

    signed res = snprintf(*buf, size, MODFORMAT, (char )volume, mute);

    return res > 0 ? (size_t )res : 0;
}

