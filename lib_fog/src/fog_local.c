#include "mico.h"
#include "fog_local.h"
#include "fog_task.h"
#include "CoAPExport.h"
#include "CoAPPlatform.h"

#define coaps_log(M, ...) custom_log("coap", M, ##__VA_ARGS__)

static CoAPContext *coap_context = NULL;

void device_info_callback( CoAPContext *context, const char *paths, NetworkAddr *remote, CoAPMessage *request )
{
    int ret = COAP_SUCCESS;
    char *dev_info = "{\"product_id\":\"%s\",\"type_id\":\"%s\",\"device_id\":\"%s\"}";
    unsigned char payload[256] = { 0 };
    CoAPMessage response;
    unsigned int observe = 0;

    CoAPMessage_init( &response );
    CoAPMessageType_set( &response, COAP_MESSAGE_TYPE_NON );
    CoAPMessageCode_set( &response, COAP_MSG_CODE_205_CONTENT );
    CoAPMessageId_set( &response, request->header.msgid );
    CoAPMessageToken_set( &response, request->token, request->header.tokenlen );

    CoAPUintOption_add( &response, COAP_OPTION_CONTENT_FORMAT, COAP_CT_APP_JSON );
    snprintf( (char *) payload, sizeof(payload), dev_info, fog_product_id_get( ), fog_type_id_get( ), fog_context_get()->config.activate_confiog.device_id );
    CoAPMessagePayload_set( &response, payload, strlen( (char *) payload ) );

    coaps_log("Send a response message:%s", payload);
    CoAPMessage_send( context, remote, &response );
    CoAPMessage_destory( &response );
}

void device_status_callback( CoAPContext *context, const char *paths, NetworkAddr *remote, CoAPMessage *request )
{
    int ret = COAP_SUCCESS;
    CoAPMessage response;
    unsigned int observe = 0;

    CoAPMessage_init( &response );
    CoAPMessageType_set( &response, COAP_MESSAGE_TYPE_NON );
    CoAPMessageCode_set( &response, COAP_MSG_CODE_205_CONTENT );
    CoAPMessageId_set( &response, request->header.msgid );
    CoAPMessageToken_set( &response, request->token, request->header.tokenlen );

    ret = CoAPUintOption_get( request, COAP_OPTION_OBSERVE, &observe );
    if ( COAP_SUCCESS == ret && 0 == observe )
    {
        CoAPObsServer_add( context, paths, remote, request );
        CoAPUintOption_add( &response, COAP_OPTION_OBSERVE, 0 );
    }

    CoAPUintOption_add( &response, COAP_OPTION_CONTENT_FORMAT, COAP_CT_APP_JSON );
    CoAPMessagePayload_set( &response, (unsigned char *) COAP_RESP_OK, strlen( COAP_RESP_OK ) );

    CoAPMessage_send( context, remote, &response );
    CoAPMessage_destory( &response );

    task_queue_push( NULL, 0, MSG_TYPE_LOCAL_CMD );
}

void device_control_callback( CoAPContext *context, const char *paths, NetworkAddr *remote, CoAPMessage *request )
{
    int ret = COAP_SUCCESS;
    CoAPMessage response;
    unsigned int observe = 0;

    CoAPMessage_init( &response );
    CoAPMessageType_set( &response, COAP_MESSAGE_TYPE_NON );
    CoAPMessageCode_set( &response, COAP_MSG_CODE_205_CONTENT );
    CoAPMessageId_set( &response, request->header.msgid );
    CoAPMessageToken_set( &response, request->token, request->header.tokenlen );

    CoAPUintOption_add( &response, COAP_OPTION_CONTENT_FORMAT, COAP_CT_APP_JSON );
    CoAPMessagePayload_set( &response, (unsigned char *) COAP_RESP_OK, strlen( COAP_RESP_OK ) );

    CoAPMessage_send( context, remote, &response );
    CoAPMessage_destory( &response );

    coaps_log("payload:%s", request->payload);
    task_queue_push( (char *) request->payload, request->payloadlen, MSG_TYPE_LOCAL_CTL );
}

void fog_locol_report( char *data, uint32_t len )
{
    if ( (coap_context == NULL) || (data == NULL) || (len == 0) )
        return;

    CoAPObsServer_notify( coap_context, DEV_STATUS_URL, (unsigned char *) data, len, NULL );
}

static void coap_server_thread( uint32_t arg )
{
    CoAPInitParam param;

    coaps_log("coap server init");

    param.appdata = NULL;
    param.group = MULTICAST_ADDR;
    param.notifier = NULL;
    param.obs_maxcount = 16;
    param.res_maxcount = 32;
    param.port = COAP_PORT;
    param.send_maxcount = 16;
    param.waittime = 2000;

    coap_context = CoAPContext_create( &param );

    CoAPResource_register( coap_context, DEV_INFO_URI, COAP_PERM_GET, COAP_CT_APP_JSON, 60, device_info_callback );
    CoAPResource_register( coap_context, DEV_STATUS_URL, COAP_PERM_GET, COAP_CT_APP_JSON, 60, device_status_callback );
    CoAPResource_register( coap_context, DEV_CONTROL, COAP_PERM_POST, COAP_CT_APP_JSON, 60, device_control_callback );

    while ( 1 )
    {
        CoAPMessage_cycle( coap_context );
        mico_rtos_thread_msleep( 50 );
    }
}

OSStatus coap_server_start( void )
{
    return mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "coap", coap_server_thread, 0x800, NULL );
}

