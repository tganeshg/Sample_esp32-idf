#ifndef	_COMMON_H_
#define	_COMMON_H_

#pragma pack(1)

//Macros
#define	TRUE	1
#define	FALSE	0

#define	MAX_MODBUS_SLAVE	32
#define	MAX_PARAMS			64

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

#define	SINGLE	1
#define	DOUBLE	2
//Enums
// Size constants
typedef enum
{
	SIZE_4		=	4,
	SIZE_6		=	6,
	SIZE_8		=	8,
	SIZE_16		=	16,
	SIZE_32		=	32,
	SIZE_64		=	64,
	SIZE_128	=	128,
	SIZE_256	=	256,
	SIZE_512	=	512,
	SIZE_640	=	640,
	SIZE_1024	=	1024,
	SIZE_2048	=	2048,
	SIZE_6144	=	6144
}BUFFER_SIZES;

//Error Code
typedef enum
{
	RET_FAILURE	=	-1,
	RET_OK		=	0
	
}ERROR_CODE;

//Modbus function codes
typedef enum
{
	READ_DISCRETE_COIL		=	1,
	READ_DISCRETE_INPUT		=	2,
	READ_HOLDING_REG		=	3,
	READ_INPUT_REG			=	4,
	WRITE_DISCRETE_COIL		=	5,
	WRITE_HOLDING_REG		=	6,
	WRITE_MUL_DISCRETE_COIL	=	15,
	WRITE_MUL_HOLDING_REG	=	16
}FUNCTION_CODE;

typedef enum
{
	BIT_16,
	BIT_32
}REG_SIZE;

typedef enum
{
	LOW_HIGH_BYTE,
	HIGH_LOW_BYTE
}BYTEE_ORDER;

typedef enum
{
	LOW_HIGH_WORD,
	HIGH_LOW_WORD
}WORD_ORDER;

typedef enum
{
	BOOLEAN_BIT,
	IEEEFLOAT_32BIT,
	UINT_32BIT,
	INT_32BIT,
	UINT_16BIT,
	INT_16BIT
}VALUE_DATA_TYPE;

typedef enum
{
	PERIODIC = 1,
	SCHEDULE = 2
}TIME_SELECT;

typedef enum
{
	RTU 	= 1,
	TCP_IP 	= 2
}SEL_MTR_COM_TYPE;

//Structures
/* Required fields for make a modbus query */
typedef struct
{
	unsigned char	nodeAddr;
	unsigned char	funcCode;
	unsigned short	startAddr;
	unsigned short	noOfRegis;
	BYTEE_ORDER		byteOrder; // if its 1 - normal or swift byte
	WORD_ORDER 		wordOrder; // if its 1 - normal or swift word
	REG_SIZE		regSize;   // if its 0 - 16bit ,1 - 32bit
	VALUE_DATA_TYPE	typeOfValue; // 0 - boolean bit , 1 - IEEE float(32bit), 2 - Uint(32bit), 3 - int(32bit), 4 - Uint(16bit), 5 - int(16bit)
	char			paramName[SIZE_64];
}MODBUS_CONFIG;

#endif
/* EOF */
