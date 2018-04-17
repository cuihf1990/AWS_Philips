#ifndef __FOG_PA_CLOUD_H__
#define __FOG_PA_CLOUD_H__

#include "common.h"

#define MQTT_THREAD_STACK_SIZE  0x2000
#define MQTTS_THREAD_STACK_SIZE 0x4000

#define FOG_PA_SYS_TOPIC_NUM_TOKENS 40

#define CLOUD_MSG_MAX       1500
#define MQTT_RECV_TIMEOUT   20

#define FOG_ID_LEN          32
#define PRODUCT_ID_LEN      32
#define DEVICE_SN_LEN       16
#define DEVICE_SECRET_LEN   32
#define LAN_SECRET_LEN      32
#define TYPED_ID_LEN        5
#define REGION_LEN          10
#define ENDUSER_ID_LEN      32
#define SIGN_LEN            100

#define MQTT_HOST_MAX       80
#define MQTT_CLIENT_ID_MAX  80
#define MQTT_USERNAME_MAX   80
#define MQTT_PASSWORD_MAX   80
#define MQTT_TOPIC_MAX      256

#define MQTT_LWT_MSG                "{\"type\":\"online\",\"data\":{\"online\":false}}"

#define GET_STATUS_MSG              "{\"type\":\"get_status\"}"

#define MQTT_PUB_ONLINE_TOPIC       "/online/json"

#define MQTT_PUB_DATA_TOPIC         "/update"
#define MQTT_SUB_CTL_TOPIC          "/control/json"

#define MQTT_PUB_API_TOPIC          "/api/json"
#define MQTT_SUB_SYS_TOPIC          "/sys/json"

#define MQTT_PUB_STATUS_TOPIC       "/status/json"
#define MQTT_SUB_CMD_TOPIC          "/command/json"

typedef enum _mqtt_pub_msgtype_e
{
    MQTT_PUB_DATA,
    MQTT_PUB_STATUS,
    MQTT_PUB_API
} mqtt_pub_msgtype_e;

typedef struct _cloud_msg_t
{
    uint32_t len;
    uint8_t type;
    uint8_t *msg;
} cloud_msg_t;

typedef struct _fog_cloud_config_t
{
    char type_id[TYPED_ID_LEN+1];
    char region[REGION_LEN+1];
    char enduser_id[ENDUSER_ID_LEN+1];
    bool need_unbind;
    bool need_bind;
} fog_cloud_config_t;

typedef struct _fog_mqtt_login_t
{
    char mqtt_host[MQTT_HOST_MAX];
    int  mqtt_port;
    char mqtt_client_id[MQTT_CLIENT_ID_MAX];
    char mqtt_username[MQTT_USERNAME_MAX];
    char mqtt_password[MQTT_PASSWORD_MAX];
} fog_mqtt_login_t;

typedef struct _fog_cloud_status_t
{
    fog_mqtt_login_t mqtt_login;
    bool connect_status;
    char mqtt_topic[MQTT_TOPIC_MAX];
    void (*connect_status_cb)( uint8_t status );
    void (*get_device_status_cb)( void );
    void (*set_device_status_cb)( char *buf, uint32_t len );
} fog_cloud_status_t;

OSStatus mqtt_client_start( void *context );

bool fog_connect_status( void );
OSStatus fog_cloud_report( uint8_t type, char *buf, uint32_t len );

#endif
