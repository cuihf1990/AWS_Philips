#include "mico_rtos.h"
#include "debug.h"
#include "aws_iot_config.h"
#include "aws_iot_shadow_interface.h"
#include "mico_rtos_common.h"
#include "Remote_Client.h"
#include "mico.h"
#include "json_parser.h"
#include "Device.h"

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
uint8_t Aws_Mqtt_satus = 0;
extern mico_queue_t Uart_push_queue;
extern system_context_t* sys_context;

uint32_t publishCount = 0;

typedef struct {
 int  type;
 char * name;

}data_process_t;

enum{
  Switch = 1,
  WindSpeed,
  Countdown,
  childLock,
  UILight,
  AQILight,
  WorkMode
};


data_process_t data[9] = {
    {Switch,"Switch"},
    {WindSpeed,"WindSpeed"},
    {Countdown,"Countdown"},
    {childLock,"childLock"},
    {UILight,"UILight"},
    {AQILight,"AQILight"},
    {WorkMode,"WorkMode"},
    {0,NULL}
};

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
    sprintf( clientid, "AWS_MiCO_%s", mac_str );

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

  //  Json_Process(valueLength,pJsonValueBuffer);

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

    iot_log("aws callback return  %d",status);

    if(SHADOW_ACK_TIMEOUT == status) {
        iot_log("Update Timeout--");
    } else if(SHADOW_ACK_REJECTED == status) {
        iot_log("Update RejectedXX");
    } else if(SHADOW_ACK_ACCEPTED == status) {
        iot_log("Update Accepted !!");
    }
}

void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data) {
	iot_log("MQTT Disconnect");

	IoT_Error_t rc = MQTT_FAILURE;

	if(NULL == pClient) {
		return;
	}

	IOT_UNUSED(data);

	if(mqtt_is_autoreconnect_enabled(pClient)) {
		iot_log("Auto Reconnect is enabled, Reconnecting attempt will start now");
	} else {
		iot_log("Auto Reconnect not enabled. Starting manual reconnect...");
		rc = mqtt_attempt_reconnect(pClient);
		if(NETWORK_RECONNECTED == rc) {
			iot_log("Manual Reconnect Successful");
		} else {
			iot_log("Manual Reconnect Failed - %d", rc);
		}
	}
}

void User_Data_Process(uint8_t type , uint8_t value)
{
	  set_port_t port_t;
	  iot_log(" i = %d",type);
	  switch(type)
	  {
	  case Switch:
	    {
	      port_t.port = 3;
	      port_t.prop = 2;
	      port_t.value = value;
	      Putprops(port_t);
	    }break;
	  case WorkMode:
	    {
	      port_t.port = 3;
	      port_t.prop = 8;
	      if(value == 1 )
	        port_t.value = 0;
	      else if(value == 2)
	        port_t.value = 1;
	      else if(value ==3 )
	        port_t.value =2 ;
	      else if(value == 4)
	        port_t.value =3 ;
	      Putprops(port_t);
	    }break;
	  case WindSpeed:
	    {
	      port_t.port = 3;
	      port_t.prop = 1;
	      if(value == 0)
	    	 port_t.value = 4;
	      else if(value == 4)
	    	 port_t.value = 5;
	      else
	    	 port_t.value = value ;
	      Putprops(port_t);
	    }break;
	  case Countdown:
	    {
	      port_t.port = 3;
	      port_t.prop = 6;
	      port_t.value = value;
	      Putprops(port_t);
	    }break;
	  case childLock:
	    {
	      port_t.port = 3;
	      port_t.prop = 0x03;
	      port_t.value = value;
	      Putprops(port_t);
	    }break;
	  case AQILight:
	    {
	      port_t.port = 3;
	      port_t.prop = 0x04;
	      if(value == 0)
	        port_t.value = 0;
	      else if(value == 1)
	        port_t.value = 25;
	      else if(value == 2)
	        port_t.value = 50;
	      else if(value == 3)
	        port_t.value = 75;
	      else if(value == 4)
	        port_t.value = 100;
	      Putprops(port_t);
	    }break;
	  case UILight:
	    {
	      port_t.port = 3;
	      port_t.prop = 0x05;
	      port_t.value = value;
	      Putprops(port_t);
	    }break;
	  default:break;
	  }
}


void Process_Cloud_data( char* payload , int payloadLen)
{
    int err = -1;

	jsontok_t json_tokens[10];    // Define an array of JSON tokens
	jobj_t jobj;
    char json_data[50] = {0};  ///////�ص���payload��������δ���������
	char  Set_Data[5];

    strncpy(json_data,payload,payloadLen);

//	iot_log("json_data = %s,json data = %s,payloadLen = %d",json_data,payload,payloadLen);

    err = json_init( &jobj, json_tokens, 10, (char *) json_data, payloadLen );
    if(err != 0) iot_log("parse error!");

    for(int i = 0; data[i].name != NULL; i++) {
    	iot_log("  i = %d , name = %s",i,(char *)data[i].name);
        err = json_get_val_str( &jobj, (char *)data[i].name, Set_Data, 5 );
        if(err == 0)  {
        	iot_log("send command = %s ,value = %d ",(char *)data[i].name,atoi(Set_Data));
        	User_Data_Process(data[i].type,atoi(Set_Data));
        }
  	  }
}

