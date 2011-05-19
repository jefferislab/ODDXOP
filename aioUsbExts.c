/*
 *  aioUsbExts.c
 *  XFUNC3
 *
 *  Created by Gregory Jefferis on 2011-05-19.
 *  Copyright 2011 Gregory Jefferis. All rights reserved.
 *
 */

#include "aioUsbExts.h"
#include "libusb.h"

/********************************************************************/
//
//  Function Name : AIO_Usb_DIO_ReadTrigger
//
//  Description   : 
//
//  Returns       :	
//
//  Notes		:Added by Alex Hodge to permit one port of a usb-dio-96 to be used as a trigger
//
//  History	  :
// 
/********************************************************************/
unsigned long
AIO_Usb_DIO_ReadTrigger (unsigned long   devIdx,
						 unsigned char  *pData,
						 int trgTO)
{
	int                 ret;
    int					difference;
	int					temp;
	int					startTime; 
	int					triggerTimeout;
	
	struct libusb_device_handle *handle;
	
	ret=AIO_Usb_DIO_GetHandle(devIdx,handle);

	if (ret > ERROR_SUCCESS)
	{
		debug("DBG>> AIO_Usb_DIO_ReadTrigger : could not get device handle devIdx=%d \n",(unsigned int)devIdx);
		return (ERROR_COULD_NOT_GET_DEVHANDLE);
		
	}
	else
	{
		startTime = time(NULL);
		difference = 0;	
		
		pData[11]=0;
		
		triggerTimeout=trgTO;
		//count = 0;
		
		//TODO: Change this so that temp is explicitly stated (probably 0) to save on the subtraction operation		
		while (pData[11] == 0&&time(NULL)<=startTime+triggerTimeout) {
			//while (difference == 0&&time(NULL)<=startTime+triggerTimeout) {
			temp = pData[11];
			
			ret = libusb_control_transfer(handle,
										  USB_READ_FROM_DEV,
										  DIO_READ, 
										  0, 
										  0,
										  pData,
										  14, //changed from the original 4 to work with 96-channel board
										  TIMEOUT_1_SEC);
			
			//			difference = temp - pData[11];	//use for generic trigger
			//			difference = pData[11];			//use if using "difference" in if statement, otherwise don't need
			//triggerTimeout=20;				//allows time for pulse train to finish
			//count++;
			
		}
		
		libusb_close(handle);
		if (time(NULL)>startTime+triggerTimeout) {
			return(15);
		}else {
			return(10);
		}
		
		if (ret < 0)
		{
			debug("DBG>> AIO_Usb_ReadALL : usb_control_msg failed dev = 0x%0x err=%d",(unsigned int)devIdx,ret);
			return (ERROR_USB_CONTROL_MSG_FAILED);
		}
		else
			return (ERROR_SUCCESS);
	}
}

/********************************************************************/
//
//  Function Name : AIO_Usb_DIO_GetHandle
//
//  Description   : Given an AIO devide index returns a handle
//
//  Returns       :	lib_usb_device_handle for specified device
//
//  Notes		: Added by Greg Jefferis to permit faster batched read/writes
//				  NB don't expect this to be used throughout a session
//				  rather for a single sweep or perhaps series of sweeps
//
//  History	  :
// 
/********************************************************************/

// Actually this seems more or less equivalent to getDevHandle

unsigned long AIO_Usb_DIO_GetHandle(unsigned long devIdx, struct libusb_device_handle *handle)
{
	int                          ret;
	
//	if (pData == NULL)
//	{
//		debug("DBG>>AIO_ReadAll : pData is NULL \n"); 
//		return (ERROR_INVALID_PARAM); 
//	}
	
	ret = AIO_UsbValidateDeviceIndex(devIdx);
	
	if (ret > ERROR_SUCCESS)
	{
		debug("DBG>>AIO_Usb_DIO_GetHandle: invalid dev Index = 0x%0x \n",(unsigned int)devIdx); 
		return (ret);
	}
	
	ret = validateProductID(devIdx);
	if (ret > ERROR_SUCCESS)
	{
		debug("DBG>>AIO_Usb_DIO_GetHandle: invalid Product ID for device = 0x%0x \n",(unsigned int)devIdx); 
		return (ret);
	}
	
	handle = getDevHandle(devIdx);
	if (handle == NULL)
	{
		debug("DBG>> AIO_Usb_DIO_GetHandle : could not get device handle devIdx=%d \n",(unsigned int)devIdx);
		return (ERROR_COULD_NOT_GET_DEVHANDLE);
		
	}
	return ERROR_SUCCESS;	
}
