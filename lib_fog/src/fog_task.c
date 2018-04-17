#include "mico.h"
#include "fog_cloud.h"
#include "fog_task.h"
#include "fog_product.h"
#include "fog_ota.h"
#include "json_parser.h"

#define task_log(M, ...) custom_log("task", M, ##__VA_ARGS__)

static fog_context_t *fog_context = NULL;
static mico_queue_t task_queue = NULL;
static mico_queue_t lan_send_queue = NULL;

fog_context_t *fog_context_get( void )
{
    return fog_context;
}

static void fog_dump_info( void )
{
    task_log("dump info start ~");
    task_log("type: %s", fog_context->config.cloud_config.type_id);
    task_log("firm ver: %s", fog_context->status.ota_status.new_version);
    task_log("device sn: %s", fog_context->status.activate_status.device_sn);
    task_log("product id: %s", fog_context->status.activate_status.product_id);
    task_log("enduser id: %s", fog_context->status.activate_status.enduser_id);
    task_log("device id: %s", fog_context->config.activate_confiog.device_id);
    task_log("region: %s", fog_context->config.cloud_config.region);
    task_log("timezero: %ld", fog_context->config.time_config.timezero);
    task_log("SDK ver:%d.%d.%d", FOG_SDK_VER_MAJOR, FOG_SDK_VER_MINOR, FOG_SDK_VER_REVISION);
    task_log("dump info end ~");
}

void fog_context_init( void )
{
    if ( fog_context == NULL )
    {
        fog_context = malloc( sizeof(fog_context_t) );
        memset( fog_context, 0x00, sizeof(fog_context_t) );
    }
}

static void task_thread( uint32_t arg )
{
    OSStatus err = kNoErr;
    task_msg_t msg;
    jsontok_t json_tokens[FOG_PA_SYS_TOPIC_NUM_TOKENS];    // Define an array of JSON tokens
    jobj_t jobj;
    char string[30];

    while ( 1 )
    {
        err = mico_rtos_pop_from_queue( &task_queue, &msg, MICO_WAIT_FOREVER );
        if ( err != kNoErr ) continue;
        // task_log("msg (%ld):%.*s", msg.len, (int)msg.len, msg.msg );

        if ( (msg.msg_type == MSG_TYPE_CLOUD_CTL) || (msg.msg_type == MSG_TYPE_LOCAL_CTL) )
        {
            if ( fog_context->status.cloud_status.set_device_status_cb )
                (fog_context->status.cloud_status.set_device_status_cb)( (char *) msg.msg, msg.len );
        }else if( msg.msg_type == MSG_TYPE_CLOUD_CMD )
        {
            err = json_init( &jobj, json_tokens, FOG_PA_SYS_TOPIC_NUM_TOKENS, (char *) (msg.msg), msg.len );
            require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "parse error!" );
            
            err = json_get_val_str( &jobj, "type", string, sizeof(string) );
            require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get code error!" );

            if ( strcmp( string, "get_status" ) == 0 )
            {
                if ( fog_context->status.cloud_status.get_device_status_cb )
                    (fog_context->status.cloud_status.get_device_status_cb)( );
            }

        }else if( msg.msg_type == MSG_TYPE_LOCAL_CMD )
        {
            if ( fog_context->status.cloud_status.get_device_status_cb )
                (fog_context->status.cloud_status.get_device_status_cb)( );
        }

        exit:
        if ( msg.msg ) free( msg.msg );
    }
}

//msg_type = 0; control
//msg_type = 0; command
OSStatus task_queue_push( char *buf, uint32_t len, uint8_t msg_type )
{
    OSStatus err = kNoErr;
    task_msg_t msg;

    require_noerr_action( task_queue == NULL, exit, err = kParamErr );

    msg.msg_type = msg_type;
    msg.len = len;
    if( buf == NULL )
    {
        msg.msg = NULL;
    }else
    {
        msg.msg = malloc( len+1 );
        require_noerr_action( msg.msg == NULL, exit, err = kNoMemoryErr );
        memcpy( msg.msg, buf, len );
    }

    err = mico_rtos_push_to_queue( &task_queue, &msg, 1000 );

    exit:
    return err;
}

