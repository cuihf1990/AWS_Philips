#include "mico.h"
#include "fog_cloud.h"
#include "fog_ota.h"
#include "fog_task.h"
#include "fog_time.h"
#include "CheckSumUtils.h"
#include "StringUtils.h"


extern USED void PlatformEasyLinkButtonLongPressedCallback(void);

#define DEVICE_SECRET_SEED "Dingqq"
#define LAN_SECRET_SEED "Denial"
#define SIGN_LEN 100
static char fog_id[PRODUCT_ID_LEN + 1];
static char product_id[PRODUCT_ID_LEN + 1];
static char device_sn[DEVICE_SN_LEN + 1];
static char deivce_secret[DEVICE_SECRET_LEN + 1];
static char lan_secret[LAN_SECRET_LEN + 1];
static char sign[SIGN_LEN+1];

typedef struct _fog_id_flash_t
{
    char fog_id[FOG_ID_LEN + 1];
    uint16_t crc;
} fog_id_flash_t;

typedef struct _product_id_flash_t
{
    char product_id[PRODUCT_ID_LEN + 1];
    uint16_t crc;
} product_id_flash_t;

typedef struct _sign_flash_t
{
    char sign[SIGN_LEN + 1];
    uint16_t crc;
} sign_flash_t;

/*
 * fog id 烧录到user分区
 * fog id 偏移量0
 */
void fog_fog_id_set( char *id, int len )
{
    uint32_t para_offset = 0;
    CRC16_Context crc16;
    fog_id_flash_t fog_id_flash;
    uint8_t buf[512];
    mico_user_partition_t partition = 0;

#if defined(EMW1062)
    partition = MICO_PARTITION_FILESYS;
#elif defined(MOC108)
    partition =MICO_PARTITION_PARAMETER_3;
#else
    partition = MICO_PARTITION_USER;
#endif

    memset( &fog_id_flash, 0x0, sizeof(fog_id_flash_t) );
    memcpy( fog_id_flash.fog_id, id, FOG_ID_LEN );

#ifdef EMW1062
    MicoFlashRead( partition, &para_offset, (uint8_t *) buf, sizeof(buf) );
#else
    MicoFlashRead( partition, &para_offset, (uint8_t *) buf, sizeof(buf) );
#endif

    CRC16_Init( &crc16 );
    CRC16_Update( &crc16, fog_id_flash.fog_id, FOG_ID_LEN );
    CRC16_Final( &crc16, &fog_id_flash.crc );

    memcpy( buf, &fog_id_flash, sizeof(fog_id_flash_t) );

    para_offset = 0;
    MicoFlashErase( partition, para_offset, sizeof(buf) );
    MicoFlashWrite( partition, &para_offset, (uint8_t *) buf, sizeof(buf) );
}

char *fog_fog_id_get( void )
{
    uint32_t para_offset = 0;
    uint16_t crc = 0;
    CRC16_Context crc16;
    fog_id_flash_t fog_id_flash;
    mico_user_partition_t partition = 0;

#if defined(EMW1062)
    partition = MICO_PARTITION_FILESYS;
#elif defined(MOC108)
    partition =MICO_PARTITION_PARAMETER_3;
#else
    partition = MICO_PARTITION_USER;
#endif

    memset( &fog_id_flash, 0x0, sizeof(fog_id_flash_t) );
    MicoFlashRead( partition, &para_offset, (uint8_t *) &fog_id_flash, sizeof(fog_id_flash_t) );

    CRC16_Init( &crc16 );
    CRC16_Update( &crc16, fog_id_flash.fog_id, FOG_ID_LEN );
    CRC16_Final( &crc16, &crc );

    if ( crc == fog_id_flash.crc )
    {
        strcpy( fog_id, fog_id_flash.fog_id );
        return fog_id;
    } else
    {
        return NULL;
    }
}


/*
 * signature 烧录到user分区
 * signature 偏移量80
 */
void fog_signature_set( char *sign, int len )
{
    uint32_t para_offset = 0;
    CRC16_Context crc16;
    sign_flash_t sign_flash;
    int8_t buf[512];
    mico_user_partition_t partition = 0;

#if defined(EMW1062)
    partition = MICO_PARTITION_FILESYS;
#elif defined(MOC108)
    partition =MICO_PARTITION_PARAMETER_3;
#else
    partition = MICO_PARTITION_USER;
#endif

    memset( &sign_flash, 0x0, sizeof(sign_flash) );
    memcpy( sign_flash.sign, sign, SIGN_LEN );

    MicoFlashRead( partition, &para_offset, (uint8_t *) buf, sizeof(buf) );

    CRC16_Init( &crc16 );
    CRC16_Update( &crc16, sign_flash.sign, SIGN_LEN );
    CRC16_Final( &crc16, &sign_flash.crc );

    memcpy( buf + 80, &sign_flash, sizeof(sign_flash_t) );

    para_offset = 0;
    MicoFlashErase( partition, para_offset, sizeof(buf) );
    MicoFlashWrite( partition, &para_offset, (uint8_t *) buf, sizeof(buf) );
}

