/* 
*	Project	: Smaple Program using Wifi-MQTT and Uart-Modbus
*	
*	Author	: Ganesh
*	Help	: Gopi
*	Date	: 16/05/2019
*	Desp	: main task and all other subtasks.
*/

/*** Includes ***/
#include "main.h"
#include "icons.h"
#include "mqttData.h"

/*** Local Macros ***/
#define SET_NEXT_WIFI_STATE(nState)	(wifiFlowCntrl.state = nState)
#define SET_NEXT_MOD_STATE(nState)	(modFlowCntrl.state = nState)
#define SET_NEXT_MQTT_STATE(nState)	(mqttFlowCntrl.state = nState)
#define SET_NEXT_SNTP_STATE(nState)	(sntpFlowCntrl.state = nState)
#define	STATUS_LED					2
 
/*** Externs ***/
extern MODBUS_COM_CONFIG	meterComConfig;
extern MODBUS_CONFIG		modConfig;
	
/*** Globals ***/
TaskHandle_t				tWifiHandler,tUartHandler,tMqttHandler,tDhtHandler;
time_t 						now;
struct tm 					timeinfo;

/* DHT11 */
static const dht_sensor_type_t sensorType = DHT_TYPE_DHT11;
static const gpio_num_t dhtGpio = GPIO_NUM_21;

/* Debug Tags */
static const char	*mainTag = "main ";
static const char	*nvsTag = "nvsFlashInit ";
static const char	*gpioTag = "gpioInit ";
static const char	*wifiEventHandlerTag = "wifiEventHandler ";
static const char	*wifiTaskTag = "mainTask ";
static const char	*uartTaskTag = "mUartTask ";
static const char	*mqttTaskTag = "mqttTask ";
static const char	*mqttEventHandlerTag = "mqttEventHandler ";
static const char	*encryptMsgTag = "mainTask ";
static const char	*dhtTestTaskTag = "dhtTestTask ";

/*** Structure Variables ***/
WIFI_CFG_FLOW		wifiFlowCntrl;		
MODBUS_CFG_FLOW		modFlowCntrl;
MQTT_CFG_FLOW		mqttFlowCntrl;
DSP_CFG_FLOW		dspFlowCntrl;
SNTP_CFG_FLOW		sntpFlowCntrl;
UART_BASE_CFG		uartCfgFlow;

nBdelayId			mqttPubDelayId;

IPC_COMM			ipcCommArea;

/* Constants in Structure Variables */
static const TASK_INFO	taskDetails[TOTAL_TASK] = {
	{mainTask,"WIFI Task",8192,NULL,( 1 | portPRIVILEGE_BIT ),&tWifiHandler,0},
	{mqttTask,"MQTT Task",8192,NULL,( 1 | portPRIVILEGE_BIT ),&tMqttHandler,1},
	{mUartTask,"UART Task",8192,NULL,( 1 | portPRIVILEGE_BIT ),&tUartHandler,1},
	{dhtTest,"DHT Task",8192,NULL,( 1 | portPRIVILEGE_BIT ),&tDhtHandler,1}
	/* Add task info if you want new task and modify 'TOTAL_TASK' */
};

