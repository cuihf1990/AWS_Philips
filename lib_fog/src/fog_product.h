#ifndef __FOG_PA_PRODUCT_H__
#define __FOG_PA_PRODUCT_H__

void fog_register_commands( void );

void fog_fog_id_set( char *id, int len );
char *fog_fog_id_get( void );

void fog_signature_set( char *sign, int len );
char *fog_signature_get( void );

void fog_product_id_set( char *id, int len );
char *fog_product_id_get( void );

void fog_region_set( char *region );
char *fog_region_get( void );

void fog_type_id_set( char *type_id );
char *fog_type_id_get( void );

char *fog_device_sn_get( void );

void fog_device_secret_set( char *dsn);
char *fog_deivce_secret_get( void );

void fog_lan_secret_set(char *fid, char *pid, char *dsn);
char *fog_lan_secret_get( void );

void fog_enduser_id_set( char *id );
char *fog_enduser_id_get( void );

void fog_need_bind_set( bool status );
bool fog_need_bind_get( void );

void fog_timezero_set( int timezero );
int fog_timezero_get( void );

char *fog_softap_name_get( void );

#endif
