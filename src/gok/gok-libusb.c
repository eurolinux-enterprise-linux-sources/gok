/* gok-libusb.c
*
* Copyright 2006 University Of Toronto
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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <usb.h>
#include <string.h>
#include "gok-libusb.h"
#include "main.h"

#define USB_SUBCLASS_MOUSE 0x1
#define USB_PROTOCOL_MOUSE 0x2 
#define USB_ENDPOINT_PACKET_SIZE_MASK 0x7ff /* only the first 11 bits specify data size*/

struct input_device {
	usb_dev_handle *handle;
	guint8 ep_address;
	guint16 ep_data_size;
	guint ep_polling_interval;
};

struct _gok_libusb_t
{
	GMutex *destroy_input_thread_mutex;
	gboolean destroy_input_thread; 			
	GMutex *idle_input_data_queue_mutex;
	GQueue *idle_input_data_queue;
	struct input_device *input_dev;
	GThread *libusb_thread;
	
};

struct idle_input_data
{
	GMutex *idle_input_data_queue_mutex;
	GQueue *idle_input_data_queue;
	char* event_buffer;
	gint buffer_size;
};

static gint twopow(gint exp)
{
	gint retval = 1;
	while(--exp)
	{
		retval *= 2;
	}
	return  retval;
}

static gint convert_from_twos_comp(char number) 
{
	return (number & 0x80) ? -(~number + 1) : number;
}

static gboolean gok_libusb_idle_input_handler (gpointer data)
{
	static char saved_state = 0;
	gint i, x=0, y=0;
	char current_state;
	struct idle_input_data *handle = (struct idle_input_data*) data;
	char event[handle->buffer_size]; 

	memcpy(event, handle->event_buffer, handle->buffer_size);
	g_free(handle->event_buffer);
	/* remove the data from the queue */
	g_mutex_lock(handle->idle_input_data_queue_mutex);
	g_queue_remove(handle->idle_input_data_queue, handle);
	g_mutex_unlock(handle->idle_input_data_queue_mutex);

	gok_scanner_input_motion_libusb(convert_from_twos_comp(event[1]), convert_from_twos_comp(event[2]));
	
	for (i = 0; i < 8; i++) {
	    current_state = (event[0]>>i)&0x1;
	    /* if button changed state */
	    if (((saved_state>>i)&0x1) != current_state) {
		gok_main_button_listener(i+1, current_state, 0, 0, TRUE); /* 3rd and 4th parameters not used */
	    	(current_state == 0x1) ? (saved_state |= (0x1<<i)) : (saved_state &= ~(0x1<<i));
	    }
	}
	
	g_free(handle);
	return FALSE;
}

static gpointer usb_event_handler(gpointer data) 
{
	struct idle_input_data *idle_data;
	float seconds;
	gboolean destroy_thread = FALSE;
	gboolean alloc_new_idle_data = TRUE;
	gok_libusb_t *handle = (gok_libusb_t*) data;
	GTimer *timer =  g_timer_new();
	
	while (destroy_thread == FALSE)
	{
		if (alloc_new_idle_data == TRUE) {
			idle_data = g_new0(struct idle_input_data, 1);
			idle_data->event_buffer = g_malloc0(handle->input_dev->ep_data_size);
			idle_data->buffer_size = handle->input_dev->ep_data_size; 
			idle_data->idle_input_data_queue = handle->idle_input_data_queue;
	    		idle_data->idle_input_data_queue_mutex = handle->idle_input_data_queue_mutex;
		}
		g_timer_stop(timer);
		/* we can't be certain if the handler code took more than the polling interval because we 
		 * can't account for the code that checks this condition. as a heuristic, check to see if
		 * the elapsed time is less than half of the polling interval time */
		if ((seconds = g_timer_elapsed(timer, NULL)) > (handle->input_dev->ep_polling_interval*0.001)/2) 
		{
			g_warning("The libusb event handler may have taken more time than the polling interval for the device. Some events from the device may have been lost.");
		}
		
		/* we're using a 500ms timeout so that there won't be a deadlock when this thread is destroyed */
		if (usb_interrupt_read(handle->input_dev->handle, handle->input_dev->ep_address, idle_data->event_buffer, idle_data->buffer_size, 500) < 0) 
		{
			/* didn't read anything into the buffer */
			g_timer_start(timer);
			alloc_new_idle_data = FALSE;
		} 
		else 
		{
			/* this chunk of code should take less then the usb interrupt polling interval or events may be lost */
			g_timer_start(timer);
			g_idle_add_full (G_PRIORITY_HIGH_IDLE, gok_libusb_idle_input_handler, idle_data, NULL);
			alloc_new_idle_data = TRUE;
			
			/* remove the data from the queue */
			g_mutex_lock(handle->idle_input_data_queue_mutex);
			g_queue_push_head(handle->idle_input_data_queue, idle_data);
			g_mutex_unlock(handle->idle_input_data_queue_mutex);

		}
		g_mutex_lock(handle->destroy_input_thread_mutex);
		destroy_thread = handle->destroy_input_thread;
		g_mutex_unlock(handle->destroy_input_thread_mutex);
	}
	g_timer_destroy(timer);
	return NULL;
}

