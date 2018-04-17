#include "fog_time.h"
#include "fog_product.h"
#include "fog_task.h"
#include "http_short_connection.h"
#include "json_parser.h"
#include "StringUtils.h"

#if (FOG_TIME_DEBUG_ENABLE == 1)
#define time_log(format, ...)  custom_log("time", format, ##__VA_ARGS__)
#elif (FOG_TIME_DEBUG_ENABLE == 0)
#define time_log(format, ...)
#else
#error "FOG_TIME_DEBUG_ENABLE not define!"
#endif


static uint8_t is_week( uint8_t current_week, timer_task_t *timer )
{
    switch ( current_week )
    {
        case 0:
            if ( timer->freq.sun == 1 ) return 1;
            break;
        case 1:
            if ( timer->freq.mon == 1 ) return 1;
            break;
        case 2:
            if ( timer->freq.tues == 1 ) return 1;
            break;
        case 3:
            if ( timer->freq.wed == 1 ) return 1;
            break;
        case 4:
            if ( timer->freq.thur == 1 ) return 1;
            break;
        case 5:
            if ( timer->freq.fri == 1 ) return 1;
            break;
        case 6:
            if ( timer->freq.sat == 1 ) return 1;
            break;
        default:
            break;
    }

    return 0;
}

/**
 * @brief  process get time json response
 * @param  http_response[in]: json response
 * @param  fog_server_time[in/out]: time information
 * @retval kNoErr is returned on success, otherwise, kXXXErr is returned.
 */
static OSStatus process_get_time_response( const char *http_response, int *timestamp )
{
    OSStatus err = kGeneralErr;
    jsontok_t json_tokens[FOG_TIME_NUM_TOKENS];    // Define an array of JSON tokens
    jobj_t jobj;
    int code = -1;

    require_string( http_response != NULL, exit, "body is NULL!!!" );

    require_string( ((*http_response == '{') && (*(http_response + strlen( http_response ) - 1) == '}')), exit, "http body JSON format error" );

    // Initialise the JSON parser and parse the given string
    err = json_init( &jobj, json_tokens, FOG_TIME_NUM_TOKENS, (char *) http_response, strlen( http_response ) );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "parse error!" );

    err = json_get_composite_object( &jobj, "meta" );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get meta error!" );

    //get code
    err = json_get_val_int( &jobj, "code", &code );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get code error!" );

    err = json_release_composite_object( &jobj );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "release meta error!" );

    require_action_string( code == 0, exit, err = kGeneralErr, "get timestamp response code error!" );

    err = json_get_composite_object( &jobj, "data" );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get data error!" );

    //get timestamp
    err = json_get_val_int( &jobj, "timestamp1", timestamp );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get timestamp1 error!" );

    exit:
    if ( err != kNoErr )
    {
        time_log("http get time response err, code = %d, response : %s", code, http_response);
    }

    return err;
}

/**
 * @brief  fog v3 get server time.
 * @param  fog_server_time[in/out]: time information
 * @retval kNoErr is returned on success, otherwise, kXXXErr is returned.
 */
OSStatus fog_service_get_time( int *timestamp )
{
    OSStatus err = kGeneralErr;
    HTTP_REQ_S http_req = HTTP_REQ_INIT_PARAM;
    char domain[30] = {0};

    require_action( timestamp != NULL, exit, err = kParamErr );

    memset( &http_req, 0, sizeof(http_req) );

    sprintf(domain, "%s%s", fog_context_get()->config.cloud_config.region, FOG_DOMAIN);

    http_req.domain_name = domain;
    http_req.port = FOG_TIME_PORT_NORAML;
    http_req.is_success = false;
    http_req.timeout_ms = FOG_TIME_TIMEOUT_NORMAL;

    http_req.http_req = malloc( FOG_TIME_REQ_BUFF_LEN );
    require_string( http_req.http_req != NULL, exit, "malloc error" );
    memset( http_req.http_req, 0, FOG_TIME_REQ_BUFF_LEN );

    http_req.http_res = malloc( FOG_TIME_RES_BUFF_LEN );
    require_string( http_req.http_res != NULL, exit, "malloc error" );
    memset( http_req.http_res, 0, FOG_TIME_RES_BUFF_LEN );
    http_req.res_len = FOG_TIME_RES_BUFF_LEN;

    sprintf( http_req.http_req, FOG_TIME_STR, FOG_TIME_URI_NORMAL, domain );
    http_req.req_len = strlen( http_req.http_req );

    time_log("http request:\r\n%s", http_req.http_req);

    err = http_short_connection_nossl( &http_req );
    require_noerr_string( err, exit, "fog_get_time error!" );

    require_action_string( http_req.is_success == true, exit, err = kGeneralErr, "http request error!" );
    time_log("[get time]http response:%s", http_req.http_res);

    time_log("response:%s", http_req.http_res);

    process_get_time_response( http_req.http_res, timestamp );

    exit:
    //free request buff
    if ( http_req.http_req != NULL )
    {
        free( http_req.http_req );
        http_req.http_req = NULL;
    }

    //free response buff
    if ( http_req.http_res != NULL )
    {
        free( http_req.http_res );
        http_req.http_res = NULL;
    }

    return err;
}

