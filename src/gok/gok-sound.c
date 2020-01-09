/*
 * gok-sound.c
 *
 * Copyright 2002-2009 Sun Microsystems, Inc.,
 * Copyright 2002-2009 University Of Toronto
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <canberra-gtk.h>

#include "gok-sound.h"
#include "gok-log.h"

#define SOUND_ID 0

/**
 * gok_sound_initialize:
 *
 * Initialises gok-sound.
 **/
void
gok_sound_initialize (void)
{
    ca_proplist *prop;

    gok_log_enter ();

    /* attach some common properties */
    ca_proplist_create (&prop);
    ca_proplist_sets (prop, CA_PROP_APPLICATION_NAME, _("GOK"));
    ca_proplist_sets (prop, CA_PROP_APPLICATION_VERSION, VERSION);
    ca_proplist_sets (prop, CA_PROP_APPLICATION_ICON_NAME, "gok");
    /*
     * i18n: "Key Feedback" is a sound event description. The string
     * doesn't appear in the user interface, it is part of the metadata
     * attached to sounds coming from GOK. Sounds are played whenever
     * a key on the virtual keyboard is pressed.
     */
    ca_proplist_sets (prop, CA_PROP_EVENT_DESCRIPTION, _("Key Feedback"));

    ca_proplist_sets (prop, CA_PROP_CANBERRA_CACHE_CONTROL, "permanent");

    ca_context_change_props_full (ca_gtk_context_get (), prop);
    ca_proplist_destroy (prop);

    gok_log ("Sound initialized.");

    gok_log_leave ();
}

/**
 * gok_sound_play:
 * @soundfile: The sound file to play.
 *
 * Plays @soundfile.
 *
 * Returns: 0 on success, negative value otherwise.
 **/
gint
gok_sound_play (const gchar *soundfile)
{
    gint res;

    g_return_val_if_fail (soundfile != NULL, -1);

    gok_log_enter ();

    res = ca_context_play (ca_gtk_context_get (),
			   SOUND_ID,
			   CA_PROP_MEDIA_FILENAME, soundfile,
			   NULL);

    if (res == CA_ERROR_DISABLED)
	gok_log ("System sounds are disabled.");
    else if (res == CA_SUCCESS)
	gok_log ("Playing soundfile %s.", soundfile);
    else
	gok_log ("Failed to play soundfile %s. Code %i: %s.",
		 soundfile, res, ca_strerror (res));

    gok_log_leave ();

    return res;
}
