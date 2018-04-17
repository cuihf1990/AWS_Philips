#include "mico.h"
#include "json_parser.h"
#include "fog_task.h"
#include "fog_product.h"
#include "http_file_download.h"
#include "StringUtils.h"

#define ota_log(M, ...) custom_log("ota", M, ##__VA_ARGS__)

#define OTA_PROGRESS_STR "{\"type\":\"ota_progress\",\"data\":{\"ota_progress\":%d}}"
#define OTA_LOG_STR      "{\"type\":\"ota_log\",\"data\":{\"password\":\"%s\",\"ot_id\":%d,\"update_ok\":%s,\"req_id\":%d}}"

#define OTA_BIN_BUFF_LEN                     (2048)
#define OTA_MD5_MAX_LEN                      (64)

static FILE_DOWNLOAD_CONTEXT ota_file_context = NULL;
static bool ota_file_download = false;

OSStatus fog_ota_progress_report( int progress )
{
    char buf[125];

    sprintf( buf, OTA_PROGRESS_STR, progress );

    return fog_cloud_report( MQTT_PUB_STATUS, buf, strlen( buf ) );
}

OSStatus fog_ota_log_report( int is_ok )
{
    char buf[125];

    sprintf( buf, OTA_LOG_STR, fog_deivce_secret_get(), fog_context_get()->config.ota_config.ot_id, (is_ok == 1)?"true":"false", (int)mico_rtos_get_time() );

    return fog_cloud_report( MQTT_PUB_API, buf, strlen( buf ) );
}

static OSStatus ota_file_check( uint32_t ota_file_len )
{
    OSStatus err = kGeneralErr;
    md5_context ctx;
    uint8_t md5_calc[16] = { 0 };
    uint8_t md5_recv[16] = { 0 };
    uint16_t crc = 0;
    CRC16_Context crc16_contex;
    uint8_t *bin_buf = NULL;
    uint32_t read_index = 0;
    uint32_t file_len = ota_file_len;
    uint32_t need_read_len = 0;
    fog_context_t * context = fog_context_get( );

    require_string( MicoFlashGetInfo( MICO_PARTITION_OTA_TEMP )->partition_owner != MICO_FLASH_NONE, exit, "OTA storage is not exist" );

    InitMd5( &ctx );
    CRC16_Init( &crc16_contex );

    bin_buf = malloc( OTA_BIN_BUFF_LEN );
    require_string( bin_buf != NULL, exit, "malloc bin_buff failed" );

    while ( 1 )
    {
        if ( file_len - read_index >= OTA_BIN_BUFF_LEN )
        {
            need_read_len = OTA_BIN_BUFF_LEN;
        } else
        {
            need_read_len = file_len - read_index;
        }

        err = MicoFlashRead( MICO_PARTITION_OTA_TEMP, &read_index, bin_buf, need_read_len );
        require_noerr( err, exit );

        Md5Update( &ctx, bin_buf, need_read_len );
        CRC16_Update( &crc16_contex, bin_buf, need_read_len );

        if ( (read_index == ota_file_len) && (read_index != 0) )
        {
            break;
        }
    }

    Md5Final( &ctx, md5_calc );
    CRC16_Final( &crc16_contex, &crc );

    ota_log("FLASH READ: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
        md5_calc[0],md5_calc[1],md5_calc[2],md5_calc[3],
        md5_calc[4],md5_calc[5],md5_calc[6],md5_calc[7],
        md5_calc[8],md5_calc[9],md5_calc[10],md5_calc[11],
        md5_calc[12],md5_calc[13],md5_calc[14],md5_calc[15]);

    str2hex( (unsigned char *) context->status.ota_status.ota.file_md5, md5_recv, sizeof(md5_recv) );

    if ( memcmp( md5_recv, md5_calc, sizeof(md5_recv) ) == 0 )
    {
        err = mico_ota_switch_to_new_fw( ota_file_len, crc );
        require_noerr( err, exit );

        ota_log( "OTA SUCCESS!\r\n" );
    } else
    {
        ota_log("ERROR!! MD5 Error.");
        ota_log("HTTP RECV:   %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
            md5_recv[0],md5_recv[1],md5_recv[2],md5_recv[3],
            md5_recv[4],md5_recv[5],md5_recv[6],md5_recv[7],
            md5_recv[8],md5_recv[9],md5_recv[10],md5_recv[11],
            md5_recv[12],md5_recv[13],md5_recv[14],md5_recv[15]);

        err = kGeneralErr;
    }

    exit:
    if ( bin_buf != NULL )
    {
        free( bin_buf );
        bin_buf = NULL;
    }

    return err;
}

