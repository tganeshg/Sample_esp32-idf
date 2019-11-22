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

/*** Local Macros ***/
#define SET_NEXT_WIFI_STATE(nState)	(wifiFlowCntrl.state = nState)
#define SET_NEXT_MOD_STATE(nState)	(modFlowCntrl.state = nState)
#define SET_NEXT_MQTT_STATE(nState)	(mqttFlowCntrl.state = nState)
#define SET_NEXT_DSP_STATE(nState)	(dspFlowCntrl.state = nState)
#define	STATUS_LED					2
 
/*** Externs ***/
extern MODBUS_COM_CONFIG	meterComConfig;
extern MODBUS_CONFIG		modConfig;
	
/*** Globals ***/
TaskHandle_t	tWifiHandler,tUartHandler,tMqttHandler,tOledHandler;

const char *data = "{																					\
  \"id\": \"a0c46778fd7f97ffad8d1a2f12c5d1b1\",															\
  \"method\": \"POST\",																					\
  \"resource\": \"/v2/devices/a0c46778fd7f97ffad8d1a2f12c5d1b1/streams/Coil_1_Status/values\",			\
  \"agent\": \"M2X-Demo-Client/0.0.1\",																	\
  \"body\": {																							\
    \"values\": [																						\
      {																									\
        \"timestamp\": \"2014-08-19T13:37:09Z\",														\
        \"value\": \"1\"																				\
      }																									\
    ]																									\
  }																										\
}";																										
/* Debug Tags */
static const char	*mainTag = "main ";
static const char	*nvsTag = "nvsFlashInit ";
static const char	*gpioTag = "gpioInit ";
static const char	*wifiEventHandlerTag = "wifiEventHandler ";
static const char	*wifiTaskTag = "wifiTask ";
static const char	*uartTaskTag = "mUartTask ";
static const char	*mqttTaskTag = "mqttTask ";
static const char	*mqttEventHandlerTag = "mqttEventHandler ";

/*** Structure Variables ***/
WIFI_CFG_FLOW		wifiFlowCntrl;		
MODBUS_CFG_FLOW		modFlowCntrl;
MQTT_CFG_FLOW		mqttFlowCntrl;
DSP_CFG_FLOW		dspFlowCntrl;

UART_BASE_CFG		uartCfgFlow;

/* Constants in Structure Variables */
static const TASK_INFO	taskDetails[TOTAL_TASK] = {
	{mUartTask,"UART Task",8192,NULL,1,&tUartHandler,1},
	{wifiTask,"WIFI Task",8192,NULL,1,&tWifiHandler,0},
	{mqttTask,"MQTT Task",8192,NULL,1,&tMqttHandler,1},
	{displayTask,"OLED Task",8192,NULL,1,&tOledHandler,1}
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
		}
		break;
		case SYSTEM_EVENT_STA_DISCONNECTED:
		{
			xEventGroupClearBits(uOpt->stWifiEventGroup, WIFI_CONNECTED_BIT);
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
		}
        break;
        case MQTT_EVENT_DISCONNECTED:
		{
			ESP_LOGI(mqttEventHandlerTag,"MQTT Disconnected from %s",context->mqttCoreCfg.host);
			xEventGroupClearBits(context->mqttEventGroup, MQTT_CONNECTED_BIT);
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
            ESP_LOGI(mqttEventHandlerTag, "MQTT_EVENT_PUBLISHED, msgId=%d", event->msg_id);
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

static int setDefaultConfig(void)
{
	/* Wifi Configs */
	wifiFlowCntrl.wifiStConfig.enable = TRUE;
	strcpy(wifiFlowCntrl.wifiStConfig.clientConf.ssid,WIFI_SSID);
	strcpy(wifiFlowCntrl.wifiStConfig.clientConf.passWd,WIFI_PASSWD);
	memset(wifiFlowCntrl.wifiStConfig.clientConf.cIpAddr,0,SIZE_16);

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
	mqttFlowCntrl.enable = mqttFlowCntrl.mqttCfg.enable = FALSE;
	strcpy(mqttFlowCntrl.mqttCfg.brokerIP,ATNT_BROKER_URL);
	strcpy(mqttFlowCntrl.mqttCfg.userName,ATNT_API_KEY);
	strcpy(mqttFlowCntrl.mqttCfg.passWord,"");
	strcpy(mqttFlowCntrl.mqttCfg.clientId,ATNT_CLIENT_ID);
	mqttFlowCntrl.mqttCfg.brokerPort = MQTT_PORT;

	/* Display */
	dspFlowCntrl.enable = TRUE;

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
	u8g2_SetBitmapMode(&dspInitFlow->u8g2Handler, 0);
	u8g2_SetDrawColor(&dspInitFlow->u8g2Handler, 1);
	u8g2_SetFontMode(&dspInitFlow->u8g2Handler,0);
	u8g2_SetFont(&dspInitFlow->u8g2Handler, u8g2_font_timR14_tf);
	/*u8g2_DrawStr(&dspInitFlow->u8g2Handler, 2,40,"Hello World!");
	u8g2_SendBuffer(&dspInitFlow->u8g2Handler); */

	return RET_OK;
}

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
		vTaskDelay(200);
	}

	return RET_OK;
}

