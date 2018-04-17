/*
 * Remote_Client.h
 *
 *  Created on: 2018Äê3ÔÂ27ÈÕ
 *      Author: Cuihf
 */

#ifndef AWS_IOT_REMOTE_CLIENT_H_
#define AWS_IOT_REMOTE_CLIENT_H_

#define MAX_TCP_LENGH 1024
#define HTTP_RESPONSE_BODY_MAX_LEN 2048
#define INFO_MAX_DATA 200
#define CA_MAX_DATA  2048

#define GET_CA_URL "https://otau6avky1.execute-api.cn-north-1.amazonaws.com.cn/api_done/certificate_for_philips?mac=%s&chipid=asdfadfa&sign=adsfasdfasdfasdf"

#define CA_REGISTER "https://otau6avky1.execute-api.cn-north-1.amazonaws.com.cn/api_done/activate_for_philips?mac=%s&passwd=asd34rtaef&productid=sadfas"

#define HOST "otau6avky1.execute-api.cn-north-1.amazonaws.com.cn"

#define MAX_PARAMS_NUM 6                                                           //////////B0F89315C09

#define HTTP_GET  "GET %s HTTP/1.1\r\n"\
            "Host: %s\r\n"\
            "User-Agent: Fiddler\r\n\r\n"

void Remote_Client_Thread();
void Start_Login();
void Cloud_Data_Process(uint8_t * Tcp_Buffer , int Buffer_Length);
void Save_cloud_data(int num , char * data);
bool Need_Register();

enum {
	CERT_ID = 1,
	CERT_URL,
	PRIVA_ID,
	MQTT_PORT,
	MQTT_HOST,
	DEVICE_ID,
};


typedef struct
{
	int idx;
	const char *name;
} device_func_index;


typedef struct _http_context_t{
  char *content;
  uint32_t content_length;
} http_context_t;


#endif /* AWS_IOT_REMOTE_CLIENT_H_ */
