/* gok-input.h
*
* Copyright 2002-2005 Sun Microsystems, Inc.,
* Copyright 2002-2005 University Of Toronto
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

#ifndef __GOKINPUT_H__
#define __GOKINPUT_H__

#ifdef HAVE_XINPUT
#include <X11/Xlib.h>
#include <X11/extensions/XInput.h>
#endif

#define N_INPUT_TYPES 5

typedef enum {
  GOK_INPUT_TYPE_MOTION = 0,
  GOK_INPUT_TYPE_BUTTON_PRESS   = 1,
  GOK_INPUT_TYPE_BUTTON_RELEASE = 2,
  GOK_INPUT_TYPE_KEY_PRESS      = 3,
  GOK_INPUT_TYPE_KEY_RELEASE    = 4
} GokInputType;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _GokInput GokInput;
struct _GokInput {
        char        *name;
#ifdef HAVE_XINPUT
        XDeviceInfo *info; 
        XDevice     *device;
        gboolean    open;
#endif
};

extern int gok_input_types[N_INPUT_TYPES];

GokInput * gok_input_find_by_name (char *name, gboolean extended_only); 
GokInput * gok_input_find_by_device_id (guint device_id, gboolean extended_only); 
gboolean   gok_input_init (GdkFilterFunc filter_func); 
void       gok_input_open_all (void);
void       gok_input_close_others (void);
GokInput * gok_input_get_current (void);
void       gok_input_set_current (GokInput *input);
GSList *   gok_input_get_device_list (void);
gboolean   gok_input_set_extension_device_by_name (char *name);
gboolean   gok_input_corepointer_detached (void);
gchar *    gok_input_get_extension_device_name (void);
void       gok_input_free (GokInput *input);
GdkFilterReturn gok_input_extension_filter (GdkXEvent *gdk_xevent, GdkEvent *event, gpointer data);
void       gok_input_restore_corepointer (void);
void       gok_input_detach_corepointer (void);
gboolean   gok_input_ext_devices_exist (void);
#ifdef HAVE_XINPUT
void       gok_input_mismatch_warn (XDeviceInfo *devinfo);
#endif
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef __GOKINPUT_H__ */
