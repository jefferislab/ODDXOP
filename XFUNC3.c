/*	XFUNC3.c -- illustrates Igor external string functions.
 
 
 This XOP enables Igor Pro to operate the ACCES DIO cards and operate the solenoid valved on the ODD modules.
 From Igor Pro, it takes two strings as arguments: one for the config file and one for the logfile:
 
 use:
 oddRun("whateverConfigFile.odd, anyLogFile.log")
 
 Alex Hodge
 
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>	// for nanosleep() call
#include <math.h>
#include <pthread.h>						

#include <assert.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

#include "libusb.h"
#include "aioUsbExts.h"
#include "XOPStandardHeaders.h"				// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "XFUNC3.h"

#define MAX_ODOURS_PER_LINE 5
#define MAX_ODOUR_PORTS 8 
#define BITS_PER_PORT 8
#define MAX_ODOURS (BITS_PER_PORT * MAX_ODOUR_PORTS)
#define BLANK_NOT_SET -1

int				devIdx;
struct libusb_device_handle *usbhandle=NULL;
pthread_t pulseThread;	// this is our thread identifier, used to call odourPulses() from its own thread

char			anyKey;
int				ctlC;

int				stimTime;
int				delayTime;
int				postDelay;
int				odour;
int				doIHaveAnError;
int				triggerTimeout;

char			configFile[255];
char			logFile[255];
char			cfg[255];
char			lg[255];

//For the ACCES API calls
unsigned char	mask[2]; 
unsigned char	data[12];
int				triState; 
unsigned char   byte;
unsigned int	byteIdx;


//Pointers to logfile and configfile
FILE* fi; 
FILE* fo; 


//Values to be read from config file
char ch, s[80], chID1[10], chID2[10], chID3[10], chID4[10], chID5[10];
int d1,p1,o1,d2,p2,o2,d3,p3,o3,d4,p4,o4,d5,p5,o5;


//Functions
int triggerDetectFaster();
int odourPulses(char *cfgFileName);
int validateIndex(int devIdx);
int initialise();
void dataReset(int blankOdour);
uint64_t GetAbsTimeInNanoseconds(void);

struct xstrcatParams  {
	Handle str3;
	Handle str2;
	Handle result;						//Not currently used, but can pass a string back to Igor 
};

aioDeviceInfo aioDevices;

typedef struct xstrcatParams xstrcatParams;


//////////////////////////////////////////////////
//My Functions
//////////////////////////////////////////////////

// See
// http://developer.apple.com/library/mac/#qa/qa2004/qa1398.html
uint64_t GetAbsTimeInNanoseconds(void)
{
    uint64_t        abstime;
    uint64_t        abstimenano;
    static mach_timebase_info_data_t    sTimebaseInfo;

    // Start the clock.

    abstime = mach_absolute_time();

    // If this is the first time we've run, get the timebase.
    // We can use denom == 0 to indicate that sTimebaseInfo is
    // uninitialised because it makes no sense to have a zero
    // denominator is a fraction.

    if ( sTimebaseInfo.denom == 0 ) {
        (void) mach_timebase_info(&sTimebaseInfo);
    }

    // Do the maths.  We hope that the multiplication doesn't
    // overflow; the price you pay for working in fixed point.

    abstimenano= abstime * sTimebaseInfo.numer / sTimebaseInfo.denom;

    return abstimenano;
}


void									//Old
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
		XOPNotice("\015Invalid device\015");
	}
	
	//XOPNotice("\015Device OK\015");
	
	return  ret;
}


void 
*threadFunc(void *arg)		/* This is the thread function.  It is like main(), but for a thread, and just runs the odourPulses() function*/
{
	char *str;
	str=(char*)arg;
	int	tmp;
	
	XOPNotice("\015New thread started.");
	//XOPNotice(cfg);
	
	if(strlen(cfg)<1)
	{
		XOPNotice("\015No config file entered. Let's use the default:\015");
		XOPNotice("\015TODO: output the .odd file\015");
		tmp = odourPulses(str);				//This allows a default value to be set
	}else {
		XOPNotice("\015Config file entered. Let's use it.\015");
		XOPNotice(cfg);
		tmp = odourPulses(cfg);				//works
	}
		
		if (tmp==10) {
			XOPNotice("\015ERROR: Could not open config file. Try again ya wee lass.\015");
		}
		if (tmp==11) {
			XOPNotice("\015ERROR: Could not open log file. Does the directory path exist?\015");
		}
	
	return NULL;
}


