#include "mico.h"
#include "json_parser.h"
#include "fog_parser.h"

#define parser_log(M, ...) custom_log("parser", M, ##__VA_ARGS__)

#define MAX_PARSERS 20
#define FOG_PARSER_NUM_TOKENS 70

typedef struct _parser_t
{
    const struct fog_parser_t *parsers[MAX_PARSERS];
    uint8_t flag[MAX_PARSERS];
    int paeser_num;
} parser_t;

static parser_t *parser = NULL;

OSStatus fog_parser_register( const struct fog_parser_t *fog_parser, int parser_num )
{
    OSStatus err = kNoErr;
    int i = 0;
    parser = malloc( sizeof(parser_t) );
    require_action( parser, exit, err = kNoMemoryErr );

    memset( parser, 0x00, sizeof(parser_t) );

    for ( i = 0; i < parser_num; i++ )
    {
        parser->paeser_num ++;
        parser->parsers[i] = fog_parser;
        fog_parser++;
        parser_log("register parser:%s", parser->parsers[i]->key_name);
    }

    exit:
    return err;
}

OSStatus fog_parser_value( char *json_buff, uint32_t len )
{
    OSStatus err = kGeneralErr;

    jsontok_t json_tokens[FOG_PARSER_NUM_TOKENS];    // Define an array of JSON tokens
    jobj_t jobj;
    char array_value[FOG_PARSER_KEY_NAME_MAX];
    char *string = NULL;
    int number = 0;
    int i, j, array_num = 0;
    fog_parser_array_t parser_array;
    int num_elements = 0;

    require_string( json_buff != NULL, exit, "body is NULL!!!" );
    require_string( ((*json_buff == '{') && (*(json_buff + len - 1) == '}')), exit, "JSON format error" );

    err = json_init( &jobj, json_tokens, FOG_PARSER_NUM_TOKENS, json_buff, len );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "json error!" );

    err = json_get_array_object( &jobj, "dp_key_array", &array_num );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get array error!" );

    for ( i = 0; i < array_num; i++ )
    {
        err = json_array_get_str( &jobj, i, array_value, FOG_PARSER_KEY_NAME_MAX );
        require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get array error!" );

        for ( j = 0; j < parser->paeser_num; j++ )
        {
            if ( strcmp( array_value, parser->parsers[j]->key_name ) == 0 )
            {
                parser->flag[j] = 1;
            }
        }
    }

    err = json_release_array_object( &jobj );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "release array error!" );

    err = json_get_composite_object( &jobj, "dp_value" );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get dp_value error!" );

    for ( i = 0; i < parser->paeser_num; i++ )
    {
        if ( parser->flag[i] == 1 )
        {
            parser->flag[i] = 0;
            if ( parser->parsers[i]->value_type == TYPE_INIT )
            {
                err = json_get_val_int( &jobj, (char *)(parser->parsers[i]->key_name), &number );
                require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get int error!" );
                if ( parser->parsers[i]->parser_cb != NULL ) parser->parsers[i]->parser_cb( number );
            } else if( parser->parsers[i]->value_type == TYPE_STRING )
            {
                string = malloc(parser->parsers[i]->value_size + 1);
                json_get_val_str(&jobj, (char *)(parser->parsers[i]->key_name), string, parser->parsers[i]->value_size);
                require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get string error!" );
                if ( parser->parsers[i]->parser_cb != NULL ) parser->parsers[i]->parser_cb( (uint32_t)string );
                if( string != NULL)
                {
                    free(string);
                    string = NULL;
                }
            } else if( parser->parsers[i]->value_type == TYPE_ARRAY )
            {
                err = json_get_array_object(&jobj, (char *)(parser->parsers[i]->key_name), &num_elements);
                require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "get array error!" );
                parser_array.jobj = &jobj;
                parser_array.num_elements = num_elements;

                if ( parser->parsers[i]->parser_cb != NULL ) parser->parsers[i]->parser_cb( (uint32_t)&parser_array );

                err = json_release_array_object(&jobj);
                require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "release array error!" );
            }

        }
    }

    err = json_release_composite_object( &jobj );
    require_action_string( err == WM_SUCCESS, exit, err = kGeneralErr, "release dp_value error!" );

    exit:
    return err;
}

