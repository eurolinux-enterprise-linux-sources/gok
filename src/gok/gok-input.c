/* gok-input.c
*
* Copyright 2002 Sun Microsystems, Inc.,
* Copyright 2002 University Of Toronto
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
#  include <config.h>
#endif

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gconf/gconf-client.h>
#include "gok-gconf-keys.h"
#include "gok-log.h"
#include "gok-input.h"
#include "gok-gconf.h"
#include "main.h"

static gchar *saved_input_device = NULL;

static int saved_corepointer_id = 0;

static GokInput *current_input = NULL;

static gboolean corepointer_detached = FALSE;

int gok_input_types[N_INPUT_TYPES];

#ifdef HAVE_XINPUT
static gint
gok_input_init_device_event_list (XDeviceInfo *info, XDevice *device, XEventClass *event_list)
{
	gint number = 0, i;
	for (i = 0; i < info->num_classes; i++) {
		switch (device->classes[i].input_class) 
		{
		case KeyClass:
			DeviceKeyPress(device, 
				       gok_input_types[GOK_INPUT_TYPE_KEY_PRESS], 
				       event_list[number]);
			gok_log ("event_list[%d]=%d", number, event_list[number]); 
			number++;
			DeviceKeyRelease(device, 
					 gok_input_types[GOK_INPUT_TYPE_KEY_RELEASE], 
					 event_list[number]); 
			gok_log ("event_list[%d]=%d", number, event_list[number]); 
			number++;
			break;      
		case ButtonClass:
			DeviceButtonPress(device, 
					  gok_input_types[GOK_INPUT_TYPE_BUTTON_PRESS], 
					  event_list[number]); 
			gok_log ("event_list[%d]=%d (button release)", number, event_list[number]); 
			number++;
			DeviceButtonRelease(device, 
					    gok_input_types[GOK_INPUT_TYPE_BUTTON_RELEASE], 
					    event_list[number]); 
			gok_log ("event_list[%d]=%d (button press)", number, event_list[number]); 
			number++;
			
			break;
		case ValuatorClass:
			DeviceMotionNotify(device, 
					   gok_input_types[GOK_INPUT_TYPE_MOTION], 
					   event_list[number]); 
			gok_log ("event_list[%d]=%d (motion)", number, event_list[number]); 
			number++;
		}
	}
	return number;
}
#endif

GdkFilterReturn
gok_input_extension_filter (GdkXEvent *gdk_xevent, GdkEvent *event, gpointer data)
{
	XEvent *xevent = gdk_xevent;
#ifdef HAVE_XINPUT

	if (xevent->type == gok_input_types[GOK_INPUT_TYPE_MOTION])
	{
		XDeviceMotionEvent *motion = (XDeviceMotionEvent *) xevent;
		if (!gok_input_get_current() ||
		    motion->deviceid != gok_input_get_current ()->device->device_id)
		{
		        gok_log ("motion event from unexpected device...\n");
			GokInput *input;
			if (gok_main_attach_new_devices ()) {
			  input = gok_input_find_by_device_id (motion->deviceid, TRUE);
			  if (input) gok_input_mismatch_warn (input->info);
			}
		}
		else {
		  gok_main_motion_listener (motion->axes_count, motion->axis_data, 
					    motion->device_state, (long) motion->time);
		}
	}
	else if ((xevent->type == gok_input_types[GOK_INPUT_TYPE_BUTTON_PRESS]) || 
		 (xevent->type == gok_input_types[GOK_INPUT_TYPE_BUTTON_RELEASE]))
	{
		XDeviceButtonEvent *button = (XDeviceButtonEvent *) xevent;
		if (gok_input_get_current() &&
		    button->deviceid != gok_input_get_current ()->device->device_id)
		{
			GokInput *input = gok_input_find_by_device_id (button->deviceid, TRUE);
			if (input) gok_input_mismatch_warn (input->info);
		}
		gok_main_button_listener (button->button, 
					  (xevent->type == gok_input_types[GOK_INPUT_TYPE_BUTTON_PRESS]) ? 1 : 0, 
					  button->state, 
					  button->time,
					  FALSE);
	}
	else 
	{
		return GDK_FILTER_CONTINUE;
	}

	return GDK_FILTER_REMOVE;
#else
	return GDK_FILTER_CONTINUE;
#endif
}

void
gok_input_detach_corepointer (void)
{
  int		num_devices, i;
  Display      *display;
  GtkWidget    *pMainWindow = gok_main_get_main_window ();
#ifdef HAVE_XINPUT
  XDeviceInfo *devices;
  if (!corepointer_detached && pMainWindow && pMainWindow->window) 
  {
	  XDeviceInfo *pointer = NULL, *ext = NULL;
	  display = GDK_WINDOW_XDISPLAY (pMainWindow->window); 
	  devices = XListInputDevices(display, &num_devices);
	  for (i = 0; i < num_devices; i++) 
	  {
		  if (!pointer && (devices[i].use == IsXPointer)) 
		  {
			  pointer = &devices[i];  
		  }
		  else if (!ext && (devices[i].use == IsXExtensionDevice))
		  {
			  XDevice *new_pointer = XOpenDevice (display, devices[i].id);
			  int j;
			  gok_log ("new pointer %s: %d classes", devices[i].name, devices[i].num_classes);
			  for (j = 0; j < devices[i].num_classes; j++) {
				  if ((new_pointer->classes[j].input_class == ButtonClass) || 
				      (new_pointer->classes[j].input_class == ValuatorClass))
				  {
					  ext = &devices[i];
					  break;
				  }
			  }
		  }
	  }
	  if (pointer && ext)
	  {
		  gint number;
		  XEventClass ev_list[40];
		  XDevice *newdev;
		  GdkWindow       *root;
	          saved_corepointer_id = pointer->id;
		  XChangePointerDevice (display, XOpenDevice (display, ext->id), 0, 1);
		  newdev = XOpenDevice (display, pointer->id);
		  number = gok_input_init_device_event_list (pointer, newdev, ev_list);
		  root = gdk_screen_get_root_window (gdk_drawable_get_screen (pMainWindow->window));
		  if (XSelectExtensionEvent(GDK_WINDOW_XDISPLAY (root), 
					    GDK_WINDOW_XWINDOW (root),
					    ev_list, number)) 
		  {
			  g_warning ("Can't connect to input device!");
			  XChangePointerDevice (display, newdev, 0, 1);
		  }
		  else	  
		  {
			  gok_log ("pointer swapped.");
			  corepointer_detached = TRUE;
		  }
		  XFreeDeviceList (devices);
	  }
	  else g_warning ("No extension devices or no corepointer device found.");
  }
  gok_main_center_corepointer (gok_main_get_main_window ()); /* get it out of the way */
