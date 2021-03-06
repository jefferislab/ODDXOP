/*
	XFUNC3.h -- equates for XFUNC3 XOP
*/

/* XFUNC3 custom error codes */

#define REQUIRES_IGOR_200 1 + FIRST_XOP_ERR
#define UNKNOWN_XFUNC 2 + FIRST_XOP_ERR
#define MISSING_INPUT_PARAM 3 + FIRST_XOP_ERR
#define CANT_ACCESS_ACCES 4 + FIRST_XOP_ERR
#define ACCES_STILL_RUNNING 5 + FIRST_XOP_ERR

/* Prototypes */
HOST_IMPORT void main(IORecHandle ioRecHandle);