OSStatus fog_sync_utc_time( void )
{
    OSStatus err = kNoErr;
    mico_utc_time_ms_t utc_time_ms;
    int timestamp = 0;

    err = fog_service_get_time( &timestamp );
    if ( err == kNoErr )
    {
        utc_time_ms = (uint64_t) timestamp * (uint64_t) 1000;
        mico_time_set_utc_time_ms( &utc_time_ms );
        time_log( "set utc time: %d", timestamp );
    }

    return err;
}

void local_time_get_utc_time( mico_utc_time_t *utc_time )
{
    int timezero = 0;

    mico_utc_time_t mico_utc_time;
    mico_time_get_utc_time( &mico_utc_time );
    timezero = (fog_timezero_get( ) * 60);

    *utc_time = mico_utc_time + timezero;
}

void fog_rtc_time_get( mico_rtc_time_t *rtc_time )
{
    mico_utc_time_t utc_time;
    struct tm * currentTime;

    local_time_get_utc_time( &utc_time );

    currentTime = localtime( (const time_t *) &utc_time );

    rtc_time->year = (currentTime->tm_year + 1900) % 100;
    rtc_time->month = currentTime->tm_mon + 1;
    rtc_time->date = currentTime->tm_mday;
    rtc_time->weekday = currentTime->tm_wday;
    rtc_time->hr = currentTime->tm_hour;
    rtc_time->min = currentTime->tm_min;
    rtc_time->sec = currentTime->tm_sec;
}

OSStatus fog_timer_task_set( char *buf, int len )
{
    int i = 0;
    fog_timer_config_t *timer = &(fog_context_get( )->config.time_config.timer_config);
    uint8_t *temp = (uint8_t *) timer;

    if ( (buf == NULL) || ( len == 0 ) )
        return -1;

    if ( (len / 2) > (sizeof(fog_timer_config_t)) )
        return -1;

    memset( (uint8_t *) timer, 0x00, sizeof(fog_timer_config_t) );

    for ( i = 0; i < len; i += 2 )
    {
        temp[i / 2] = a2x( buf[i] ) << 4;
        temp[i / 2] += a2x( buf[i + 1] );
        // time_log("temp[%d]:%02X", i/2, temp[i / 2] );
    }

    if( timer->timer_num > FOG_TIMER_TASK_MAX )
    {
        return -1;
    }

    for ( i = 0; i < (timer->timer_num); i++ )
    {
        time_log( "timer[%d], enable: %d. loop: %d", i, timer->timer_task[i].enable, timer->timer_task[i].freq.loop );
        time_log( "timer[%d], Schedule sun:%d mon:%d tus:%d wed:%d thur:%d fri:%d sat:%d", i,
            timer->timer_task[i].freq.sun, timer->timer_task[i].freq.mon, timer->timer_task[i].freq.tues,
            timer->timer_task[i].freq.wed, timer->timer_task[i].freq.thur, timer->timer_task[i].freq.fri,
            timer->timer_task[i].freq.sat);
        time_log( "timer[%d], group[0] enable:%d", i, timer->timer_task[i].group[0].enable);
        time_log( "timer[%d], group[0] hour:%d", i, timer->timer_task[i].group[0].hour );
        time_log( "timer[%d], group[0] min:%d", i, timer->timer_task[i].group[0].min );
        time_log( "timer[%d], group[0] sec:%d", i, timer->timer_task[i].group[0].sec );
        time_log( "timer[%d], group[1] enable:%d", i, timer->timer_task[i].group[1].enable);
        time_log( "timer[%d], group[1] hour:%d", i, timer->timer_task[i].group[1].hour );
        time_log( "timer[%d], group[1] min:%d", i, timer->timer_task[i].group[1].min );
        time_log( "timer[%d], group[1] sec:%d", i, timer->timer_task[i].group[1].sec );
    }

    fog_flash_write( );
    return kNoErr;
}

