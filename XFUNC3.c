/*	XFUNC3.c -- illustrates Igor external string functions.
 
 
 This XOP enables Igor Pro to operate the ACCES DIO cards and operate the solenoid valved on the ODD modules.
 
 
 
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <pthread.h>				// for nanosleep() call

#include "aioUsbApi.h"
#include "XOPStandardHeaders.h"		// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "XFUNC3.h"
#include "XOPStructureAlignmentTwoByte.h"	// All structures passed to Igor are two-byte aligned.
#include "XOPStructureAlignmentReset.h"


int				devIdx;
char			anyKey;
int				ctlC;


int				ret;
int				tmp;
int				i;
//int				hangTime;
int				stimTime;
int				delayTime;
int				odour;
int				doIHaveAnError;
int				triggerTimeout;

char			configFile[80];
char			logFile[80];
char			cfg[20];
char			lg[20];


unsigned char	pins0_7;
unsigned char	pins8_15;
unsigned char	pins16_23;
unsigned char	pins24_31; 
int				p0_7Input;
int				p8_15Input;
int				p16_23Input;
int				p24_31Input;

unsigned char	mask; 
unsigned char	data[4];
int				triState; 

//Values to be read from config file
char ch, s[80], chID1[10], chID2[10], chID3[10], chID4[10], chID5[10];
int d1,p1,o1,d2,p2,o2,d3,p3,o3,d4,p4,o4,d5,p5,o5;


//Functions
int triggerDetect();
int odourPulse(int delay, int odour, int duration);
int odourPulsesSimple(int delay, int odour, int duration);
int odourPulses(char *cfgFileName);
int oddRunTest();
int validateIndex(int devIdx);
int initialise();

static void MyHello(void);
void catchInterrupt (int signum);


struct xstrcatParams  {
	Handle str3;
	Handle str2;
	Handle result;
};

aioDeviceInfo aioDevices;


typedef struct xstrcatParams xstrcatParams;


//////////////////////////////////////////////////
//My Functions
//////////////////////////////////////////////////

int 
oddRunTest()
{
	XOPNotice("\015 Testing...\015");
	return(0);
}


static void
MyHello(void)
{
	XOPNotice("Hello. Now you're calling a function...\015");
	doIHaveAnError=oddRunTest();
	if (doIHaveAnError==0) {
		XOPNotice("That should have worked. \015");

	}else{
		XOPNotice("\015Massive failure...\015");
	}
	
}


void
catchInterrupt (int signum) 
{
    ctlC = 1;
}


int
validateIndex(int devIdx)
{
	//	int ret;
	
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
	
	XOPNotice("\015New thread started.");
	//XOPNotice(cfg);
	
	if(strlen(cfg)<1)
	{
		XOPNotice("\015No config file entered. Let's use the default:\015");
		XOPNotice("\015TODO: output the .odd file\015");
		tmp = odourPulses(str);				//works
		
		
		
		
	}else {
		
		XOPNotice("\015Config file entered. Let's use it.\015");
		//XOPNotice("\015TODO: output the .odd file\015");
		XOPNotice(cfg);
		tmp = odourPulses(cfg);				//works
		
		if (tmp==10) {
			XOPNotice("\015ERROR: Could not open config file. Try again ya wee lass.\015");
		}
		if (tmp==11) {
			XOPNotice("\015ERROR: Could not open log file. Does the directory path exist?\015");
		}
		
		
	}
	
	return NULL;
}



int 
initialise()
{
	
	printf("For testing. This function just tests each channel");
	
	
	/*	
	 int			 ret;
	 int			 tmp;
	 
	 unsigned char  pins0_7;
	 unsigned char  pins8_15;
	 unsigned char  pins16_23;
	 unsigned char  pins24_31; 
	 int            p0_7Input;
	 int            p8_15Input;
	 int			 p16_23Input;
	 int			 p24_31Input;
	 */	
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


