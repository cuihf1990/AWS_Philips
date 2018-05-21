/*
 * Device.c
 *
 *  Created on: 2018年3月20日
 *      Author: Cuihf
 */
#include "MICO.h"
#include "Device.h"
#include "system_internal.h"
#include "Uart_Receive.h"
#include "mico_rtos_common.h"

#define device_log(M, ...) custom_log("APP", M, ##__VA_ARGS__)
#define device_log_trace() custom_log_trace("APP")


extern devcie_running_status_t  device_running_status;
extern system_context_t* sys_context;
extern mico_queue_t Uart_push_queue;
extern mico_semaphore_t Subscribe_Sem;
extern mico_semaphore_t connect_sem;

static mico_timer_t _factory_connect_timer;
static uint8_t Report_Device_Data[MAX_REPORT_LENGTH];

uart_cmd_t uartcmd;

char * Json_Philips = "{ \"ErrorCode\":\"%d\", "\
		"\"Notify\": \"%d\","\
		"\"PM25\": \"%d\","\
		"\"Switch\": \"%d\","\
		"\"WindSpeed\": \"%d\","\
		"\"Countdown\": \"%d\","\
		"\"Prompt\": \"%d\","\
		"\"childLock\": \"%d\","\
		"\"FilterLife1\": \"%d\","\
		"\"FilterLife2\": \"%d\","\
		"\"FilterType0\": \"%d\","\
		"\"UILight\": \"%d\","\
		"\"FilterType1\": \"%d\","\
		"\"FilterType2\": \"%d\","\
		"\"AQILight\": \"%d\","\
		"\"WorkMode\": \"%d\","\
		"\"Runtime\": \"%d\","\
		"\"WifiVersion\":\"%s\","\
		"\"DeviceVersion\":\"%s\","\
		"\"ProductId\":\"%s\","\
		"\"Online\":\"True\"  "\
		"}";

char * Report_frank = "{\"state\":{\"reported\" :{ \"ErrorCode\":\"%d\", "\
        "\"Notify\": \"%d\","\
        "\"PM25\": \"%d\","\
        "\"Switch\": \"%d\","\
        "\"WindSpeed\": \"%d\","\
        "\"Countdown\": \"%d\","\
        "\"Prompt\": \"%d\","\
        "\"childLock\": \"%d\","\
        "\"FilterLife1\": \"%d\","\
        "\"FilterLife2\": \"%d\","\
        "\"FilterType0\": \"%d\","\
        "\"UILight\": \"%d\","\
        "\"FilterType1\": \"%d\","\
        "\"FilterType2\": \"%d\","\
        "\"AQILight\": \"%d\","\
        "\"WorkMode\": \"%d\","\
        "\"Runtime\": \"%d\","\
        "\"WifiVersion\":\"%s\","\
        "\"DeviceVersion\":\"%s\","\
        "\"ProductId\":\"%s\","\
        "\"Online\":\"True\"  "\
        "}}}";


uint16_t ctrl_crc16(unsigned char* pDataIn, int iLenIn)
{
  unsigned char const CrcOffSet=0;
  uint16_t  wTemp = 0;
  uint16_t wCRC = 0xffff;
  uint16_t const POLY16=0x1021;//多项式简式
  uint16_t i,j;

  for(i = 0; i < iLenIn; i++)
  {
    for(j = 0; j < 8; j++)
    {
      wTemp = ((pDataIn[i+CrcOffSet] << j) & 0x80 ) ^ ((wCRC & 0x8000) >> 8);
      wCRC <<= 1;
      if(wTemp != 0)
      {
        wCRC ^= POLY16;
      }
    }
  }
  return(wCRC);
}

void Getprops(int port)
{
  uint16_t crc16_value;
  uint8_t cmd_buf[8] = {0xfe,0xff,0x04,0x00,0x01,0x00,0x00,0x00};
  cmd_buf[5] = port;
  crc16_value = ctrl_crc16(cmd_buf+5,1);
  cmd_buf[6] = (crc16_value>>8)&0xff;
  cmd_buf[7] = crc16_value&0xff;
  MicoUartSend (UART_FOR_APP, cmd_buf,8);
}

