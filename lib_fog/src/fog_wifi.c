#include "mico.h"
#include "json_parser.h"
#include "fog_task.h"
#include "fog_product.h"
#include "fog_wifi_config.h"

#define wifi_log(format, ...)  custom_log("wifi", format, ##__VA_ARGS__)

static char pre_ssid[MAX_SSID_LEN + 1];
static char pre_key[MAX_KEY_LEN + 1];
static bool aws_softap_combo_mode = false;

static void aws_softap_up(void)
{
    micoWlanStopAws( );
}

void mico_system_delegate_config_will_start( void )
{
    if( aws_softap_combo_mode == true )
    {
        wifi_log("combo mode, softap:%s", fog_softap_name_get());
       // mico_wlan_monitor_enable_softap(fog_softap_name_get(), 6, aws_softap_up);
    }
    wifi_log("config will start");
}

void mico_system_delegate_config_will_stop( void )
{
    wifi_log("config will stop");
    MicoSysLed( true );
}

void mico_system_delegate_config_success( mico_config_source_t source )
{
    wifi_log("config success: %d", source);
    fog_wifi_pre_set(pre_ssid, pre_key);
}

void mico_system_delegate_easylink_timeout( system_context_t *context )
{
    wifi_log("easylink timeout");
    fog_softap_start( );
    fog_wifi_connect_manage( );
}

void mico_system_delegate_config_recv_ssid( char *ssid, char *key )
{
    wifi_log("config recv ssid:%s, key:%s", ssid, key);
    strncpy(pre_ssid, ssid, MAX_SSID_LEN);
    strncpy(pre_key, key, MAX_KEY_LEN);
}

void mico_easylink_aws_delegate_send_notify_msg( char *aws_notify_msg )
{
    sprintf( aws_notify_msg, "%s%s", aws_notify_msg, fog_type_id_get() );
}

void mico_easylink_aws_delegate_recv_notify_msg( char *aws_notify_msg )
{
    char *data = NULL;
    wifi_log("aws notify msg:%s", aws_notify_msg);

    data = strstr(aws_notify_msg, "\"IP\"");
    if( data != NULL )
    {
       sprintf(aws_notify_msg, "{%s", data);
       wifi_log("aws notify msg:%s", aws_notify_msg);
       aws_delegate_recv_notify( aws_notify_msg, strlen(aws_notify_msg) );
    }

}

static mico_semaphore_t net_semp = NULL;
static void awsNotify_WifiStatusHandler( WiFiEvent event, mico_Context_t * const inContext )
{
    require( inContext, exit );

    switch ( event )
    {
        case NOTIFY_STATION_UP:
            if ( net_semp != NULL )
            {
                mico_rtos_set_semaphore( &net_semp );
            }
            break;
        default:
            break;
    }
    exit:
    return;
}

static void awsNotify_DHCPCompletedlHandler( IPStatusTypedef *pnet,
                                             mico_Context_t * const inContext )
{
    require( inContext, exit );
    if ( net_semp != NULL )
    {
        mico_rtos_set_semaphore( &net_semp );
    }
    wifi_log("ip %s", pnet->ip);
    wifi_log("mask %s", pnet->mask);
    wifi_log("gate %s", pnet->gate);
    wifi_log("dns %s", pnet->gate);
    exit:
    return;
}

void fog_softap_start( void )
{
    OSStatus err = kNoErr;
    network_InitTypeDef_st wNetConfig;

    wNetConfig.wifi_mode = Soft_AP;
    sprintf( wNetConfig.wifi_ssid, fog_softap_name_get() );
    wNetConfig.dhcpMode = DHCP_Server;
    strcpy( wNetConfig.local_ip_addr, "10.10.10.1" );
    strcpy( wNetConfig.net_mask, "255.255.255.0" );
    strcpy( wNetConfig.gateway_ip_addr, "10.10.10.1" );

    err = micoWlanStart( &wNetConfig );
    require_noerr( err, exit );

    wifi_log("softap start: %s", wNetConfig.wifi_ssid);

exit:
    return;
}