int 
odourPulses(char *cfgFileName)		//Main function. The others are mostly just for testing, but I left them in case they are of some use in the future
{
	
	/*	
	 
	 int			 ret;
	 int			 tmp;
	 int			 i;
	 int			 stimTime;
	 int			 delayTime;
	 int          odour;
	 
	 
	 unsigned char  pins0_7;
	 unsigned char  pins8_15;
	 unsigned char  pins16_23;
	 unsigned char  pins24_31; 
	 //	int            p0_7Input;
	 //	int            p8_15Input;
	 //	int			   p16_23Input;
	 //	int			   p24_31Input;
	 
	 */ 
	
//	unsigned char  mask; 
//	unsigned char  data[4];
	//	int            triState; 
	
	mask = 0;
	mask = pins24_31 << 3;;			//these are shifted HEX values
	mask = mask | pins16_23 << 2; 
	mask = mask | pins8_15 << 1; 
	mask = mask | pins0_7; 
	
	
	//	triState = 0;
	
	
	strcpy(configFile,"/Users/ahodge/Desktop/");
	strcat(configFile,cfgFileName);
	
	XOPNotice("\015Using the config file:\015");
	XOPNotice(configFile);
	XOPNotice("\015\015");
	
	strcpy(logFile,"/Users/ahodge/Desktop/logfiles/");
	if (strlen(lg)<1) {
		strcat(logFile,"defaultLogFile.txt");
	}else{
		strcat(logFile,(char*)lg);
	}
	
	XOPNotice("\015Using the log file:\015");
	XOPNotice(logFile);
	XOPNotice("\015\015");
	//time
	time_t sec;
	sec = time(NULL);
	//printf("%ld",sec);
	
	//printf("\nCreating file pointers...");
	XOPNotice("\015Creating file pointers...");
	FILE* fi; 
	FILE* fo; 
	
	//printf("done");
	XOPNotice("done");
	
	
	//printf("\nOpening files...");
	XOPNotice("\015Opening files...");
	
	if((fi=fopen(configFile,"r"))==NULL) {										// open cfg file and return with an error value if fopen fails
		return(10);		
	}
	
	if((fo=fopen(logFile,"w"))==NULL) {	// open log file and return with an error value if fopen fails
	//if((fo=fopen("/Users/ahodge/Desktop/logfiles/tempLog.txt","w"))==NULL) {	// open log file and return with an error value if fopen fails
		return(11);
	}
	fprintf(fo, "%s", logFile);
	
	XOPNotice("done");
	
	
	triggerTimeout=5;

	
	//TODO: Change this to depend on the number of lines in the .odd file
	for(i=0;i<=10;i++)
	{
		
		fgets(s,80,fi);
		//	printf("Read...\n");
		//	printf("%s",s);
		//	fprintf(fo,"%s",s);
		sscanf(s,"%s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",(char*)chID1, &d1, &p1, &o1, &d2, &p2, &o2, &d3, &p3, &o3, &d4, &p4, &o4, &d5, &p5, &o5);
		//sscanf(s,"%s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",&chID1, &d1, &p1, &o1, &d2, &p2, &o2, &d3, &p3, &o3, &d4, &p4, &o4, &d5, &p5, &o5);

		
		
		if (p1==0) {
			XOPNotice("\015The first entry in this line is for zero duration. Not waiting for a trigger.\015");
			fprintf(fo, "\nThe first entry in line %d is for zero duration. Not waiting for a trigger.",i);
		}
		
		else
		{
			XOPNotice("\015I found an entry in the .odd file. Can I please have a trigger?\015");
			fprintf(fo, "\nNonzero p1 detected. Running line %d: %s",i,s);
			tmp=triggerDetect();	
			if (tmp==10) {
				XOPNotice("\015Trigger detected. Executing protocol...");
				fprintf(fo, "\nTrigger detected. Executing sequnce.");
				//TODO: force error message to appear reliably. 				
			}else if (tmp==15) {
				XOPNotice("\015TriggerDetect() timed out.");
				fprintf(fo, "\nTriggerDetect() timed out. Aborting");
				fclose(fi);fclose(fo);

				return(0);
			}else if (tmp==0) {
				XOPNotice("\015TriggerDetect() failed.");
				fprintf(fo, "\nTriggerDetect() failed for some reason besides timeout.");
				fclose(fi);fclose(fo);

				return(0);
			}
			//TODO: if/then for triggerDetect() return value

			triggerTimeout=20;

			
			stimTime=p1;
			odour=o1;
			delayTime=d1;
			
			if (stimTime!=0) {
				
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
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
				}else if (odour>7&&odour<16) {
					
					data[0]=0;
					data[1]=pow(2,odour-8);
					data[2]=0;
					data[3]=0;
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
				}else if (odour>15&&odour<24) {
					
					data[0]=0;
					data[1]=0;
					data[2]=pow(2,odour-16);
					data[3]=0;
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
				}else{
					printf("ERROR: Invalid odour");
					return(0);
				}
			}
			stimTime=p2;
			odour=o2;
			delayTime=d2;
			
			if (stimTime!=0) {
				
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
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
				}else if (odour>7&&odour<16) {
					
					data[0]=0;
					data[1]=pow(2,odour-8);
					data[2]=0;
					data[3]=0;
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
				}else if (odour>15&&odour<24) {
					
					data[0]=0;
					data[1]=0;
					data[2]=pow(2,odour-16);
					data[3]=0;
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
				}else{
					printf("ERROR: Invalid odour");
					return(0);
				}
			}
			stimTime=p3;
			odour=o3;
			delayTime=d3;
			
			if (stimTime!=0) {
				
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
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
				}else if (odour>7&&odour<16) {
					
					data[0]=0;
					data[1]=pow(2,odour-8);
					data[2]=0;
					data[3]=0;
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
				}else if (odour>15&&odour<24) {
					
					data[0]=0;
					data[1]=0;
					data[2]=pow(2,odour-16);
					data[3]=0;
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
				}else{
					printf("ERROR: Invalid odour");
					return(0);
				}
			}
			stimTime=p4;
			odour=o4;
			delayTime=d4;
			
			if (stimTime!=0) {
				
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
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
				}else if (odour>7&&odour<16) {
					
					data[0]=0;
					data[1]=pow(2,odour-8);
					data[2]=0;
					data[3]=0;
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
				}else if (odour>15&&odour<24) {
					
					data[0]=0;
					data[1]=0;
					data[2]=pow(2,odour-16);
					data[3]=0;
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
				}else{
					printf("ERROR: Invalid odour");
					return(0);
				}
			}
			stimTime=p5;
			odour=o5;
			delayTime=d5;
			
			if (stimTime!=0) {
				
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
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
				}else if (odour>7&&odour<16) {
					
					data[0]=0;
					data[1]=pow(2,odour-8);
					data[2]=0;
					data[3]=0;
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
				}else if (odour>15&&odour<24) {
					
					data[0]=0;
					data[1]=0;
					data[2]=pow(2,odour-16);
					data[3]=0;
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   &mask,
												   data);
					
				}else{
					printf("ERROR: Invalid odour");
					
					return(0);
				}
				
			}
			XOPNotice("OK, Done that.\015");
			
		}
		
	}
	
	XOPNotice("\015Closing files.....");
	fclose(fi);fclose(fo);
	XOPNotice("OK\015");
	return(1);
	
}