void Putprops(set_port_t port_msg)
{
  uint16_t crc16_value;
  uint8_t cmd_buf[12] = {0xfe,0xff,0x03,0x00,0x05,0x00,0x00,0x00,0x00,0x00,0x00};
  cmd_buf[5] = port_msg.port;
  cmd_buf[6] = port_msg.prop;
  cmd_buf[7] = 1;
  cmd_buf[8] = port_msg.value;
  cmd_buf[9] = 0;
  crc16_value = ctrl_crc16(cmd_buf+5,5);
  cmd_buf[10] = (crc16_value>>8)&0xff;
  cmd_buf[11] = crc16_value&0xff;
  MicoUartSend (UART_FOR_APP, cmd_buf,12);
}

void Setup_Putprops(setup_port_t setup_port_msg)
{
  uint16_t crc16_value;
  uint8_t cmd_buf[15] = {0xfe,0xff,0x03,0x00,0x08,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff};

  cmd_buf[6] = setup_port_msg.C_prop;
  cmd_buf[7] = setup_port_msg.C_len;
  cmd_buf[8] = setup_port_msg.C_value;

  cmd_buf[9] = setup_port_msg.S_prop;
  cmd_buf[10] = setup_port_msg.S_len;
  cmd_buf[11] = setup_port_msg.S_value;

  crc16_value = ctrl_crc16(cmd_buf+5,8);
  cmd_buf[13] = (crc16_value>>8)&0xff;
  cmd_buf[14] = crc16_value&0xff;
  MicoUartSend (UART_FOR_APP, cmd_buf,15);
  device_log("send to device,  connection = %d , setup = %d",cmd_buf[8],cmd_buf[11]);
}

void Device_Initialize(uint8_t id,uint8_t cmd)
{
  uint16_t crc16_value;
  uint8_t cmd_buf[9] = {0xfe,0xff,1,0,2,1,0,0,0};
  cmd_buf[2] = id;
  cmd_buf[5] = cmd;
  crc16_value = ctrl_crc16(cmd_buf+5,2);
  cmd_buf[7] = (crc16_value>>8)&0xff;
  cmd_buf[8] = crc16_value&0xff;
  MicoUartSend (UART_FOR_APP, cmd_buf, 9);
}

void Mac_send()
{
  uint8_t mac_address[17] = {0xfe,0xff,0x03,0x00,0x0a,0x07,0x01,0x06,0x01,0x02,0x03,0x04,0x05,0x06,0x00,0xff,0xff};
  uint16_t crc16_value;
  uint8_t mac[6];

  wlan_get_mac_address (mac);
  for(int i =0;i<6;i++)
  {
    mac_address[i+8] = mac[i];
  }
  crc16_value = ctrl_crc16(mac_address+5,10);
  mac_address[15] = (crc16_value>>8)&0xff;
  mac_address[16] = crc16_value&0xff;
  MicoUartSend (UART_FOR_APP, mac_address,17);
}

void Fac_Putprops(fac_port_t fac_port,uint8_t num)
{
  uint16_t crc16_value;
  uint8_t cmd_buf[12] = {0xfe,0xff,0x03,0x00,0x05,0x04,0xff,0xff,0xff,0x00,0xff,0xff};
  if(num == 1)
  {
    cmd_buf[6] = 1;
    cmd_buf[7] = 1;
    cmd_buf[8] = fac_port.pcba;
  }else if(num == 2)
  {
    cmd_buf[6] = 2;
    cmd_buf[7] = 1;
    cmd_buf[8] = fac_port.wifi;
  }else if(num ==3)
  {
    cmd_buf[6] = 3;
    cmd_buf[7] = 1;
    cmd_buf[8] = fac_port.reset;
  }
  crc16_value = ctrl_crc16(cmd_buf+5,5);
  cmd_buf[10] = (crc16_value>>8)&0xff;
  cmd_buf[11] = crc16_value&0xff;
  MicoUartSend (UART_FOR_APP, cmd_buf,12);
}

