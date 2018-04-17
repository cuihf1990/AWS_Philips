#include "mico.h"
#include "fog_cloud.h"
#include "fog_task.h"
#include "fog_ota.h"
#include "mqtt_client_interface.h"
#include "json_parser.h"

#define mqtt_log(M, ...) custom_log("mqtt", M, ##__VA_ARGS__)

static mico_queue_t cloud_send_queue = NULL;
static char sub_ctl_topic[MQTT_TOPIC_MAX];
static char sub_sys_topic[MQTT_TOPIC_MAX];
static char sub_cmd_topic[MQTT_TOPIC_MAX];
static char lwt_topic[MQTT_TOPIC_MAX];

static void iot_subscribe_ctl_callback_handler( MQTT_Client *pClient, char *topicName,
                                                uint16_t topicNameLen,
                                                IoT_Publish_Message_Params *params,
                                                void *pData )
{
    IOT_UNUSED( pData );
    IOT_UNUSED( pClient );
    mqtt_log("Subscribe ctl callback:%.*s\r\n%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *)(params->payload));

    task_queue_push( params->payload, params->payloadLen, MSG_TYPE_CLOUD_CTL );
}

static void iot_subscribe_cmd_callback_handler( MQTT_Client *pClient, char *topicName,
                                                uint16_t topicNameLen,
                                                IoT_Publish_Message_Params *params,
                                                void *pData )
{
    IOT_UNUSED( pData );
    IOT_UNUSED( pClient );
    mqtt_log("Subscribe cmd callback:%.*s\r\n%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *)(params->payload));

    task_queue_push( params->payload, params->payloadLen, MSG_TYPE_CLOUD_CMD );
}

static void iot_subscribe_sys_callback_handler( MQTT_Client *pClient, char *topicName,
                                                uint16_t topicNameLen,
                                                IoT_Publish_Message_Params *params,
                                                void *pData )
{
    OSStatus err = 0;
    IOT_UNUSED( pData );
    IOT_UNUSED( pClient );

    jsontok_t json_tokens[FOG_PA_SYS_TOPIC_NUM_TOKENS];    // Define an array of JSON tokens
    jobj_t jobj;
    char string[30];
    int code = 0;

    mqtt_log("Subscribe sys callback:%.*s\r\n%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *)(params->payload));

    // Initialise the JSON parser and parse the given string
    err = json_init( &jobj, json_tokens, FOG_PA_SYS_TOPIC_NUM_TOKENS, (char *) (params->payload), params->payloadLen );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "parse error!" );

    err = json_get_val_str( &jobj, "type", string, sizeof(string) );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get code error!" );

    err = json_get_composite_object( &jobj, "meta" );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get meta error!" );

    //get code
    err = json_get_val_int( &jobj, "code", &code );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get code error!" );

    err = json_release_composite_object( &jobj );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "release meta error!" );

    if ( strcmp( string, "unbind" ) == 0 )
    {
        if ( code == 0 )
        {
            fog_timer_task_clean( );
            fog_context_get( )->config.cloud_config.need_unbind = false;
            fog_flash_write( );
            mqtt_log("device unbind success");
        }else{
            mqtt_log("device unbind fail");
        }
    } else if ( strcmp( string, "bind" ) == 0 )
    {

    } else if ( strcmp( string, "ota_check" ) == 0 )
    {
        if ( code == 0 )
        {
            fog_ota_process( (char *)(params->payload), (uint32_t) params->payloadLen );
        }else{
            mqtt_log("device ota check fail");
        }
    }

    exit:
    return;
}

OSStatus fog_cloud_report( uint8_t type, char *buf, uint32_t len )
{
    OSStatus err = kNoErr;
    cloud_msg_t msg;

    require_noerr_action( cloud_send_queue == NULL, exit, err = kParamErr );

    msg.type = type;
    msg.len = len;
    msg.msg = malloc( len );
    require_noerr_action( msg.msg == NULL, exit, err = kNoMemoryErr );
    memcpy( msg.msg, buf, len );

    err = mico_rtos_push_to_queue( &cloud_send_queue, &msg, 1000 );

    exit:
    return err;
}

bool fog_connect_status( void )
{
    if ( fog_context_get( ) )
    {
        return fog_context_get( )->status.cloud_status.connect_status;
    } else
    {
        return false;
    }
}

OSStatus cloud_pop_form_queue( void * message, uint32_t timeout_ms )
{
    return mico_rtos_pop_from_queue( &cloud_send_queue, message, timeout_ms );
}

static void mqtt_lwt_init( fog_context_t *context, IoT_Client_Connect_Params *pConnectParams )
{
    sprintf( lwt_topic, "%s%s", context->status.cloud_status.mqtt_topic, MQTT_PUB_ONLINE_TOPIC );
    pConnectParams->isWillMsgPresent = true;
    strcpy( pConnectParams->will.struct_id, "MQTW" );
    pConnectParams->will.pTopicName = lwt_topic;
    pConnectParams->will.topicNameLen = strlen( lwt_topic );
    pConnectParams->will.pMessage = MQTT_LWT_MSG;
    pConnectParams->will.msgLen = strlen( MQTT_LWT_MSG );
    pConnectParams->will.qos = QOS1;
    pConnectParams->will.isRetained = true;
}

static IoT_Error_t mqtt_subscribe_topic( MQTT_Client *pClient, fog_context_t *context )
{
    IoT_Error_t rc = MQTT_FAILURE;

    sprintf( sub_ctl_topic, "%s%s", context->status.cloud_status.mqtt_topic, MQTT_SUB_CTL_TOPIC );
    rc = mqtt_subscribe( pClient, sub_ctl_topic, strlen( sub_ctl_topic ), QOS0, iot_subscribe_ctl_callback_handler, NULL );
    if ( MQTT_SUCCESS != rc )
    {
        mqtt_log("subscribing:%s, Error %d ", sub_ctl_topic, rc);
        goto exit;
    }
    mqtt_log("Subscribed topic: %s", sub_ctl_topic);

    sprintf( sub_sys_topic, "%s%s", context->status.cloud_status.mqtt_topic, MQTT_SUB_SYS_TOPIC );
    rc = mqtt_subscribe( pClient, sub_sys_topic, strlen( sub_sys_topic ), QOS1, iot_subscribe_sys_callback_handler, NULL );
    if ( MQTT_SUCCESS != rc )
    {
        mqtt_log("subscribing:%s, Error %d ", sub_sys_topic, rc);
        goto exit;
    }
    mqtt_log("Subscribed topic: %s", sub_sys_topic);

    sprintf( sub_cmd_topic, "%s%s", context->status.cloud_status.mqtt_topic, MQTT_SUB_CMD_TOPIC );
    rc = mqtt_subscribe( pClient, sub_cmd_topic, strlen( sub_cmd_topic ), QOS1, iot_subscribe_cmd_callback_handler, NULL );
    if ( MQTT_SUCCESS != rc )
    {
        mqtt_log("subscribing:%s, Error %d ", sub_cmd_topic, rc);
        goto exit;
    }
    mqtt_log("Subscribed topic: %s", sub_cmd_topic);

    exit:
    return rc;
}

static IoT_Error_t mqtt_publish_online_notify( MQTT_Client *pClient )
{
    char online_msg[125] = {0};
    IoT_Error_t rc = MQTT_FAILURE;
    IoT_Publish_Message_Params paramsQOS1;

    sprintf( online_msg, "{\"type\":\"online\",\"data\":{\"online\":true,\"version\":\"%s\"}}",
             fog_context_get( )->status.ota_status.new_version);

    paramsQOS1.qos = QOS1;
    paramsQOS1.isRetained = 1;
    paramsQOS1.payload = online_msg;
    paramsQOS1.payloadLen = strlen( online_msg );

    mqtt_log("notify:%s:%s", lwt_topic, online_msg);

    rc = mqtt_publish( pClient, lwt_topic, strlen( lwt_topic ), &paramsQOS1 );
    if ( MQTT_SUCCESS != rc )
    {
        mqtt_log("online notify publish err");
    }

    if( fog_context_get( )->config.ota_config.is_ota == 1 )
    {
        if( strcmp(fog_context_get( )->status.ota_status.new_version, fog_context_get( )->config.ota_config.old_version) == 0 )
        {
            fog_ota_log_report( 0 );
        }else
        {
            fog_ota_log_report( 1 );
            strcpy(fog_context_get( )->config.ota_config.old_version, fog_context_get( )->status.ota_status.new_version);
        }
        fog_context_get( )->config.ota_config.is_ota = 0;
        fog_flash_write( );
    }
    return rc;
}

static void mqtt_thread( mico_thread_arg_t arg )
{
    IoT_Error_t rc = MQTT_FAILURE;

    fog_context_t *context = (fog_context_t *) arg;
    MQTT_Client client;
    IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
    IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;
    IoT_Publish_Message_Params paramsQOS;
    cloud_msg_t msg;
    char pub_topic[MQTT_TOPIC_MAX];

    context->status.cloud_status.connect_status = false;
    /*
     * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
     *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
     *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
     */
    mqttInitParams.enableAutoReconnect = false;
    mqttInitParams.pHostURL = context->status.cloud_status.mqtt_login.mqtt_host;
    mqttInitParams.port = context->status.cloud_status.mqtt_login.mqtt_port;
    mqttInitParams.mqttPacketTimeout_ms = 20000;
    mqttInitParams.mqttCommandTimeout_ms = 20000;
    mqttInitParams.tlsHandshakeTimeout_ms = 5000;
    mqttInitParams.disconnectHandler = NULL;
    mqttInitParams.disconnectHandlerData = NULL;
#ifdef MQTT_USE_SSL
    mqttInitParams.pRootCALocation = NULL;
    mqttInitParams.pDeviceCertLocation = NULL;
    mqttInitParams.pDevicePrivateKeyLocation = NULL;
    mqttInitParams.isSSLHostnameVerify = false;
    mqttInitParams.isClientnameVerify = false;
    mqttInitParams.isUseSSL = true;
#else
    mqttInitParams.pRootCALocation = NULL;
    mqttInitParams.pDeviceCertLocation = NULL;
    mqttInitParams.pDevicePrivateKeyLocation = NULL;
    mqttInitParams.isSSLHostnameVerify = false;
    mqttInitParams.isClientnameVerify = false;
    mqttInitParams.isUseSSL = false;
#endif

    RECONN:

    rc = mqtt_init( &client, &mqttInitParams );
    if ( MQTT_SUCCESS != rc )
    {
        mqtt_log("aws_iot_mqtt_init returned error : %d ", rc);
        goto exit;
    }

    connectParams.keepAliveIntervalInSec = 10;
    connectParams.isCleanSession = true;
    connectParams.MQTTVersion = MQTT_3_1_1;
    connectParams.pClientID = context->status.cloud_status.mqtt_login.mqtt_client_id;
    connectParams.clientIDLen = (uint16_t) strlen( context->status.cloud_status.mqtt_login.mqtt_client_id );
    connectParams.pUsername = context->status.cloud_status.mqtt_login.mqtt_username;
    connectParams.usernameLen = strlen( context->status.cloud_status.mqtt_login.mqtt_username );
    connectParams.pPassword = context->status.cloud_status.mqtt_login.mqtt_password;
    connectParams.passwordLen = strlen( context->status.cloud_status.mqtt_login.mqtt_password );

    mqtt_lwt_init( context, &connectParams );

    rc = mqtt_connect( &client, &connectParams );
    if ( MQTT_SUCCESS != rc )
    {
        context->status.cloud_status.connect_status = false;
        mqtt_log("Error(%d) connecting to %s:%d", rc, mqttInitParams.pHostURL, mqttInitParams.port);
        mico_rtos_thread_sleep( 2 );
        goto RECONN;
    }
    mqtt_log("Connected");

    /*Subscribe  start*/
    rc = mqtt_subscribe_topic( &client, context );
    if ( MQTT_SUCCESS != rc )
    {
        mqtt_log("Error subscribing : %d ", rc);
        context->status.cloud_status.connect_status = false;
        mqtt_disconnect( &client );
        goto RECONN;
    }
    /*Subscribe  end*/

    rc = mqtt_publish_online_notify( &client );
    if ( MQTT_SUCCESS != rc )
    {
        mqtt_log("Error publish online : %d ", rc);
        context->status.cloud_status.connect_status = false;
        mqtt_disconnect( &client );
        goto RECONN;
    }

    if ( context->status.cloud_status.connect_status_cb ) (context->status.cloud_status.connect_status_cb)( FOG_CONNECT );
    context->status.cloud_status.connect_status = true;

    task_queue_push( GET_STATUS_MSG, strlen(GET_STATUS_MSG), MSG_TYPE_CLOUD_CMD );

    while ( 1 )
    {
        //Max time the yield function will wait for read messages
        rc = mqtt_yield( &client, 100 );
        if ( (MQTT_SUCCESS != rc) && (MQTT_FAILURE != rc) )
        {
            // If the client is attempting to reconnect we will skip the rest of the loop.
            mico_rtos_thread_sleep( 1 );
            mqtt_log("mqtt disconnect errcode:%d", rc);
            mqtt_disconnect( &client );
            if ( context->status.cloud_status.connect_status_cb ) (context->status.cloud_status.connect_status_cb)( FOG_DISCONNECT );
            context->status.cloud_status.connect_status = false;
            goto RECONN;
        }

        if ( 0 == cloud_pop_form_queue( &msg, MQTT_RECV_TIMEOUT ) )
        {
            paramsQOS.qos = QOS1;
            paramsQOS.payload = (void *) msg.msg;
            paramsQOS.isRetained = 0;
            paramsQOS.payloadLen = msg.len;

            switch ( msg.type )
            {
                case MQTT_PUB_DATA:
                    sprintf( pub_topic, "$baidu/iot/shadow/%s%s", context->config.activate_confiog.device_id, MQTT_PUB_DATA_TOPIC );
                    break;
                case MQTT_PUB_STATUS:
                    sprintf( pub_topic, "%s%s", context->status.cloud_status.mqtt_topic, MQTT_PUB_STATUS_TOPIC );
                    break;
                case MQTT_PUB_API:
                    sprintf( pub_topic, "%s%s", context->status.cloud_status.mqtt_topic, MQTT_PUB_API_TOPIC );
                    break;
                default:
                    break;
            }
            mqtt_log("topic:\r\n%s", pub_topic);
            mqtt_log("payload:\r\n%.*s", (int)msg.len, msg.msg);
            rc = mqtt_publish( &client, pub_topic, strlen( pub_topic ), &paramsQOS );
            if ( MQTT_SUCCESS != rc )
            {
                mqtt_log("mqtt publish err");
            }

            if ( msg.msg ) free( msg.msg );
        }
    }

    exit:
    mico_rtos_delete_thread( NULL );
}

OSStatus mqtt_client_start( void *context )
{
#ifdef MQTT_USE_SSL
    uint32_t stack_size = MQTTS_THREAD_STACK_SIZE;
#else
    uint32_t stack_size = MQTT_THREAD_STACK_SIZE;
#endif

    mico_rtos_init_queue( &cloud_send_queue, "cloud queue", sizeof(cloud_msg_t), 5 );
    return mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "mqtt", mqtt_thread, stack_size, (mico_thread_arg_t) context );
}

