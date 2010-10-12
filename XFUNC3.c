/*	XFUNC3.c -- illustrates Igor external string functions.

*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h

#include "XFUNC3.h"


////////////////////////////////////////////////
//Copied from DIO examples using NI API
////////////////////////////////////////////////
//#include <stdio.h>
#include <time.h>
//#include <string.h>
//#include <stdlib.h>
//#include <math.h>
////////////////////////////////////////////////




////////////////////////////////////////////////
//Copied from ODDRun.c using ACCES API
////////////////////////////////////////////////

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <math.h>


#include <pthread.h>   // for nanosleep() call

#include "aioUsbApi.h"


int   devIdx;
char  anyKey;
int   ctlC;
aioDeviceInfo aioDevices;


int			   ret;
int			   tmp;
int			   i;
int			   hangTime;


unsigned char  pins0_7;
unsigned char  pins8_15;
unsigned char  pins16_23;
unsigned char  pins24_31; 
int            p0_7Input;
int            p8_15Input;
int			   p16_23Input;
int			   p24_31Input;

unsigned char  mask; 
unsigned char  data[4];
int            triState; 




////////////////////////////////////////////////








#define DAQmxErrChk(functionCall) { if( DAQmxFailed(error=(functionCall)) ) { goto Error; } }
int doIHaveAnError;





int oddRunTest()
{
	XOPNotice("\015 Testing...\015");
	return(0);
}





















///////////////////////////////////////////////


#include "XOPStructureAlignmentTwoByte.h"	// All structures passed to Igor are two-byte aligned.
struct xstrcatParams  {
	Handle str3;
	Handle str2;
	Handle result;
};




//////////////////////////////////////////////////
//My Functions
//////////////////////////////////////////////////
static void
MyHello(void){
	XOPNotice("Hello. Now you're calling a function...\015");
	doIHaveAnError=oddRunTest();
	XOPNotice("That should have worked. \015");
	
}



void
catchInterrupt (int signum) 
{
    ctlC = 1;
}


int 
validateIndex(int devIdx)
{
	int ret;
	
	ret = AIO_UsbValidateDeviceIndex(devIdx);
	if (ret > ERROR_SUCCESS)
	{
		printf("Invalid device (%d); Do a GetDevices() to see valid devices  \n",devIdx);
	}
	return  ret;
}













int initialise()
{

	printf("For testing. This function just tests each channel");
	
	int			 ret;
	int			 tmp;
	int			 i;
	int			 hangTime;
	
	
	unsigned char  pins0_7;
	unsigned char  pins8_15;
	unsigned char  pins16_23;
	unsigned char  pins24_31; 
	int            p0_7Input;
	int            p8_15Input;
	int			 p16_23Input;
	int			 p24_31Input;
	
	unsigned char  mask; 
	unsigned char  data[4];
	int            triState; 
	
	
	
	printf("Configuring all DIO bytes for output...\n");
	
	printf("Setting pins 0 - 7  as OUTput                                 :  \n");
	//scanf("%x",&tmp);
	pins0_7 = 1;
	p0_7Input = 1;
	printf("Setting pins 8 - 15  as OUTput                                :  \n");
	//scanf("%x",&tmp);
	pins8_15 = 1;
	p8_15Input = 1;
	printf("Setting pins 16 - 23 as OUTput                                :  \n");
	//scanf("%x",&tmp);
	pins16_23 = 1;
	p16_23Input = 1;
	printf("Setting pins 24 - 31 as INput                                 :  \n");
	//scanf("%x",&tmp);
	pins24_31 = 0;
	p24_31Input = 0;
	
	mask = 0;
	mask = pins24_31 << 3;;			//these are shifted HEX values
	mask = mask | pins16_23 << 2; 
	mask = mask | pins8_15 << 1; 
	mask = mask | pins0_7; 
	
	
	//		  printf("Enter Tristate (0=off; 1=on}                                    :  ");
	//		  scanf("%x",&tmp);
	triState= 0;
	
	if (p0_7Input == 1)
	{
		printf("Setting output byte  0 to 1                                      :  \n");
		//scanf("%x",&tmp);
		tmp = pow(2,0);
		data[0] = tmp;
	}
	
	if (p8_15Input == 1)
	{
		printf("Setting output byte  1 to 0                                      :  \n");
		//scanf("%x",&tmp);
		tmp = 0;
		data[1] = tmp;
	}
	
	
	if (p16_23Input == 1)
	{
		printf("Setting output byte  2 to 0                                      :  \n");
		//scanf("%x",&tmp);
		tmp = 0;
		data[2] = tmp;
	}
	
	if (p24_31Input == 1)
	{
		printf("Setting output byte  2 to 0                                      :  \n");
		//scanf("%x",&tmp);
		tmp = 0;
		data[3] = tmp;
	}

	
	ret =   AIO_Usb_DIO_Configure (devIdx,
								   triState,
								   &mask,
								   data);
	
	if (ret > ERROR_SUCCESS)
	{
		printf ("\nDIO_Configure Failed  dev=0x%0x err=%d \n",(unsigned int)devIdx,ret);
		return(0);
	}
	
	
	
	
	
	
	
	
	return(0);
	
	
}


int odourPulse(int delay, int odour, int duration)
{
	
	
	int			 ret;
	int			 tmp;
	int			 i;
	int			 stimTime;
	int			 delayTime;
	
	
	unsigned char  pins0_7;
	unsigned char  pins8_15;
	unsigned char  pins16_23;
	unsigned char  pins24_31; 
	int            p0_7Input;
	int            p8_15Input;
	int			   p16_23Input;
	int			   p24_31Input;
	
	unsigned char  mask; 
	unsigned char  data[4];
	int            triState; 
	
	mask = 0;
	mask = pins24_31 << 3;;			//these are shifted HEX values
	mask = mask | pins16_23 << 2; 
	mask = mask | pins8_15 << 1; 
	mask = mask | pins0_7; 
	
	
	triState = 0;
	i=odour;
	stimTime = duration*1000;
	delayTime = delay;
	
	
	printf("\n i = %d\n hangtime = %d\n",i,hangTime);
	
	
	
	usleep(1000*delayTime);
	if (odour<8) {
		
		data[0]=pow(2,i);
		data[1]=0;
		data[2]=0;
		data[3]=0;
		
		ret =   AIO_Usb_DIO_Configure (devIdx,
									   triState,
									   &mask,
									   data);
		
		usleep(stimTime);
		data[0]=1;
		data[1]=0;
		data[2]=0;
		data[3]=0;
		
		
		ret =   AIO_Usb_DIO_Configure (devIdx,
									   triState,
									   &mask,
									   data);
		return(1);
	}else if (odour>7&&odour<16) {
		
		data[0]=0;
		data[1]=pow(2,i-8);
		data[2]=0;
		data[3]=0;
		
		ret =   AIO_Usb_DIO_Configure (devIdx,
									   triState,
									   &mask,
									   data);
		
		usleep(stimTime);
		data[0]=1;
		data[1]=0;
		data[2]=0;
		data[3]=0;
		
		
		ret =   AIO_Usb_DIO_Configure (devIdx,
									   triState,
									   &mask,
									   data);
		return(1);
	}else if (odour>15&&odour<24) {
		
		data[0]=0;
		data[1]=0;
		data[2]=pow(2,i-16);
		data[3]=0;
		
		ret =   AIO_Usb_DIO_Configure (devIdx,
									   triState,
									   &mask,
									   data);
		
		usleep(stimTime);
		data[0]=1;
		data[1]=0;
		data[2]=0;
		data[3]=0;
		
		
		ret =   AIO_Usb_DIO_Configure (devIdx,
									   triState,
									   &mask,
									   data);
		return(1);
	}else{
		printf("ERROR: Invalid odour");
		return(0);
	}
}


int odourPulses(char configFile)
{
	
	int			 ret;
	int			 tmp;
	int			 i;
	int          odour;
	int			 stimTime;
	int			 delayTime;
	
	
	unsigned char  pins0_7;
	unsigned char  pins8_15;
	unsigned char  pins16_23;
	unsigned char  pins24_31; 
	int            p0_7Input;
	int            p8_15Input;
	int			   p16_23Input;
	int			   p24_31Input;
	
	unsigned char  mask; 
	unsigned char  data[4];
	int            triState; 
	
	mask = 0;
	mask = pins24_31 << 3;;			//these are shifted HEX values
	mask = mask | pins16_23 << 2; 
	mask = mask | pins8_15 << 1; 
	mask = mask | pins0_7; 
	
	
	triState = 0;

	
	XOPNotice("\015Waiting for another trigger...\015");
	tmp=triggerDetect();
	
	delayTime=1000;
	stimTime=1000;
	odour=5;
	
	
	usleep(1000*delayTime);
	if (odour<8) {
		
		data[0]=pow(2,odour);
		data[1]=0;
		data[2]=0;
		data[3]=0;
		
		ret =   AIO_Usb_DIO_Configure (devIdx,
									   triState,
									   &mask,
									   data);
		
		usleep(stimTime);
		data[0]=1;
		data[1]=0;
		data[2]=0;
		data[3]=0;
		
		
		ret =   AIO_Usb_DIO_Configure (devIdx,
									   triState,
									   &mask,
									   data);
		return(1);
	}else if (odour>7&&odour<16) {
		
		data[0]=0;
		data[1]=pow(2,odour-8);
		data[2]=0;
		data[3]=0;
		
		ret =   AIO_Usb_DIO_Configure (devIdx,
									   triState,
									   &mask,
									   data);
		
		usleep(stimTime);
		data[0]=1;
		data[1]=0;
		data[2]=0;
		data[3]=0;
		
		
		ret =   AIO_Usb_DIO_Configure (devIdx,
									   triState,
									   &mask,
									   data);
		return(1);
	}else if (odour>15&&odour<24) {
		
		data[0]=0;
		data[1]=0;
		data[2]=pow(2,odour-16);
		data[3]=0;
		
		ret =   AIO_Usb_DIO_Configure (devIdx,
									   triState,
									   &mask,
									   data);
		
		usleep(stimTime);
		data[0]=1;
		data[1]=0;
		data[2]=0;
		data[3]=0;
		
		
		ret =   AIO_Usb_DIO_Configure (devIdx,
									   triState,
									   &mask,
									   data);
		return(1);
	}else{
		printf("ERROR: Invalid odour");
		return(0);
	}
}




int triggerDetect()
{
	
	int						ret;
	unsigned char           data[4];
	unsigned char           testThing;
    int						difference;
	int						temp;
	
	
	ret = validateIndex(devIdx);
	if (ret > ERROR_SUCCESS)
		return(0);
	
	ret =   AIO_Usb_DIO_ReadAll (devIdx,
								 (unsigned char *)&data[0]); 
	
	if (ret > ERROR_SUCCESS)
	{
        printf ("\n\nReadAll Failed dev=0x%0x err=%d  \n\n",(unsigned int)devIdx,ret);
        return(0);
	}
	
	difference = 0;	
	while (difference == 0) {
		temp = data[3];
		ret =   AIO_Usb_DIO_ReadAll (devIdx,
									 (unsigned char *)&data[0]); 
		difference = temp - data[3];
		
		
	}
	
	// first 2 bytes unused for AI 16
	//printf("\n PINs 0- 7: = 0x%x\n",data[0]);
	//printf("\n PINs 0-15: = 0x%x\n",data[1]);
	//printf("\n PINs 16-23: = 0x%x\n",data[2]);
	printf("\n PINs 24:32: = 0x%x\n",data[3]);
	
	
	
	return(0);
	
	
}



















//////////////////////////////////////////////////



typedef struct xstrcatParams xstrcatParams;
#include "XOPStructureAlignmentReset.h"

static int
xstrcat(xstrcatParams* p)				/* str1 = xstrcat(str2, str3) */
{
	
	
	
	
	
	
	
	
	
	
	
	
	Handle str1;						/* output handle */
	long len2, len3;
	int err=0;
	
	str1 = NIL;							/* if error occurs, result is NIL */
	if (p->str2 == NIL) {				/* error –– input string does not exist */
		err = NO_INPUT_STRING;
		goto done;
	}
	if (p->str3 == NIL)	{				/* error –– input string does not exist */
		err = NO_INPUT_STRING;
		goto done;
	}
	
	len2 = GetHandleSize(p->str2);		/* length of string 2 */
	len3 = GetHandleSize(p->str3);		/* length of string 3 */
	str1 = NewHandle(len2 + len3);		/* get output handle */
	if (str1 == NIL) {
		err = NOMEM;
		goto done;						/* out of memory */
	}
	
	memcpy(*str1, *p->str2, len2);
	memcpy(*str1+len2, *p->str3, len3);
	
done:
	if (p->str2)
		DisposeHandle(p->str2);			/* we need to get rid of input parameters */
	if (p->str3)
		DisposeHandle(p->str3);			/* we need to get rid of input parameters */
	p->result = str1;
	
//	AIO_Init();
//	ret = AIO_Usb_GetDevices(&aioDevices);
//	devIdx = aioDevices.aioDevList[0].devIdx;
	XOPNotice("How do yorfgaeguXXXXXXX like me now?\015");
	MyHello();
	XOPNotice("MyHello OK\015");
	tmp = initialise();
	XOPNotice("initialise OK\015");
	XOPNotice("Can I have a trigger?\015");
	tmp = triggerDetect();
	
	
	
	tmp = odourPulse(2000,20,500);
	tmp = odourPulse(2000,21,500);
	tmp = odourPulse(2000,22,500);
	XOPNotice("\015odourPulse OK\015");
	tmp = odourPulses("thing.odd");
	XOPNotice("\015odourPulses OK\015");

	
	return(err);
}










