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
#include <time.h>
#include <math.h>
#include <pthread.h>						// for nanosleep() call

#include "aioUsbApi.h"
#include "XOPStandardHeaders.h"				// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h
#include "XFUNC3.h"
#include "XOPStructureAlignmentTwoByte.h"	// All structures passed to Igor are two-byte aligned.
#include "XOPStructureAlignmentReset.h"


int				devIdx;
char			anyKey;
int				ctlC;

int				ret;
int				tmp;
int				i;
int				stimTime;
int				delayTime;
int				odour;
int				doIHaveAnError;
int				triggerTimeout;

char			configFile[80];
char			logFile[80];
char			cfg[20];
char			lg[20];


//For the ACCES API calls
unsigned char	mask[2]; 
unsigned char	data[12];
int				triState; 
unsigned char	pins0_7;
unsigned char	pins8_15;
unsigned char	pins16_23;
unsigned char	pins24_31; 
int				p0_7Input;
int				p8_15Input;
int				p16_23Input;
int				p24_31Input;
unsigned char   byte;
unsigned int	byteIdx;


//Pointers to logfile and configfile
FILE* fi; 
FILE* fo; 


//Values to be read from config file
char ch, s[80], chID1[10], chID2[10], chID3[10], chID4[10], chID5[10];
int d1,p1,o1,d2,p2,o2,d3,p3,o3,d4,p4,o4,d5,p5,o5;


//Functions
int triggerDetect();
int triggerDetectFast();
int triggerDetectFaster();
int odourPulse(int delay, int odour, int duration);
int odourPulsesSimple(int delay, int odour, int duration);
int odourPulses(char *cfgFileName);
int oddRunTest();
int validateIndex(int devIdx);
int initialise();


//static void MyHello(void);				//Probably don't need
void catchInterrupt (int signum);		//Before this was an XOP, could abort with ctrl-c

//TODO: Figure out why I get an implicitly defined function wraning despite the fact that this definition is in aioUsbApi.h
unsigned long
AIO_Usb_DIO_ReadTrigger (unsigned long   devIdx,
						 unsigned char  *pData,
						 int trgTO);

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


