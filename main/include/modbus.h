#ifndef _MODBUS_H_
#define _MODBUS_H_

#include "common.h"

/*
*Macros
*/
/*
*Structure
*/
/* For generic MODBUS meter */
typedef union
{
	short int				singned16BitValue;
	unsigned short int		unSingned16BitValue;
	long int				singned32BitValue;
	unsigned long int		unSingned32BitValue;
	float					floatValue;
	char					unused[SIZE_256];
}MODDATA;

typedef struct
{
	unsigned char			storedType;
	MODDATA					modData;
	char					paramName[SIZE_64];
}MODBUS_DATA;

/* Modbus Data to shared Memory */
typedef struct
{
	unsigned int	numOfParams; 				// this is the value out of 'MAX_PARAMS'
	unsigned char 	meterCommStatus; 			//0 - communication with meter is failed , 1- success
	MODBUS_DATA		modDataValues[MAX_PARAMS];
}MODBUS_DB_DATA;

/*
*Function declarations
*/
int initModQuery(void);
unsigned short int crc( unsigned char *data, int start, int cnt);
int procModResponce(unsigned int paramIndx);

#endif
/* EOF */