#endif
  gok_main_close_warning ();
  g_message ("corepointer detached...");
  return;
}

void
gok_input_restore_corepointer (void)
{
  Display      *display;
  GtkWidget    *pMainWindow = gok_main_get_main_window ();
#ifdef HAVE_XINPUT
  if (corepointer_detached && pMainWindow && pMainWindow->window) 
  {
	  display = GDK_WINDOW_XDISPLAY (pMainWindow->window); 
	  XChangePointerDevice (display, XOpenDevice (display, saved_corepointer_id), 0, 1);
	  corepointer_detached = FALSE;
  }
#endif
  gok_main_close_warning ();
}

gboolean
gok_input_corepointer_detached (void)
{
    return corepointer_detached;
}

gboolean   
gok_input_ext_devices_exist (void)
{
	return (gok_input_get_device_list() != NULL);
}

static gboolean   
gok_input_open (GokInput *input)
{
  GtkWidget *window;

  g_return_val_if_fail (input != NULL, FALSE);
  input->open = FALSE;

#ifdef HAVE_XINPUT
  window = gok_main_get_main_window ();
  if (window && window->window && 
      input->info && (input->info->use == IsXExtensionDevice))
    {
      gdk_error_trap_push ();
      input->device = XOpenDevice(GDK_WINDOW_XDISPLAY (window->window), 
				  input->info->id);
      if (!gdk_error_trap_pop ())
	  input->open = TRUE;
    }
#endif
  if (input->name && !input->open)
    g_warning ("could not open device %s", input->name);

  return input->open;
}