static long
RegisterFunction()
{
	int funcIndex;

	funcIndex = GetXOPItem(0);		/* which function invoked ? */
	switch (funcIndex) {
		case 0:						/* str1 = oddRun(str2, str3) */
			return((long)xstrcat);	/* This uses the direct call method - preferred. */
			break;
		case 1:						/* str1 = xstrcat1(str2, str3) */
			return((long)xstrcat);	/* This uses the direct call method - preferred. */
			break;
		///////////////////////////////////////////////////////////////////////////////////////////
		case 2:						/* str1 = xstrcat1(str2, str3) */
			return(NIL);			/* This uses the message call method - generally not needed. */
			break;
		///////////////////////////////////////////////////////////////////////////////////////////
			
	}
	return(NIL);
}

static int
DoFunction()
{
	int funcIndex;
	void *p;				/* pointer to structure containing function parameters and result */
	int err;				/* error code returned by function */

	funcIndex = GetXOPItem(0);		/* which function invoked ? */
	p = (void *)GetXOPItem(1);		/* get pointer to params/result */
	switch (funcIndex) {
		case 1:						/* str1 = xstrcat2(str2, str3) */
			err = xstrcat(p);
			break;
		default:
			err = UNKNOWN_XFUNC;
			break;
	}
	return(err);
}