static bool ota_file_download_data_cb( void *context, const char *data, uint32_t data_len, uint32_t user_args )
{
    OSStatus err = kGeneralErr;
    FILE_DOWNLOAD_CONTEXT file_download_context = (FILE_DOWNLOAD_CONTEXT) context;
    mico_logic_partition_t *ota_partition = MicoFlashGetInfo( MICO_PARTITION_OTA_TEMP );
    uint32_t index = http_file_download_get_download_len( &file_download_context );

    //ota_log("file download, total len:%lu, get:%lu, len:%lu", file_download_context->file_info.file_total_len, file_download_context->file_info.download_len, data_len);

    require_action_string( file_download_check_control_state( file_download_context ) == true, exit, err = kGeneralErr, "user set stop download!" );
    require_string( MicoFlashGetInfo( MICO_PARTITION_OTA_TEMP )->partition_length > http_file_download_get_total_file_len( &file_download_context ), exit, "file len error!" );

    //copy data into flash
    require_string( ota_partition->partition_owner != MICO_FLASH_NONE, exit, "OTA storage is not exist" );

    err = MicoFlashWrite( MICO_PARTITION_OTA_TEMP, &index, (uint8_t *) data, data_len );
    require_noerr( err, exit );

    exit:
    if ( err == kNoErr )
    {
        return true;
    } else
    {
        return false;
    }

    return err;
}

static void ota_file_download_state_cb( void *context, HTTP_FILE_DOWNLOAD_STATE_E state, uint32_t sys_args, uint32_t user_args )
{
    OSStatus err = kGeneralErr;
    FILE_DOWNLOAD_CONTEXT file_download_context = (FILE_DOWNLOAD_CONTEXT) context;
    fog_context_t * fog_context = fog_context_get( );

    if ( state == HTTP_FILE_DOWNLOAD_STATE_START )
    {
        //erase flash
        require_string( MicoFlashGetInfo( MICO_PARTITION_OTA_TEMP )->partition_owner != MICO_FLASH_NONE, exit_http_start, "OTA storage is not exist!" );
#ifdef MICO_SYSTEM_MONITOR_ENABLE
        MicoWdgInitialize(60000);
#endif
        ota_log("erase MICO_PARTITION_OTA_TEMP flash start");

        err = MicoFlashErase( MICO_PARTITION_OTA_TEMP, 0, MicoFlashGetInfo( MICO_PARTITION_OTA_TEMP )->partition_length );
        require_noerr( err, exit_http_start );

        ota_log("erase MICO_PARTITION_OTA_TEMP flash success");

        if ( fog_context->status.ota_status.device_ota_state_cb )
            (fog_context->status.ota_status.device_ota_state_cb)( FOG_OTA_STATE_START, 0 );

        exit_http_start:
        if ( err != kNoErr )
        {
            fog_ota_progress_report( 0 );
            http_file_download_stop( &file_download_context, false );
        }
    } else if ( state == HTTP_FILE_DOWNLOAD_STATE_SUCCESS )
    {
        //calculate MD5 and crc16
        err = ota_file_check( http_file_download_get_total_file_len( &file_download_context ) );
        if ( err == kNoErr )
        {
            fog_ota_progress_report( 100 );
            fog_context->config.ota_config.is_ota = true;
            fog_flash_write( );
            if ( fog_context->status.ota_status.device_ota_state_cb )
                (fog_context->status.ota_status.device_ota_state_cb)( FOG_OTA_STATE_MD5_SUCCESS, 0 );
            mico_rtos_thread_sleep( 2 );
            MicoSystemReboot( );
        } else
        {
            fog_ota_log_report( 0 );
            if ( fog_context->status.ota_status.device_ota_state_cb )
                (fog_context->status.ota_status.device_ota_state_cb)( FOG_OTA_STATE_MD5_FAILED, 0 );
            ota_file_download = false;
        }
    } else if ( state == HTTP_FILE_DOWNLOAD_STATE_LOADING )
    {
        fog_ota_progress_report( sys_args );
        if ( fog_context->status.ota_status.device_ota_state_cb )
            (fog_context->status.ota_status.device_ota_state_cb)( FOG_OTA_STATE_LOADING, sys_args );
    } else if ( state == HTTP_FILE_DOWNLOAD_STATE_FAILED )
    {
        fog_ota_log_report( 0 );
        if ( fog_context->status.ota_status.device_ota_state_cb )
            (fog_context->status.ota_status.device_ota_state_cb)( FOG_OTA_STATE_FAILED, sys_args );
        ota_file_download = false;
    } else if ( state == HTTP_FILE_DOWNLOAD_STATE_FAILED_AND_RETRY )
    {
        if ( fog_context->status.ota_status.device_ota_state_cb )
            (fog_context->status.ota_status.device_ota_state_cb)( FOG_OTA_STATE_FAILED_AND_RETRY, sys_args );
    } else if ( state == HTTP_FILE_DOWNLOAD_STATE_SUCCESS )
    {
        if ( fog_context->status.ota_status.device_ota_state_cb )
            (fog_context->status.ota_status.device_ota_state_cb)( FOG_OTA_STATE_FILE_DOWNLOAD_SUCCESS, sys_args );
    }

    return;
}