fog_timer_config_t *fog_timer_task_get( void )
{
    if ( fog_context_get( ) == NULL )
    {
        return NULL;
    }
    return &(fog_context_get( )->config.time_config.timer_config);
}

char *fog_timer_task_get_string( void )
{
    char *timer = NULL;
    uint8_t num = 0;

    if ( fog_context_get( ) == NULL )
    {
        return NULL;
    }

    num = fog_timer_task_get( )->timer_num;

    timer = DataToHexString( (uint8_t*) fog_timer_task_get( ), sizeof(timer_task_t) * num + 1 );

    return timer;
}

void fog_timer_task_clean( void )
{
    fog_timer_task_set("00", 2);
}

OSStatus fog_delay_task_set( uint32_t delay_min, void *delay_cb )
{
    mico_utc_time_t utc_time;
    mico_rtc_time_t rtc_time;
    uint8_t hour = 0;
    uint8_t min = 0;
    fog_delay_status_t *delay = &(fog_context_get( )->status.time_status.delay_status);

    hour = (uint8_t) (delay_min / 60);
    min = (uint8_t) (delay_min % 60);

    local_time_get_utc_time( &utc_time );
    delay->utc = utc_time + (delay_min * 60);

    fog_rtc_time_get( &rtc_time );

    if ( (rtc_time.min + min) > 60 )
    {
        delay->min = (rtc_time.min + min) - 60;
        delay->hour++;
    } else
    {
        delay->min = min + rtc_time.min;
    }

    rtc_time.hr += hour;

    if ( rtc_time.hr > 24 )
    {
        rtc_time.hr -= 24;
    }

    delay->hour = rtc_time.hr;
    delay->sec = rtc_time.sec;
    delay->enable = 1;

    if ( delay_cb )
        fog_context_get( )->status.time_status.delay_status.delay_cb = delay_cb;

    time_log( "delay time: %ld", delay_min );
    time_log( "delay hour: %d", delay->hour );
    time_log( "delay min: %d", delay->min );
    time_log( "delay src: %d", delay->sec );
    
    return kNoErr;
}

uint32_t fog_delay_task_get( void )
{
    mico_utc_time_t utc_time;
    fog_delay_status_t *delay = &(fog_context_get( )->status.time_status.delay_status);

    if ( delay->enable == 0 ) return 0;

    local_time_get_utc_time( &utc_time );

    return (delay->utc - utc_time) / 60;
}

OSStatus fog_delay_task_cancel( void *delay_cb )
{
    fog_delay_status_t *delay = &(fog_context_get( )->status.time_status.delay_status);

    if ( delay_cb )
        fog_context_get( )->status.time_status.delay_status.delay_cb = NULL;

    delay->enable = 0;

    return kNoErr;
}

static void timer_sync_task_thread( uint32_t arg )
{
    while ( 1 )
    {
        mico_rtos_thread_sleep( FOG_TIME_SYNC );
        fog_sync_utc_time( );
    }
}