/* Event Handlers */
static esp_err_t wifiEventHandler(void *ctx, system_event_t *event)
{
	WIFI_CFG_FLOW *uOpt = (WIFI_CFG_FLOW *)ctx;

    switch(event->event_id) 
	{
		case SYSTEM_EVENT_STA_START:
			esp_wifi_connect();
		break;
		case SYSTEM_EVENT_STA_GOT_IP:
		{
			strcpy(uOpt->wifiStConfig.clientConf.cIpAddr,ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
			ESP_LOGI(wifiEventHandlerTag, "Got IP in ST: %s", uOpt->wifiStConfig.clientConf.cIpAddr);
			xEventGroupSetBits(uOpt->stWifiEventGroup, WIFI_CONNECTED_BIT);
			ipcCommArea.wifiConnected = TRUE;
		}
		break;
		case SYSTEM_EVENT_STA_DISCONNECTED:
		{
			xEventGroupClearBits(uOpt->stWifiEventGroup, WIFI_CONNECTED_BIT);
			ipcCommArea.wifiConnected = FALSE;
			strcpy(wifiFlowCntrl.wifiStConfig.clientConf.cIpAddr,"000.000.000.000");
			esp_wifi_connect();
			//ESP_LOGI(wifiEventHandlerTag,"Retrying to connect..!");
		}
		break;
		default:
        break;
    }
    return ESP_OK;
}

static esp_err_t mqttEventHandler(esp_mqtt_event_handle_t event)
{
    MQTT_CFG_FLOW *context = (MQTT_CFG_FLOW *)event->user_context;
	
    switch (event->event_id) 
	{
        case MQTT_EVENT_CONNECTED:
		{
            ESP_LOGI(mqttEventHandlerTag, "MQTT Connected to %s",context->mqttCoreCfg.host);
			xEventGroupSetBits(context->mqttEventGroup, MQTT_CONNECTED_BIT);
			ipcCommArea.mqttConnected = TRUE;
		}
        break;
        case MQTT_EVENT_DISCONNECTED:
		{
			ESP_LOGI(mqttEventHandlerTag,"MQTT Disconnected from %s",context->mqttCoreCfg.host);
			xEventGroupClearBits(context->mqttEventGroup, MQTT_CONNECTED_BIT);
			ipcCommArea.mqttConnected = FALSE;
		}
		break;
        case MQTT_EVENT_SUBSCRIBED:
		{
            ESP_LOGI(mqttEventHandlerTag, "MQTT_EVENT_SUBSCRIBED, msgId=%d", event->msg_id);
		}
        break;
        case MQTT_EVENT_UNSUBSCRIBED:
		{
            ESP_LOGI(mqttEventHandlerTag, "MQTT_EVENT_UNSUBSCRIBED, msgId=%d", event->msg_id);
		}
        break;
        case MQTT_EVENT_PUBLISHED:
		{
            //ESP_LOGI(mqttEventHandlerTag, "MQTT_EVENT_PUBLISHED, msgId=%d", event->msg_id);
			ipcCommArea.mqttPayload.pDataLoaded = FALSE; //Published
		}
        break;
        case MQTT_EVENT_DATA:
		{
            ESP_LOGI(mqttEventHandlerTag, "MQTT_EVENT_DATA");
            ESP_LOGI(mqttEventHandlerTag,"TOPIC=%.*s\r\n", event->topic_len, event->topic);
            ESP_LOGI(mqttEventHandlerTag,"DATA=%.*s\r\n", event->data_len, event->data);
		}
        break;
        case MQTT_EVENT_ERROR:
		{
            ESP_LOGI(mqttEventHandlerTag, "MQTT_EVENT_ERROR");
		}
        break;
		case MQTT_EVENT_BEFORE_CONNECT:
		{
			ESP_LOGI(mqttEventHandlerTag, "Before Connecting..!");
		}
		break;
        default:
            ESP_LOGI(mqttEventHandlerTag, "Other event id:%d", event->event_id);
        break;
    }
	
	return ESP_OK;
}

/* Private */
static int nvsFlashInit(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
	
	ESP_LOGI(nvsTag,"NVS init success !");
	return RET_OK;
}

static int gpioInit(void)
{
	gpio_config_t conf;

	//	STATUS LED 
	conf.pin_bit_mask 	= 1ULL << STATUS_LED;
	conf.mode  			= GPIO_MODE_OUTPUT;
	conf.pull_up_en 	= GPIO_PULLUP_DISABLE;
	conf.pull_down_en 	= GPIO_PULLDOWN_DISABLE;
	conf.intr_type 		= GPIO_INTR_DISABLE;
	gpio_config(&conf);

	ESP_LOGI(gpioTag,"GPIO init success !");
	return RET_OK;
}

static void initializeSntp(void)
{
    ESP_LOGI("sntp ", "Initializing SNTP...");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, INDIAN_NTP_SERVER);
    sntp_init();
	return;
}