static void gok_libusb_display_error(gchar *error)
{
	gchar *libusb_error = g_malloc0(256);
	g_sprintf(libusb_error, _("can't initialize the libusb backend - %s"), error);
	gok_main_display_fatal_error(libusb_error);
	g_free(libusb_error);
}

/* finds an endpoint from the device specified by the vid and pid. 
 * the input_device struct that is returned must be freed
 * TODO test with USB 2.0 device and with devices that has multiple endpoints 
 */
static struct input_device* find_input_endpoint(gint vid, gint pid)
{
	int i, j, k;
	struct usb_bus *bus;
	struct usb_device *dev;
	struct usb_dev_handle *handle;
	struct usb_device_descriptor device;
	gchar *error;
	
	usb_init();
	usb_find_busses();
	usb_find_devices();
	
	for (bus = usb_busses; bus; bus = bus->next) 
	{
		for (dev = bus->devices; dev; dev = dev->next) 
    		{
   			if (dev->descriptor.idVendor == vid && dev->descriptor.idProduct == pid) 
   			{
		  		g_message(_("gok-libusb.c: found device with VID:PID pair %x:%x"), vid, pid);
				if ((handle = usb_open(dev)) == NULL)
				{
					gok_libusb_display_error(usb_strerror());
					return NULL;
				}
				goto found_device;
			}
		}
	}
	error = g_malloc0(128);
	g_sprintf(error, _("could not find device with VID:PID pair %x:%x."), vid, pid);
	gok_libusb_display_error(error);
	g_free(error);
	return NULL;

found_device:

