/* 
*	Project	: Smaple Program using Wifi-MQTT and Uart-Modbus
*	
*	Author	: Ganesh
*	Help	: Gopi
*	Date	: 16/05/2019
*	Desp	: UART handling.
*/

/*** Includes ***/
#include "main.h"

/*** Local Macros ***/
 
/*** Externs ***/
 
/*** Globals ***/
static const char		*uartTag = "uartApi: ";

/*** Structure Variables ***/

/*** Functions Definition ***/
/* Private */

/* Public */

int uartInit(UART_BASE_CFG * uCfgFlw)
{
    /* Configure parameters of an UART driver,communication pins and install the driver */
    uCfgFlw->uartCoreCfg.baud_rate = uCfgFlw->uartConfig.baudRate;
    uCfgFlw->uartCoreCfg.data_bits = uCfgFlw->uartConfig.dataBits;
    uCfgFlw->uartCoreCfg.parity = uCfgFlw->uartConfig.parity;
    uCfgFlw->uartCoreCfg.stop_bits = uCfgFlw->uartConfig.stopBits;
    uCfgFlw->uartCoreCfg.flow_ctrl = uCfgFlw->uartConfig.flowCntrl;
    uCfgFlw->uartCoreCfg.rx_flow_ctrl_thresh = uCfgFlw->uartConfig.rxFlowCtrlThresh;

	ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uCfgFlw->uartCoreCfg));
	ESP_ERROR_CHECK(uart_set_pin(	UART_NUM_1, 
									GPIO_NUM_10, 
									GPIO_NUM_9, 
									UART_PIN_NO_CHANGE, 
									UART_PIN_NO_CHANGE)
								);

	//TODO : If need Rs232 and Rs485 , need to set
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, UART_BUF_SIZE,UART_BUF_SIZE, 0, NULL, 0));
	UART1.conf1.rxfifo_full_thrhd = 1;

	uart_flush(UART_NUM_1);
	ESP_LOGI(uartTag, "uart init success..!");
	return RET_OK;
}

int sendPort(unsigned char *Buffer,unsigned int len)
{
	// Write data back to the UART
	ESP_LOGI(uartTag, "uart1 send %d",len);
    if(uart_write_bytes(UART_NUM_1, (const char *)Buffer,len) < 0)
	{
		ESP_LOGI(uartTag, "uart Send Faied..!");
		return RET_FAILURE;
	}
	return RET_OK;
}

int	readPort(UART_BASE_CFG * uCfgFlw,unsigned char *Buffer,int *len)
{
	size_t length = 0, idx = 0;

	while(1)
	{
		length = uart_read_bytes(UART_NUM_1, &Buffer[idx], 1, uCfgFlw->uartConfig.rxThreshold);
		if(length <= 0)
			break;
		else
			idx++;
	}
	
	if(idx <= 0)
		return RET_FAILURE;

	*len = idx;
	return RET_OK;
}

/* EOF */