static void timer_task_thread( uint32_t arg )
{
    int i, j;
    OSStatus err = kGeneralErr;
    fog_time_config_t * time_config = &(fog_context_get( )->config.time_config);
    fog_timer_config_t *timer_config = &(fog_context_get( )->config.time_config.timer_config);
    fog_timer_status_t *timer_status = &(fog_context_get( )->status.time_status.timer_status);
    fog_delay_status_t *delay_status = &(fog_context_get( )->status.time_status.delay_status);

    mico_rtc_time_t rtc_time;
    mico_rtos_thread_sleep(2);
    err = fog_sync_utc_time( );

    while ( 1 )
    {
        if ( err != kNoErr )
        {
            mico_rtos_thread_sleep( 2 );
            err = fog_sync_utc_time( );
        }
        if ( err != kNoErr )
        {
            mico_rtos_thread_sleep( 2 );
            continue;
        }

        fog_rtc_time_get( &rtc_time );

        //timer task handle
        for ( i = 0; i < timer_config->timer_num; i++ )
        {
            if ( timer_config->timer_task[i].enable == 1 )
            {
                for ( j = 0; j < FOG_TIMER_GROUP_MAX; j++ )
                {
                    if ( (timer_config->timer_task[i].group[j].enable == 1)
                         && (timer_config->timer_task[i].group[j].hour == rtc_time.hr)
                         && (timer_config->timer_task[i].group[j].min == rtc_time.min)
                         && (timer_config->timer_task[i].group[j].sec == rtc_time.sec)
                         && (time_config->timer_status[i][j] == 0) )
                    {

                        if ( timer_config->timer_task[i].freq.loop == 0 )
                        {
                            time_config->timer_status[i][j] = 1;
                            time_log("status[0]=%d, stauts[1]=%d", time_config->timer_status[i][0], time_config->timer_status[i][1]);
                            if( (time_config->timer_status[i][0] == 1) && (time_config->timer_status[i][1] == 1) )
                            {
                                time_config->timer_status[i][0] = 0;
                                time_config->timer_status[i][1] = 0;
                                timer_config->timer_task[i].enable = 0;
                            }else if( (timer_config->timer_task[i].group[0].enable == 0) )
                            {
                                time_config->timer_status[i][0] = 0;
                                time_config->timer_status[i][1] = 0;
                                timer_config->timer_task[i].enable = 0;
                            }else if( timer_config->timer_task[i].group[1].enable == 0 )
                            {
                                time_config->timer_status[i][0] = 0;
                                time_config->timer_status[i][1] = 0;
                                timer_config->timer_task[i].enable = 0;
                            }
                            time_log("local time %d-%d-%d %d %d:%d:%d",  rtc_time.year, rtc_time.month, rtc_time.date, rtc_time.weekday,
                                rtc_time.hr, rtc_time.min, rtc_time.sec);
                            if ( timer_status->timer_cb ) timer_status->timer_cb( i, timer_config->timer_task[i].group[j].status );
                            fog_flash_write( );
                        }

                        if ( (timer_config->timer_task[i].freq.loop == 1)
                             && (is_week( rtc_time.weekday, &(timer_config->timer_task[i]) ) == 1) )
                        {
                            time_config->timer_status[i][j] = 1;
                            time_log("local time %d-%d-%d %d %d:%d:%d",  rtc_time.year, rtc_time.month, rtc_time.date, rtc_time.weekday,
                                rtc_time.hr, rtc_time.min, rtc_time.sec);
                            if ( timer_status->timer_cb ) timer_status->timer_cb( i, timer_config->timer_task[i].group[j].status );
                            fog_flash_write( );
                        }
                    }

                    if ( (timer_config->timer_task[i].group[j].hour == rtc_time.hr)
                         && (timer_config->timer_task[i].group[j].min != rtc_time.min)
                         && (time_config->timer_status[i][j] == 1)
                         && (timer_config->timer_task[i].freq.loop == 1) )
                    {
                        time_config->timer_status[i][j] = 0;
                    }
                }
            }
        }

        //delay task
        if ( (delay_status->enable) && (delay_status->hour == rtc_time.hr) && (delay_status->min == rtc_time.min) && (delay_status->sec == rtc_time.sec) )
        {
            delay_status->enable = 0;
            if ( delay_status->delay_cb ) delay_status->delay_cb( );
        }

        mico_rtos_thread_msleep( 200 );
    }
}

OSStatus fog_timer_init( void )
{
    OSStatus err = kNoErr;

    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "timer sync task", timer_sync_task_thread, 0x500, 0 );
    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "timer task", timer_task_thread, 0x600, 0 );

    return err;
}

