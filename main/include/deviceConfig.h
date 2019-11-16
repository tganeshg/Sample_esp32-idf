#ifndef	_D_CONFIG_H_
#define	_D_CONFIG_H_

#include "common.h"

/* For WIFI */
typedef struct
{
	char	ssid[SIZE_32];
	char	passWd[SIZE_64];
	char	cIpAddr[SIZE_16];
}CLIENT_CONFIG;

typedef struct
{
	unsigned char	enable;
	CLIENT_CONFIG	clientConf;
}WIFI_ST_CONFIG;

/* For UART */
typedef struct
{
	unsigned char	dataBits;
	unsigned char	parity;
	unsigned char	stopBits;
	unsigned char	flowCntrl;
	unsigned char	rxFlowCtrlThresh;
	int				baudRate;
    unsigned char   infMode;		// RS232 /485 //not used
	unsigned long	rxThreshold;	//timeout in uart receive,if its is 0,timeout need to calculate automatically as per baudrate.
}UART_CONFIG;

typedef struct
{
	unsigned char	enable;
	unsigned int	numOfMtsConnected;		//This is out of 'MAX_MODBUS_SLAVE'
	unsigned int	numOfQry;				//This is out of 'MAX_PARAMS'
	MODBUS_CONFIG	mQryCfg[MAX_PARAMS];
}MODBUS_COM_CONFIG;

/* For MQTT */
typedef struct
{
	unsigned char		enable;
	char				userName[SIZE_64];
	char				passWord[SIZE_64];
	char				clientId[SIZE_64];
	char 				brokerIP[SIZE_128];		//Fill broker IP or URL
	unsigned short  	brokerPort;				//Fill broker port ,default : 1883
	unsigned int		keepAlive;				//in sec
}MQTT_BROKER_CONFIG;							//Broker Configuration

#endif
/* EOF */