void _factory_handler (void* arg)
{
  if(device_running_status.factory_enter_flag == 1)
  {
  fac_port_t fac_port;
  fac_port.pcba = 0;
  fac_port.wifi = device_running_status.factory_enter_flag;
  fac_port.reset = 0;
  Fac_Putprops(fac_port,2);
  device_log("===================================================factory rest failed");
  }
  mico_stop_timer(&_factory_connect_timer);
  mico_deinit_timer(&_factory_connect_timer);
}

void Philips_Factory()     /////connect timeout 30s
{

    device_running_status.factory_enter_flag = 1;

  if(sys_context->flashContentInRam.micoSystemConfig.configured == wLanUnConfigured ||    /////stopeasylink
		  sys_context->flashContentInRam.micoSystemConfig.configured == unConfigured)
  {
    micoWlanStopEasyLink();
    micoWlanStopEasyLinkPlus();
  }else             ////////stop station
  {
    micoWlanSuspendStation();
  }

  network_InitTypeDef_adv_st wNetConfig;
  memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_adv_st));

  strncpy((char*)wNetConfig.ap_info.ssid, "mxchip", 6);
  wNetConfig.ap_info.security = SECURITY_TYPE_NONE;
  wNetConfig.dhcpMode = DHCP_Client;
  wNetConfig.wifi_retry_interval = 100;
  micoWlanStartAdv(&wNetConfig);                        /////can't be block because of the uart must be running
  device_log("start to connect factory router ========================================");

  mico_init_timer (&_factory_connect_timer,Factory_Connect_TIMMEOUT*1000, _factory_handler, NULL);
  mico_start_timer (&_factory_connect_timer);
}


bool Start_Report_Aws(uint8_t * buf , int data_len , int type)
{
	int err = 0;

	 if(type == 1)    /// means param
	 {
		 if(memcmp(buf,device_running_status.Storage_param,data_len-2) != 0 )
		 {
			 memset(device_running_status.Storage_param,0,sizeof(device_running_status.Storage_param));
			 memcpy(device_running_status.Storage_param,buf,data_len);

             if(device_running_status.Storage_ErrorCode != uartcmd.ErrorCode)      ////error code 优先级高
             {
                 device_running_status.Storage_ErrorCode = uartcmd.ErrorCode;
                 device_running_status.report_type = Device_notice;
             }else if(device_running_status.Storage_PM25 != uartcmd.PM25)
			 {
			     device_running_status.Storage_PM25 = uartcmd.PM25;
			     device_running_status.report_type = Device_data;
			 }else
			     device_running_status.report_type = Shadow_update;

             device_log("post type =%d",device_running_status.report_type);
		 }else
		 {
		//	 device_log("same param data, no need report");
		     goto exit;
		 }
	 }else if(type == 2)  /// means filter
	 {
		 if(memcmp(buf,device_running_status.Storage_filter,data_len-2) != 0 )
		 {
			 memset(device_running_status.Storage_filter,0,sizeof(device_running_status.Storage_filter));
			 memcpy(device_running_status.Storage_filter,buf,data_len);
			 device_running_status.report_type = Device_notice;
		 }else
		 {
		//	 device_log("same filter data, no need report");
		     goto exit;
		 }
	 }else
		 goto exit;

	 if(device_running_status.report_type == Shadow_update){
	     sprintf(Report_Device_Data,Report_frank,uartcmd.ErrorCode,uartcmd.Notify,uartcmd.PM25,uartcmd.Switch,\
	            uartcmd.WindSpeed,uartcmd.Countdown,uartcmd.Prompt,uartcmd.childLock,uartcmd.FilterLife1,\
	            uartcmd.FilterLife2,uartcmd.FilterType0,uartcmd.UILight,uartcmd.FilterType1,uartcmd.FilterType2,\
	            uartcmd.AQILight,uartcmd.WorkMode,uartcmd.Runtime,WifiVersion,DeviceVersion,sys_context->flashContentInRam.Cloud_info.device_id);
	 }else if(device_running_status.report_type == Device_data || device_running_status.report_type == Device_notice){
	     sprintf(Report_Device_Data,Json_Philips,uartcmd.ErrorCode,uartcmd.Notify,uartcmd.PM25,uartcmd.Switch,\
	             uartcmd.WindSpeed,uartcmd.Countdown,uartcmd.Prompt,uartcmd.childLock,uartcmd.FilterLife1,\
	             uartcmd.FilterLife2,uartcmd.FilterType0,uartcmd.UILight,uartcmd.FilterType1,uartcmd.FilterType2,\
	             uartcmd.AQILight,uartcmd.WorkMode,uartcmd.Runtime,WifiVersion,DeviceVersion,sys_context->flashContentInRam.Cloud_info.device_id);
	 }

   // 	device_log("report data = %s, memory = %d",Report_Device_Data,MicoGetMemoryInfo()->free_memory);

	if(mico_rtos_is_queue_full(&Uart_push_queue))
	{
	    device_log("queue is full");
	    return false;
	}

	err = mico_rtos_push_to_queue(&Uart_push_queue, Report_Device_Data, 200);
	require_noerr_action(err, exit, device_log("[error]mico_rtos_push_to_queue err %d",err));

	 return true;

exit:
	 return false;
}

