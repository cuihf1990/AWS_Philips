#ifndef __FOG_H__
#define __FOG_H__

#include "fog_task.h"
#include "fog_cloud.h"
#include "fog_local.h"
#include "fog_parser.h"
#include "fog_tools.h"


OSStatus fog_init( char *type_id, char *version );
OSStatus fog_start( void );

OSStatus fog_register_callback( uint8_t type, void *callback );
OSStatus fog_device_unbind_with_no_save( void );
OSStatus fog_device_unbind( void );

OSStatus fog_wifi_config_mode( uint8_t mode );

OSStatus fog_parser_register( const struct fog_parser_t *fog_parser, int parser_num );
OSStatus fog_parser_value( char *json_buff, uint32_t len );

OSStatus fog_timer_task_set( char *buf, int len );
fog_timer_config_t *fog_timer_task_get( void );
char *fog_timer_task_get_string( void );

OSStatus fog_delay_task_set( uint32_t delay_min, void *delay_cb );
uint32_t fog_delay_task_get( void );
OSStatus fog_delay_task_cancel( void *delay_cb );

bool fog_connect_status( void );
OSStatus fog_cloud_report( uint8_t type, char *buf, uint32_t len );

void fog_locol_report( char *data, uint32_t len );

OSStatus fog_ota_check_start( void );;


#endif