int 
odourPulse(int delay, int odour, int duration)		//For now this is unused. It helps for testing. 
{
	
	
	int			 ret;
	int			 i;
	int			 stimTime;
	int			 delayTime;
	
	
	unsigned char  pins0_7;
	unsigned char  pins8_15;
	unsigned char  pins16_23;
	unsigned char  pins24_31; 
	//	int            p0_7Input;
	//	int            p8_15Input;
	//	int			   p16_23Input;
	//	int			   p24_31Input;
	
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
		//return(1);
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
		//return(1);
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
		//return(1);
	}else{
		printf("ERROR: Invalid odour");
		return(0);
	}
	return(1);
}



int 
odourPulsesSimple(int delay, int odour, int duration)		//For now this is unused. It helps for testing.
{
	
	
	
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
		//return(1);
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
		//return(1);
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
		//return(1);
	}else{
		printf("ERROR: Invalid odour");
		return(0);
	}
	return(1);
}







int 
triggerDetect()
{
	
//	int						ret;
//	unsigned char           data[4];
    int						difference;
	int						temp;
	int						startTime;
	
	startTime = time(NULL);
	
	
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
	while (difference == 0&&time(NULL)<=startTime+triggerTimeout) {
		temp = data[3];
		ret =   AIO_Usb_DIO_ReadAll (devIdx,(unsigned char *)&data[0]); 
		difference = temp - data[3];			
		
	}
	
	if (time(NULL)>=startTime+triggerTimeout) {
		XOPNotice("\015Trigger timeout. Try to do better.\015");
		return(15);
	}else {
		XOPNotice("\015Trigger detected. Here we go....\015");
		return(10);
	}

	
	
	//XOPNotice("\015Trigger detected. Executing stimulus protocol...");
	//usleep(100);
	
	// first 2 bytes unused for AI 16
	//printf("\n PINs 0- 7: = 0x%x\n",data[0]);
	//printf("\n PINs 0-15: = 0x%x\n",data[1]);
	//printf("\n PINs 16-23: = 0x%x\n",data[2]);
	//printf("\n PINs 24:32: = 0x%x\n",data[3]);
	
	//	XOPNotice("Trigger detected. Executing protocol...");
	
	
	
	
}