static int setDefaultConfig(void)
{
	/* Wifi Configs */
	wifiFlowCntrl.wifiStConfig.enable = TRUE;
	strcpy(wifiFlowCntrl.wifiStConfig.clientConf.ssid,WIFI_SSID);
	strcpy(wifiFlowCntrl.wifiStConfig.clientConf.passWd,WIFI_PASSWD);
	strcpy(wifiFlowCntrl.wifiStConfig.clientConf.cIpAddr,"000.000.000.000");

	/* Modbus Configs */
	modFlowCntrl.enable = meterComConfig.enable = FALSE;
		/* Uart */
	uartCfgFlow.uartConfig.baudRate = 9600;
	uartCfgFlow.uartConfig.dataBits = UART_DATA_8_BITS;
	uartCfgFlow.uartConfig.parity = UART_PARITY_DISABLE;
	uartCfgFlow.uartConfig.stopBits = UART_STOP_BITS_1;
	uartCfgFlow.uartConfig.flowCntrl = UART_HW_FLOWCTRL_DISABLE;
	uartCfgFlow.uartConfig.rxFlowCtrlThresh = 0;
	uartCfgFlow.uartConfig.rxThreshold = 200;

		/* Modbus */
	meterComConfig.numOfQry = 2;
	meterComConfig.mQryCfg[0].nodeAddr 		= 1;
	meterComConfig.mQryCfg[0].funcCode 		= READ_DISCRETE_COIL;
	meterComConfig.mQryCfg[0].startAddr 	= 0;
	meterComConfig.mQryCfg[0].noOfRegis 	= SINGLE; // quantity
	meterComConfig.mQryCfg[0].byteOrder 	= HIGH_LOW_BYTE;
	meterComConfig.mQryCfg[0].wordOrder 	= HIGH_LOW_WORD;
	meterComConfig.mQryCfg[0].regSize 		= BIT_16;
	meterComConfig.mQryCfg[0].typeOfValue 	= BOOLEAN_BIT;
	strcpy(meterComConfig.mQryCfg[0].paramName,"Coil_1_Status");

	meterComConfig.mQryCfg[1].nodeAddr 		= 2;
	meterComConfig.mQryCfg[1].funcCode 		= READ_HOLDING_REG;
	meterComConfig.mQryCfg[1].startAddr 	= 0;
	meterComConfig.mQryCfg[1].noOfRegis 	= DOUBLE; // quantity
	meterComConfig.mQryCfg[1].byteOrder 	= HIGH_LOW_BYTE;
	meterComConfig.mQryCfg[1].wordOrder 	= HIGH_LOW_WORD;
	meterComConfig.mQryCfg[1].regSize 		= BIT_16;
	meterComConfig.mQryCfg[1].typeOfValue 	= IEEEFLOAT_32BIT;
	strcpy(meterComConfig.mQryCfg[1].paramName,"Reg_1_Value");
	
	/* MQTT */
	mqttFlowCntrl.enable = mqttFlowCntrl.mqttCfg.enable = MQTT_ENABLE;
	strcpy(mqttFlowCntrl.mqttCfg.brokerIP,MQTT_BROKER_URL_IP);
	strcpy(mqttFlowCntrl.mqttCfg.userName,MQTT_USERNAME);
	strcpy(mqttFlowCntrl.mqttCfg.passWord,MQTT_PASSWORD);
	strcpy(mqttFlowCntrl.mqttCfg.clientId,MQTT_CLIENT_ID);
	mqttFlowCntrl.mqttCfg.brokerPort = MQTT_PORT;

	/* Display */
	dspFlowCntrl.enable = TRUE;

	/* SNTP */
	sntpFlowCntrl.enable = TRUE;
	strcpy(sntpFlowCntrl.ntpServer,INDIAN_NTP_SERVER);

	return RET_OK;
}

static int getMqry(int idx)
{
	modConfig.nodeAddr 		= meterComConfig.mQryCfg[idx].nodeAddr;
	modConfig.funcCode 		= meterComConfig.mQryCfg[idx].funcCode;
	modConfig.startAddr 	= meterComConfig.mQryCfg[idx].startAddr;
	modConfig.noOfRegis 	= meterComConfig.mQryCfg[idx].noOfRegis;
	modConfig.byteOrder 	= meterComConfig.mQryCfg[idx].byteOrder;
	modConfig.wordOrder 	= meterComConfig.mQryCfg[idx].wordOrder;
	modConfig.regSize 		= meterComConfig.mQryCfg[idx].regSize;
	modConfig.typeOfValue 	= meterComConfig.mQryCfg[idx].typeOfValue;
	return RET_OK;
}

static int setMqttCfg(MQTT_CFG_FLOW *mqttCfgFlw)
{	
	mqttCfgFlw->mqttCoreCfg.event_handle = mqttEventHandler;
	mqttCfgFlw->mqttCoreCfg.host = mqttCfgFlw->mqttCfg.brokerIP;
	mqttCfgFlw->mqttCoreCfg.port = mqttCfgFlw->mqttCfg.brokerPort;
	mqttCfgFlw->mqttCoreCfg.username = mqttCfgFlw->mqttCfg.userName;
	mqttCfgFlw->mqttCoreCfg.password = mqttCfgFlw->mqttCfg.passWord;
	mqttCfgFlw->mqttCoreCfg.client_id = mqttCfgFlw->mqttCfg.clientId;
	mqttCfgFlw->mqttCoreCfg.user_context = (void *)mqttCfgFlw;
	return RET_OK;
}