int fog_softap_connect_ap( uint32_t timeout_ms, char *ssid, char *passwd )
{
    OSStatus err = kNoErr;

    mico_Context_t *mico_context = mico_system_context_get( );
    network_InitTypeDef_adv_st wNetConfig;
    memset( &wNetConfig, 0x0, sizeof(network_InitTypeDef_adv_st) );

    micoWlanSuspendSoftAP( );

    mico_system_delegate_config_recv_ssid( ssid, passwd );

    if ( timeout_ms != 0 )
    {
        err = mico_rtos_init_semaphore( &net_semp, 2 );
        require_noerr( err, exit );

        err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED,
                                           (void *) awsNotify_WifiStatusHandler,
                                           mico_context );
        require_noerr( err, exit );

        err = mico_system_notify_register( mico_notify_DHCP_COMPLETED,
                                           (void *) awsNotify_DHCPCompletedlHandler,
                                           mico_context );
        require_noerr( err, exit );
    }

    strncpy( (char*) wNetConfig.ap_info.ssid, ssid, maxSsidLen );
    memcpy( wNetConfig.key, passwd, maxKeyLen );
    wNetConfig.key_len = strlen( passwd );
    wNetConfig.ap_info.security = SECURITY_TYPE_AUTO;
    wNetConfig.dhcpMode = true;
    wNetConfig.wifi_retry_interval = 100;

    micoWlanStartAdv( &wNetConfig );

    if ( timeout_ms != 0 )
    {
        err = mico_rtos_get_semaphore( &net_semp, timeout_ms ); //wait until get semaphore
        require_noerr_action( err, exit, wifi_log("DHCP ERR") );

        err = mico_rtos_get_semaphore( &net_semp, timeout_ms ); //wait until get semaphore
        require_noerr_action( err, exit, wifi_log("CONNECTED ERR") );
    }
    memcpy( mico_context->micoSystemConfig.ssid, ssid, maxSsidLen );
    memcpy( mico_context->micoSystemConfig.user_key, passwd, maxKeyLen );
    mico_context->micoSystemConfig.user_keyLength = strlen( passwd );
    memcpy( mico_context->micoSystemConfig.key,passwd, maxKeyLen );
    mico_context->micoSystemConfig.keyLength = strlen( passwd );
    mico_context->micoSystemConfig.configured = allConfigured;
    mico_context->micoSystemConfig.dhcpEnable = true;
    mico_system_context_update( mico_context ); //Update Flash content

    mico_system_delegate_config_will_stop( );

    exit:
    if ( timeout_ms != 0 )
    {
        mico_system_notify_remove( mico_notify_WIFI_STATUS_CHANGED,
                                   (void *) awsNotify_WifiStatusHandler );
        mico_system_notify_remove( mico_notify_DHCP_COMPLETED,
                                   (void *) awsNotify_DHCPCompletedlHandler );
        mico_rtos_deinit_semaphore( &net_semp );
    }
    net_semp = NULL;
    if ( err != kNoErr )
    {
        micoWlanSuspendStation( );
    } else
    {
        mico_system_delegate_config_success( CONFIG_BY_SOFT_AP );
    }

    return (err == kNoErr) ? 0 : -1;

}

void fog_wifi_pre_set( char *ssid, char *password )
{
    fog_context_t *context = fog_context_get( );
    int i = 0;

    for ( i = 0; i < MAX_WIFI_PRE_NUM; i++ )
    {

        if ( (strcmp( context->config.wifi_config.wifi_pre[i].ssid, ssid ) == 0) &&
             (strcmp( context->config.wifi_config.wifi_pre[i].key, password ) == 0) )
        {
            return;
        }

        if ( strlen( context->config.wifi_config.wifi_pre[i].ssid ) == 0 )
        {
            break;
        }
    }

    if ( i != 0 )
    {
        if ( i == MAX_WIFI_PRE_NUM ) i--;
        for ( ; i > 0; i-- )
        {
            strcpy( context->config.wifi_config.wifi_pre[i].ssid, context->config.wifi_config.wifi_pre[i - 1].ssid );
            strcpy( context->config.wifi_config.wifi_pre[i].key, context->config.wifi_config.wifi_pre[i - 1].key );
        }
    }

    strncpy( context->config.wifi_config.wifi_pre[0].ssid, ssid, MAX_SSID_LEN );
    strncpy( context->config.wifi_config.wifi_pre[0].key, password, MAX_KEY_LEN );

    fog_flash_write( );

}

static mico_semaphore_t scan_semp = NULL;
static uint8_t id = -1;

static void fogNotify_WifiScanHandler( ScanResult *pApList, void* arg )
{
    int i, j;
    fog_context_t *fog_context = fog_context_get( );

    wifi_log( "scan num: %d", pApList->ApNum );

    for ( i = 0; i < pApList->ApNum; i++ )
    {
        for ( j = 0; j < MAX_WIFI_PRE_NUM; j++ )
        {
            if ( strlen( fog_context->config.wifi_config.wifi_pre[j].ssid ) == 0 ) break;
            if ( strncmp( fog_context->config.wifi_config.wifi_pre[j].ssid, pApList->ApList[i].ssid, 32 ) == 0 )
            {
                id = j;
                goto exit;
            }
        }
    }

    exit:
    if ( scan_semp != NULL ) mico_rtos_set_semaphore( &scan_semp );
}