int 
initialise()						//Just sets up the board for our use: all but one byte to be used as output
{	
	unsigned char  mask[2]; 
//	unsigned short  mask;			//for DIO_32
//	unsigned char  data[4];			//for DIO_32
	unsigned char  data[12];		//for DIO_96
	int            triState; 
	int	ret;

	mask[0]=0xFF;	//sets the first 8 ports to output
//	mask[1]=0x0;	//sets the rest of the board for input
	mask[1]=0x3;	//sets ports 8 and 9 for output, 10 and 11 for input. Port 11 is reserved for the trigger and port 10 can be GPIO
	
	triState= 0;
	
	data[0]=0x01;	//sets the data lines. Defaults to position 0 on data 0 "open" to allow gas flow through the first blank
	data[1]=0x00;
	data[2]=0x00;
	data[3]=0x00;
	data[4]=0x00;
	data[5]=0x00;
	data[6]=0x00;
	data[7]=0x00;
	data[8]=0x00;
	data[9]=0x00;
	data[10]=0x00;
	data[11]=0x00;
	
	ret =   AIO_Usb_DIO_Configure (devIdx,	//writes and configures
								   triState,
								   mask,
								   data);
	
	if (ret > ERROR_SUCCESS)
	{
		char temp[256];
		sprintf(temp, "\015DIO_Configure Failed dev=0x%0x err=%d\015Have you run AccesLoader?\015",(unsigned int)devIdx,ret);
		XOPNotice(temp);
		return(0);
	} else {
		XOPNotice("Successfully intialised Acces DIO card!\015");
	}
	return(0);	
}


void
dataReset(int blankOdour)
{
	int ret;
	
	data[0]=0;
	data[1]=0;
	data[2]=0;
	data[3]=0;
	data[4]=0;
	data[5]=0;
	data[6]=0;
	data[7]=0;
	
	data[9]=0x00;
	
	if (blankOdour >= 0 && blankOdour < MAX_ODOURS ) {
		int port = blankOdour/BITS_PER_PORT;
		data[port]=pow(2, blankOdour % BITS_PER_PORT);
	}
	
	ret =   AIO_Usb_WriteAllH(usbhandle,
							  data);
}

