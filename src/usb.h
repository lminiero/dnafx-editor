#ifndef DNAFX_USB
#define DNAFX_USB

#include <stddef.h>
#include <stdint.h>

#include <libusb.h>

#include "tasks.h"

/* USB management */
int dnafx_usb_init(int debug_level);
const struct libusb_pollfd **dnafx_usb_fds(void);
int dnafx_usb_get_next_timeout(struct timeval *tv);
void dnafx_usb_step(void);
void dnafx_usb_deinit(void);

/* Requests */
void dnafx_send_init(dnafx_task_type what);
void dnafx_send_get_presets(dnafx_task_type what);
void dnafx_send_get_extras(dnafx_task_type what);
void dnafx_send_change_preset(int preset);
//~ void dnafx_send_rename_preset(int preset, const char *name);
void dnafx_send_upload_preset(dnafx_task_type what);
void dnafx_send_interrupt(void);

#endif