int uart_data_process(uint8_t * buf , int data_len)
{

  OSStatus err = kNoErr;

  fac_port_t fac_port;

  switch(buf[2]){
  case 2:  ///初始化信息
   mico_rtos_set_semaphore(&Subscribe_Sem);
   break;
  case 7:  ///数据信息
   switch(buf[5]){
   case 7:
	   if(buf[6] == 0x07  && buf[7] == 0x00)   ////////mac no such operation id  ， need  add  command into timer
	     {
	      device_log("mac no such operation id");
	     }
	   break;
   case 0:{
	   switch(buf[6]){
	   case 2:
		 //   device_log("buf[9] = %d buf[12] = %d device_easylink_status =%d",buf[9],buf[12],sys_context->flashContentInRam.micoSystemConfig.device_easylink_status);
           if(sys_context->flashContentInRam.micoSystemConfig.need_easylink_return != 1)
           {
        	   sys_context->flashContentInRam.micoSystemConfig.need_easylink_return = 1;
        	   internal_update_config(sys_context);
           }
           ///////buf[9]   connect     buf[12] setup
           if(buf[9] == 0x01 && buf[12] == 0x02){    ////not connect   requested
            if( sys_context->flashContentInRam.micoSystemConfig.device_easylink_status == 0)
            {
             PlatformEasyLinkButtonClickedCallback();
             internal_update_config(sys_context);
             msleep(200);
             }/////start easylink      null  active
           }else if(buf[9] ==0x02 && buf[12] == 0x01){

             mico_rtos_set_semaphore (&connect_sem);
               ////// connecting    active
             Setup_Putprops(device_running_status.setup_port_msg);
           }
           if(buf[12] == 0xff)   ///// null  clear
           {
        	 PlatformEasyLinkButtonLongPressedCallback();
             msleep(200);
             device_log("start Reset......reboot ");
             internal_update_config(sys_context);
             MicoSystemReboot();
           }
         break;
	   case 3:
       {
          uartcmd.childLock = buf[15];      ///// childlock   15
          uartcmd.Countdown = buf[24];       ////devicetimer        24
          uartcmd.PM25 = (buf[34]*256 + buf[35]);         /////pm2.5           35
          if(buf[43]  != 0 ){
        	if( buf[39] >=  buf[43] )
        		uartcmd.Prompt = 1;             ////aqit              43
        	else
        		uartcmd.Prompt = 0;
          } else
            uartcmd.Prompt = 0;
          uartcmd.Runtime = (buf[27]*256 +buf[28]);        ////Runtime       28
          uartcmd.Switch = buf[12];       ////power     12

          if(buf[9] == 4)
        	  uartcmd.WindSpeed = 0;
          else if(buf[9] == 5)
        	  uartcmd.WindSpeed = 4;
          else
        	  uartcmd.WindSpeed = buf[9];      //// WindSpeed          9

          uartcmd.WorkMode = buf[31];
#if 0
          if(buf[31]  == 0){
              uartcmd.WorkMode = 1;
          }else   if(buf[31]  == 1){
              uartcmd.WorkMode = 2;
          }else   if(buf[31]  == 2){
              uartcmd.WorkMode = 3;
          }else   if(buf[31]  == 3){
              uartcmd.WorkMode = 4;
          }
#endif
          if(buf[49] == 0x00)
            uartcmd.ErrorCode = 0;
          else if(buf[49] == 0x80)
            uartcmd.ErrorCode = 1;
          else if(buf[49] == 0x81)
            uartcmd.ErrorCode = 2;
          else if(buf[49] == 0x82)
            uartcmd.ErrorCode = 3;
          else if((buf[49]&0x08) == 0x08)
            uartcmd.ErrorCode = 5;
          else if((buf[49]&0x20) == 0x20)
            uartcmd.ErrorCode = 6;

        //  device_log("get errorcode  = %d",buf[49]);
          Process_notify(buf[49]);

          uartcmd.UILight = buf[21];/////UILight     21
          if( buf[18] == 0)
            uartcmd.AQILight = 0;/////AQILight     18
          else if(buf[18] == 25)
            uartcmd.AQILight = 1;/////AQILight     18
          else if(buf[18] == 50)
            uartcmd.AQILight = 2;/////AQILight     18
          else if(buf[18] == 75)
            uartcmd.AQILight = 3;/////AQILight     18
          else if(buf[18] == 100)
            uartcmd.AQILight = 4;/////AQILight     18
          /////////////
          if(device_running_status.AWS_Connect_status > 0 )
              Start_Report_Aws(buf,data_len,1);
        }
		   break;
	   case 4:
		//   device_log(" ========%d======%d===%d==device test=================",buf[9],buf[12],buf[15]);
           if(buf[9] == 1 && buf[12] == 0 && buf[15] == 0 )   /////pcba test
           {
             if(device_running_status.factory_send_mac == 0)
             {
               Mac_send();
               device_running_status.factory_send_mac = 1;
             }
             device_log("step 1 , reset alink info");
             fac_port.pcba = 0;
             fac_port.wifi = 0;
             fac_port.reset = 0;
             Fac_Putprops(fac_port,1);
           }else if(buf[9] == 0 && buf[12] == 1 && buf[15] == 0)  /////wifi test
           {
             if(device_running_status.factory_enter_flag == 0){
                device_log("step 2");
                Philips_Factory();
             }
           }else if(buf[9] == 0 && buf[12] == 0 && buf[15] == 1)   //////reset test
           {
             device_log("step 3");
             device_running_status.factory_send_mac = 0;
             fac_port.pcba = 0;
             fac_port.wifi = 0;
             fac_port.reset = 0;     //////default
             Fac_Putprops(fac_port,3);
             PlatformEasyLinkButtonLongPressedCallback();
             internal_update_config(sys_context);
             msleep(200);
             MicoSystemReboot();
            }
		   break;
	   case 5:
           uartcmd.FilterType1 = buf[9];/////FilterType1   9
           uartcmd.FilterType2 = buf[12];/////FilterType2   12
           uartcmd.FilterType0 =  (buf[15]*256 +buf[16]);
           uartcmd.FilterLife1 = (buf[19]*256 +buf[20]);
           uartcmd.FilterLife2 = (buf[23]*256 +buf[24]);///LifeTimeFilter2   buf[23]*256 +buf[24]
		   break;
	   default:
		   break;
	   }
   }
   break;
   default:
	   break;
   }
   break;
   default:
	   break;
  }

  return 0;

  exit:
  	 device_log("push error");
     return -1;

}
