#ifndef __FOG_WIFI_CONFIG_H__
#define __FOG_WIFI_CONFIG_H__

#include "common.h"

#define SOFTAP_CONN_TIMEOUT 60*1000

#define FOG_WIFI_CONFIG_SERVER_PORT       30123
#define FOG_WIFI_CONFIG_SERVER_BUFF_LEN   2048
#define MAX_WIFI_PRE_NUM 3
#define MAX_SSID_LEN    32
#define MAX_KEY_LEN     64

#define FOG_WIFI_SCAN_TIMEOUT       3*1000
#define FOG_WIFI_SOFTAP_TIMEOUT     10*60
#define FOG_WIFI_STATION_TIMEOUT    2*60

typedef enum _wifi_config_mode_e
{
  FOG_SOFTAP_MODE,
  FOG_AWS_SOFTAP_MODE,
  FOG_AWS_SOFTAP_COMBO_MODE,
} wifi_config_mode_e;

typedef struct _wifi_config_t
{
    char ssid[MAX_SSID_LEN + 1];
    char key[MAX_KEY_LEN + 1];
    OSStatus err;
} wifi_config_t;

typedef struct _wifi_pre_t
{
    char ssid[MAX_SSID_LEN + 1];
    char key[MAX_KEY_LEN + 1];
} wifi_pre_t;

typedef struct _fog_wifi_config_t
{
    wifi_pre_t wifi_pre[MAX_WIFI_PRE_NUM];
} fog_wifi_config_t;

OSStatus fog_wifi_config_mode( uint8_t mode );

void fog_wifi_config_start( void );
bool fog_wifi_config_complete_flag( void );
void aws_delegate_recv_notify( char *data, uint32_t data_len );
void fog_wifi_pre_set( char *ssid, char *password );
int fog_softap_connect_ap( uint32_t timeout_ms, char *ssid, char *passwd );
void fog_softap_start( void );
void fog_wifi_connect_manage( void );
#endif
