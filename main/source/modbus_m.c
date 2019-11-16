/*******************************************************************************
* File : modbus.c
* Summary : Modbus Protocol
*
*
* Author : Ganesh
*******************************************************************************/
//Include
#include "main.h"
#include "modbus.h"

//Local macro

//Globals
unsigned short			numOfReqParams,tcpTransId,tcpLength=6;
unsigned short 			tmpCrc;
unsigned char			modQueryBuff[UART_BUF_SIZE];			//Formally this is Tx Buffer
unsigned char			readBuff[UART_BUF_SIZE];

static const char		*modbusMTag = "modbusMaster: ";

//Structure variables
extern UART_BASE_CFG	uartCfgFlow;
/* Config from user */
MODBUS_COM_CONFIG		meterComConfig;

MODBUS_CONFIG			modConfig;
MODBUS_DATA				modbusDataValues[MAX_PARAMS];

/**
 *  Function    : initModQuery
 *  Return [out]: Return_Description
 *  Details     : It will assign the values of parameters and construct the modbus query	
 */
int initModQuery(void)
{
	unsigned int index=0;
	unsigned char *ptr=NULL;
	
	memset(modQueryBuff,0,sizeof(modQueryBuff));
	modQueryBuff[index++] = modConfig.nodeAddr;
	modQueryBuff[index++] = modConfig.funcCode;
	
	ptr = (unsigned char *)&modConfig.startAddr;
	modQueryBuff[index++] = ptr[1];
	modQueryBuff[index++] = ptr[0];

	ptr = (unsigned char *)&modConfig.noOfRegis;
	modQueryBuff[index++] = ptr[1];
	modQueryBuff[index++] = ptr[0];
	
	tmpCrc = crc( modQueryBuff,0,(int)index);
	ptr = (unsigned char *)&tmpCrc;
	modQueryBuff[index++] = ptr[1];
	modQueryBuff[index++] = ptr[0];

	if(sendPort(modQueryBuff,index) != RET_OK)
		return RET_FAILURE;
	
	return RET_OK;
}

/**
 *  Function : procModResponce
 *  param [in] : paramIndx Parameter_Description
 *  return [out] : Return_Description
 *  details : Details
 */