/* Display */
static int setBasicInfo(DSP_CFG_FLOW *dspCtrl)
{
	u8g2_SetBitmapMode(&dspCtrl->u8g2Handler, 0);
	u8g2_SetDrawColor(&dspCtrl->u8g2Handler, 1);
	u8g2_SetFontMode(&dspCtrl->u8g2Handler,0);
	u8g2_SetFont(&dspCtrl->u8g2Handler, u8g2_font_timR14_tf);

	return RET_OK;
}

static int initDisplay(DSP_CFG_FLOW *dspInitFlow)
{
	/* SPI Init */
	u8g2_esp32_hal_t u8g2Esp32HalCfg	= U8G2_ESP32_HAL_DEFAULT;
	u8g2Esp32HalCfg.clk 				= GPIO_NUM_19;
	u8g2Esp32HalCfg.mosi 				= GPIO_NUM_4;
	u8g2Esp32HalCfg.cs 					= GPIO_NUM_18;
	u8g2Esp32HalCfg.reset 				= GPIO_NUM_16;
	u8g2Esp32HalCfg.dc 					= GPIO_NUM_17;
	
	u8g2_esp32_hal_init(u8g2Esp32HalCfg);
	u8g2_Setup_ssd1306_128x64_noname_f(	&dspInitFlow->u8g2Handler, U8G2_R0,
										u8g2_esp32_spi_byte_cb, 
										u8g2_esp32_gpio_and_delay_cb );
	u8g2_InitDisplay(&dspInitFlow->u8g2Handler);

	u8g2_SetPowerSave(&dspInitFlow->u8g2Handler, 0);
	u8g2_SetContrast(&dspInitFlow->u8g2Handler, 128);
	u8g2_ClearBuffer(&dspInitFlow->u8g2Handler);
	setBasicInfo(dspInitFlow);
	u8g2_DrawStr(&dspInitFlow->u8g2Handler, 2,40,"Loading..!");
	u8g2_SendBuffer(&dspInitFlow->u8g2Handler);

	return RET_OK;
}

#if MQTT_ENCRYPT_ENABLE
static int encryptMsg(const unsigned char *msg,unsigned long long msgLen,unsigned char *ciphertext)
{
	unsigned char nonce[crypto_secretbox_NONCEBYTES]="nonce";
	unsigned char key[crypto_secretbox_KEYBYTES]="key";

	/* Generate a secure random key and nonce */
	//randombytes_buf(nonce, sizeof(nonce));
	//randombytes_buf(key, sizeof(key));

	/* Encrypt the message with the given nonce and key, putting the result in ciphertext */
	if(crypto_secretbox_easy(ciphertext, msg, msgLen, nonce, key) < 0)
	{
		ESP_LOGI(encryptMsgTag, "MQTT Payload encrypt failed..!");
		return FALSE;
	}

#if 0
	unsigned char decrypted[SIZE_4096];
	if (crypto_secretbox_open_easy(decrypted, ciphertext, (SIZE_4096 + crypto_secretbox_MACBYTES), nonce, key) != 0)
	{
		/* If we get here, the Message was a forgery. This means someone (or the network) somehow tried to tamper with the message*/
		ESP_LOGI(encryptMsgTag,">>>>>>> Error\n");
	}
	else
	{
		ESP_LOGI(encryptMsgTag,">>>>>>> %s\n",decrypted);
	}
#endif
	return TRUE;
}
#endif

/* Public */
int taskInit(void)
{
	unsigned char tIdx=0;
	for(tIdx=0; tIdx < TOTAL_TASK; tIdx++)
	{
		xTaskCreatePinnedToCore(	taskDetails[tIdx].pTaskFunc,
									taskDetails[tIdx].taskName,
									taskDetails[tIdx].pStackSize,
									taskDetails[tIdx].optionalParam,
									taskDetails[tIdx].tPriority,
									taskDetails[tIdx].tHandler,
									taskDetails[tIdx].xCoreID
								);
		delayMs(200);
	}

	return RET_OK;
}

int	nBdelay(nBdelayId *dId,uint64_t dlMs)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	dId->eSec = (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));

	if(!dlMs)
		dId->sSec = dId->eSec;
	else
	{
		if( (dId->eSec - dId->sSec) > dlMs)
			return TRUE;
	}
	return FALSE;
}

void delayMs(const TickType_t mSec)
{
	vTaskDelay(mSec / portTICK_RATE_MS);
}

