/*
 *  aioUsbExts.h
 *  XFUNC3
 *
 *  Created by Gregory Jefferis on 2011-05-19.
 *  Copyright 2011 Gregory Jefferis. All rights reserved.
 * 
 */

// File to 

#include "aioUsbApi.h"

unsigned long
AIO_Usb_DIO_ReadTrigger (unsigned long   devIdx,
						 unsigned char  *pData,
						 int trgTO);

unsigned long
	AIO_Usb_DIO_GetHandle(unsigned long devIdx,
						  struct libusb_device_handle *handle);