int procModResponce(unsigned int paramIndx)
{
	char fnCode=0;
	int  temp =0,ret=0,i=0,j=0;
	unsigned char *ptr=NULL;
	unsigned short locCrc=0,coilInx=0;
	unsigned int index=0;
	short int paramValue1=0;
	unsigned short int paramValue2=0;
	float paramValue3=0.0;
	unsigned long paramValue4=0;
	long int paramValue5=0;
	memset((void *)readBuff,0,sizeof(readBuff));
	
	if(readPort(&uartCfgFlow,readBuff,&ret) != RET_OK)
	{
		ESP_LOGI(modbusMTag, "Responce timeout..!");
		return RET_FAILURE;
	}
	
	ESP_LOGI(modbusMTag, "Received %d",ret);
	
	//check CRC first
	tmpCrc = crc(readBuff,0,(int)(ret-2));
	ptr = (unsigned char *)&locCrc;
	ptr[1] = readBuff[ret-2];
	ptr[0] = readBuff[ret-1];

	if(tmpCrc != locCrc)
	{
		ESP_LOGI(modbusMTag,"CRC Error..!");
		return RET_FAILURE;
	}
	//Check Node address
	index=0;
	//ESP_LOGI(modbusMTag, "Node addr %d",(unsigned char)readBuff[index]);
	index++;

	//Take Functionn Code
	fnCode = (unsigned char)readBuff[index++];
	if(CHECK_BIT(fnCode,8))
	{
		ESP_LOGI(modbusMTag, "Exception %02x",(unsigned char)readBuff[index]);
		return RET_FAILURE;
	}
	
	switch(fnCode)
	{
		case READ_DISCRETE_COIL:
		case READ_DISCRETE_INPUT:
		{
			temp = (int)readBuff[index++];
			ESP_LOGI(modbusMTag, "No#Bytes : %d",temp);
			coilInx = modConfig.startAddr;
			for(i=0;i<temp;i++,index++)
			{
				for(j=0;j<8;j++)
				{
					ESP_LOGI(modbusMTag, "Coil %d : %d",coilInx++,(CHECK_BIT(readBuff[index],j)?1:0));
					if((coilInx - modConfig.startAddr) >= modConfig.noOfRegis)
						return RET_OK;
				}
			}
		}
		break;
		case READ_HOLDING_REG:
		case READ_INPUT_REG:
		{
			temp = (int)readBuff[index++];
			ESP_LOGI(modbusMTag, "No#Bytes : %d",temp);
			if(modConfig.regSize == BIT_32)
			{
				if(modConfig.typeOfValue == IEEEFLOAT_32BIT)
				{
					for(i=0;i<modConfig.noOfRegis;i++)
					{
						ptr = (unsigned char *)&paramValue3;
						if(modConfig.wordOrder == LOW_HIGH_WORD)
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
							}
							else
							{
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
							}
						}
						else
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
							}
							else
							{
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
							}
						}
						ESP_LOGI(modbusMTag, "Reg %d = %f",(modConfig.startAddr+i),paramValue3);
						modbusDataValues[paramIndx].storedType = IEEEFLOAT_32BIT;
						modbusDataValues[paramIndx].modData.floatValue = paramValue3;
					}
				}
				else if(modConfig.typeOfValue == UINT_32BIT)
				{
					for(i=0;i<modConfig.noOfRegis;i++)
					{
						ptr = (unsigned char *)&paramValue4;
						if(modConfig.wordOrder == LOW_HIGH_WORD)
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
							}
							else
							{
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
							}
						}
						else
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
							}
							else
							{
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
							}
						}
						ESP_LOGI(modbusMTag, "Reg %d = %lu",(modConfig.startAddr+i),(unsigned long)paramValue4);
						modbusDataValues[paramIndx].storedType = UINT_32BIT;
						modbusDataValues[paramIndx].modData.unSingned32BitValue = (unsigned long int)paramValue4;
					}
				}
				else if(modConfig.typeOfValue == INT_32BIT)
				{
					for(i=0;i<modConfig.noOfRegis;i++)
					{
						ptr = (unsigned char *)&paramValue5;
						if(modConfig.wordOrder == LOW_HIGH_WORD)
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
							}
							else
							{
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
							}
						}
						else
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
							}
							else
							{
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
							}
						}
						ESP_LOGI(modbusMTag, "Reg %d = %ld",(modConfig.startAddr+i),paramValue5);
						modbusDataValues[paramIndx].storedType = INT_32BIT;
						modbusDataValues[paramIndx].modData.singned32BitValue = (long int)paramValue5;
					}
				}
				else
					ESP_LOGI(modbusMTag, "Type of value incorrect..! 32 bit");
			}
			else
			{
				if(modConfig.typeOfValue == UINT_16BIT)
				{
					for(i=0;i<modConfig.noOfRegis;i++)
					{
						ptr = (unsigned char *)&paramValue2;
						if(modConfig.byteOrder == LOW_HIGH_BYTE)
						{
							ptr[0] = readBuff[index++];
							ptr[1] = readBuff[index++];
						}
						else
						{
							ptr[1] = readBuff[index++];
							ptr[0] = readBuff[index++];
						}
						ESP_LOGI(modbusMTag, "Reg %d = %d",(modConfig.startAddr+i),paramValue2);
						modbusDataValues[paramIndx].storedType = UINT_16BIT;
						modbusDataValues[paramIndx].modData.unSingned16BitValue = (unsigned short int)paramValue2;
					}
				}
				else if(modConfig.typeOfValue == INT_16BIT)
				{
					for(i=0;i<modConfig.noOfRegis;i++)
					{
						ptr = (unsigned char *)&paramValue1;
						if(modConfig.byteOrder == LOW_HIGH_BYTE)
						{
							ptr[0] = readBuff[index++];
							ptr[1] = readBuff[index++];
						}
						else
						{
							ptr[1] = readBuff[index++];
							ptr[0] = readBuff[index++];
						}
						ESP_LOGI(modbusMTag, "Reg %d = %d",(modConfig.startAddr+i),paramValue1);
						modbusDataValues[paramIndx].storedType = INT_16BIT;
						modbusDataValues[paramIndx].modData.singned16BitValue = (short int)paramValue1;
					}
				}
				else if(modConfig.typeOfValue == IEEEFLOAT_32BIT)
				{
					for(i=0;i<(modConfig.noOfRegis/2);i++)
					{
						ptr = (unsigned char *)&paramValue3;
						if(modConfig.wordOrder == LOW_HIGH_WORD)
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
							}
							else
							{
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
							}
						}
						else
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
							}
							else
							{
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
							}
						}
						ESP_LOGI(modbusMTag, "Reg %d = %f",(modConfig.startAddr+(i*2)),paramValue3);
						modbusDataValues[paramIndx].storedType = IEEEFLOAT_32BIT;
						modbusDataValues[paramIndx].modData.floatValue = paramValue3;
					}
				}
				else if(modConfig.typeOfValue == UINT_32BIT)
				{
					for(i=0;i<(modConfig.noOfRegis/2);i++)
					{
						ptr = (unsigned char *)&paramValue4;
						if(modConfig.wordOrder == LOW_HIGH_WORD)
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
							}
							else
							{
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
							}
						}
						else
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
							}
							else
							{
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
							}
						}
						ESP_LOGI(modbusMTag, "Reg %d = %lu",(modConfig.startAddr+(i*2)),(unsigned long)paramValue4);
						modbusDataValues[paramIndx].storedType = UINT_32BIT;
						modbusDataValues[paramIndx].modData.unSingned32BitValue = (unsigned long int)paramValue4;
					}
				}
				else if(modConfig.typeOfValue == INT_32BIT)
				{
					for(i=0;i<(modConfig.noOfRegis/2);i++)
					{
						ptr = (unsigned char *)&paramValue5;
						if(modConfig.wordOrder == LOW_HIGH_WORD)
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
							}
							else
							{
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
							}
						}
						else
						{
							if(modConfig.byteOrder == LOW_HIGH_BYTE)
							{
								ptr[2] = readBuff[index++];
								ptr[3] = readBuff[index++];
								ptr[0] = readBuff[index++];
								ptr[1] = readBuff[index++];
							}
							else
							{
								ptr[3] = readBuff[index++];
								ptr[2] = readBuff[index++];
								ptr[1] = readBuff[index++];
								ptr[0] = readBuff[index++];
							}
						}
						ESP_LOGI(modbusMTag, "Reg %d = %ld",(modConfig.startAddr+(i*2)),paramValue5);
						modbusDataValues[paramIndx].storedType = INT_32BIT;
						modbusDataValues[paramIndx].modData.singned32BitValue = (long int)paramValue5;
					}
				}
				else
					ESP_LOGI(modbusMTag,"Type of value incorrect..! 16 bit %d",modConfig.typeOfValue);
			}
		}
		break;
		default:
			ESP_LOGI(modbusMTag,"Invalid Function code..!");
		break;
	}

	//Parameter name copied from query to responce structure
	strcpy(modbusDataValues[paramIndx].paramName,modConfig.paramName);

	return RET_OK;
}

/**
 *  Function    : crc
 *  Params [in] : "data"  Parameter_Description
 *  Params [in] : "start" Parameter_Description
 *  Params [in] : "cnt"   Parameter_Description
 *  Return [out]: Return_Description
 *  Details     : CRC calculation
 */
unsigned short int crc( unsigned char *data, int start, int cnt)
{
	int         i=0,j=0;
	unsigned    temp,temp2,flag;

	temp=0xFFFF;

	for ( i=start; i<cnt; i++ )
	{
		temp=temp ^ data[i];
		for ( j=1; j<=8; j++ )
		{
			flag=temp & 0x0001;
			temp=temp >> 1;
			if ( flag ) 
				temp=temp ^ 0xA001;
		}
	}

	// Reverse byte order. 
	temp2=temp >> 8;
	temp=(temp << 8) | temp2;
	temp &= 0xFFFF;

	return(temp);
}

/* EOF */
