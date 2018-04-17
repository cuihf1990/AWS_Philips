#ifndef __FOG_PARSER_H__
#define __FOG_PARSER_H__

#include "json_parser.h"

#define FOG_PARSER_KEY_NAME_MAX 20

typedef enum _parser_type_e
{
    TYPE_INIT,
    TYPE_STRING,
    TYPE_ARRAY
} parser_type_e;

typedef struct _fog_parser_array_t
{
    jobj_t *jobj;
    int num_elements;
} fog_parser_array_t;

struct fog_parser_t
{
    char key_name[FOG_PARSER_KEY_NAME_MAX];
    parser_type_e value_type;
    int value_size;
    void (*parser_cb)( uint32_t arg );
} ;

OSStatus fog_parser_register( const struct fog_parser_t *fog_parser, int parser_num );
OSStatus fog_parser_value( char *json_buff, uint32_t len );

#endif
