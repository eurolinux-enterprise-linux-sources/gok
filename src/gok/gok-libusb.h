#ifndef GOKLIBUSB_H_
#define GOKLIBUSB_H_

#include <usb.h>

typedef struct _gok_libusb_t gok_libusb_t;

extern gok_libusb_t * gok_libusb_init(gint vid, gint pid);
extern gboolean gok_libusb_cleanup(gok_libusb_t *handle);

#endif /*GOKLIBUSB_H_*/