void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
									IoT_Publish_Message_Params *params, void *pData) {
	IOT_UNUSED(pData);
	IOT_UNUSED(pClient);
	iot_log("Subscribe callback");
	//iot_log("%.*s\t%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *) params->payload);
    Process_Cloud_data((char *) params->payload,(int) params->payloadLen);
    iot_log("Free memory %d bytes", MicoGetMemoryInfo()->free_memory);
}

static void aws_iot_shadow_main( mico_thread_arg_t arg )
{
    IoT_Error_t rc = MQTT_FAILURE;

    // initialize the mqtt client
    AWS_IoT_Client mqttClient;
    char clientid[40];
    static uint8_t * recv_message = NULL;  ///���հ忨���ݻ���
    char Subscribe[50];
    char Publish[50];

    IoT_Publish_Message_Params msgParams;

    if(recv_message == NULL)
    	recv_message = malloc(1024);

    memset(recv_message,0,1024);

    ShadowInitParameters_t sp = ShadowInitParametersDefault;
    sp.pHost =sys_context->flashContentInRam.Cloud_info.mqtt_host;//	AWS_IOT_MQTT_HOST;//
    sp.port =	sys_context->flashContentInRam.Cloud_info.mqtt_port;//AWS_IOT_MQTT_PORT;//
    sp.pClientCRT =  sys_context->flashContentInRam.Cloud_info.certificate;//AWS_IOT_CERTIFICATE_FILENAME;//
    sp.pClientKey = sys_context->flashContentInRam.Cloud_info.privatekey;//AWS_IOT_PRIVATE_KEY_FILENAME;//
    sp.pRootCA = AWS_IOT_ROOT_CA_FILENAME;
    sp.enableAutoReconnect = false;
    sp.disconnectHandler = NULL;

    iot_log(" info = %s",sp.pHost );
    iot_log(" info = %d",sp.port);
    iot_log(" info = %s , %s",sp.pClientCRT,sys_context->flashContentInRam.Cloud_info.certificate);
    iot_log(" info = %s", sp.pClientKey );
    iot_log(" info = %s", sys_context->flashContentInRam.Cloud_info.device_id );

    iot_log("Shadow Init");
    rc = aws_iot_shadow_init(&mqttClient, &sp);
    if (MQTT_SUCCESS != rc) {
        iot_log("Shadow Connection Error");
        goto exit;
    }

    ShadowConnectParameters_t scp = ShadowConnectParametersDefault;
    scp.pMyThingName = sys_context->flashContentInRam.Cloud_info.device_id;// AWS_IOT_MY_THING_NAME;//
    scp.pMqttClientId = sys_context->flashContentInRam.Cloud_info.device_id;//mqtt_client_id_get(clientid);
    scp.mqttClientIdLen =(uint16_t) strlen(sys_context->flashContentInRam.Cloud_info.device_id); //(uint16_t) strlen(mqtt_client_id_get(clientid));

RECONN:
    iot_log("Shadow Connect...");
    rc = aws_iot_shadow_connect(&mqttClient, &scp);
    if (MQTT_SUCCESS != rc) {
        sleep(1);
        iot_log("Shadow Connection Error = %d",rc);
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

    Aws_Mqtt_satus = 1;

    iot_log("Subscribing...");
    sprintf(Subscribe,"Philips/%s/command",sys_context->flashContentInRam.Cloud_info.device_id);
    rc = mqtt_subscribe(&mqttClient, Subscribe, strlen(Subscribe), QOS0, iot_subscribe_callback_handler, NULL);
    if(MQTT_SUCCESS != rc) {
    	iot_log("Error subscribing : %d ", rc);
    	return rc;
    }
    // Now wait in the loop to receive any message sent from the console
    while (NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || MQTT_SUCCESS == rc) {
        /*
         * Lets check for the incoming messages for 200 ms.
         */
        rc = aws_iot_shadow_yield(&mqttClient, 100);

        if (NETWORK_ATTEMPTING_RECONNECT == rc) {
            mico_rtos_thread_sleep(1);
            Aws_Mqtt_satus = 0;
            // If the client is attempting to reconnect we will skip the rest of the loop.
            continue;
        }else if( NETWORK_RECONNECTED == rc ){
            iot_log("Reconnect MQTT_SUCCESSful");
            Aws_Mqtt_satus = 1;
        }

        if( mico_rtos_pop_from_queue(&Uart_push_queue,recv_message,200)  == 0){
        //	iot_log("recv data = %s",recv_message);
        	msgParams.qos = QOS0;
        	msgParams.payloadLen = strlen(recv_message);
        	msgParams.payload = (char *) recv_message;
        	sprintf(Publish,"Philips/%s/data",sys_context->flashContentInRam.Cloud_info.device_id);
        	rc = mqtt_publish(&mqttClient, Publish, (uint16_t) strlen(Publish),&msgParams);
       // 	iot_log("rc == %d, data = %s",rc,msgParams.payload);
        }
        // sleep for some time in seconds
        mico_rtos_thread_msleep(500);
    }

    if (MQTT_SUCCESS != rc) {
        iot_log("try reconnect");
        rc = aws_iot_shadow_disconnect(&mqttClient);
        goto RECONN;
    }

    iot_log("Disconnecting");
    rc = aws_iot_shadow_disconnect(&mqttClient);

    if (MQTT_SUCCESS != rc) {
        iot_log("Disconnect error %d", rc);
    }

exit:
	if(recv_message)  free(recv_message);
    mico_rtos_delete_thread(NULL);
}

OSStatus start_aws_iot_shadow( void )
{
    return mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "aws shadow", aws_iot_shadow_main,
                                        0x3000,
                                        0 );
}