void									//Old
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
		tmp = odourPulses(str);				//This allows a default value to be set
		
		if (tmp==10) {
			XOPNotice("\015ERROR: Could not open config file. Try again ya wee lass.\015");
		}
		if (tmp==11) {
			XOPNotice("\015ERROR: Could not open log file. Does the directory path exist?\015");
		}
		
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
initialise()						//Just sets up the board for our use: all but one byte to be used as output
{	
	unsigned char  mask[2]; 
//	unsigned short  mask; 
//	unsigned char  data[4];			//for DIO_32
	unsigned char  data[12];		//for DIO_96
	int            triState; 
	pins0_7 = 1;
	p0_7Input = 1;
	
	pins8_15 = 1;
	p8_15Input = 1;
	
	pins16_23 = 1;
	p16_23Input = 1;
	
	pins24_31 = 0;
	p24_31Input = 0;

	
	
/*	
	mask = 0;
	mask = pins24_31 << 11;			//these are shifted HEX values
	mask = mask | pins24_31 << 10; 
	mask = mask | pins24_31 << 9; 
	mask = mask | pins16_23 << 8; 
	mask = mask | pins16_23 << 7; 
	mask = mask | pins16_23 << 6; 
	mask = mask | pins16_23 << 5; 
	mask = mask | pins16_23 << 4; 
	mask = mask | pins16_23 << 3; 
	mask = mask | pins16_23 << 2; 
	mask = mask | pins8_15 << 1; 
	mask = mask | pins0_7; 

*/	
	mask[0]=0xFF;	//sets the first 8 ports to output
	mask[1]=0x0;	//sets the rest of the board for input
	
	triState= 0;
	
	if (p0_7Input == 1)
	{
		tmp = pow(2,0);
		data[0] = tmp;
	}
	
	if (p8_15Input == 1)
	{
		tmp = 0;
		data[1] = tmp;
	}
	
	
	if (p16_23Input == 1)
	{
		tmp = 0;
		data[2] = tmp;
	}
	
	if (p24_31Input == 1)
	{
		tmp = 0;
		data[3] = tmp;
	}
	
/*	data[0]=0x11;
	data[1]=0x22;
	data[2]=0x33;
	data[3]=0x44;
	data[4]=0x55;
	data[5]=0x66;
	data[6]=0x77;
	data[7]=0x88;
	data[8]=0x99;
	data[9]=0xaa;
	data[10]=0xbb;
	data[11]=0xcc;
*/	
	data[0]=0x01;
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
	
	ret =   AIO_Usb_DIO_Configure (devIdx,
								   triState,
								   mask,
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
	mask = 0;
	mask = pins16_23 << 11;			//these are shifted HEX values
	mask = mask | pins16_23 << 10; 
	mask = mask | pins16_23 << 9; 
	mask = mask | pins16_23 << 8; 
	mask = mask | pins16_23 << 7; 
	mask = mask | pins16_23 << 6; 
	mask = mask | pins16_23 << 5; 
	mask = mask | pins16_23 << 4; 
	mask = mask | pins16_23 << 3; 
	mask = mask | pins16_23 << 2; 
	mask = mask | pins8_15 << 1; 
	mask = mask | pins0_7; 
*/
	
	mask[0]=0xFF;	//sets the first 8 ports to output
	mask[1]=0x0;	//sets all remaining ports to input
		
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
	
//TODO: Change this to depend on the number of lines in the .odd file
	for(i=1;i<=13;i++)
	{
		fgets(s,80,fi);
		sscanf(s,"%s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",(char*)chID1, &d1, &p1, &o1, &d2, &p2, &o2, &d3, &p3, &o3, &d4, &p4, &o4, &d5, &p5, &o5);
		
		if (p1==0) {
			XOPNotice("\015The first entry in this line is for zero duration. Not waiting for a trigger.\015");
			fprintf(fo, "\nThe first entry in line %d is for zero duration. Not waiting for a trigger and not executing sequence:\n%s",i,s);
		}		
		else
		{
			XOPNotice("\015I found an entry in the .odd file. Can I please have a trigger?\015");
			fprintf(fo, "\nNonzero p1 detected. Running line %d: %s",i,s);
			tmp=triggerDetectFaster();	
			//tmp=triggerDetectFast();	
			//tmp=triggerDetect();	
			if (tmp==10) {
				XOPNotice("\015Trigger detected. Executing protocol...");
				fprintf(fo, "Trigger detected. Executing sequence.\n");
				//TODO: force error message to appear reliably. 				
			}else if (tmp==15) {
				XOPNotice("\015TriggerDetect() timed out.");
				fprintf(fo, "TriggerDetect() timed out. Aborting");
				fclose(fi);fclose(fo);

				return(0);
			}else if (tmp==0) {
				XOPNotice("\015TriggerDetect() failed.");
				fprintf(fo, "TriggerDetect() failed for some reason besides timeout.");
				fclose(fi);fclose(fo);

				return(0);
			}else {
				XOPNotice("\015TriggerDetect() timed out.");
				fprintf(fo, "\nTime dout with counter = %d\n",tmp);
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
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					
//					ret =   AIO_Usb_DIO_Configure (devIdx,
//												   triState,
//												   mask,
//												   data);

					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>7&&odour<16) {					
					data[0]=0;
					data[1]=pow(2,odour-8);
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>15&&odour<24) {					
					data[0]=0;
					data[1]=0;
					data[2]=pow(2,odour-16);
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>23&&odour<32) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=pow(2,odour-24);
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>31&&odour<40) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=pow(2,odour-32);
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>39&&odour<48) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=pow(2,odour-40);
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>47&&odour<56) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=pow(2,odour-48);
					//data[6]=pow(2,odour-48);
					//data[6]=0;//x05; //won't write 0x2 or 0x4 to port 6
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>55&&odour<64) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=pow(2,odour-56);
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else{
					fprintf(fo,"\nERROR: you've asked for an odour that I can't provide. I'm quitting");
					XOPNotice("\015ERROR: you've asked for an odour that I can't provide. I'm quitting");
					fclose(fi);fclose(fo);
					return(0);
				}
				fprintf(fo, "Applied odour %d for %dms, after a %dms delay\n",odour,stimTime,delayTime);
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
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>7&&odour<16) {					
					data[0]=0;
					data[1]=pow(2,odour-8);
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>15&&odour<24) {					
					data[0]=0;
					data[1]=0;
					data[2]=pow(2,odour-16);
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>23&&odour<32) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=pow(2,odour-24);
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>31&&odour<40) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=pow(2,odour-32);
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>39&&odour<48) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=pow(2,odour-40);
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>47&&odour<56) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					//data[6]=pow(2,odour-48);
					data[6]=pow(2,odour-48);
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>55&&odour<64) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=pow(2,odour-56);
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else{
					fprintf(fo,"\nERROR: you've asked for an odour that I can't provide. I'm quitting");
					XOPNotice("\015ERROR: you've asked for an odour that I can't provide. I'm quitting");
					fclose(fi);fclose(fo);
					return(0);
				}
				fprintf(fo, "Applied odour %d for %dms, after a %dms delay\n",odour,stimTime,delayTime);
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
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>7&&odour<16) {					
					data[0]=0;
					data[1]=pow(2,odour-8);
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>15&&odour<24) {					
					data[0]=0;
					data[1]=0;
					data[2]=pow(2,odour-16);
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>23&&odour<32) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=pow(2,odour-24);
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>31&&odour<40) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=pow(2,odour-32);
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>39&&odour<48) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=pow(2,odour-40);
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>47&&odour<56) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=pow(2,odour-48);
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					ret =   AIO_Usb_DIO_Configure (devIdx,
												   triState,
												   mask,
												   data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>55&&odour<64) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=pow(2,odour-56);
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else{
					fprintf(fo,"\nERROR: you've asked for an odour that I can't provide. I'm quitting");
					XOPNotice("\015ERROR: you've asked for an odour that I can't provide. I'm quitting");
					fclose(fi);fclose(fo);
					return(0);
				}
				fprintf(fo, "Applied odour %d for %dms, after a %dms delay\n",odour,stimTime,delayTime);
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
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>7&&odour<16) {					
					data[0]=0;
					data[1]=pow(2,odour-8);
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>15&&odour<24) {					
					data[0]=0;
					data[1]=0;
					data[2]=pow(2,odour-16);
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>23&&odour<32) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=pow(2,odour-24);
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>31&&odour<40) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=pow(2,odour-32);
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>39&&odour<48) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=pow(2,odour-40);
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>47&&odour<56) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=pow(2,odour-48);
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>55&&odour<64) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=pow(2,odour-56);
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else{
					fprintf(fo,"\nERROR: you've asked for an odour that I can't provide. I'm quitting");
					XOPNotice("\015ERROR: you've asked for an odour that I can't provide. I'm quitting");
					fclose(fi);fclose(fo);
					return(0);
				}
				fprintf(fo, "Applied odour %d for %dms, after a %dms delay\n",odour,stimTime,delayTime);
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
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>7&&odour<16) {					
					data[0]=0;
					data[1]=pow(2,odour-8);
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>15&&odour<24) {					
					data[0]=0;
					data[1]=0;
					data[2]=pow(2,odour-16);
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>23&&odour<32) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=pow(2,odour-24);
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>31&&odour<40) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=pow(2,odour-32);
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>39&&odour<48) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=pow(2,odour-40);
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>47&&odour<56) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=pow(2,odour-48);
					//data[6]=0x2;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else if (odour>55&&odour<64) {					
					data[0]=0;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=pow(2,odour-56);
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
					usleep(1000*stimTime);
					data[0]=1;
					data[1]=0;
					data[2]=0;
					data[3]=0;
					data[4]=0;
					data[5]=0;
					data[6]=0;
					data[7]=0;
					data[8]=0;
					data[9]=0;
					data[10]=0;
					data[11]=0;
					
					//					ret =   AIO_Usb_DIO_Configure (devIdx,
					//												   triState,
					//												   mask,
					//												   data);
					
					ret =   AIO_Usb_WriteAll (devIdx,
											  data);
					
				}else{
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
	XOPNotice("\015Closing files.....");
	fclose(fi);fclose(fo);
	XOPNotice("OK\015");
	return(1);
}