char *fog_signature_get( void )
{
    uint32_t para_offset = 80;
    uint16_t crc = 0;
    CRC16_Context crc16;
    sign_flash_t sign_flash;
    mico_user_partition_t partition = 0;

#if defined(EMW1062)
    partition = MICO_PARTITION_FILESYS;
#elif defined(MOC108)
    partition =MICO_PARTITION_PARAMETER_3;
#else
    partition = MICO_PARTITION_USER;
#endif

    memset( &sign_flash, 0x0, sizeof(sign_flash_t) );
    MicoFlashRead( partition, &para_offset, (uint8_t *) &sign_flash, sizeof(sign_flash_t) );

    CRC16_Init( &crc16 );
    CRC16_Update( &crc16, sign_flash.sign, SIGN_LEN );
    CRC16_Final( &crc16, &crc );

    if ( crc == sign_flash.crc )
    {
        strcpy( sign, sign_flash.sign );
        return sign;
    } else
    {
        return NULL;
    }
}

/*
 * product id 烧录到user分区
 * product id 偏移量40
 */
void fog_product_id_set( char *id, int len )
{
    uint32_t para_offset = 0;
    CRC16_Context crc16;
    product_id_flash_t product_id_flash;
    int8_t buf[512];
    mico_user_partition_t partition = 0;

#if defined(EMW1062)
    partition = MICO_PARTITION_FILESYS;
#elif defined(MOC108)
    partition =MICO_PARTITION_PARAMETER_3;
#else
    partition = MICO_PARTITION_USER;
#endif

    memset( &product_id_flash, 0x0, sizeof(fog_id_flash_t) );
    memcpy( product_id_flash.product_id, id, PRODUCT_ID_LEN );

    MicoFlashRead( partition, &para_offset, (uint8_t *) buf, sizeof(buf) );

    CRC16_Init( &crc16 );
    CRC16_Update( &crc16, product_id_flash.product_id, PRODUCT_ID_LEN );
    CRC16_Final( &crc16, &product_id_flash.crc );

    memcpy( buf + 40, &product_id_flash, sizeof(product_id_flash_t) );

    para_offset = 0;
    MicoFlashErase( partition, para_offset, sizeof(buf) );
    MicoFlashWrite( partition, &para_offset, (uint8_t *) buf, sizeof(buf) );
}

char *fog_product_id_get( void )
{
    uint32_t para_offset = 40;
    uint16_t crc = 0;
    CRC16_Context crc16;
    product_id_flash_t product_id_flash;
    mico_user_partition_t partition = 0;

#if defined(EMW1062)
    partition = MICO_PARTITION_FILESYS;
#elif defined(MOC108)
    partition =MICO_PARTITION_PARAMETER_3;
#else
    partition = MICO_PARTITION_USER;
#endif

    memset( &product_id_flash, 0x0, sizeof(product_id_flash_t) );
    MicoFlashRead( partition, &para_offset, (uint8_t *) &product_id_flash, sizeof(product_id_flash_t) );

    CRC16_Init( &crc16 );
    CRC16_Update( &crc16, product_id_flash.product_id, PRODUCT_ID_LEN );
    CRC16_Final( &crc16, &crc );

    if ( crc == product_id_flash.crc )
    {
        strcpy( product_id, product_id_flash.product_id );
        return product_id;
    } else
    {
        return NULL;
    }
}


void fog_region_set( char *region )
{
    fog_context_get( );
    strncpy( fog_context_get( )->config.cloud_config.region, region, REGION_LEN );
}

char *fog_region_get( void )
{
    if ( fog_context_get( ) )
    {
        return fog_context_get( )->config.cloud_config.region;
    } else
    {
        return NULL;
    }
}

void fog_type_id_set( char *type_id )
{
    fog_context_get( );
    strncpy( fog_context_get( )->config.cloud_config.type_id, type_id, REGION_LEN );
}

char *fog_type_id_get( void )
{
    if ( fog_context_get( ) )
    {
        return fog_context_get( )->config.cloud_config.type_id;
    } else
    {
        return NULL;
    }
}