OSStatus fog_register_callback( uint8_t type, void *callback )
{
    OSStatus err = kNoErr;

    fog_context_init( );

    switch ( type )
    {
        case FOG_FLASH_READ:
            fog_context->status.flash_cb.flash_read_cb = callback;
            break;
        case FOG_FLASH_WRITE:
            fog_context->status.flash_cb.flash_write_cb = callback;
            break;
        case FOG_CONNECT_STATUS:
            fog_context->status.cloud_status.connect_status_cb = callback;
            break;
        case FOG_GET_DEVICE_STATUS:
            fog_context->status.cloud_status.get_device_status_cb = callback;
            break;
        case FOG_SET_DEVICE_STATUS:
            fog_context->status.cloud_status.set_device_status_cb = callback;
            break;
        case FOG_DEVICE_TIMER:
            fog_context->status.time_status.timer_status.timer_cb = callback;
            break;
        case FOG_DEVICE_OTA_NOTIFY:
            fog_context->status.ota_status.device_ota_notify_cb = callback;
            break;
        case FOG_DEVICE_OTA_STATE:
            fog_context->status.ota_status.device_ota_state_cb = callback;
            break;
        default:
            break;
    }
    
    return err;
}

OSStatus fog_device_unbind_with_no_save( void )
{
    OSStatus err = kNoErr;
    if ( fog_context == NULL )
    {
        return err = kGeneralErr;
    }

    fog_context->config.cloud_config.need_unbind = true;
    return err;
}

OSStatus fog_device_unbind( void )
{
    OSStatus err = kNoErr;
    char *unbind_msg = "{\"type\":\"unbind\",\"data\":{\"device_id\":\"%s\",\"password\":\"%s\",\"req_id\":%d}}";
    char buf[256] = { 0 };

    if ( fog_context == NULL )
    {
        err = kGeneralErr;
        goto exit;
    }

    fog_context->config.cloud_config.need_unbind = true;
    fog_flash_write( );

    if ( fog_connect_status( ) == false )
    {
        goto exit;
    }

    sprintf( buf, unbind_msg, fog_context->config.activate_confiog.device_id,
             fog_context->status.activate_status.device_secret,
             mico_rtos_get_time( ) );
    err = fog_cloud_report( MQTT_PUB_API, buf, strlen( buf ) );

    exit:
    return err;
}

OSStatus fog_device_bind( char *enduser_id )
{
    OSStatus err = kNoErr;

    fog_enduser_id_set( enduser_id );
    fog_need_bind_set( true );

    return err;
}

OSStatus fog_flash_read( uint8_t *buf, uint32_t len )
{
    if ( (fog_context == NULL) || (fog_context->status.flash_cb.flash_read_cb == NULL) )
    {
        return -1;
    }

    (fog_context->status.flash_cb.flash_read_cb)( buf, len );

    return 0;
}

OSStatus fog_flash_write( void )
{
    CRC16_Context crc16;
    uint16_t crc;

    if ( (fog_context == NULL) || (fog_context->status.flash_cb.flash_write_cb == NULL) )
    {
        return -1;
    }

    CRC16_Init( &crc16 );
    CRC16_Update( &crc16, (uint8_t *)&(fog_context->config)+2, sizeof(fog_config_t)-2 );
    CRC16_Final( &crc16, &crc );

    fog_context->config.crc16 = crc;
    task_log("crc16: %d", crc);

    (fog_context->status.flash_cb.flash_write_cb)( (uint8_t *) &(fog_context->config), sizeof(fog_config_t) );
    return 0;
}

#ifndef LOCAL_CONTROL_MODE_ENABLE
void fog_locol_report( char *data, uint32_t len )
{

}
#endif

OSStatus fog_init( char *type_id, char *version )
{
    fog_context_init( );
    CRC16_Context crc16;
    uint16_t crc;

    fog_flash_read( (uint8_t *) &(fog_context->config), sizeof(fog_config_t) );

    CRC16_Init( &crc16 );
    CRC16_Update( &crc16, (uint8_t *)&(fog_context->config)+2, sizeof(fog_config_t)-2 );
    CRC16_Final( &crc16, &crc );

    task_log("cal crc16: %d, read crc16: %d", crc, fog_context->config.crc16);
    if ( fog_context->config.crc16 != crc )
    {
        memset( (uint8_t *) &(fog_context->config), 0x00, sizeof(fog_config_t) );
        task_log("default");
    }
    strncpy( fog_context->config.cloud_config.type_id, type_id, TYPED_ID_LEN );
    strncpy( fog_context->status.ota_status.new_version, version, FOG_OTA_VERSION_STRING_MAX_LEN );

    if( (strlen(fog_context->config.ota_config.old_version) == 0)
        || (fog_context->config.ota_config.is_ota == 0) )
    {
        strncpy( fog_context->config.ota_config.old_version, version, FOG_OTA_VERSION_STRING_MAX_LEN );
    }

    return 0;
}