/*
int 
odourPulse(int delay, int odour, int duration)		//For now this is unused. It helps for testing so I'll just comment it out. 
{
	
	
	int			 ret;
	int			 i;
	int			 stimTime;
	int			 delayTime;
	
	unsigned char  pins0_7;
	unsigned char  pins8_15;
	unsigned char  pins16_23;
	unsigned char  pins24_31; 
	
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
*/

/*
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
*/


int 
triggerDetectFast()			//I thought that this triggerDetect() would be faster since is calls AIO_Usb_DIO_Read8()
{							//HOWEVER, AIO_Usb_DIO_Read8() just uses AIO_Usb_DIO_ReadAll() anyway, so in fact this is slower
	int						temp;
	time_t					startTime;
	unsigned char  byte;
	byte = 0;
	
	startTime = time(NULL);
	temp = 0;
	
	
	
	
	
	
	
	
	
	while (temp == 0&&time(NULL)<=startTime+triggerTimeout) {
		ret = AIO_Usb_DIO_Read8 (devIdx,1,&byte);		//This is strange. The sequence goes 2,3,0,1, so the 
		//last byte (DIO DXX has index 1 instead of 3 as you'd expect
		//form: ret = AIO_Usb_DIO_Read8 (devIdx,byteIdx,&byte);
		fprintf(fo," \nPINs 24-32, using read8: 0x%x\n",(unsigned char)byte);
		temp=(unsigned char)byte;
	}
	
	
	if (time(NULL)>=startTime+triggerTimeout) {
		XOPNotice("\015Trigger timeout. Try to do better.\015");
		return(15);
	}else {
		fprintf(fo,"\nTrigger PINs 24-32, using AIO_USB_read8: 0x%x\n",(unsigned char)byte);
		XOPNotice("\015Trigger detected. Here we go....\015");
		return(10);
	}
	
}


