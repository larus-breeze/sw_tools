/*
 * CAN_USB_gateway.cpp
 *
 *  Created on: Sep 6, 2022
 *      Author: schaefer
 */

#include "USB_serial.h"
#include "generic_CAN_driver.h"

bool CAN_send( const CANpacket &p, unsigned)
{
  CAN_gateway_packet output( p);
  write_usb_serial( (uint8_t *) &output, sizeof output);
  return true;
}