/* Task */
void mainTask(void *arg)
{
	wifi_ap_record_t ap_info;
	char timeBuffer[SIZE_32];
	uint8_t mac[6] = {0};

	ESP_LOGI(wifiTaskTag, "ESP WIFI Station Mode Task..!");
	if(wifiFlowCntrl.wifiStConfig.enable)
		wifiFlowCntrl.state = WIFI_STATE_INIT;
	else
		wifiFlowCntrl.state = WIFI_STATE_DO_NOTHIG;

	if(sntpFlowCntrl.enable)
		sntpFlowCntrl.state = SNTP_STATE_INIT;
	else
		sntpFlowCntrl.state = SNTP_STATE_DO_NOTHIG;

	while(TRUE)
	{
		switch(wifiFlowCntrl.state)
		{
			case WIFI_STATE_INIT:
			{
				/* Init Display */
				initDisplay(&dspFlowCntrl);

				/* FreeRTOS event group to signal when we are connected*/
				wifiFlowCntrl.stWifiEventGroup = xEventGroupCreate();

				tcpip_adapter_init();
				ESP_ERROR_CHECK(esp_event_loop_init(wifiEventHandler, (void *)&wifiFlowCntrl));

				wifi_init_config_t stInitConfig = WIFI_INIT_CONFIG_DEFAULT();
				ESP_ERROR_CHECK(esp_wifi_init(&stInitConfig));

				SET_NEXT_WIFI_STATE(WIFI_STATE_REINIT);
			}
			break;
			case WIFI_STATE_REINIT:
			{
				memset(wifiFlowCntrl.wifiCoreConfig.sta.ssid,0,sizeof(wifiFlowCntrl.wifiCoreConfig.sta.ssid));
				memset(wifiFlowCntrl.wifiCoreConfig.sta.password,0,sizeof(wifiFlowCntrl.wifiCoreConfig.sta.password));
				strcpy((char *)wifiFlowCntrl.wifiCoreConfig.sta.ssid,wifiFlowCntrl.wifiStConfig.clientConf.ssid);
				strcpy((char *)wifiFlowCntrl.wifiCoreConfig.sta.password,wifiFlowCntrl.wifiStConfig.clientConf.passWd);

				ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
				ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA,mac));
				sprintf(ipcCommArea.macAddr,"%02x:%02x:%02x:%02x:%02x:%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

				ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiFlowCntrl.wifiCoreConfig));
				ESP_ERROR_CHECK(esp_wifi_start());

				ESP_LOGI(wifiTaskTag, "WIFI Station Mode Init Finished..!");
				ESP_LOGI(wifiTaskTag, "Connect to SSID:%s Password:%s",	wifiFlowCntrl.wifiCoreConfig.sta.ssid, 
																		wifiFlowCntrl.wifiCoreConfig.sta.password);
				SET_NEXT_WIFI_STATE(WIFI_STATE_IDLE);
			}
			break;
			case WIFI_STATE_IDLE:
			{
				/* Update Display Basic view */
				u8g2_ClearBuffer(&dspFlowCntrl.u8g2Handler);
				setBasicInfo(&dspFlowCntrl);
				u8g2_DrawLine(&dspFlowCntrl.u8g2Handler, TOP_LINE_S_X_AXIS,TOP_LINE_S_Y_AXIS,TOP_LINE_E_X_AXIS,TOP_LINE_E_Y_AXIS); //Top Line 
				u8g2_DrawLine(&dspFlowCntrl.u8g2Handler, BOTTOM_LINE_S_X_AXIS,BOTTOM_LINE_S_Y_AXIS,BOTTOM_LINE_E_X_AXIS,BOTTOM_LINE_E_Y_AXIS); //Bottom Line
				u8g2_DrawStr(&dspFlowCntrl.u8g2Handler, DSP_WIFI_ST_IP_X,DSP_WIFI_ST_IP_Y,wifiFlowCntrl.wifiStConfig.clientConf.cIpAddr);

				/* If Wifi Connected */
				if( xEventGroupGetBits(wifiFlowCntrl.stWifiEventGroup) & WIFI_CONNECTED_BIT )
				{
					/* Wifi Signal Strength Updation */
					ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&ap_info));
					if( ap_info.rssi >= WIFI_SS_100P ) 																					//100 Percentage
						u8g2_DrawXBM(&dspFlowCntrl.u8g2Handler, DSP_X_AXIS_SS, DSP_Y_AXIS_SS, DSP_SS_W, DSP_SS_H, wifi_ss_100_bits);
					else if( (ap_info.rssi < WIFI_SS_100P) && (ap_info.rssi >= WIFI_SS_75P) ) 											//75 Percentage
						u8g2_DrawXBM(&dspFlowCntrl.u8g2Handler, DSP_X_AXIS_SS, DSP_Y_AXIS_SS, DSP_SS_W, DSP_SS_H, wifi_ss_75_bits);
					else if( (ap_info.rssi < WIFI_SS_75P) && (ap_info.rssi >= WIFI_SS_50P) ) 											//50 Percentage
						u8g2_DrawXBM(&dspFlowCntrl.u8g2Handler, DSP_X_AXIS_SS, DSP_Y_AXIS_SS, DSP_SS_W, DSP_SS_H, wifi_ss_50_bits);
					else if( (ap_info.rssi < WIFI_SS_50P) && (ap_info.rssi >= WIFI_SS_25P) ) 											//25 Percentage
						u8g2_DrawXBM(&dspFlowCntrl.u8g2Handler, DSP_X_AXIS_SS, DSP_Y_AXIS_SS, DSP_SS_W, DSP_SS_H, wifi_ss_25_bits);
					else																												//0 Percentage
						u8g2_DrawXBM(&dspFlowCntrl.u8g2Handler, DSP_X_AXIS_SS, DSP_Y_AXIS_SS, DSP_SS_W, DSP_SS_H, wifi_ss_0_bits);

					/* SNTP - Date and Time Update */
					if( !(xEventGroupGetBits(wifiFlowCntrl.stWifiEventGroup) & TIME_UPDATED_BIT) )
					{
						switch(sntpFlowCntrl.state)
						{
							case SNTP_STATE_INIT:
							{
								initializeSntp();
								SET_NEXT_SNTP_STATE(SNTP_STATE_IDLE);
							}
							break;
							case SNTP_STATE_IDLE:
							{
								if(timeinfo.tm_year < (2019 - 1900))
								{
									delayMs(2000); //Need to replace with non blocking delay
									time(&now);
									localtime_r(&now, &timeinfo);
								}
								else
								{
#if APPLY_INDIAN_TIME_ZONE
									// Set timezone to Eastern Standard Time and print local time.
									setenv("TZ","GMT -5:30", 1);
									tzset();
#endif
									time(&now);
									localtime_r(&now, &timeinfo);
									ESP_LOGI(wifiTaskTag, "Obtained Time : %02d/%02d/%04d %02d:%02d:%02d",	timeinfo.tm_mday,
																											(timeinfo.tm_mon + 1),
																											(timeinfo.tm_year + 1900),
																											timeinfo.tm_hour,
																											timeinfo.tm_min,
																											timeinfo.tm_sec);
									xEventGroupSetBits(wifiFlowCntrl.stWifiEventGroup, TIME_UPDATED_BIT);
									SET_NEXT_SNTP_STATE(SNTP_STATE_DO_NOTHIG);
								}
							}
							break;
							default:
							break;
						}
					}
				}

				/* If Date Time Updated */
				if( (xEventGroupGetBits(wifiFlowCntrl.stWifiEventGroup) & TIME_UPDATED_BIT) )
				{
					/* Time Update to Display */
					memset(timeBuffer,0,sizeof(timeBuffer));
					time(&now);
					localtime_r(&now, &timeinfo);
					sprintf(timeBuffer,"Time %02d:%02d:%02d",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
					u8g2_DrawStr(&dspFlowCntrl.u8g2Handler, DSP_WIFI_TIME_X,DSP_WIFI_TIME_Y,timeBuffer);
				}

				/* If MQTT Broker Connected */
				if(ipcCommArea.mqttConnected)
				{
					u8g2_DrawXBM(&dspFlowCntrl.u8g2Handler, DSP_X_AXIS_MQTT, DSP_Y_AXIS_MQTT, DSP_MQTT_W, DSP_MQTT_H, mqtt_bits);
				}

				/* If MQTT payload publish initiated */
				if(ipcCommArea.mqttPayload.pDataLoaded)
				{
					u8g2_DrawXBM(&dspFlowCntrl.u8g2Handler, DSP_X_AXIS_MQTT_PUB, DSP_Y_AXIS_MQTT_PUB, DSP_MQTT_PUB_W, DSP_MQTT_PUB_H, mqtt_publish_bits);
				}

				/* Display Buffer Push */
				u8g2_SendBuffer(&dspFlowCntrl.u8g2Handler);
				SET_NEXT_WIFI_STATE(WIFI_STATE_IDLE);
			}
			break;
			default: //WIFI_STATE_DO_NOTHIG
				delayMs(200);
			break;
		}
	}
}

