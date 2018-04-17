#include "fog_wifi_config.h"

#include "mico.h"
#include "fog_product.h"
#include "fog_task.h"
#include "SocketUtils.h"
#include "json_parser.h"
#include "json_generator.h"
#include "StringUtils.h"

#define config_log(format, ...)  custom_log("config", format, ##__VA_ARGS__)

#define WIFI_SCAN_TIMEOUT 5*1000
#define WIFI_RESULT_BUFF 2048
static bool should_close = false;
static mico_semaphore_t scan = NULL;
static wifi_config_t *wifi_config = NULL;
static bool wifi_config_complete_flag = true;

void aws_delegate_recv_notify(char *data, uint32_t data_len)
{
    OSStatus err = kNoErr;
    jobj_t jobj;
    jsontok_t json_tokens[20];
    char string[33];

    require_action_string( ((*data == '{') && (*(data + data_len - 1) == '}')), exit, err = kParamErr, "body JSON format error" );

    err = json_init( &jobj, json_tokens, 50, (char *) data, data_len );
    require_noerr( err, exit );

    err = json_get_val_str( &jobj, "IP", string, sizeof(string) );
    require_noerr( err, exit );

    strcpy(fog_context_get()->status.activate_status.aws_notify_ip, string);

    exit:
    return;
}


/*
 * return 1 ,should close
 */
static int wifi_config_handle( int fd, uint8_t *data, uint32_t data_len )
{
    OSStatus err = kNoErr;
    jobj_t jobj;
    jsontok_t json_tokens[20];
    char string[33];
    int timezero = 0;
    char *result = NULL; //"{\"status\":\"ok\"}";
    struct json_str jstr;

    result = malloc( WIFI_RESULT_BUFF );
    require_action( result, exit, err = kNoMemoryErr );
    memset( result, 0x0, WIFI_RESULT_BUFF );

    json_str_init( &jstr, result, WIFI_RESULT_BUFF );

    require_action_string( ((*data == '{') && (*(data + data_len - 1) == '}')), exit, err = kParamErr, "body JSON format error" );

    config_log("config recv:%s", data);
    err = json_init( &jobj, json_tokens, 50, (char *) data, data_len );
    require_noerr( err, exit );

    err = json_get_val_str( &jobj, "type", string, sizeof(string) );
    require_noerr( err, exit );

    if ( strcmp( string, "config" ) == 0 )
    {
        err = json_get_composite_object( &jobj, "data" );
        require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get data error!" );

        err = json_get_val_str( &jobj, "product_id", string, sizeof(string) );
        require_noerr( err, exit );
        config_log("product_id:%s", string);
        fog_product_id_set( string, strlen( string ) );

        err = json_get_val_str( &jobj, "type_id", string, sizeof(string) );
        require_noerr( err, exit );
        config_log("type_id:%s", string);
        fog_type_id_set( string );

        err = json_get_val_str( &jobj, "region", string, sizeof(string) );
        require_noerr( err, exit );
        config_log("region:%s", string);
        fog_region_set( string );

        err = json_get_val_int( &jobj, "timezero", &timezero );
        require_noerr( err, exit );
        config_log("timezero:%d", timezero);
        fog_timezero_set( timezero );

        err = json_get_val_str( &jobj, "enduser_id", string, sizeof(string) );
        require_noerr( err, exit );
        config_log("enduser_id:%s", string);
        fog_enduser_id_set( string );

        err = json_get_val_str( &jobj, "ssid", string, sizeof(string) );
        if( err == kNoErr )
        {
            config_log("ssid:%s", string);
            strncpy( wifi_config->ssid, string, MAX_SSID_LEN );
        }

        err = json_get_val_str( &jobj, "password", string, sizeof(string) );
        if( err == kNoErr )
        {
            config_log("password:%s", string);
            strncpy( wifi_config->key, string, MAX_KEY_LEN );
        }

        err = json_release_composite_object( &jobj );
        require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "release data error!" );

    }

    exit:

    json_start_object( &jstr );
    json_set_val_str( &jstr, "type", "config" );
    json_push_object( &jstr, "meta" );
    if( err == kNoErr )
    {
        json_set_val_str(&jstr, "message", "config success");
        json_set_val_int(&jstr, "code", 0);
        err = 1;
    }else if( err == kParamErr )
    {
        json_set_val_str(&jstr, "message", "no json");
        json_set_val_int(&jstr, "code", 1000);
    }else{
        json_set_val_str(&jstr, "message", "param err");
         json_set_val_int(&jstr, "code", 1001);
    }
    json_pop_object( &jstr );
    json_close_object( &jstr );

    config_log("send:%s", result);
    SocketSend( fd, (const uint8_t *) result, strlen( result ) );

    if ( result ) free( result );
    return err;
}

