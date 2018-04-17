#include "mico.h"
#include "Device.h"
#include "Uart_Receive.h"
#include "Remote_Client.h"
#include "Local_Udp.h"
#include "fog_wifi_config.h"

#define app_log(M, ...) custom_log("APP", M, ##__VA_ARGS__)
#define app_log_trace() custom_log_trace("APP")

static mico_semaphore_t wait_sem = NULL;

extern system_context_t* sys_context;

extern uint8_t need_change_handle;
extern uint8_t wifi_factory_flag;

setup_port_t setup_port_msg;

#define APPLICATION_GETPORTS_TIMMEOUT  2    /////秒  获取状态间隔
mico_timer_t _device_timer;  ////wifi主动刷新设备状态
mico_semaphore_t Subscribe_Sem;  ////初次上电的设备初始化

mico_semaphore_t login_sem = NULL;
mico_semaphore_t get_ca_sem = NULL;

extern void aws_iot_thread(void);

/* MICO system callback: Restore default configuration provided by application */
void appRestoreDefault_callback( void * const user_config_data, uint32_t size )
{
    memset( user_config_data, 0x0, size );
}

static void micoNotify_WifiStatusHandler( WiFiEvent status, void* const inContext )
{
    switch ( status )
    {
        case NOTIFY_STATION_UP:
            if(wifi_factory_flag == 1)
            {
            	app_log("=======================================factory rest success");
              wifi_factory_flag = 0;
              fac_port_t fac_port;
              fac_port.pcba = 0;
              fac_port.wifi = wifi_factory_flag;
              fac_port.reset = 0;
              Fac_Putprops(fac_port,2);
            }else {
              MicoRfLed (true);
              setup_port_msg.C_prop =1;
              setup_port_msg.C_len = 1;
              setup_port_msg.C_value =4;
              setup_port_msg.S_prop=2;
              setup_port_msg.S_len = 1;
              setup_port_msg.S_value =1;
              Setup_Putprops(setup_port_msg);
              mico_rtos_thread_msleep(200);
            }
            if( wait_sem != NULL ){
                mico_rtos_set_semaphore( &wait_sem );
            }
            break;
        case NOTIFY_STATION_DOWN:
        	app_log ("Station down");
            setup_port_msg.C_prop =1;
            setup_port_msg.C_len = 1;
            setup_port_msg.C_value =3;
            setup_port_msg.S_prop=2;
            setup_port_msg.S_len = 1;
            setup_port_msg.S_value =3;
            Setup_Putprops(setup_port_msg);
            msleep(200);
            case NOTIFY_AP_UP:
            case NOTIFY_AP_DOWN:
            break;
    }
}


void _Getprops_handler (void* arg)
{

  (void) (arg);
   Getprops(1);  ///////getprops port 2 wifiui
   msleep(150);
   Getprops(2);
   msleep(150);
   Getprops(3);
   msleep(150);
   Getprops(4);
   msleep(150);
   Getprops(5);
   msleep(150);
   if(need_change_handle){
	   Mac_send();
	   msleep(200);
   }
}


int  user_get_device_mac( char *mac_buff, uint32_t mac_buff_len )
{
    int err = -1;
    uint8_t mac[10] = {0};

    require((mac_buff != NULL) && (mac_buff_len >= 16), exit);

    wlan_get_mac_address(mac);

    memset(mac_buff, 0, mac_buff_len);
    sprintf(mac_buff, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    err = 0;
    exit:
    return err;
}

int application_start( void )
{
    app_log_trace();
    OSStatus err = kNoErr;
    mico_Context_t* mico_context;

    mico_rtos_init_semaphore( &wait_sem, 1 );
    mico_rtos_init_semaphore( &get_ca_sem, 1 );
    mico_rtos_init_semaphore( &login_sem, 1 );
    mico_rtos_init_semaphore( &Subscribe_Sem, 1 );
    /*Register user function for MiCO nitification: WiFi status changed */
    err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED,
                                       (void *) micoNotify_WifiStatusHandler,
                                       NULL );
    require_noerr( err, exit );


    /* Create mico system context and read application's config data from flash */
    mico_context = mico_system_context_init( 0 );

    Uart_Init();

    if( sys_context->flashContentInRam.micoSystemConfig.need_easylink_return == 1)
    {
      app_log("Device_Initialize..................");
      mico_rtos_init_semaphore(&Subscribe_Sem, 1);
      Device_Initialize(1,1);
      mico_rtos_get_semaphore(&Subscribe_Sem, 1*1000);
    }else
      app_log("need not Device_Initialize..................");


    /* mico system initialize */
    err = mico_system_init( mico_context );
    require_noerr( err, exit );

    fog_wifi_config_mode( FOG_AWS_SOFTAP_COMBO_MODE );

    /* Wait for wlan connection*/
    mico_rtos_get_semaphore( &wait_sem, MICO_WAIT_FOREVER );
    app_log("wifi connected successful");

    mico_init_timer (&_device_timer ,APPLICATION_GETPORTS_TIMMEOUT*1000/2 , _Getprops_handler, NULL);
    mico_start_timer (&_device_timer);

    //memset(&sys_context->flashContentInRam.Cloud_info,0,sizeof(Cloud_info_t));

    if(sys_context->flashContentInRam.Cloud_info.device_id[0] == '\0'){
    	user_get_device_mac(sys_context->flashContentInRam.Cloud_info.mac_address,sizeof(sys_context->flashContentInRam.Cloud_info.mac_address));
		app_log("start log , mac =%s",sys_context->flashContentInRam.Cloud_info.mac_address);
		Start_Login();
		mico_rtos_get_semaphore( &login_sem, MICO_WAIT_FOREVER );

		app_log("start Ca ");
		Get_CA_INFO();
		mico_rtos_get_semaphore( &get_ca_sem, MICO_WAIT_FOREVER );
		app_log("save info");
		internal_update_config(sys_context);
    }

    start_aws_iot_shadow();

    start_local_udp();

    exit:
    mico_system_notify_remove( mico_notify_WIFI_STATUS_CHANGED,
                               (void *) micoNotify_WifiStatusHandler );
    mico_rtos_deinit_semaphore( &wait_sem );
    mico_rtos_delete_thread( NULL );
    return err;
}