int 
odourPulses(char *cfgFileName)		//Main function. The others are mostly just for testing, but I left them in case they are of some use in the future
{
	
	//setting the mask here is redundant since I am now using writeAll() instead of Configure()
	mask[0]=0xFF;	//sets the first 8 ports to output
//	mask[1]=0x0;	//sets all remaining ports to input
	mask[1]=0x3;	//sets ports 8 and 9 for output, 10 and 11 for input. Port 11 is reserved for the trigger and port 10 can be GPIO
		
	//strcpy(configFile,"/Users/ahodge/Desktop/");
	strcpy(configFile,cfgFileName);
	
	XOPNotice("\015Using the config file:\015");
	XOPNotice(configFile);
	XOPNotice("\015\015");
	
	strcpy(logFile,(char*)lg);
	//strcpy(logFile,"/Users/ahodge/Desktop/logfiles/");
//	if (strlen(lg)<1) {
//		strcat(logFile,"defaultLogFile.txt");
//	}else{
//		strcat(logFile,(char*)lg);
//	}
	
	XOPNotice("\015Using the log file:\015");
	XOPNotice(logFile);
	XOPNotice("\015\015");
	
	//time
	time_t sec;
	sec = time(NULL);	
	
	//printf("\nOpening files...");
	XOPNotice("\015Opening files...");
	
	if((fi=fopen(configFile,"r"))==NULL) {// open cfg file and return with an error value if fopen fails
		return(10);		
	}
	
	if((fo=fopen(logFile,"w"))==NULL) {	// open log file and return with an error value if fopen fails
	//if((fo=fopen("/Users/ahodge/Desktop/logfiles/tempLog.txt","w"))==NULL) {	// open log file and return with an error value if fopen fails
		return(11);
	}
	fprintf(fo, "%s\n\nThis log file was generated by the odourPulses() function called using the oddRun() XOP function from Igor Pro\n", logFile);
	
	XOPNotice("done");
	
	triggerTimeout=5;
	
	postDelay=1000;
	
//TODO: Change this to depend on the number of lines in the .odd file
	int i=0;
	int stimTimes[MAX_ODOURS_PER_LINE];
	int odours[MAX_ODOURS_PER_LINE];
	int delayTimes[MAX_ODOURS_PER_LINE];
	char key[50];
    int values[2];
	int blankOdour = BLANK_NOT_SET;
	int tmp,ret;
	int retval=1;

	while (fgets(s, 80, fi) != NULL) {
		
		if (s[0]=='#') continue; // Comments
		if (s[0]=='!') {
			// Instructions in form:
			// ! keyword = intval1 [intval2]
			if(1 != sscanf(s,"! %s = ", key)){
				fprintf(fo,"\nERROR: malformed key value pair. Skipping line.");
				XOPNotice("\015ERROR: malformed key value pair. Skipping line.");
				continue;
			}
			if (0==strcmp("blank", key)) {
				if (2 == sscanf(s,"! %s = %d", key, &values[0])){
					blankOdour = values[0];
				} else {
					fprintf(fo,"\nERROR: malformed key value pair. Skipping line.");
					XOPNotice("\015ERROR: malformed key value pair. Skipping line.");
				}
			} else {
				fprintf(fo,"\nERROR: unrecognised key value. Skipping line.");
				XOPNotice("\015ERROR: malformed key value. Skipping line.");
			}
			continue;
		}
		
		i++;

		sscanf(s,"%s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",(char*)chID1, 
			   &delayTimes[0], &stimTimes[0], &odours[0],
			   &delayTimes[1], &stimTimes[1], &odours[1],
			   &delayTimes[2], &stimTimes[2], &odours[2],
			   &delayTimes[3], &stimTimes[3], &odours[3],
			   &delayTimes[4], &stimTimes[4], &odours[4]);
		
		//this might be unnecessary but will enable default blank selection from the config file later
				
		if (stimTimes[0]==0) {
			XOPNotice("\015The first entry in this line is for zero duration. Not waiting for a trigger.\015");
			fprintf(fo, "\nThe first entry in line %d is for zero duration. Not waiting for a trigger and not executing sequence:\n%s",i,s);
			continue;
		}
		
		XOPNotice("\015I found an entry in the .odd file. Can I please have a trigger?\015");
		fprintf(fo, "\nNonzero p1 detected. Running line %d: %s",i,s);
		tmp=triggerDetectFaster();
		uint64_t triggerTime=GetAbsTimeInNanoseconds();
		//tmp=triggerDetectFast();
		//tmp=triggerDetect();
		if (tmp==10) {
			XOPNotice("\015Trigger detected. Executing protocol...");
			fprintf(fo, "Trigger detected. Executing sequence.\n");
			//TODO: force error message to appear reliably. 				
		}else if (tmp==15) {
			XOPNotice("\015TriggerDetect() timed out.");
			fprintf(fo, "TriggerDetect() timed out. Aborting");
			retval =0;
			goto threaddone;

			return(0);
		}else if (tmp==0) {
			XOPNotice("\015TriggerDetect() failed.");
			fprintf(fo, "TriggerDetect() failed for some reason besides timeout.");
			retval =0;
			goto threaddone;
		}else {
			XOPNotice("\015TriggerDetect() timed out.");
			fprintf(fo, "\nTimed out with counter = %d\n",tmp);
			retval =0;
			goto threaddone;
		}

		triggerTimeout=20;
		
		int j, port;
		for(j=0;j < MAX_ODOURS_PER_LINE; j++){
			
			stimTime=stimTimes[j];
			odour=odours[j];
			delayTime=delayTimes[j];
			
			
			if (stimTime!=0) {
				port = odour/BITS_PER_PORT;
				if (odour>=0 && odour<MAX_ODOURS) {
					if (blankOdour == BLANK_NOT_SET) {
						dataReset(port*BITS_PER_PORT);
					} else {
						
						dataReset(blankOdour);
					}
					data[9]=1;
					
					usleep(1000*delayTime);
					uint64_t stimStartTime = 1e6*delayTime+triggerTime; // nb convert ms -> ns
					uint64_t deltaStartTime;
					while (1) {
						deltaStartTime=GetAbsTimeInNanoseconds()-stimStartTime;
						if	(deltaStartTime>=0) {
							break;
						}
					}
					data[port]=pow(2, odour % BITS_PER_PORT);
					ret =   AIO_Usb_WriteAllH(usbhandle,
											  data);
					usleep(1000*stimTime);
					if (blankOdour == BLANK_NOT_SET) {
						dataReset(port*BITS_PER_PORT);
					} else {
						dataReset(blankOdour);
					}
					usleep(1000*postDelay);
					fprintf(fo,"\015INFO: deltaStartTime was %g.",(double) deltaStartTime);
					
					
				} else {
					fprintf(fo,"\nERROR: you've asked for an odour that I can't provide. I'm quitting");
					XOPNotice("\015ERROR: you've asked for an odour that I can't provide. I'm quitting");
					fclose(fi);fclose(fo);
					return(0);
				}
				fprintf(fo, "Applied odour %d for %dms, after a %dms delay\n",odour,stimTime,delayTime);
			}

			XOPNotice("OK, Odours done.\015");
		}
	}
threaddone:
	XOPNotice("\015Closing files.....");
	fclose(fi);fclose(fo);
	XOPNotice("OK\015");
	XOPNotice("\015Closing usb handle .....");
	libusb_close(usbhandle);
	// set handle to null explicitly (thought this would happen anyway, but ...
	usbhandle=NULL;
	// set pthread to null
	pulseThread = NULL;
	return(retval);
}