static void local_client_thread( mico_thread_arg_t arg )
{
    OSStatus err = kNoErr;
    int socket_fd = *(int *) arg;
    int len;
    uint8_t *inDataBuffer = NULL;
    struct timeval t;
    fd_set readfds;

    inDataBuffer = calloc( 1, FOG_WIFI_CONFIG_SERVER_BUFF_LEN );
    require_action( inDataBuffer, exit, err = kNoMemoryErr );

    while ( 1 )
    {
        t.tv_sec = 2;
        t.tv_usec = 0;

        FD_ZERO( &readfds );
        FD_SET( socket_fd, &readfds );

        select( socket_fd + 1, &readfds, NULL, NULL, &t );

        if ( FD_ISSET( socket_fd, &readfds ) )
        {
            len = recv( socket_fd, inDataBuffer, FOG_WIFI_CONFIG_SERVER_BUFF_LEN, 0 );
            if ( len <= 0 )
            {
                goto exit;
            }

            if ( 1 == wifi_config_handle( socket_fd, inDataBuffer, len ) )
            {
                fog_need_bind_set( true );
                should_close = true;
                wifi_config->err = kNoErr;
                goto exit;
            }
        }
    }

    exit:
    if ( socket_fd >= 0 ) close( socket_fd );
    if ( inDataBuffer ) free( inDataBuffer );

    config_log( "close port %d, err:%d", socket_fd, err);
    mico_rtos_delete_thread( NULL );
}

OSStatus wifi_config_server_start( void )
{
    OSStatus err = kNoErr;
    int local_fd = -1;
    int client_fd = -1;
    struct sockaddr_in addr;
    fd_set readfds;
    int sockaddr_t_size;
    struct timeval t;

    local_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    require_action( IsValidSocket( local_fd ), exit, err = kNoResourcesErr );

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons( FOG_WIFI_CONFIG_SERVER_PORT );
    err = bind( local_fd, (struct sockaddr *) &addr, sizeof(addr) );
    require_noerr( err, exit );

    err = listen( local_fd, 0 );
    require_noerr( err, exit );

    config_log( "config server established at port %d, fd %d", FOG_WIFI_CONFIG_SERVER_PORT, local_fd );

    t.tv_sec = 1;
    t.tv_usec = 0;

    while ( 1 )
    {
        FD_ZERO( &readfds );
        FD_SET( local_fd, &readfds );

        select( local_fd + 1, &readfds, NULL, NULL, &t );

        if ( FD_ISSET( local_fd, &readfds ) )
        {
            sockaddr_t_size = sizeof(struct sockaddr_in);
            client_fd = accept( local_fd, (struct sockaddr *) &addr, (socklen_t *) &sockaddr_t_size );

            if ( IsValidFD( client_fd ) )
            {

                config_log( "Client %s:%d connected, fd: %d", inet_ntoa( addr.sin_addr ), ntohs( addr.sin_port ), client_fd );

                if ( kNoErr
                     != mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "local clients", local_client_thread,
                                                 0x1000,
                                                 (uint32_t) &client_fd ) )
                {
                    close( client_fd );
                    client_fd = -1;
                }
            }
        }

        if ( should_close == true )
        {
            should_close = false;
            goto exit;
        }
    }

    exit:
    close( local_fd );
    config_log("config server exit");
    return err;
}

static void wifi_config_thread( mico_thread_arg_t arg )
{
    OSStatus err = kNoErr;

    if ( wifi_config == NULL ){
        wifi_config = malloc( sizeof(wifi_config_t) );
    }
    memset( wifi_config, 0x00, sizeof(wifi_config_t) );

    wifi_config->err = kGeneralErr;

    wifi_config_server_start( );

    if ( (wifi_config->err == kNoErr) && (strlen(wifi_config->ssid) > 0) )
    {
        err = fog_softap_connect_ap( SOFTAP_CONN_TIMEOUT, wifi_config->ssid, wifi_config->key);
        if ( err != kNoErr )
        {
            config_log(" wifi connect err ");
        }
    }

    config_log( "wifi config thread exit, %d", err );
    if ( wifi_config )
    {
        free( wifi_config );
        wifi_config = NULL;
        }
    wifi_config_complete_flag = true;

    mico_rtos_delete_thread( NULL );
}

bool fog_wifi_config_complete_flag( void )
{
    return wifi_config_complete_flag;
}

void fog_wifi_config_start( void )
{
    static int softap_flag;
    wifi_config_complete_flag = false;
    
    if ( softap_flag != 1 )
    {
        softap_flag = 1;
        mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "wifi config", wifi_config_thread, 0x800, 0 );
    }
}

