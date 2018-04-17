#ifndef __FOG_PA_ACT_H__
#define __FOG_PA_ACT_H__

#include "fog_cloud.h"


#define FOG_ACTIVATE_METHOD          "POST"
#define FOG_ACTIVATE_URI             "/v3_1/dm_v3/device/activation/"
#define FOG_ACTIVATE_REQ_BUFF_LEN    1024
#define FOG_ACTIVATE_RES_BUFF_LEN    1024
#define FOG_ACTIVATE_TIMEOUT         2*1000

#define FOG_ACTIVATE_RES_HOST_LEN    80
#define FOG_ACTIVATE_RES_ENAME_LEN   80
#define FOG_ACTIVATE_RES_PWD_LEN     80
#define FOG_ACTIVATE_RES_DSN_LEN     80
#define FOG_ACTIVATE_NUM_TOKENS      30

#define FOG_UNBIND_METHOD            "PUT"
#define FOG_UNBIND_URI               "/v3_1/dm_v3/device/unbind/"
#define FOG_UNBIND_REQ_BUFF_LEN      1024
#define FOG_UNBIND_RES_BUFF_LEN      1024
#define FOG_UNBIND_TIMEOUT           2*1000

#define FOG_UNBIND_NUM_TOKENS      30

typedef struct _fog_activate_config_t
{
    char host[FOG_ACTIVATE_RES_HOST_LEN];
    int common_port;
    int ssl_port;
    char endpoint_name[FOG_ACTIVATE_RES_ENAME_LEN];
    char password[FOG_ACTIVATE_RES_PWD_LEN];
    char device_id[FOG_ACTIVATE_RES_DSN_LEN];
} fog_activate_config_t;

typedef struct _fog_activate_status_t
{
    char fog_id[FOG_ID_LEN+1];
    char product_id[PRODUCT_ID_LEN+1];
    char device_sn[DEVICE_SN_LEN+1];
    char device_secret[DEVICE_SECRET_LEN+1];
    char lan_secret[LAN_SECRET_LEN+1];
    char enduser_id[ENDUSER_ID_LEN+1];
    char sign[SIGN_LEN+1];
    char aws_notify_ip[16];
} fog_activate_status_t;

OSStatus fog_activate( void *context, bool is_bind );
OSStatus fog_http_unbind( void *context );
#endif