static void ota_file_download_process( char *url )
{
    http_file_download_start( &ota_file_context, (const char *) url, ota_file_download_state_cb, ota_file_download_data_cb, 0 );
}

OSStatus fog_ota_process( char *buf, uint32_t len )
{
    OSStatus err = 0;
    jsontok_t json_tokens[FOG_PA_SYS_TOPIC_NUM_TOKENS];    // Define an array of JSON tokens
    jobj_t jobj;
    int component_num = 0;
    int i = 0;
    fog_context_t * context = fog_context_get( );

    if ( ota_file_download == true )
    {
        goto exit;
    }

    err = json_init( &jobj, json_tokens, FOG_PA_SYS_TOPIC_NUM_TOKENS, buf, len );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "parse error!" );

    err = json_get_composite_object( &jobj, "data" );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get data error!" );

    err = json_get_array_object( &jobj, "files", &component_num );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get files error!" );

    for ( i = 0; i < component_num; i++ )
    {
        err = json_array_get_composite_object( &jobj, i );
        require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get array error!" );

        err = json_get_val_str( &jobj, "file_url", context->status.ota_status.ota.file_url, FOG_OTA_FILE_URL_MAX_LEN );
        require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get file url error!" );

        err = json_get_val_str( &jobj, "component", context->status.ota_status.ota.component_name, FOG_OTA_COMPONENT_NAME_MAX_LEN );
        require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get file url error!" );

        err = json_get_val_str( &jobj, "md5", context->status.ota_status.ota.file_md5, FOG_OTA_COMPONENT_MD5_MAX_LEN );
        require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get file md5 error!" );

        err = json_get_val_str( &jobj, "version", context->status.ota_status.ota.ota_version, FOG_OTA_COMPONENT_VERSION_MAX_LEN );
        require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get file ver error!" );

        err = json_get_val_str( &jobj, "customize", context->status.ota_status.ota.custom_string, FOG_OTA_CUSTOM_STRING_MAX_LEN );
        require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get file url error!" );

        err = json_array_release_composite_object( &jobj );
        require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get array error!" );
    }

    err = json_release_array_object( &jobj );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "release array error!" );

    err = json_get_val_int( &jobj, "ot_id", &(context->config.ota_config.ot_id) );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get file url error!" );

    err = json_release_composite_object( &jobj );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "release data error!" );

    if ( context->status.ota_status.device_ota_notify_cb )
    {
        if ( 1 == (context->status.ota_status.device_ota_notify_cb)( &(context->status.ota_status.ota) ) )
        {
            ota_file_download = true;
            ota_file_download_process( context->status.ota_status.ota.file_url );
        }
    }

    exit:
    return err;
}

OSStatus fog_ota_check_start( void )
{
    OSStatus err = kNoErr;
    char *ota_msg = "{\"type\":\"ota_check\",\"data\":{\"password\":\"%s\",\"req_id\":%d}}";
    char buf[256] = { 0 };

    if ( (fog_context_get( ) == NULL) || (fog_connect_status( ) == false) )
    {
        err = kGeneralErr;
        goto exit;
    }

    sprintf( buf, ota_msg, fog_deivce_secret_get( ), mico_rtos_get_time( ) );
    err = fog_cloud_report( MQTT_PUB_API, buf, strlen( buf ) );

    exit:
    return err;
}

