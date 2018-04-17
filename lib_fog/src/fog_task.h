#ifndef __FOG_PA_TASK_H__
#define __FOG_PA_TASK_H__

#include "common.h"
#include "fog_act.h"
#include "fog_cloud.h"
#include "fog_time.h"
#include "fog_ota.h"
#include "fog_wifi_config.h"

#define FOG_SDK_VER_MAJOR        0
#define FOG_SDK_VER_MINOR        0
#define FOG_SDK_VER_REVISION     1

//#define USE_TEST_SITE
#define MQTT_USE_SSL
#define FOG_DOMAIN          "device.fogcloud.io"
#define FOG_PORT            443

typedef enum _conn_e
{
    FOG_CONNECT,
    FOG_DISCONNECT
} conn_e;

typedef enum _cb_e
{
    FOG_FLASH_READ,              //void    (*flash_read)(uint8_t *buf, uint32_t len);
    FOG_FLASH_WRITE,             //void    (*flash_write)(uint8_t *buf, uint32_t len);
    FOG_CONNECT_STATUS,          //void    (*connect_status)( uint8_t status );
    FOG_GET_DEVICE_STATUS,       //void    (*get_device_status)( void );
    FOG_SET_DEVICE_STATUS,       //void    (*set_device_status)( char *buf, uint32_t len );
    FOG_DEVICE_TIMER,            //void    (*timer_cb)( uint8_t status );
    FOG_DEVICE_OTA_NOTIFY,       //uint8_t (*device_ota_notify_cb)( fog_ota_t *ota );
    FOG_DEVICE_OTA_STATE,        //void    (*device_ota_state_cb)( FOG_OTA_STATE_E state, uint32_t sys_args );
} cb_e;

typedef enum _msg_type_e
{
    MSG_TYPE_CLOUD_CTL,
    MSG_TYPE_CLOUD_CMD,
    MSG_TYPE_LOCAL_CTL,
    MSG_TYPE_LOCAL_CMD,
} msg_type_e;

typedef struct _task_msg_t
{
    uint8_t msg_type;
    uint32_t len;
    uint8_t *msg;
} task_msg_t;

typedef struct _fog_config_t
{
    uint16_t crc16;
    fog_wifi_config_t wifi_config;
    fog_activate_config_t activate_confiog;
    fog_cloud_config_t cloud_config;
    fog_time_config_t time_config;
    fog_ota_config_t ota_config;
} fog_config_t;

typedef struct _fog_flash_cb_t
{
    void (*flash_read_cb)( uint8_t *buf, uint32_t len );
    void (*flash_write_cb)( uint8_t *buf, uint32_t len );
} fog_flash_cb_t;

typedef struct _fog_status_t
{
    fog_flash_cb_t flash_cb;
    fog_activate_status_t activate_status;
    fog_cloud_status_t cloud_status;
    fog_ota_status_t ota_status;
    fog_time_status_t time_status;
} fog_status_t;

typedef struct _fog_context_t
{
    fog_config_t config;
    fog_status_t status;
} fog_context_t;

void fog_context_init( void );
fog_context_t *fog_context_get( void );

OSStatus fog_flash_read( uint8_t *buf, uint32_t len );
OSStatus fog_flash_write( void );

OSStatus fog_init( char *type_id, char *version );
OSStatus fog_start( void );
OSStatus task_queue_push( char *buf, uint32_t len, uint8_t msg_type );
OSStatus cloud_pop_form_queue( void * message, uint32_t timeout_ms );
OSStatus fog_register_callback( uint8_t type, void *callback );
OSStatus fog_device_bind( char *enduser_id );
OSStatus fog_device_unbind_with_no_save( void );
OSStatus fog_device_unbind( void );
#endif