void mqttTask(void *arg)
{
	char buffer[SIZE_64]={0};

	ESP_LOGI(mqttTaskTag, "ESP mqtt Task..!");
	if(mqttFlowCntrl.enable)
		mqttFlowCntrl.state = MQTT_STATE_INIT;
	else
		mqttFlowCntrl.state = MQTT_STATE_DO_NOTHIG;

	while(TRUE)
	{
		switch(mqttFlowCntrl.state)
		{
			case MQTT_STATE_INIT:
			{
				/* FreeRTOS event group to signal when we are connected*/
				mqttFlowCntrl.mqttEventGroup = xEventGroupCreate();

				setMqttCfg(&mqttFlowCntrl);
				mqttFlowCntrl.mqttClient = esp_mqtt_client_init(&mqttFlowCntrl.mqttCoreCfg);

				SET_NEXT_MQTT_STATE(MQTT_STATE_CONNECT);
			}
			break;
			case MQTT_STATE_CONNECT:
			{
				//if( xEventGroupGetBits(wifiFlowCntrl.stWifiEventGroup) & WIFI_CONNECTED_BIT )
				if(ipcCommArea.wifiConnected)
				{
					ESP_ERROR_CHECK(esp_mqtt_client_start(mqttFlowCntrl.mqttClient));
					SET_NEXT_MQTT_STATE(MQTT_STATE_IDLE);
				}
				else
				{
					delayMs(500);
					//ESP_LOGI(mqttTaskTag, "Waiting to wifi Connect..");
					SET_NEXT_MQTT_STATE(MQTT_STATE_CONNECT);
				}
			}
			break;
			case MQTT_STATE_IDLE:
			{
				if( xEventGroupGetBits(mqttFlowCntrl.mqttEventGroup) & MQTT_CONNECTED_BIT )
				{
					time(&now);
					localtime_r(&now, &timeinfo);
					memset(buffer,0,sizeof(buffer));
#if (MQTT_BROKER == ATNT_BROKER)
					strftime(buffer,sizeof(buffer),"%FT%TZ",&timeinfo);
					sprintf((char *)ipcCommArea.mqttPayload.pTopic,atntTopic,ATNT_API_KEY);
					sprintf((char *)ipcCommArea.mqttPayload.pBuffer,atntData,buffer,esp_random());
#else
					strftime(buffer,sizeof(buffer),"%F %T",&timeinfo);
					sprintf((char *)ipcCommArea.mqttPayload.pTopic,mosquittoTopic,MQTT_CLIENT_ID);
					sprintf((char *)ipcCommArea.mqttPayload.pBuffer,mosquittoData,ESP_SERIAL_NO,ipcCommArea.macAddr,
																	buffer,ipcCommArea.tempSenData.temperature,
																	ipcCommArea.tempSenData.humidity);
					ipcCommArea.mqttPayload.pDataLoaded = TRUE;
#if MQTT_ENCRYPT_ENABLE
					ipcCommArea.mqttPayload.pDataLoaded = encryptMsg(	(const unsigned char *)ipcCommArea.mqttPayload.pBuffer,
																		(unsigned long long)sizeof(ipcCommArea.mqttPayload.pBuffer),
																		(unsigned char *)ipcCommArea.mqttPayload.pEncryptBuffer);

#endif

#endif
					/* ESP_LOGI(mqttTaskTag, "Publish topic %s",ipcCommArea.mqttPayload.pTopic);
					ESP_LOGI(mqttTaskTag, "Publish payload %s",ipcCommArea.mqttPayload.pBuffer); */

					if(ipcCommArea.mqttPayload.pDataLoaded)
					{
						ipcCommArea.mqttPayload.pMsgId = esp_mqtt_client_publish( mqttFlowCntrl.mqttClient,
														(const char *)ipcCommArea.mqttPayload.pTopic, 
#if MQTT_ENCRYPT_ENABLE
														(const char *)ipcCommArea.mqttPayload.pEncryptBuffer,
#else
														(const char *)ipcCommArea.mqttPayload.pBuffer,
#endif
#if MQTT_ENCRYPT_ENABLE
														(SIZE_4096 + crypto_secretbox_MACBYTES),
#else
														0,
#endif
														1, 0);
						//ESP_LOGI(mqttTaskTag, "Publish initiated.., msgId=%d\n", ipcCommArea.mqttPayload.pMsgId);
					}
				}

				delayMs(1000);
				SET_NEXT_MQTT_STATE(MQTT_STATE_IDLE);
			}
			break;
			default: // MQTT_STATE_DO_NOTHIG
				delayMs(200);
			break;
		}
	}
}

