#include "mico.h"
#include "http_short_connection.h"
#include "json_parser.h"
#include "fog_task.h"

#define act_log(M, ...) custom_log("act", M, ##__VA_ARGS__)

#define COMMON_HTTP_REQUSET      \
("%s %s HTTP/1.1\r\n\
Host: %s\r\n\
Content-Type: application/json\r\n\
Connection: Close\r\n\
Content-Length: %d\r\n\r\n%s")

/**
 * @brief  generate http common request
 * @param  requset_buff[in/out]: user buff
 * @param  request_buff_len[in]: user buff length
 * @param  method[in]: http method
 * @param  uri[in]: http uri
 * @param  host[in]: http host
 * @param  http_body[in]: http body string
 * @retval kNoErr is returned on success, otherwise, kXXXErr is returned.
 */
static OSStatus generate_http_common_request( char *requset_buff, uint32_t request_buff_len, char *method, const char *uri, const char *host, const char *http_body )
{
    OSStatus err = kGeneralErr;

    require_string( requset_buff != NULL && uri != NULL && host != NULL && http_body != NULL, exit, "param error" );
    require_string( (strlen( http_body ) + 384) < request_buff_len, exit, "param error" );

    memset( requset_buff, 0, request_buff_len );

    sprintf( requset_buff, COMMON_HTTP_REQUSET, method, uri, host, strlen( http_body ), http_body );

    err = kNoErr;

    exit:
    return err;
}

static OSStatus process_device_activate_response( fog_context_t *fog_context, const char *http_response )
{
    OSStatus err = kGeneralErr;
    jsontok_t json_tokens[FOG_ACTIVATE_NUM_TOKENS];    // Define an array of JSON tokens
    jobj_t jobj;
    int code = -1;

    require_string( http_response != NULL, exit, "body is NULL!!!" );

    require_string( ((*http_response == '{') && (*(http_response + strlen( http_response ) - 1) == '}')), exit, "http body JSON format error" );

    // Initialise the JSON parser and parse the given string
    err = json_init( &jobj, json_tokens, FOG_ACTIVATE_NUM_TOKENS, (char *) http_response, strlen( http_response ) );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "parse error!" );

    err = json_get_composite_object( &jobj, "meta" );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get meta error!" );

    //get code
    err = json_get_val_int( &jobj, "code", &code );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get code error!" );

    err = json_release_composite_object( &jobj );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "release meta error!" );

    require_action_string( code == 0, exit, err = kGeneralErr, "activate response code error!" );

    err = json_get_composite_object( &jobj, "data" );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get data error!" );

    //get fog mqtt parament
    err = json_get_val_str( &jobj, "host",
                            fog_context->config.activate_confiog.host,
                            sizeof(fog_context->config.activate_confiog.host) );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get host error!" );

    err = json_get_val_int( &jobj, "common_port",
                            &(fog_context->config.activate_confiog.common_port) );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get common_port error!" );

    err = json_get_val_int( &jobj, "ssl_port",
                            &(fog_context->config.activate_confiog.ssl_port) );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get ssl_port error!" );

    err = json_get_val_str( &jobj, "device_id",
                            fog_context->config.activate_confiog.device_id,
                            sizeof(fog_context->config.activate_confiog.device_id) );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get device_id error!" );

    err = json_get_val_str( &jobj, "endpoint_name",
                            fog_context->config.activate_confiog.endpoint_name,
                            sizeof(fog_context->config.activate_confiog.endpoint_name) );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get endpoint_name error!" );

    err = json_get_val_str( &jobj, "password",
                            fog_context->config.activate_confiog.password,
                            sizeof(fog_context->config.activate_confiog.password) );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get password error!" );

    err = json_release_composite_object( &jobj );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "release data error!" );

    exit:
    if ( err != kNoErr )
    {
        act_log("http activate response err, code = %d", code);
    }

    return err;
}


OSStatus fog_activate( void *context, bool is_bind )
{
    OSStatus err = kGeneralErr;
    HTTP_REQ_S https_req = HTTP_REQ_INIT_PARAM;
    const char *bind_body_str = "{\"fog_id\":\"%s\",\"product_id\":\"%s\",\"dsn\":\"%s\",\"password\":\"%s\",\"enduser_id\":\"%s\",\"sign\":\"%s\"}";
    const char *act_body_str = "{\"fog_id\":\"%s\",\"product_id\":\"%s\",\"dsn\":\"%s\",\"password\":\"%s\",\"sign\":\"%s\"}";
    char http_body_buff[512] = { 0 };
    char domain[30] = {0};
    fog_context_t *fog_context = (fog_context_t *) context;

    require_string( fog_context != NULL, exit, "param error" );

    memset( &https_req, 0, sizeof(https_req) );

    sprintf(domain, "%s%s", fog_context->config.cloud_config.region, FOG_DOMAIN);

    https_req.domain_name = domain;
    https_req.port = FOG_PORT;
    https_req.is_success = false;
    https_req.timeout_ms = FOG_ACTIVATE_TIMEOUT;

    https_req.http_req = malloc( FOG_ACTIVATE_REQ_BUFF_LEN );
    require_string( https_req.http_req != NULL, exit, "malloc error" );
    memset( https_req.http_req, 0, FOG_ACTIVATE_REQ_BUFF_LEN );

    https_req.http_res = malloc( FOG_ACTIVATE_RES_BUFF_LEN );
    require_string( https_req.http_res != NULL, exit, "malloc error" );
    memset( https_req.http_res, 0, FOG_ACTIVATE_RES_BUFF_LEN );
    https_req.res_len = FOG_ACTIVATE_RES_BUFF_LEN;

    if ( is_bind == true )
    {
        sprintf( http_body_buff, bind_body_str,
                 fog_context->status.activate_status.fog_id,
                 fog_context->status.activate_status.product_id,
                 fog_context->status.activate_status.device_sn,
                 fog_context->status.activate_status.device_secret,
                 fog_context->status.activate_status.enduser_id,
                 fog_context->status.activate_status.sign);
    } else
    {
        sprintf( http_body_buff, act_body_str,
                 fog_context->status.activate_status.fog_id,
                 fog_context->status.activate_status.product_id,
                 fog_context->status.activate_status.device_sn,
                 fog_context->status.activate_status.device_secret,
                 fog_context->status.activate_status.sign);
    }

    generate_http_common_request( https_req.http_req, FOG_ACTIVATE_REQ_BUFF_LEN,
    FOG_ACTIVATE_METHOD,
                                  FOG_ACTIVATE_URI,
                                  domain,
                                  http_body_buff );

    https_req.req_len = strlen( https_req.http_req );

    act_log( "http request:\r\n%s", https_req.http_req );

    do
    {
        err = http_short_connection_ssl( &https_req );
        if ( https_req.is_success == false )
        {
            mico_rtos_thread_sleep( 2 );
        }
    } while ( https_req.is_success == false );

    act_log( "[activate]http response:\r\n%s", https_req.http_res );

    err = process_device_activate_response( fog_context, (const char *) https_req.http_res );
    require_noerr_string( err, exit, "process device activate response error!" );

    exit:
    //free request buff
    if ( https_req.http_req != NULL )
    {
        free( https_req.http_req );
        https_req.http_req = NULL;
    }

    //free response buff
    if ( https_req.http_res != NULL )
    {
        free( https_req.http_res );
        https_req.http_res = NULL;
    }

    return err;
}