void fog_enduser_id_set( char *id )
{
    fog_context_get( );
    strncpy( fog_context_get( )->config.cloud_config.enduser_id, id, ENDUSER_ID_LEN );
}

char *fog_enduser_id_get( void )
{
    if ( fog_context_get( ) )
    {
        return fog_context_get( )->config.cloud_config.enduser_id;
    } else
    {
        return NULL;
    }
}

char *fog_device_sn_get( void )
{
    unsigned char mac[6];

    mico_wlan_get_mac_address( mac );
    sprintf( device_sn, "%02X%02X%02X%02X%02X%02X",
             mac[0],
             mac[1], mac[2], mac[3], mac[4], mac[5] );
    return device_sn;
}

/*
 * md5=fog_id+product_id+device_sn+seed
 */
void fog_device_secret_set(  char *dsn )
{
    md5_context ctx;
    char *secret = NULL;
    unsigned char output[16];

    InitMd5( &ctx );
    Md5Update( &ctx, (unsigned char *) dsn, strlen( dsn ) );
    Md5Update( &ctx, (unsigned char *) DEVICE_SECRET_SEED, strlen( DEVICE_SECRET_SEED ) );
    Md5Final( &ctx, output );

    secret = DataToHexString( (const uint8_t *) output, 16 );

    strcpy( deivce_secret, secret );

    if ( secret ) free( secret );
}

char *fog_deivce_secret_get( void )
{
    if ( (fog_fog_id_get( ) == NULL) || (fog_product_id_get( ) == NULL) )
        return NULL;

    return deivce_secret;
}

/*
 * md5=fog_id+product_id+mac+seed
 */
void fog_lan_secret_set( char *fid, char *pid, char *dsn )
{
    md5_context ctx;
    char *secret = NULL;
    unsigned char output[16];

    InitMd5( &ctx );
    Md5Update( &ctx, (unsigned char *) fid, strlen( fid ) );
    Md5Update( &ctx, (unsigned char *) pid, strlen( pid ) );
    Md5Update( &ctx, (unsigned char *) dsn, strlen( dsn ) );
    Md5Update( &ctx, (unsigned char *) LAN_SECRET_SEED, strlen( LAN_SECRET_SEED ) );
    Md5Final( &ctx, output );

    secret = DataToHexString( (const uint8_t *) output, 16 );

    strcpy( lan_secret, secret );

    if ( secret ) free( secret );
}

char *fog_lan_secret_get( void )
{
    if ( (fog_fog_id_get( ) == NULL) || (fog_product_id_get( ) == NULL) )
        return NULL;

    return lan_secret;
}

void fog_need_bind_set( bool status )
{
    if ( fog_context_get( ) == NULL )
        return;

    fog_context_get( )->config.cloud_config.need_bind = status;
    fog_flash_write( );
}

bool fog_need_bind_get( void )
{
    if ( fog_context_get( ) == NULL )
        return false;

    return fog_context_get( )->config.cloud_config.need_bind;
}

void fog_timezero_set( int timezero )
{
    if ( fog_context_get( ) == NULL )
        return;

    fog_context_get( )->config.time_config.timezero = timezero;
    fog_flash_write( );
}

int fog_timezero_get( void )
{
    if ( fog_context_get( ) == NULL )
        return 0;

    return fog_context_get( )->config.time_config.timezero;
}

static char softap_name[64] = {0};
char *fog_softap_name_get( void )
{
    sprintf( softap_name, "smart-%s-%s", fog_type_id_get(), fog_device_sn_get()+6);
    return (char *)softap_name;
}

static char sign_temp[100]={0};
static int sign_temp_len =0;

static void fog_conf_cmd( char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv )
{
    if ( argc == 4 )
    {
        if( strcmp( argv[1], "sign" ) == 0 )
            {
                if( strcmp(argv[2], "1" ) == 0 )
                {
                    memcpy(sign_temp, argv[3], strlen(argv[3]));
                    sign_temp_len = strlen(argv[3]);
                }else if( strcmp(argv[2], "2" ) == 0 )
                {
                    memcpy(sign_temp+sign_temp_len, argv[3], strlen(argv[3]));
                    sign_temp_len += strlen(argv[3]);
                    fog_signature_set(sign_temp, sign_temp_len );
                    sign_temp_len = 0;
                }
            }
    } else if ( argc == 3 )
    {
        if ( strcmp( argv[1], "fog_id" ) == 0 )
        {
            fog_fog_id_set( argv[2], strlen( argv[2] ) );
        } else if ( strcmp( argv[1], "product_id" ) == 0 )
        {
            fog_product_id_set( argv[2], strlen( argv[2] ) );
        }
    } else if ( argc == 2 )
    {
        if ( strcmp( argv[1], "show" ) == 0 )
        {
            cli_printf( "fog_id:%s\r\n", fog_fog_id_get( ) );
            cli_printf( "product_id:%s\r\n", fog_product_id_get( ) );
            cli_printf( "signature:%s\r\n", fog_signature_get( ) );
        }
    }
}

