#ifndef	_MQTT_DATE_H_
#define	_MQTT_DATE_H_

const char *atntTopic = "m2x/%s/requests";
const char *atntData = "{\"id\":\"a0c46778fd7f97ffad8d1a2f12c5d1b1\",\"method\":\"POST\",\"resource\":\"/v2/devices/a0c46778fd7f97ffad8d1a2f12c5d1b1/streams/Coil_1_Status/values\",\"agent\":\"M2X-Demo-Client/0.0.1\",\"body\": {\"values\":[{\"timestamp\": \"%s\",\"value\": \"%u\"}]}}";

const char *mosquittoTopic = "mosquittoTest_esp32/%s/randamValueJson";
const char *mosquittoData = "{\"Time\":\"%s\",\"Temperature\":\"%u\",\"Humidity\":\"%u\"}";

#endif

/* EOF */