static OSStatus process_device_unbind_response( fog_context_t *fog_context, const char *http_response )
{
    OSStatus err = kGeneralErr;
    jsontok_t json_tokens[FOG_UNBIND_NUM_TOKENS];    // Define an array of JSON tokens
    jobj_t jobj;
    int code = -1;

    require_string( http_response != NULL, exit, "body is NULL!!!" );

    require_string( ((*http_response == '{') && (*(http_response + strlen( http_response ) - 1) == '}')), exit, "http body JSON format error" );

    // Initialise the JSON parser and parse the given string
    err = json_init( &jobj, json_tokens, FOG_ACTIVATE_NUM_TOKENS, (char *) http_response, strlen( http_response ) );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "parse error!" );

    err = json_get_composite_object( &jobj, "meta" );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get meta error!" );

    //get code
    err = json_get_val_int( &jobj, "code", &code );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get code error!" );

    err = json_release_composite_object( &jobj );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "release meta error!" );

    require_action_string( code == 0, exit, err = kGeneralErr, "activate response code error!" );

    exit:
    if ( err != kNoErr )
    {
        act_log("http unbind response err, code = %d", code);
    }

    return err;
}

OSStatus fog_http_unbind( void *context )
{
    OSStatus err = kGeneralErr;
    HTTP_REQ_S https_req = HTTP_REQ_INIT_PARAM;
    const char *unbind_body_str = "{\"device_id\":\"%s\",\"password\":\"%s\",\"req_type\":0,\"req_id\":\"%d\"}";
    char http_body_buff[250] = { 0 };
    char domain[30] = {0};
    fog_context_t *fog_context = (fog_context_t *) context;

    require_string( fog_context != NULL, exit, "param error" );

    memset( &https_req, 0, sizeof(https_req) );

    sprintf(domain, "%s%s", fog_context->config.cloud_config.region, FOG_DOMAIN);

    https_req.domain_name = domain;
    https_req.port = FOG_PORT;
    https_req.is_success = false;
    https_req.timeout_ms = FOG_UNBIND_TIMEOUT;

    https_req.http_req = malloc( FOG_UNBIND_REQ_BUFF_LEN );
    require_string( https_req.http_req != NULL, exit, "malloc error" );
    memset( https_req.http_req, 0, FOG_UNBIND_REQ_BUFF_LEN );

    https_req.http_res = malloc( FOG_UNBIND_RES_BUFF_LEN );
    require_string( https_req.http_res != NULL, exit, "malloc error" );
    memset( https_req.http_res, 0, FOG_UNBIND_RES_BUFF_LEN );
    https_req.res_len = FOG_UNBIND_RES_BUFF_LEN;

    sprintf( http_body_buff, unbind_body_str,
             fog_context->config.activate_confiog.device_id,
             fog_context->status.activate_status.device_secret,
             mico_rtos_get_time( ) );

    generate_http_common_request( https_req.http_req, FOG_UNBIND_REQ_BUFF_LEN,
                                  FOG_UNBIND_METHOD,
                                  FOG_UNBIND_URI,
                                  domain,
                                  http_body_buff );

    https_req.req_len = strlen( https_req.http_req );

    act_log( "http request:\r\n%s", https_req.http_req );

    do
    {
        err = http_short_connection_ssl( &https_req );
        if ( https_req.is_success == false )
        {
            mico_rtos_thread_sleep( 2 );
        }
    } while ( https_req.is_success == false );

    act_log( "[activate]http response:\r\n%s", https_req.http_res );

    err = process_device_unbind_response( fog_context, (const char *) https_req.http_res );
    require_noerr_string( err, exit, "process device unbind response error!" );

    fog_timer_task_clean( );

    exit:
    //free request buff
    if ( https_req.http_req != NULL )
    {
        free( https_req.http_req );
        https_req.http_req = NULL;
    }

    //free response buff
    if ( https_req.http_res != NULL )
    {
        free( https_req.http_res );
        https_req.http_res = NULL;
    }

    return err;
}