/*	XOPEntry()

	This is the entry point from the host application to the XOP for all messages after the
	INIT message.
*/

static void
XOPEntry(void)
{	
	long result = 0;

	switch (GetXOPMessage()) {
		case FUNCTION:								/* our external function being invoked ? */
			result = DoFunction();
			break;
			
		case FUNCADDRS:
			result = RegisterFunction();
			break;
	}
	SetXOPResult(result);
}

/*	main(ioRecHandle)

	This is the initial entry point at which the host application calls XOP.
	The message sent by the host must be INIT.
	main() does any necessary initialization and then sets the XOPEntry field of the
	ioRecHandle to the address to be called for future messages.
*/

HOST_IMPORT void
main(IORecHandle ioRecHandle)
{	
	XOPInit(ioRecHandle);							/* do standard XOP initialization */
	SetXOPEntry(XOPEntry);							/* set entry point for future calls */

	
	AIO_Init();
	
	// This should called BEFORE any other function in the API
	// to populate the list of Acces devices
	ret = AIO_Usb_GetDevices(&aioDevices);
	
	// use the first device found
	devIdx = aioDevices.aioDevList[0].devIdx;
	
	
	
	tmp = initialise();
	
	
	
	
	if (igorVersion < 200)
		SetXOPResult(REQUIRES_IGOR_200);
	else
		SetXOPResult(0L);
}