int						//This triggerDetect() is OK, but it calls AIO_Usb_DIO_ReadAll(), which is pretty slow. 
triggerDetect()			//Count on a 100ms trigger pulse at least
{
	
    int						difference;
	time_t					temp;
	int						startTime;
	
	startTime = time(NULL);
	
	
	ret = validateIndex(devIdx);
	if (ret > ERROR_SUCCESS)
		return(0);
	
	ret =   AIO_Usb_DIO_ReadAll (devIdx,(unsigned char *)&data[0]); 
	if (ret > ERROR_SUCCESS)
	{
        fprintf (fo,"\n\nReadAll Failed dev=0x%0x err=%d  \n\n",(unsigned int)devIdx,ret);
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
	
}
//TODO: Standardise the return values. These ones are kinda crazy



int 
triggerDetectFaster()		//This triggerDetect calls a function AIO_Usb_DIO_ReadTrigger() that I added to the API
{							//It is sufficiently fast
	ret = validateIndex(devIdx);
	if (ret > ERROR_SUCCESS)
		return(0);

	//Only for testing the 96!!!!! Delete this later
	//return(10);
	
	
	
	XOPNotice("\015Attempting to use the fast trigger loop...");
	ret =   AIO_Usb_DIO_ReadTrigger (devIdx,(unsigned char *)&data[0],triggerTimeout); 
	
	
	
	if (ret > ERROR_SUCCESS)
	{
        fprintf (fo,"\n\nReadAll Failed dev=0x%0x err=%d  \n\n",(unsigned int)devIdx,ret);
        return(0);
	}
	
	XOPNotice("...that worked. I'm back from the fast trigger loop\015");

	if (ret==15) {
		XOPNotice("Trigger timeout. Try to do better.\015");
	}else if (ret==10) {
		XOPNotice("Trigger detected. Here we go....\015");
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
		
	pthread_t pulseThread;	// this is our thread identifier, used to call odourPulses() from its own thread
	
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
