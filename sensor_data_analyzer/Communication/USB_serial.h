/*
 * USB_serial.h
 *
 *  Created on: Aug 25, 2022
 *      Author: schaefer
 */

#ifndef USB_SERIAL_H_
#define USB_SERIAL_H_
#include "stdint.h"

bool open_USB_serial ( char *portname = (char *)"/dev/ttyUSB0");
bool write_usb_serial( uint8_t * data, unsigned size);

#endif /* USB_SERIAL_H_ */
