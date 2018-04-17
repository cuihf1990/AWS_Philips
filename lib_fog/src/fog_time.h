#ifndef __FOG_PA_TIME_H__
#define __FOG_PA_TIME_H__

#include "mico.h"

#define FOG_TIME_DEBUG_ENABLE                 (1)

#define FOG_TIME_NUM_TOKENS                   (30)

#define FOG_TIME_REQ_BUFF_LEN                 (512)
#define FOG_TIME_RES_BUFF_LEN                 (1024)

#define FOG_TIME_PORT_NORAML                  (80)
#define FOG_TIME_URI_NORMAL                   ("/server/time/")
#define FOG_HOST                              ""
#define FOG_TIME_STR                          ("GET %s HTTP/1.1\r\nHost: %s\r\nConnection: Close\r\nContent-Length: 0\r\n\r\n")
#define FOG_TIME_TIMEOUT_NORMAL               (4000) //4s

#define FOG_TIME_SYNC						  (12*60*60)
#define FOG_TIMER_TASK_MAX					  (10)
#define FOG_TIMER_GROUP_MAX                   (2)

typedef struct _timer_freq_t
{
	uint8_t mon:1;
	uint8_t tues:1;
	uint8_t wed:1;
	uint8_t thur:1;
	uint8_t fri:1;
	uint8_t sat:1;
	uint8_t sun:1;
	uint8_t loop:1;
} timer_freq_t;

typedef struct _timer_group_t
{
	uint8_t enable;
	uint8_t status;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
} timer_group_t;

typedef struct _timer_task_t
{
	uint8_t id;
	uint8_t enable;
	timer_freq_t freq;
	timer_group_t group[FOG_TIMER_GROUP_MAX];
} timer_task_t;

#pragma pack(1)
typedef struct _fog_timer_config_t
{
	uint8_t timer_num;
	timer_task_t timer_task[FOG_TIMER_TASK_MAX];
} fog_timer_config_t;
#pragma pack()

typedef struct _fog_time_config_t
{
	int32_t timezero;
	uint8_t timer_status[FOG_TIMER_TASK_MAX][FOG_TIMER_GROUP_MAX];
	fog_timer_config_t timer_config;
} fog_time_config_t;

typedef struct _fog_delay_status_t
{
    uint8_t enable;
    uint8_t hour;
    uint8_t min;
	uint8_t sec;
	uint32_t utc;
    void (*delay_cb)( void );
} fog_delay_status_t;

typedef struct _fog_timer_status_t
{
    void (*timer_cb)( uint8_t task_id, uint8_t status );
} fog_timer_status_t;

typedef struct _fog_time_status_t
{
    fog_timer_status_t timer_status;
	fog_delay_status_t delay_status;
} fog_time_status_t;

/**
 * @brief  fog pa get server time.
 * @param  timestamp_p[in/out]: timestamp point
 * @retval kNoErr is returned on success, otherwise, kXXXErr is returned.
 */
OSStatus fog_service_get_time( int *timestamp );

void fog_rtc_time_get( mico_rtc_time_t *rtc_time );

OSStatus fog_sync_utc_time( void );

OSStatus fog_timer_init( void );

OSStatus fog_timer_task_set( char *buf, int len );
fog_timer_config_t *fog_timer_task_get( void );
char *fog_timer_task_get_string( void );

OSStatus fog_delay_task_set( uint32_t delay_min, void *delay_cb );
uint32_t fog_delay_task_get( void );
OSStatus fog_delay_task_cancel( void *delay_cb );
#endif