static void fog_status_cmd( char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv )
{
    if ( argc == 2 )
    {
        if ( (strcmp( argv[1], "clear" ) == 0) )
        {
            PlatformEasyLinkButtonLongPressedCallback( );
        } else if ( (strcmp( argv[1], "show" ) == 0) )
        {
            cli_printf( "device_sn:%s\r\n", fog_device_sn_get( ) );
            cli_printf( "region:%s\r\n", fog_region_get( ) );
            cli_printf( "type_id:%s\r\n", fog_type_id_get( ) );
            cli_printf( "device_secret:%s\r\n", fog_deivce_secret_get( ) );
            cli_printf( "lan_secret:%s\r\n", fog_lan_secret_get( ) );
        }
    }
}

static void fog_wifi_cmd( char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv )
{
    if ( argc == 2 )
    {
        if ( strcmp( argv[1], "stop" ) == 0 )
        {
            micoWlanSuspend( );
        } else if ( strcmp( argv[1], "clear" ) == 0 )
        {
            mico_system_context_get( )->micoSystemConfig.configured = unConfigured;
            mico_system_context_update( mico_system_context_get( ) );
        } else if( strcmp( argv[1], "show" ) == 0 )
        {
            for( int i=0; i<MAX_WIFI_PRE_NUM; i++ )
            {
                cli_printf("ssid[%d]:%s,key[%d]:%s\r\n", i, fog_context_get( )->config.wifi_config.wifi_pre[i].ssid,
                           i, fog_context_get( )->config.wifi_config.wifi_pre[i].key);
            }
        }
    } else if ( argc == 3 )
    {
        if ( strcmp( argv[1], "start" ) == 0 )
            fog_softap_connect_ap( 0, argv[2], NULL );
    } else if ( argc == 4 )
    {
        if ( strcmp( argv[1], "start" ) == 0 )
            fog_softap_connect_ap( 0, argv[2], argv[3] );
    }
}

static void fog_pub_cmd( char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv )
{
    if ( argc != 2 )
        return;

    fog_cloud_report( MQTT_PUB_STATUS, argv[1], strlen( argv[1] ) );
}

static void fog_bind_cmd( char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv )
{
    if ( argc == 2 )
    {
        fog_device_bind( argv[1] );
    }
}

static void fog_unbind_cmd( char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv )
{
    fog_device_unbind( );
}

static void fog_ota_check_cmd( char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv )
{
    fog_ota_check_start( );
}

static void fog_time_cmd( char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv )
{
    mico_rtc_time_t rtc_time;

    if( argc == 2 )
    {
        if( strcmp( argv[1], "sync" ) == 0 )
        {
            fog_sync_utc_time( );
        }else if( strcmp( argv[1], "show" ) == 0 )
        {
            fog_rtc_time_get( &rtc_time );
            cli_printf( "timezero:%d, %d-%d-%d %d %d:%d:%d", fog_timezero_get(),
                        rtc_time.year, rtc_time.month, rtc_time.date, rtc_time.weekday,
                        rtc_time.hr, rtc_time.min, rtc_time.sec);
        }
    }else if( argc == 3 )
    {
        if( strcmp( argv[1], "set" ) == 0 )
        {
            fog_timezero_set( atoi(argv[2]) );
        }
        if( strcmp( argv[1], "task" ) == 0 )
        {
            fog_timer_task_set( argv[2], strlen(argv[2]) );
        }
    }
}

static const struct cli_command user_clis[] = {
    { "conf", "conf <show|fog_id|product_id> [id_string]", fog_conf_cmd },
    { "status", "status <clear|show>", fog_status_cmd },
    { "wifi", "wifi <show|start|stop|clear> [ssid] [key]", fog_wifi_cmd },
    { "report", "report <string>", fog_pub_cmd },
    { "bind", "bind <enduser_id>", fog_bind_cmd },
    { "unbind", "device unbind", fog_unbind_cmd },
    { "ota_check", "check remote firmware", fog_ota_check_cmd },
    { "timer", "timer <sync>|<show>|<zero> [timezore]|<task> [task string]", fog_time_cmd },
};

void fog_register_commands( void )
{
    cli_register_commands( user_clis, sizeof(user_clis) / sizeof(struct cli_command) );
}

