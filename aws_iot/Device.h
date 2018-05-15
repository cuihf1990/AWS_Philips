/*
 * Devcie.h
 *
 *  Created on: 2018Äê3ÔÂ20ÈÕ
 *      Author: Cuihf
 */

#ifndef AWS_IOT_DEVICE_H_
#define AWS_IOT_DEVICE_H_

#define  WifiVersion "w2018.02"
#define  DeviceVersion "1.0.32"

#define Factory_Connect_TIMMEOUT 30
#define MAX_REPORT_LENGTH 1024

#define Report_TimeOut  10*60*100
#define URL_MAX_DATA    1024

typedef struct _setup_port{
  uint8_t C_prop;
  uint8_t C_len;
  uint8_t C_value;
  uint8_t S_prop;
  uint8_t S_len;
  uint8_t S_value;
}setup_port_t;

typedef struct _set_port{
  uint8_t port;
  uint8_t prop;
  uint8_t value;
}set_port_t;

typedef struct _fac_port{
  uint8_t pcba;
  uint8_t wifi;
  uint8_t reset;
}fac_port_t;

typedef struct _uart_cmd{
	  uint8_t childLock;
	  uint8_t Countdown;
	  uint8_t ErrorCode;
	  uint8_t Notify;
	  uint16_t PM25;
	  uint8_t PM25Level;
	  uint8_t Prompt;
	  uint32_t Runtime;
	  uint8_t Switch;
	  uint8_t WindSpeed;
	  uint16_t FilterLife1;
	  uint16_t FilterLife2;
	  uint16_t  FilterType0;
	  uint8_t   UILight;
	  uint8_t   FilterType1;
	  uint8_t FilterType2;
	  uint8_t  AQILight;
	  uint8_t WorkMode;
}uart_cmd_t;


enum _publish_type
{
    Shadow_update = 0,
    Shadow_delete,
    Shadow_get,
    Device_notice,
    Device_data,
};

typedef struct _device_running_status{
    enum _publish_type report_type;     ///
    uint8_t Storage_param[100];
    uint8_t Storage_filter[100];
    uint8_t factory_send_mac;
    uint8_t factory_enter_flag;
    setup_port_t setup_port_msg;
    uint8_t AWS_Connect_status;
    uint16_t Storage_PM25;
    uint8_t Storage_ErrorCode;
    char  certificate_url[URL_MAX_DATA];
    char  privatekey_url[URL_MAX_DATA];
}devcie_running_status_t;

int uart_data_process(uint8_t * input_buffer , int data_len);
void Setup_Putprops(setup_port_t setup_port_msg);
void Device_Initialize(uint8_t id,uint8_t cmd);
void Putprops(set_port_t port_msg);
void Getprops(int port);
uint16_t ctrl_crc16(unsigned char* pDataIn, int iLenIn);
void Mac_send();
void Fac_Putprops(fac_port_t fac_port,uint8_t num);
void _factory_handler (void* arg);
void Philips_Factory();

#endif /* AWS_IOT_DEVICE_H_ */