/* Task */
void wifiTask(void *arg)
{
	ESP_LOGI(wifiTaskTag, "ESP WIFI Station Mode Task..!");
	if(wifiFlowCntrl.wifiStConfig.enable)
		wifiFlowCntrl.state = WIFI_STATE_INIT;
	else
		wifiFlowCntrl.state = WIFI_STATE_DO_NOTHIG;

	while(TRUE)
	{
		switch(wifiFlowCntrl.state)
		{
			case WIFI_STATE_INIT:
			{
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
				ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiFlowCntrl.wifiCoreConfig));
				ESP_ERROR_CHECK(esp_wifi_start());
				
				ESP_LOGI(wifiTaskTag, "WIFI Station Mode Init Finished..!");
				ESP_LOGI(wifiTaskTag, "Connect to SSID:%s Password:%s",wifiFlowCntrl.wifiCoreConfig.sta.ssid, 
																			wifiFlowCntrl.wifiCoreConfig.sta.password);
				SET_NEXT_WIFI_STATE(WIFI_STATE_IDLE);
			}
			break;
			default:	//WIFI_STATE_DO_NOTHIG
				vTaskDelay(200);
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
				vTaskDelay(600);
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
				vTaskDelay(200);
			break;
		}
	}
}

void mqttTask(void *arg)
{
	int msgId = 0;
	
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
				if( xEventGroupGetBits(wifiFlowCntrl.stWifiEventGroup) & WIFI_CONNECTED_BIT )
				{
					ESP_ERROR_CHECK(esp_mqtt_client_start(mqttFlowCntrl.mqttClient));
					SET_NEXT_MQTT_STATE(MQTT_STATE_IDLE);
				}
				else
				{
					vTaskDelay(500);
					ESP_LOGI(mqttTaskTag, "Waiting to wifi Connect..");
					SET_NEXT_MQTT_STATE(MQTT_STATE_CONNECT);
				}
			}
			break;
			case MQTT_STATE_IDLE:
			{
				vTaskDelay(2000);

			    msgId = esp_mqtt_client_publish(mqttFlowCntrl.mqttClient, 
												"m2x/0569fed2d45691932babd1dccc81e672/requests", data, 0, 1, 0);
				ESP_LOGI(mqttTaskTag, "Publish initiated.., msgId=%d", msgId);
				SET_NEXT_MQTT_STATE(MQTT_STATE_IDLE);
			}
			break;
			default: // MQTT_STATE_DO_NOTHIG
				vTaskDelay(200);
			break;
		}
	}
}

void displayTask(void *arg)
{
	unsigned short int i = 0;
	char buff[32] = {'\0'};

	ESP_LOGI(mqttTaskTag, "ESP Display Task..!");
	if(dspFlowCntrl.enable)
		dspFlowCntrl.state = DSP_STATE_INIT;
	else
		dspFlowCntrl.state = DSP_STATE_DO_NOTHIG;

	while(TRUE)
	{
		switch(dspFlowCntrl.state)
		{
			case DSP_STATE_INIT:
			{
				if( !(xEventGroupGetBits(wifiFlowCntrl.stWifiEventGroup) & WIFI_CONNECTED_BIT) )
					break;

				/* Init Display */
				initDisplay(&dspFlowCntrl);
				SET_NEXT_DSP_STATE(DSP_STATE_CNT_UPDATE);
			}
			break;
			case DSP_STATE_CNT_UPDATE:
			{
				// draw the hourglass animation, full-half-empty
				#if 0
				u8g2_ClearBuffer(&dspFlowCntrl.u8g2Handler);
				u8g2_DrawXBM(&dspFlowCntrl.u8g2Handler, 34, 2, 60, 60, hourglass_full);
				u8g2_SendBuffer(&dspFlowCntrl.u8g2Handler);
				vTaskDelay(100 / portTICK_RATE_MS);
				
				u8g2_ClearBuffer(&dspFlowCntrl.u8g2Handler);
				u8g2_DrawXBM(&dspFlowCntrl.u8g2Handler, 34, 2, 60, 60, hourglass_half);
				u8g2_SendBuffer(&dspFlowCntrl.u8g2Handler);
				vTaskDelay(100 / portTICK_RATE_MS);

				u8g2_ClearBuffer(&dspFlowCntrl.u8g2Handler);
				u8g2_DrawXBM(&dspFlowCntrl.u8g2Handler, 34, 2, 60, 60, hourglass_empty);
				u8g2_SendBuffer(&dspFlowCntrl.u8g2Handler);
				vTaskDelay(100 / portTICK_RATE_MS);	
				#endif

				sprintf(buff,"Count=%d",i++);
				// draw top line
				u8g2_ClearBuffer(&dspFlowCntrl.u8g2Handler);
				u8g2_DrawLine(&dspFlowCntrl.u8g2Handler, 0, 15, 127, 15);
				u8g2_DrawLine(&dspFlowCntrl.u8g2Handler, 0, 48, 127, 48);
				u8g2_DrawStr(&dspFlowCntrl.u8g2Handler, 1,14,wifiFlowCntrl.wifiStConfig.clientConf.cIpAddr);
				u8g2_DrawStr(&dspFlowCntrl.u8g2Handler, 1,42,buff);
				u8g2_DrawStr(&dspFlowCntrl.u8g2Handler, 1,63,"!Btm Line!");
				u8g2_SendBuffer(&dspFlowCntrl.u8g2Handler);
				vTaskDelay(1000 / portTICK_RATE_MS);
				SET_NEXT_DSP_STATE(DSP_STATE_CNT_UPDATE);
			}
			break;
			default:	//DSP_STATE_DO_NOTHIG
				vTaskDelay(200);
			break;
		}
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
		vTaskDelay(600);
		gpio_set_level(STATUS_LED,1);
		vTaskDelay(600);
	}

	return;
}

/* EOF */