static void
gok_input_close (GokInput *input)
{
  GtkWidget *window = gok_main_get_main_window ();

#ifdef HAVE_XINPUT  
  if (window && window->window && 
      input && input->info && (input->info->use == IsXExtensionDevice))
    {
      XCloseDevice(GDK_WINDOW_XDISPLAY (window->window), 
		   input->device);
      input->open = FALSE;
    }
#endif
}

GSList *
gok_input_get_device_list (void)
{
  static GSList *input_device_list = NULL;
  int		i;
  int		num_devices;
  Display      *display;
  GtkWidget    *pMainWindow = gok_main_get_main_window ();

#ifdef HAVE_XINPUT
  XDeviceInfo  *devices;

  if (!input_device_list && pMainWindow && pMainWindow->window) {
	  display = GDK_WINDOW_XDISPLAY (pMainWindow->window); 
 
	  devices = XListInputDevices(display, &num_devices);
	  for (i = 0; i < num_devices; i++) {
		  GokInput *pInput;
		  gok_log ("device %s, id=%d", devices[i].name, devices[i].id);
		  if (devices[i].use == IsXExtensionDevice) {
			  GSList *sl;
			  GokInput *input;
			  gboolean duplicate;

			  duplicate = FALSE;
			  for (sl = input_device_list; sl; sl = sl->next) {

			      input = (GokInput *)sl->data;
			      if (!strcmp (devices[i].name, input->name)) {
				duplicate = TRUE;
				break;
			      }
			  }
			  if (!duplicate) {
			      pInput = g_new0 (GokInput, 1);
			      pInput->name = g_strdup (devices[i].name);
			      pInput->info = &devices[i];
			      if (gok_input_open (pInput))
				input_device_list = g_slist_prepend (input_device_list, pInput);
			  }
		  }
	  }
  }
#endif
  return input_device_list;
}

#ifdef HAVE_XINPUT
static GokInput*
find_input (Display	*display,
	    char	*name,
	    gboolean     extended_only)
{
	GSList *device_list = gok_input_get_device_list ();
	
	while (device_list && device_list->data) {
	        GokInput *input = device_list->data;
	        XDeviceInfo *info = input->info;
		if (info && info->name && !strcmp(info->name, name)) {
		    return input;
		}
		device_list = device_list->next;
	}
	return NULL;
}

static GokInput*
find_input_by_id (Display     *display,
		  guint        id,
		  gboolean     extended_only)
{
	GSList *devices = gok_input_get_device_list ();
	
	while (devices && devices->data) {
	        GokInput *input = devices->data;
	        XDevice *device = input->device;
		if (input->open && device && (guint) device->device_id == id) {
		    return input;
		}
		devices = devices->next;
	}
	return NULL;
}

#endif

gboolean
gok_input_init (GdkFilterFunc filter_func)
{
#ifdef HAVE_XINPUT
	XEventClass      event_list[40];
	int              number;
	GtkWidget       *window = gok_main_get_main_window ();
	GdkWindow       *root;
	GSList          *device_list = gok_input_get_device_list ();
	
	while (device_list && device_list->data) {
	        GokInput *input = device_list->data;
		if (!input->open && !gok_input_open (input)) {
			g_warning ("Cannot open input device!\n");
		}
		else {
		  number = gok_input_init_device_event_list (input->info, input->device, event_list);
		  root = gdk_screen_get_root_window (gdk_drawable_get_screen (window->window));
	
		  if (XSelectExtensionEvent(GDK_WINDOW_XDISPLAY (root), 
					    GDK_WINDOW_XWINDOW (root),
					    event_list, number)) 
		  {
		    g_warning ("Can't connect to input device!");
		  }
		}	
		device_list = device_list->next;
	}
	gok_log ("%d event types available\n", number);
	
	gdk_window_add_filter (NULL,
			       filter_func,
			       NULL);
	return TRUE;
#else
	return FALSE;
#endif
}

