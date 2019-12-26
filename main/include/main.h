#ifndef	_MAIN_H_
#define	_MAIN_H_

/*** Includes ***/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "driver/uart.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/apps/sntp.h"
#include "mqtt_client.h"
#include "esp_system.h"
#include "cJSON.h"

#include "common.h"
#include "deviceConfig.h"
#include "modbus.h"
#include "u8g2_esp32_hal.h"

/*** Macros ***/

/* WIFI Credentials */
#define WIFI_SSID				"ganesh"
#define WIFI_PASSWD				"ganesh@234"

#define	WIFI_SS_100P			-30
#define	WIFI_SS_75P				-67
#define	WIFI_SS_50P				-70
#define	WIFI_SS_25P				-80
#define	WIFI_SS_0P				-90

#define	APPLY_INDIAN_TIME_ZONE	FALSE
#define	INDIAN_NTP_SERVER		"0.in.pool.ntp.org"
/* Increment while adding new task */
#define TOTAL_TASK				3

/* Uart Tx and Rx Buffer size */
#define	UART_BUF_SIZE			512

/* Flag's Bit */
	/* WIFI */
#define WIFI_CONNECTED_BIT		BIT0
#define TIME_UPDATED_BIT		BIT1

	/* MQTT */
#define MQTT_CONNECTED_BIT		BIT0
#define MQTT_PORT				1883

/* MQTT Cloud - Ganesh */
#define	GAN_BROKER_URL			"162.255.87.22"

/* MQTT Cloud - AT&T */
#define	ATNT_BROKER_URL			"api-m2x.att.com"
#define	ATNT_API_KEY			"5ddc15fa702eb039c7b05d182d8fb6ef"
#define	ATNT_DEVICE_ID			"a0c46778fd7f97ffad8d1a2f12c5d1b1"
#define	ATNT_CLIENT_ID			"esp32_0001"

/* MQTT Mosquitto free Broker */
#define	MOSQUITTO_BROKER_URL	"test.mosquitto.org"
#define	MOSQUITTO_CLIENT_ID		"esp32_0001_2019"

/* Display */
#define	TOP_LINE_S_X_AXIS		0
#define TOP_LINE_S_Y_AXIS		17
#define TOP_LINE_E_X_AXIS		127
#define TOP_LINE_E_Y_AXIS		17

#define	BOTTOM_LINE_S_X_AXIS	0	
#define BOTTOM_LINE_S_Y_AXIS	46
#define BOTTOM_LINE_E_X_AXIS	127
#define BOTTOM_LINE_E_Y_AXIS	46

#define DSP_X_AXIS_SS			102
#define DSP_Y_AXIS_SS			19
#define DSP_SS_W				26
#define DSP_SS_H				26

#define DSP_WIFI_ST_IP_X		1
#define DSP_WIFI_ST_IP_Y		14

#define DSP_WIFI_TIME_X			1
#define DSP_WIFI_TIME_Y			62

#define DSP_X_AXIS_MQTT			1
#define DSP_Y_AXIS_MQTT			19
#define DSP_MQTT_W				26
#define DSP_MQTT_H				26

/*** Enums ***/
typedef enum
{
	WIFI_STATE_INIT,
	WIFI_STATE_REINIT,
	WIFI_STATE_CONNECT,
	WIFI_STATE_IDLE,
	WIFI_STATE_DO_NOTHIG
}WIFI_STATES;

typedef enum
{
	MOD_STATE_INIT,
	MOD_STATE_READ_QRY,
	MOD_STATE_SEND_QRY,
	MOD_STATE_RCV_RSP,
	MOD_STATE_RSP_PROC,
	MOD_STATE_IDLE,
	MOD_STATE_DO_NOTHIG
}MOD_STATES;

typedef enum
{
	MQTT_STATE_INIT,
	MQTT_STATE_CONNECT,
	MQTT_STATE_PUB,
	MQTT_STATE_SUB,
	MQTT_STATE_MSG_PROC,
	MQTT_STATE_IDLE,
	MQTT_STATE_DO_NOTHIG
}MQTT_STATES;

typedef enum
{
	SNTP_STATE_INIT,
	SNTP_STATE_IDLE,
	SNTP_STATE_DO_NOTHIG
}SNTP_STATES;

/*** Structures ***/

/* General */
typedef struct
{
	uint64_t	sSec;
	uint64_t	eSec;
}nBdelayId;

/* Task */
typedef struct
{
	TaskFunction_t		pTaskFunc;
	const char 			*taskName;
	const uint32_t 		pStackSize;
	void 				*optionalParam;
	UBaseType_t			tPriority;
	TaskHandle_t		*tHandler;
	const BaseType_t 	xCoreID;
}TASK_INFO;

/* WIFI */
typedef struct
{
	WIFI_STATES			state;
	EventGroupHandle_t 	stWifiEventGroup;
	wifi_config_t 		wifiCoreConfig;
	WIFI_ST_CONFIG		wifiStConfig;
}WIFI_CFG_FLOW;

/* UART */
typedef struct
{
	unsigned char		*txBuffer;
	unsigned char		*rxBuffer;
	unsigned int		txBuffLen;
	unsigned int		rxBuffLen;
	uart_config_t 		uartCoreCfg;
	UART_CONFIG			uartConfig;
}UART_BASE_CFG;

typedef struct
{
	unsigned char		enable;
	int					mQryIdx;
	MOD_STATES			state;
}MODBUS_CFG_FLOW;

/* MQTT */
typedef struct
{
	unsigned char				enable;
	MQTT_STATES					state;
	EventGroupHandle_t 			mqttEventGroup;
	esp_mqtt_client_config_t	mqttCoreCfg;
	esp_mqtt_client_handle_t 	mqttClient;
	MQTT_BROKER_CONFIG			mqttCfg;
}MQTT_CFG_FLOW;

/* Display */
typedef struct
{
	unsigned char				enable;
	u8g2_t 						u8g2Handler;
}DSP_CFG_FLOW;

/* SNTP */
typedef struct
{
	unsigned char				enable;
	char						ntpServer[SIZE_64];
	SNTP_STATES					state;
}SNTP_CFG_FLOW;

/* IPC */
typedef struct
{
	uint8_t	pBuffer[SIZE_4096]; //payload buffer
	uint8_t	pTopic[SIZE_128];	//topic
	int		pLen;				//payload length
	int		pQos;				//QOS	
	int		pRetain;			//Retain flag
}MQTT_PAYLOADER;
/* Data or Values transfer among tasks */
typedef struct
{
	bool 			wifiConnected;
	bool 			mqttConnected;
	MQTT_PAYLOADER	mqttPayload;
}IPC_COMM;

/*** Functions Declarations ***/
/* Tasks */
void mainTask(void *arg);
void mUartTask(void *arg);
void mqttTask(void *arg);
void displayTask(void *arg);

/* Public functions */
void delayMs(const TickType_t mSec); // Blocking
int	nBdelay(nBdelayId *dId,uint64_t dlMs); //Non Blocking

/* uart.c */
int uartInit(UART_BASE_CFG * uCfgFlw);
int sendPort(unsigned char *Buffer,unsigned int len);
int	readPort(UART_BASE_CFG * uCfgFlw,unsigned char *Buffer,int *len);

#endif

/* EOF */
