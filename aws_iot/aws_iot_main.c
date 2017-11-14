#include "mico_rtos.h"
#include "debug.h"
#include "aws_iot_config.h"
#include "aws_iot_shadow_interface.h"

/**
 * @file shadow_console_echo.c
 * @brief  Echo received Delta message
 *
 * This application will echo the message received in delta, as reported.
 * for example:
 * Received Delta message
 * {
 *    "state": {
 *       "switch": "on"
 *   }
 * }
 * This delta message means the desired switch position has changed to "on"
 *
 * This application will take this delta message and publish it back as the reported message from the device.
 * {
 *    "state": {
 *     "reported": {
 *       "switch": "on"
 *      }
 *    }
 * }
 *
 * This update message will remove the delta that was created. If this message was not removed then the AWS IoT Thing Shadow is going to always have a delta and keep sending delta any time an update is applied to the Shadow
 * This example will not use any of the json builder/helper functions provided in the aws_iot_shadow_json_data.h.
 * @note Ensure the buffer sizes in aws_iot_config.h are big enough to receive the delta message. The delta message will also contain the metadata with the timestamps
 */

#define iot_log(M, ...) custom_log("iot", M, ##__VA_ARGS__)


static bool messageArrivedOnDelta = false;

/*
 * @note The delta message is always sent on the "state" key in the json
 * @note Any time messages are bigger than AWS_IOT_MQTT_RX_BUF_LEN the underlying MQTT library will ignore it. The maximum size of the message that can be received is limited to the AWS_IOT_MQTT_RX_BUF_LEN
 */
static char stringToEchoDelta[SHADOW_MAX_SIZE_OF_RX_BUFFER];

char *mqtt_client_id_get( char clientid[30] )
{
    uint8_t mac[6];
    char mac_str[13];

    mico_wlan_get_mac_address( mac );
    sprintf( mac_str, "%02X%02X%02X%02X%02X%02X",
             mac[0],
             mac[1], mac[2], mac[3], mac[4], mac[5] );
    sprintf( clientid, "MiCO_%s", mac_str );

    return clientid;
}
/**
 * @brief This function builds a full Shadow expected JSON document by putting the data in the reported section
 *
 * @param pJsonDocument Buffer to be filled up with the JSON data
 * @param maxSizeOfJsonDocument maximum size of the buffer that could be used to fill
 * @param pReceivedDeltaData This is the data that will be embedded in the reported section of the JSON document
 * @param lengthDelta Length of the data
 */
bool buildJSONForReported(char *pJsonDocument, size_t maxSizeOfJsonDocument, const char *pReceivedDeltaData, uint32_t lengthDelta) {
    int32_t ret;

    if (NULL == pJsonDocument) {
        return false;
    }

    char tempClientTokenBuffer[MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE];

    if(aws_iot_fill_with_client_token(tempClientTokenBuffer, MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE) != MQTT_SUCCESS){
        return false;
    }

    ret = snprintf(pJsonDocument, maxSizeOfJsonDocument, "{\"state\":{\"reported\":%.*s}, \"clientToken\":\"%s\"}", (int)lengthDelta, pReceivedDeltaData, tempClientTokenBuffer);

    if (ret >= maxSizeOfJsonDocument || ret < 0) {
        return false;
    }

    return true;
}

void DeltaCallback(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t) {
    IOT_UNUSED(pJsonStruct_t);

    iot_log("Received Delta message \r\n%.*s", (int)valueLength, pJsonValueBuffer);

    if (buildJSONForReported(stringToEchoDelta, SHADOW_MAX_SIZE_OF_RX_BUFFER, pJsonValueBuffer, valueLength)) {
        messageArrivedOnDelta = true;
    }
}

void UpdateStatusCallback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
        const char *pReceivedJsonDocument, void *pContextData) {
    IOT_UNUSED(pThingName);
    IOT_UNUSED(action);
    IOT_UNUSED(pReceivedJsonDocument);
    IOT_UNUSED(pContextData);

    if(SHADOW_ACK_TIMEOUT == status) {
        iot_log("Update Timeout--");
    } else if(SHADOW_ACK_REJECTED == status) {
        iot_log("Update RejectedXX");
    } else if(SHADOW_ACK_ACCEPTED == status) {
        iot_log("Update Accepted !!");
    }
}

