#ifndef __FOG_OTA_H__
#define __FOG_OTA_H__

#define FOG_OTA_FILE_URL_MAX_LEN             256
#define FOG_OTA_COMPONENT_NAME_MAX_LEN       40
#define FOG_OTA_COMPONENT_MD5_MAX_LEN        40
#define FOG_OTA_COMPONENT_VERSION_MAX_LEN    32
#define FOG_OTA_CUSTOM_STRING_MAX_LEN        256
#define FOG_OTA_VERSION_STRING_MAX_LEN       30

typedef enum
{
    FOG_OTA_STATE_START,
    FOG_OTA_STATE_LOADING,
    FOG_OTA_STATE_FAILED,
    FOG_OTA_STATE_FAILED_AND_RETRY,
    FOG_OTA_STATE_FILE_DOWNLOAD_SUCCESS,
    FOG_OTA_STATE_MD5_SUCCESS,
    FOG_OTA_STATE_MD5_FAILED
} FOG_OTA_STATE_E; //ota state

typedef struct _fog_ota_t
{
    char file_url[FOG_OTA_FILE_URL_MAX_LEN + 1];
    char component_name[FOG_OTA_COMPONENT_NAME_MAX_LEN + 1];
    char file_md5[FOG_OTA_COMPONENT_MD5_MAX_LEN + 1];
    char ota_version[FOG_OTA_COMPONENT_VERSION_MAX_LEN + 1];
    char custom_string[FOG_OTA_CUSTOM_STRING_MAX_LEN + 1];
} fog_ota_t;

typedef struct _fog_ota_config_t
{
    uint8_t is_ota;
    char old_version[FOG_OTA_VERSION_STRING_MAX_LEN];
    int ot_id;
} fog_ota_config_t;

typedef struct _fog_ota_status_t
{
    uint8_t (*device_ota_notify_cb)( fog_ota_t *ota );
    void (*device_ota_state_cb)( FOG_OTA_STATE_E state, uint32_t sys_args );
    fog_ota_t ota;
    char new_version[FOG_OTA_VERSION_STRING_MAX_LEN];
} fog_ota_status_t;

OSStatus fog_ota_process( char *buf, uint32_t len );
OSStatus fog_ota_check_start( void );

OSStatus fog_ota_log_report( int is_ok );

#endif