gboolean
gok_input_set_extension_device_by_name (char *name)
{
        gok_log ("SETTING INPUT DEVICE TO %s\n", name);
	gok_gconf_set_string (gconf_client_get_default(), 
			      GOK_GCONF_INPUT_DEVICE,
			      name);
	current_input = gok_input_find_by_name (name, TRUE);
	return TRUE;
}

gchar *
gok_input_get_extension_device_name (void)
{
        gchar *cp;
        if (gok_gconf_get_string (gconf_client_get_default(), 
				  GOK_GCONF_INPUT_DEVICE,
				  &cp)) {
	        return cp;
	}
	else
	        return NULL;
}

void
gok_input_free (GokInput *pInput)
{
	if (pInput->open) 
	    gok_input_close (pInput);
	g_free (pInput->name);
	g_free (pInput);
}

GokInput * 
gok_input_find_by_device_id (guint id, gboolean extended_only)
{
  GtkWidget *window = gok_main_get_main_window ();
  GokInput *input = NULL;
#ifdef HAVE_XINPUT
  
  if (window && window->window) 
    {
      input = find_input_by_id (GDK_WINDOW_XDISPLAY (window->window), id, extended_only);
    }
#else
  g_message ("Warning, XInput extension not present, cannot open input device.");
#endif
  return input;
}

GokInput * 
gok_input_find_by_name (char *name, gboolean extended_only)
{
  GtkWidget *window = gok_main_get_main_window ();
  GokInput *input = NULL;
#ifdef HAVE_XINPUT
  
  if (name && window && window->window) 
    {
      input = find_input (GDK_WINDOW_XDISPLAY (window->window), name, extended_only);
    }
#else
  g_message ("Warning, XInput extension not present, cannot open input device.");
#endif
  return input;
}

void
gok_input_close_others (void)
{
	GSList *devices = gok_input_get_device_list ();
	GokInput *input = gok_input_get_current ();
	if (input && input->info) 
	    gok_log ("input [%s], closing others", input->info->name);
	while (devices)
	{
		GokInput *tmp = devices->data;
		if (input && tmp && input->device && tmp->device && 
		    input->open && input->device->device_id != tmp->device->device_id) 
			gok_input_close (tmp);
		devices = devices->next;
	}
	gok_main_close_warning ();
}

GokInput * gok_input_get_current (void)
{
  char *input_device_name;

  if (!current_input)
  {
      /* was the input device specified on the command line? */
      input_device_name = gok_main_get_inputdevice_name ();
      if ((input_device_name == NULL) &&  
	  (gok_gconf_get_string (gconf_client_get_default (),
				 GOK_GCONF_INPUT_DEVICE,  &input_device_name))) {
	  /* use gconf setting */
	  current_input = gok_input_find_by_name (input_device_name, TRUE);
	  g_free (input_device_name);
      }
      else
      {
	  current_input = gok_input_find_by_name (input_device_name, TRUE);
      }
  }
  gok_log ("current input device=%x\n", current_input);
  return current_input;
}

void
gok_input_restore (void)
{
        gok_log (stderr, "gok input restore\n");
	gok_input_set_extension_device_by_name (saved_input_device);
	gok_input_close_others (); 
}

void
gok_input_mismatch_warn (XDeviceInfo *device)
{
	gchar *message = g_strdup_printf (_("GOK has detected activity from a new hardware device named \'%s\'.  "
					    "Would you like to use this device instead of device \'%s\'?"),
					  device->name, gok_input_get_extension_device_name ());
	fprintf (stderr, "%s", message);
	saved_input_device = gok_input_get_extension_device_name ();
	gok_input_set_extension_device_by_name (device->name);
	gok_main_warn (message, TRUE, gok_input_restore, gok_input_close_others, FALSE);
	g_free (message);
}