static void aws_iot_shadow_main( mico_thread_arg_t arg )
{
    IoT_Error_t rc = MQTT_FAILURE;

    // initialize the mqtt client
    AWS_IoT_Client mqttClient;
    char clientid[40];

    ShadowInitParameters_t sp = ShadowInitParametersDefault;
    sp.pHost = AWS_IOT_MQTT_HOST;
    sp.port = AWS_IOT_MQTT_PORT;
    sp.pClientCRT = AWS_IOT_CERTIFICATE_FILENAME;
    sp.pClientKey = AWS_IOT_PRIVATE_KEY_FILENAME;
    sp.pRootCA = AWS_IOT_ROOT_CA_FILENAME;
    sp.enableAutoReconnect = false;
    sp.disconnectHandler = NULL;

    iot_log("Shadow Init");
    rc = aws_iot_shadow_init(&mqttClient, &sp);
    if (MQTT_SUCCESS != rc) {
        iot_log("Shadow Connection Error");
        goto exit;
    }

    ShadowConnectParameters_t scp = ShadowConnectParametersDefault;
    scp.pMyThingName = AWS_IOT_MY_THING_NAME;
    scp.pMqttClientId = mqtt_client_id_get(clientid);
    scp.mqttClientIdLen = (uint16_t) strlen(mqtt_client_id_get(clientid));

RECONN:
    iot_log("Shadow Connect...");
    rc = aws_iot_shadow_connect(&mqttClient, &scp);
    if (MQTT_SUCCESS != rc) {
        sleep(1);
        iot_log("Shadow Connection Error");
        goto RECONN;
    }

    /*
     * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
     *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
     *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
     */
    rc = aws_iot_shadow_set_autoreconnect_status(&mqttClient, true);
    if(MQTT_SUCCESS != rc){
        iot_log("Unable to set Auto Reconnect to true - %d", rc);
        goto RECONN;
    }

    jsonStruct_t deltaObject;
    deltaObject.pData = stringToEchoDelta;
    deltaObject.pKey = "state";
    deltaObject.type = SHADOW_JSON_OBJECT;
    deltaObject.cb = DeltaCallback;

    /*
     * Register the jsonStruct object
     */
    iot_log("Register the jsonStruct object");
    rc = aws_iot_shadow_register_delta(&mqttClient, &deltaObject);

    // Now wait in the loop to receive any message sent from the console
    while (NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || MQTT_SUCCESS == rc) {
        /*
         * Lets check for the incoming messages for 200 ms.
         */
        rc = aws_iot_shadow_yield(&mqttClient, 100);

        if (NETWORK_ATTEMPTING_RECONNECT == rc) {
            mico_rtos_thread_sleep(1);
            // If the client is attempting to reconnect we will skip the rest of the loop.
            continue;
        }else if( NETWORK_RECONNECTED == rc ){
            iot_log("Reconnect MQTT_SUCCESSful");
        }

        if (messageArrivedOnDelta) {
            iot_log("Sending delta message back \r\n%s", stringToEchoDelta);
            rc = aws_iot_shadow_update(&mqttClient, AWS_IOT_MY_THING_NAME, stringToEchoDelta, UpdateStatusCallback, NULL, 2, true);
            messageArrivedOnDelta = false;
        }

        // sleep for some time in seconds
        mico_rtos_thread_sleep(1);
    }

    if (MQTT_SUCCESS != rc) {
        goto RECONN;
    }

    iot_log("Disconnecting");
    rc = aws_iot_shadow_disconnect(&mqttClient);

    if (MQTT_SUCCESS != rc) {
        iot_log("Disconnect error %d", rc);
    }

exit:
    mico_rtos_delete_thread(NULL);
}

OSStatus start_aws_iot_shadow( void )
{
    return mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "aws shadow", aws_iot_shadow_main,
                                        0x3000,
                                        0 );
}
