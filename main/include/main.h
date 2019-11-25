#ifndef	_MAIN_H_
#define	_MAIN_H_

/*** Includes ***/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

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
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "mqtt_client.h"
#include "cJSON.h"

#include "common.h"
#include "deviceConfig.h"
#include "modbus.h"
#include "u8g2_esp32_hal.h"

/*** Macros ***/

/* WIFI Credentials */
#define WIFI_SSID			"ganesh"
#define WIFI_PASSWD			"ganesh@234"

/* Increment while adding new task */
#define TOTAL_TASK				3

/* Uart Tx and Rx Buffer size */
#define	UART_BUF_SIZE			512

/* Flag's Bit */
	/* WIFI */
#define WIFI_CONNECTED_BIT		BIT0
	/* MQTT */
#define MQTT_CONNECTED_BIT		BIT0
#define MQTT_PORT				1883

/* MQTT Cloud - Ganesh */
#define	GAN_BROKER_URL			"162.255.87.22"

/* MQTT Cloud - AT&T */
#define	ATNT_BROKER_URL			"api-m2x.att.com"
#define	ATNT_API_KEY			"0569fed2d45691932babd1dccc81e672"
#define	ATNT_DEVICE_ID			"a0c46778fd7f97ffad8d1a2f12c5d1b1"
#define	ATNT_CLIENT_ID			"esp32_0001"


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

/*** Structures ***/
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

/*** Functions Declarations ***/
/* Tasks */
void wifiTask(void *arg);
void mUartTask(void *arg);
void mqttTask(void *arg);
void displayTask(void *arg);

/* Public functions */
/* uart.c */
int uartInit(UART_BASE_CFG * uCfgFlw);
int sendPort(unsigned char *Buffer,unsigned int len);
int	readPort(UART_BASE_CFG * uCfgFlw,unsigned char *Buffer,int *len);

#endif

/* EOF */