void mUartTask(void *arg)
{
	ESP_LOGI(uartTaskTag, "ESP mUart Task..!");
	if(modFlowCntrl.enable)
		modFlowCntrl.state = MOD_STATE_INIT;
	else
		modFlowCntrl.state = MOD_STATE_DO_NOTHIG;

	while(TRUE)
	{
		switch(modFlowCntrl.state)
		{
			case MOD_STATE_INIT:
			{
				/* Init Uart */
				modFlowCntrl.mQryIdx = 0;
				uartInit(&uartCfgFlow);
				SET_NEXT_MOD_STATE(MOD_STATE_READ_QRY);
			}
			break;
			case MOD_STATE_READ_QRY:
			{
				delayMs(600);
				modFlowCntrl.mQryIdx = (modFlowCntrl.mQryIdx < meterComConfig.numOfQry) ? modFlowCntrl.mQryIdx : 0;
				ESP_LOGI(uartTaskTag, "Send Qry Idx %d",modFlowCntrl.mQryIdx);
				getMqry(modFlowCntrl.mQryIdx);
				SET_NEXT_MOD_STATE(MOD_STATE_SEND_QRY);
			}
			break;
			case MOD_STATE_SEND_QRY:
			{
				if(initModQuery() != RET_OK)
				{
					SET_NEXT_MOD_STATE(MOD_STATE_READ_QRY);
				}
				else
				{
					SET_NEXT_MOD_STATE(MOD_STATE_RCV_RSP);
				}
			}
			break;
			case MOD_STATE_RCV_RSP:
			{
				if(procModResponce(modFlowCntrl.mQryIdx++) != RET_OK)
				{
					ESP_LOGI(uartTaskTag, "ESP Modbus Proc Failed..!");
				}
				else
				{
					ESP_LOGI(uartTaskTag, "ESP Modbus Proc Success..!");
				}
				SET_NEXT_MOD_STATE(MOD_STATE_READ_QRY);
			}
			break;
			default:	//MOD_STATE_DO_NOTHIG
				delayMs(200);
			break;
		}
	}
}