int 
triggerDetectFaster()		//This triggerDetect calls a function AIO_Usb_DIO_ReadTrigger() that I added to the API
{							//It is sufficiently fast
	int ret;
	ret = validateIndex(devIdx);
	if (ret > ERROR_SUCCESS)
		return(0);

	//Only for testing the 96!!!!! use this if you don't want to wait for triggers. Delete this later
	//return(10);
	
	XOPNotice("\015Attempting to use the fast trigger loop...");
	ret =   AIO_Usb_DIO_ReadTriggerH(usbhandle,(unsigned char *)&data[0],triggerTimeout);
	
	if (ret > ERROR_SUCCESS)
	{
        fprintf (fo,"\n\nReadAll Failed dev=0x%0x err=%d  \n\n",(unsigned int)devIdx,ret);
        return(0);
	}
	
//	XOPNotice("...that worked. I'm back from the fast trigger loop\015");

	if (ret==15) {
		XOPNotice("Trigger timeout. Try to do better.\015");
	}else if (ret==10) {
//		XOPNotice("Trigger detected. Here we go....\015");
	}
	
	return(ret);
}
//TODO: Standardise the return values. These ones are kinda crazy

static int
xstrcat(xstrcatParams* p)				/* str1 = xstrcat(str2, str3) */
{
	
	Handle str1;						/* output handle */
	long len2, len3;
	int err=0;
	
	str1 = NIL;							/* if error occurs, result is NIL */
	if (p->str2 == NIL) {				/* error –– input string does not exist */
		err = MISSING_INPUT_PARAM;
		goto done;
	}
	if (p->str3 == NIL)	{				/* error –– input string does not exist */
		err = MISSING_INPUT_PARAM;
		goto done;
	}
	
	len2 = GetHandleSize(p->str2);		/* length of string 2 */
	//	stringLen = GetHandleSize(p->str2);
	len3 = GetHandleSize(p->str3);		/* length of string 3 */
	str1 = NewHandle(len2 + len3);		/* get output handle */
	if (str1 == NIL) {
		err = NOMEM;
		goto done;						/* out of memory */
	}
	
/*	
	XOPNotice("\015\015*p->str2");
	XOPNotice(*p->str2);
	XOPNotice("\015");
*/	
	
	memcpy(*str1, *p->str2, len2);
	memcpy(*str1+len2, *p->str3, len3);
	
	//strcpy(cfg,"cfgFile.odd");//working
	//strcpy(cfg,*p->str2);
	GetCStringFromHandle(p->str2, cfg, 255);
	GetCStringFromHandle(p->str3, lg, 255);
		
	// Get a handle for current USB device
	if(usbhandle!=NULL){
		// already open - we need to close it
		// and kill the thread
		XOPNotice("Usb handle already open - bailing to wait for previous thread to timeout");
		err = ACCES_STILL_RUNNING;
		goto done;
	}
//	if(pulseThread!=NULL){
//		XOPNotice("Cancelling existing thread");
//		int ret = pthread_cancel(pulseThread);
//		// bail if we failed to kill thread - this is problaby going to mean 
//		// a restart
//		if (ret>0) {
//			XOPNotice("Failed to cancel existing thread, bailing out");
//			goto done;
//		}
//		pulseThread=NULL;
//	}
	// Get device handle
	XOPNotice("Obtaining ACCES DIO device handle\015");
	usbhandle=getDevHandle(devIdx);
//	ret=AIO_Usb_DIO_GetHandle(devIdx, usbhandle);
	if(usbhandle==NULL){
		XOPNotice("Failed to get a handle to Access DIO USB device");
		err = CANT_ACCESS_ACCES;
		goto done;
	}
	
//	if(ret!=ERROR_SUCCESS){
//		// We didn't manage to get a handle
//		XOPNotice("Failed to get a handle to Access DIO USB device");
//		goto done;
//	}
	
	//pthread_create(&pth,NULL,threadFunc,"foo");			//Original
	pthread_create(&pulseThread,NULL,threadFunc,"cfgFile.odd");		//GOOD: sets cfgFile.odd as the default config file in case there is no input
	pthread_detach(pulseThread);									//Don't want the original thread to wait for odourPulses() to finish
	
	
	//	printf("main waiting for thread to terminate...\n");	//Old
	//	pthread_join(pth,NULL);									//Old
	
done:
	if (p->str2)
		DisposeHandle(p->str2);			/* we need to get rid of input parameters */
	if (p->str3)
		DisposeHandle(p->str3);			/* we need to get rid of input parameters */
	p->result = str1;
	
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
	void *p;						/* pointer to structure containing function parameters and result */
	int err;						/* error code returned by function */
	
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
	int tmp,ret;
		
	XOPInit(ioRecHandle);							/* do standard XOP initialization */
	SetXOPEntry(XOPEntry);							/* set entry point for future calls */
	
	
	AIO_Init();										// This should called BEFORE any other function in the API
	// to populate the list of Acces devices
	
	ret = AIO_Usb_GetDevices(&aioDevices);
	
	devIdx = aioDevices.aioDevList[0].devIdx;		// use the first device found
	
	// Initialise the Acces DIO board so that pins have a default direction and state
	// (so that air starts to flow through ODD)
	tmp = initialise();
	
	
	if (igorVersion < 200)
		SetXOPResult(REQUIRES_IGOR_200);
	else
		SetXOPResult(0L);
}