OSStatus fog_start( void )
{
    OSStatus err = kUnknownErr;

    fog_context_init( );

    for(;;)
    {
        if( fog_wifi_config_complete_flag( ) )
            break;
        mico_rtos_thread_msleep(100);
    }

    mico_rtos_init_queue( &task_queue, "task queue", sizeof(task_msg_t), 5 );
    mico_rtos_init_queue( &lan_send_queue, "lan queue", sizeof(task_msg_t), 5 );

    mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "task", task_thread, 0x1200, 0 );

    strcpy( fog_context->status.activate_status.fog_id, fog_fog_id_get( ) );
    strcpy( fog_context->status.activate_status.product_id, fog_product_id_get( ) );
    strcpy( fog_context->status.activate_status.device_sn, fog_device_sn_get( ) );

    fog_device_secret_set( fog_context->status.activate_status.device_sn );
    strcpy( fog_context->status.activate_status.device_secret, fog_deivce_secret_get( ) );
    strcpy( fog_context->status.activate_status.enduser_id, fog_enduser_id_get( ) );
    strcpy( fog_context->status.activate_status.sign, fog_signature_get() );

#ifdef USE_TEST_SITE
    strncpy( fog_context_get( )->config.cloud_config.region, "test", REGION_LEN );
#endif

    fog_dump_info( );

    if( (strlen(fog_fog_id_get())<3) || (strlen(fog_signature_get())<3) )
    {
        task_log("Fog ID or Signature Error");
        goto exit;
    }

    if ( (fog_context->config.cloud_config.need_unbind == true) &&
        strlen( fog_context->config.activate_confiog.device_id ) > 0 )
    {
        err = fog_http_unbind( fog_context );
        if( err == kNoErr )
        {
            fog_context->config.cloud_config.need_unbind = false;
            memset( &(fog_context->config.activate_confiog), 0x00, sizeof(fog_activate_config_t) );
        }
    }

#ifdef MQTT_USE_SSL
    if ( fog_context->config.activate_confiog.ssl_port == 0 || fog_need_bind_get() == true )
    #else
    if ( (fog_context->config.activate_confiog.common_port == 0) || fog_need_bind_get() == true )
#endif
    {
        err = fog_activate( fog_context, fog_need_bind_get( ) );
        require_noerr( err, exit );
        fog_need_bind_set( false );
    }

    fog_activation_notify( );

    //mqtt login
    strcpy( fog_context->status.cloud_status.mqtt_login.mqtt_host, fog_context->config.activate_confiog.host );
#ifdef MQTT_USE_SSL
    fog_context->status.cloud_status.mqtt_login.mqtt_port = fog_context->config.activate_confiog.ssl_port;
#else
    fog_context->status.cloud_status.mqtt_login.mqtt_port = fog_context->config.activate_confiog.common_port;
#endif
    strcpy( fog_context->status.cloud_status.mqtt_login.mqtt_client_id, fog_context->config.activate_confiog.device_id );
    sprintf( fog_context->status.cloud_status.mqtt_login.mqtt_username, "%s/%s",
             fog_context->config.activate_confiog.endpoint_name,
             fog_context->config.activate_confiog.device_id );
    strcpy( fog_context->status.cloud_status.mqtt_login.mqtt_password, fog_context->config.activate_confiog.password );
    sprintf( fog_context->status.cloud_status.mqtt_topic, "$baidu/iot/define/%s/%s",
             fog_context->config.activate_confiog.device_id,
             fog_context->status.activate_status.product_id );

    task_log("mqtt login\r\nhost:%s\r\nport:%d\r\nclinet_id:%s\r\nusername:%s\r\npassword:%s",
        fog_context->status.cloud_status.mqtt_login.mqtt_host,
        fog_context->status.cloud_status.mqtt_login.mqtt_port,
        fog_context->status.cloud_status.mqtt_login.mqtt_client_id,
        fog_context->status.cloud_status.mqtt_login.mqtt_username,
        fog_context->status.cloud_status.mqtt_login.mqtt_password);

    err = mqtt_client_start( fog_context );
    require_noerr( err, exit );

#ifdef LOCAL_CONTROL_MODE_ENABLE
    err = coap_server_start();
    require_noerr( err, exit );
#endif

    fog_timer_init( );

    exit:
    return err;
}

