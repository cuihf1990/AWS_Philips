/*
 * Remote_Client.h
 *
 *  Created on: 2018Äê3ÔÂ27ÈÕ
 *      Author: Cuihf
 */

#ifndef AWS_IOT_REMOTE_CLIENT_H_
#define AWS_IOT_REMOTE_CLIENT_H_

#define  FRANKFURT  1

#define MAX_TCP_LENGH 1024
#define HTTP_RESPONSE_BODY_MAX_LEN 2048
#define INFO_MAX_DATA 200
#define CA_MAX_DATA  2048

#ifndef FRANKFURT
#define GET_CA_URL "https://r349v9s93m.execute-api.cn-north-1.amazonaws.com.cn/v2/certification?mac=%s&chipid=AAQAAAu%%2BAQRa8m1yAgEBAwEB&sign=MEQCIB3YBzk7r8k4SlKzt6x40khyyHudcnDSf/xnz9Fw4YVMAiAooFNSmLUCt/9NUd1HjYn5sQd7WdEcMf4sMFUyzY5poQ=="
#define CA_REGISTER "https://r349v9s93m.execute-api.cn-north-1.amazonaws.com.cn/v2/activation?mac=%s&passwd=asd34rtaef&productid=sadfas"
#else
#define GET_CA_URL "https://2y1t4xnx84.execute-api.eu-central-1.amazonaws.com/v2/certificate-for-philips-v2?mac=%s&chipid=AAQAAAu+AQRa8m1yAgEBAwEB&sign=MEQCIB3YBzk7r8k4SlKzt6x40khyyHudcnDSf/xnz9Fw4YVMAiAooFNSmLUCt/9NUd1HjYn5sQd7WdEcMf4sMFUyzY5poQ=="
#define CA_REGISTER "https://2y1t4xnx84.execute-api.eu-central-1.amazonaws.com/v2/activate-for-philips-v2?mac=%s&passwd=asd34rtaef&productid=sadfas"
#endif

#ifndef FRANKFURT
#define HOST "otau6avky1.execute-api.cn-north-1.amazonaws.com.cn"
#else
#define HOST  "2y1t4xnx84.execute-api.eu-central-1.amazonaws.com"
#endif

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