void dhtTest(void *arg)
{
    // DHT sensors that come mounted on a PCB generally have
    // pull-up resistors on the data pin.  It is recommended
    // to provide an external pull-up resistor otherwise...

    //gpio_set_pull_mode(dht_gpio, GPIO_PULLUP_ONLY);

    while(TRUE)
    {
		if(ipcCommArea.wifiConnected)
		{
			if(dht_read_data(sensorType, dhtGpio, &ipcCommArea.tempSenData.humidity, &ipcCommArea.tempSenData.temperature) == ESP_OK)
			{
				ipcCommArea.tempSenData.humidity = ipcCommArea.tempSenData.humidity / 10;
				ipcCommArea.tempSenData.temperature = ipcCommArea.tempSenData.temperature / 10;
				//ESP_LOGI(dhtTestTaskTag, "Humidity: %d Temp: %d\n", ipcCommArea.tempSenData.humidity, ipcCommArea.tempSenData.temperature);
			}
			else
			{
				ipcCommArea.tempSenData.humidity = 0;
				ipcCommArea.tempSenData.temperature = 0;
				ESP_LOGI(dhtTestTaskTag, "Could not read data from sensor\n");
			}
		}
        // If you read the sensor data too often, it will heat up
        // http://www.kandrsmith.org/RJS/Misc/Hygrometers/dht_sht_how_fast.html
        delayMs(2000);
    }
}

/* Main */
void app_main(void)
{
    ESP_LOGI(mainTag,"! Sample Programe !");
	ESP_LOGI(mainTag, "IDF version: %s", esp_get_idf_version());
    ESP_LOGI(mainTag, "cJSON Version: %s", cJSON_Version());

	/* Configuration Init */
	setDefaultConfig();

	/* Init Dependencies */
	nvsFlashInit();
	gpioInit();
	taskInit();

	/* Live Loop */
	while(TRUE)
	{
		gpio_set_level(STATUS_LED,0);
		delayMs(600);
		gpio_set_level(STATUS_LED,1);
		delayMs(600);
	}

	return;
}

/* EOF */