uint8_t fog_wifi_scan( int timeout_ms )
{
    OSStatus err = kNoErr;
    id = -1;

    err = mico_rtos_init_semaphore( &scan_semp, 1 );
    require_noerr( err, exit );

    err = mico_system_notify_register( mico_notify_WIFI_SCAN_COMPLETED,
                                       (void *) fogNotify_WifiScanHandler,
                                       0 );
    require_noerr( err, exit );

    micoWlanStartScan( );

    err = mico_rtos_get_semaphore( &scan_semp, timeout_ms ); //wait until get semaphore
    require_noerr_action( err, exit, wifi_log("scan timeout") );

    exit:
    mico_system_notify_remove( mico_notify_WIFI_SCAN_COMPLETED, (void *) fogNotify_WifiScanHandler );

    if ( scan_semp != NULL )
    {
        mico_rtos_deinit_semaphore( &scan_semp );
        scan_semp = NULL;
    }
    return id;
}

static bool fog_wifi_connected = false;

static void fogNotify_WifiStatusHandler( WiFiEvent event, mico_Context_t * const inContext )
{

    switch ( event )
    {
        case NOTIFY_STATION_UP:
            fog_wifi_connected = true;
            break;
        case NOTIFY_STATION_DOWN:
            fog_wifi_connected = false;
            break;
        default:
            break;
    }

    return;
}

static void wifi_manage_thread( uint32_t arg )
{
    OSStatus err = kNoErr;
    int pre_index = -1;
    mico_Context_t *mico_context = mico_system_context_get( );
    fog_context_t *fog_context = fog_context_get( );

    if ( mico_context->micoSystemConfig.configured == unConfigured )
    {
        goto exit;
    }

    while ( mico_context->micoSystemConfig.configured == wLanUnConfigured )
    {

        mico_rtos_thread_sleep( FOG_WIFI_SOFTAP_TIMEOUT );

        if( fog_wifi_connected == true ) goto exit;

        pre_index = fog_wifi_scan( FOG_WIFI_SCAN_TIMEOUT );
        if( pre_index < 0 ) continue;

        err = fog_softap_connect_ap( SOFTAP_CONN_TIMEOUT, fog_context->config.wifi_config.wifi_pre[pre_index].ssid,
                               fog_context->config.wifi_config.wifi_pre[pre_index].key );
        if( err == kNoErr ) goto exit;
    }

    while ( mico_context->micoSystemConfig.configured == allConfigured )
    {
        mico_rtos_thread_sleep( FOG_WIFI_STATION_TIMEOUT );

        if( fog_wifi_connected == true ) goto exit;

        pre_index = fog_wifi_scan( FOG_WIFI_SCAN_TIMEOUT );
        if( pre_index < 0 ) continue;

        fog_softap_connect_ap( 0, fog_context->config.wifi_config.wifi_pre[pre_index].ssid,
                               fog_context->config.wifi_config.wifi_pre[pre_index].key );

    }

    exit:
    wifi_log("wifi manage exit");
    mico_rtos_delete_thread( NULL );
}

void fog_wifi_connect_manage( void )
{
    mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED, (void *) fogNotify_WifiStatusHandler, 0 );
    mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "wifi manage", wifi_manage_thread, 0x800, 0 );
}

OSStatus fog_wifi_config_mode( uint8_t mode )
{
    mico_Context_t *mico_context = mico_system_context_get( );

    if ( mico_context->micoSystemConfig.configured == unConfigured )
    {
        system_log("Empty configuration. Starting configuration mode...");
        if( mode == FOG_SOFTAP_MODE )
        {
            fog_softap_start( );
        }else if( mode == FOG_AWS_SOFTAP_MODE )
        {
            mico_easylink_aws( mico_context, MICO_TRUE );
        }
//        else if( mode == FOG_AWS_SOFTAP_COMBO_MODE )
//        {
//            wifi_log("combo mode, softap:%s", fog_softap_name_get());
//            mico_wlan_monitor_enable_softap(fog_softap_name_get(), 6, aws_softap_up);
//            mico_easylink_aws( mico_context, MICO_TRUE );
//        }
        fog_wifi_config_start( );
    }
    #ifdef EasyLink_Needs_Reboot
    else if( mico_context->micoSystemConfig.configured == wLanUnConfigured )
    {
        system_log("Re-config wlan configuration. Starting configuration mode...");
        if( mode == FOG_SOFTAP_MODE )
        {
            fog_softap_start( );
        }else if( mode == FOG_AWS_SOFTAP_MODE )
        {
            mico_easylink_aws( mico_context, MICO_TRUE );
        }else if( mode == FOG_AWS_SOFTAP_COMBO_MODE )
        {
            aws_softap_combo_mode = true;
            mico_easylink_aws( mico_context, MICO_TRUE );
        }
        fog_wifi_config_start( );
    }
#endif

    else
    {
        fog_wifi_connect_manage( );
        system_log("Available configuration. Starting Wi-Fi connection...");
        system_connect_wifi_fast( system_context( ) );
    }
    return 0;
}