	if ((usb_get_descriptor(handle, USB_DT_DEVICE, 0, &device, USB_DT_DEVICE_SIZE)) < 0)
	{	
		g_debug("gok-libusb.c: %s", usb_strerror()); 
		error = g_malloc0(128);
		/* FIXME: print the full path for dev->filename */
		g_sprintf(error, _("there are incorrect permissions on %s"), dev->filename); /* TODO need to send full path */
		gok_libusb_display_error(error);
		g_free(error);
		return NULL;
	}
	for (i = 0; i < device.bNumConfigurations; i++)
	{
		struct usb_config_descriptor config;
		if ((usb_get_descriptor(handle, USB_DT_CONFIG, i, &config, USB_DT_CONFIG_SIZE)) < 0)
		{
			gok_libusb_display_error(usb_strerror());
			return NULL;
		}
		
		for (j = 0; j < config.bNumInterfaces; j++) 
		{
			struct usb_interface_descriptor interface;
			if ((usb_get_descriptor(handle, USB_DT_INTERFACE, j, &interface, USB_DT_INTERFACE_SIZE)) < 0)
			{	
				gok_libusb_display_error(usb_strerror());
				return NULL;
			}
			
			if (interface.bInterfaceClass != USB_CLASS_HID || 
			    interface.bInterfaceSubClass != USB_SUBCLASS_MOUSE ||
			    interface.bInterfaceProtocol != USB_PROTOCOL_MOUSE)
			{
				continue;
			}
			
			for (k = 0; k < interface.bNumEndpoints; k++)
			{
				struct usb_endpoint_descriptor ep;
				if ((usb_get_descriptor(handle, USB_DT_ENDPOINT, k, &ep, USB_DT_ENDPOINT_SIZE)) < 0)
				{	
					gok_libusb_display_error(usb_strerror());
					return NULL;
				}
				if ((ep.bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_INTERRUPT &&
					(ep.bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_IN &&
				    (ep.wMaxPacketSize & USB_ENDPOINT_PACKET_SIZE_MASK) >= 3) /* 1 packet for button events, x motion events and y motion events */
				{
					/* we've found a device that we can use */
					struct input_device *input_dev = g_new0(struct input_device, 1);
					input_dev->handle = handle;
					input_dev->ep_address = ep.bEndpointAddress & USB_ENDPOINT_ADDRESS_MASK;
					input_dev->ep_data_size = ep.wMaxPacketSize & USB_ENDPOINT_PACKET_SIZE_MASK;
					/* the polling interval is calculated differently for USB 2.x devices */
					input_dev->ep_polling_interval = (((device.bcdUSB>>8)&0xf) >= 2) ? twopow(ep.bInterval - 1) : ep.bInterval;
					/* check if the device is attached to a kernel driver and attempt to detach if it is
					 * if the OS doesn't support detaching from the kernel driver, just try to claim the interface anyway
					 */
#if defined(LIBUSB_HAS_GET_DRIVER_NP) && defined(LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP)
					{ 
						gchar *driver_name = g_malloc0(128);
						if (usb_get_driver_np(handle, j, driver_name, 128) >= 0) 
						{
							if (g_utf8_strlen(driver_name, 128) > 0)
							{
								if (usb_detach_kernel_driver_np(handle, j) < 0)
								{
									g_free(input_dev);
									g_free(driver_name);
									gok_libusb_display_error(usb_strerror());
									return NULL;
								}
							}
						} 
						else
						{
							g_message("gok-libusb.c: %s\n", usb_strerror());
						}
						g_free(driver_name);
					}
#endif

					if (usb_claim_interface(handle, j) < 0)
					{
						g_free(input_dev);
						gok_libusb_display_error(usb_strerror());
						return NULL;
					}	

					return input_dev;
				} 
			}
		}
	}
	error = g_malloc(128);
	g_sprintf(error, _("no suitable USB endpoints found in device %x:%x"), vid, pid);
	gok_libusb_display_error(error);
	g_free(error);
	return NULL;
} 

gok_libusb_t * gok_libusb_init(gint vid, gint pid)
{
	gok_libusb_t *handle;
	handle = g_new0(gok_libusb_t, 1);
	if (!g_thread_supported())
		g_thread_init(NULL);

	if ((handle->input_dev = find_input_endpoint(vid, pid)) == NULL)
	{
		return NULL;
	}

	handle->idle_input_data_queue_mutex = g_mutex_new();
	handle->destroy_input_thread_mutex = g_mutex_new();
	/* don't need to protect access here */
	handle->idle_input_data_queue = g_queue_new();
	handle->destroy_input_thread = FALSE;
	handle->libusb_thread = g_thread_create(usb_event_handler, handle, TRUE, NULL);
	
	return handle;
}

gboolean gok_libusb_cleanup(gok_libusb_t *handle)
{	
	/* destroy the input_thread */
	g_mutex_lock(handle->destroy_input_thread_mutex);
	handle->destroy_input_thread = TRUE;
	g_mutex_unlock(handle->destroy_input_thread_mutex);
	g_thread_join(handle->libusb_thread);
	
	/* clear the pending idle input handlers and free the event data */
	g_mutex_lock(handle->idle_input_data_queue_mutex);
	while (!g_queue_is_empty(handle->idle_input_data_queue))
	{
		gpointer data = g_queue_pop_head(handle->idle_input_data_queue);
		if (g_idle_remove_by_data(data))
		{
			struct idle_input_data *idle_data= (struct idle_input_data*) data;
			g_free(idle_data->event_buffer);
			g_free(idle_data);
		}
	}
	g_mutex_unlock(handle->idle_input_data_queue_mutex);

	/* clean up */
	g_mutex_free(handle->idle_input_data_queue_mutex);
	g_mutex_free(handle->destroy_input_thread_mutex);
	usb_close(handle->input_dev->handle);
	g_free(handle->input_dev);
	g_queue_free(handle->idle_input_data_queue);
	g_free(handle);
	return TRUE;
}