//typedef struct xstrcatParams xstrcatParams;
//#include "XOPStructureAlignmentReset.h"

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
	GetCStringFromHandle(p->str2, cfg, 20);
	GetCStringFromHandle(p->str3, lg, 20);
	
/*	
	MyHello();
	XOPNotice("MyHello OK\015");
	tmp = initialise();
	XOPNotice("initialise OK\015");
	
	//TODO: Handle errors here	
	
*/	
/*	
	char *stringThing;
	stringThing=(char*)cfg;
	
	XOPNotice("\015str = ");
	XOPNotice(stringThing);
	XOPNotice("\015");
*/	
	
	pthread_t pth;	// this is our thread identifier, used to call odourPulses() from its own thread
	
	//pthread_create(&pth,NULL,threadFunc,"foo");			//Original
	pthread_create(&pth,NULL,threadFunc,"cfgFile.odd");		//GOOD: sets cfgFile.odd as the default config file in case there is no input
	pthread_detach(pth);									//Don't want the original thread to wait for odourPulses() to finish
	
	
	//	printf("main waiting for thread to terminate...\n");	//Old
	//	pthread_join(pth,NULL);									//Old
	
	
	
	//tmp = odourPulses(*p->str2);
	//	tmp = odourPulses(cfg);				//works  //Disable for testing threads
	//tmp = odourPulses("cfgFile.odd");		//works
	//	XOPNotice("\015odourPulses OK\015");
	
	
	
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
	XOPInit(ioRecHandle);							/* do standard XOP initialization */
	SetXOPEntry(XOPEntry);							/* set entry point for future calls */
	
	
	AIO_Init();										// This should called BEFORE any other function in the API
	// to populate the list of Acces devices
	
	ret = AIO_Usb_GetDevices(&aioDevices);
	
	devIdx = aioDevices.aioDevList[0].devIdx;		// use the first device found
	
	tmp = initialise();
	
	
	
	if (igorVersion < 200)
		SetXOPResult(REQUIRES_IGOR_200);
	else
		SetXOPResult(0L);
}
