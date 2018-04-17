#include "mico.h"
#include "http_file_download.h"

#if (HTTP_FILE_DOWNLOAD_DEBUG_ENABLE == 1)
#define app_log(M, ...)             custom_log("HTTP_FILE_DOWNLOAD", M, ##__VA_ARGS__)
#elif (HTTP_FILE_DOWNLOAD_DEBUG_ENABLE == 0)
#define app_log(M, ...)             {;}
#else
#error "HTTP_FILE_DOWNLOAD_DEBUG_ENABLE not define!"
#endif

//user api
/**
 * @brief  http file download start
 * @param  file_download_context_user[in/out]: file download context point
 * @param  file_url[in]: file url
 * @param  download_state_cb[in]: user download state callback
 * @param  download_data_cb[in]: user download data callback
 * @param  user_args[in]: this param will return in download_state_cb and download_data_cb func
 * @retval kNoErr is returned on success, otherwise, kXXXErr is returned
 */
OSStatus http_file_download_start( FILE_DOWNLOAD_CONTEXT *file_download_context_user, const char *file_url, FILE_DOWNLOAD_STATE_CB download_state_cb, FILE_DOWNLOAD_DATA_CB download_data_cb, uint32_t user_args);

/**
 * @brief  get http file download state
 * @param  file_download_context[in/out]: file download context point
 * @retval the control state of file download
 */
HTTP_FILE_DOWNLOAD_CONTROL_STATE_E http_file_download_get_state( FILE_DOWNLOAD_CONTEXT *file_download_context );

/**
 * @brief  http file downlaod pause
 * @param  file_download_context[in/out]: file download context point
 * @retval kNoErr is returned on success, otherwise, kXXXErr is returned
 */
OSStatus http_file_download_pause( FILE_DOWNLOAD_CONTEXT *file_download_context );

/**
 * @brief  http file downlaod continue
 * @param  file_download_context[in/out]: file download context point
 * @retval kNoErr is returned on success, otherwise, kXXXErr is returned
 */
OSStatus http_file_download_continue( FILE_DOWNLOAD_CONTEXT *file_download_context );

/**
 * @brief  http file downlaod stop
 * @param  file_download_context[in/out]: file download context point
 * @param  is_block[in]: is block
 * @retval kNoErr is returned on success, otherwise, kXXXErr is returned
 */
OSStatus http_file_download_stop( FILE_DOWNLOAD_CONTEXT *file_download_context, bool is_block);

/**
 * @brief  http file downlaod get total file length
 * @param  file_download_context[in/out]: file download context point
 * @retval total length of user file
 */
uint32_t http_file_download_get_total_file_len(FILE_DOWNLOAD_CONTEXT *file_download_context);

/**
 * @brief  http file downlaod get already download length
 * @param  file_download_context[in/out]: file download context point
 * @retval already download length of user file
 */
uint32_t http_file_download_get_download_len(FILE_DOWNLOAD_CONTEXT *file_download_context);


//get wifi status
static bool file_download_get_wifi_status(void)
{
    LinkStatusTypeDef link_status;

    memset(&link_status, 0, sizeof(link_status));

    micoWlanGetLinkStatus(&link_status);

    return (bool)(link_status.is_connected);
}

OSStatus http_file_download_start( FILE_DOWNLOAD_CONTEXT *file_download_context_user, const char *file_url, FILE_DOWNLOAD_STATE_CB download_state_cb, FILE_DOWNLOAD_DATA_CB download_data_cb, uint32_t user_args)
{
    OSStatus err = kGeneralErr;
    uint32_t url_len = 0;
    uint32_t stack_size = 0;
    FILE_DOWNLOAD_CONTEXT_S *file_download_context = NULL;

    require_string( (file_download_context_user != NULL)&& (*file_download_context_user == NULL), exit, "context must be NULL, you may not stop download service?" );
    require_string( (file_url != NULL) && (download_state_cb != NULL) && (download_data_cb != NULL), exit, "param error" );
    require_string(http_file_download_get_thread_num() < HTTP_DOWNLOAD_FILE_MAX_THREAD_NUM, exit, "already has 2 file download thread!");
    require_string(file_download_get_wifi_status() == true, exit, "wifi is not connected");

    if ( strstr( file_url, "https" ) != NULL )
    {
        stack_size = HTTP_THREAD_STACK_SIZE_SSL;
    } else if ( strstr( file_url, "http" ) != NULL )
    {
        stack_size = HTTP_THREAD_STACK_SIZE_NOSSL;
    } else
    {
        err = kGeneralErr;
        app_log("url error!");
        goto exit;
    }

    file_download_context = (FILE_DOWNLOAD_CONTEXT_S *) malloc( sizeof(FILE_DOWNLOAD_CONTEXT_S) );
    require_string( file_download_context != NULL, exit, "malloc error" );
    memset( file_download_context, 0, sizeof(FILE_DOWNLOAD_CONTEXT_S) );

    url_len = strlen( file_url ) + 2; //remember '\0'

    file_download_context->url = malloc( url_len );
    require_string( file_download_context->url != NULL, exit, "malloc error" );
    memset( file_download_context->url, 0, url_len );

    memcpy( file_download_context->url, file_url, url_len );

    file_download_context->http_req_len = url_len + 512;

    file_download_context->http_req = malloc( file_download_context->http_req_len );
    require_string( file_download_context->http_req != NULL, exit, "malloc error" );
    memset( file_download_context->http_req, 0, file_download_context->http_req_len );

    file_download_context->download_data_cb = download_data_cb;
    file_download_context->download_state_cb = download_state_cb;
    file_download_context->user_args = user_args;
    file_download_context->is_redirect = false;
    file_download_context->is_success = false;
    file_download_context->retry_count = 0;
    file_download_context->file_info.download_len = 0;
    file_download_context->file_info.file_total_len = 0;
    file_download_context->ip_info.http_fd = -1;
    file_download_context->ip_info.http_ssl = NULL;
    file_download_context->ip_info.serurity_type = HTTP_SECURITY_NONE;
    file_download_context->control_state = HTTP_FILE_DOWNLOAD_CONTROL_STATE_RUN;

    *file_download_context_user = file_download_context;

    require_action_string(file_download_check_control_state(file_download_context) == true, exit, err = kGeneralErr, "user set stop download!");

    /* Create a new thread */
    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "file download", http_file_download_thread, stack_size, (mico_thread_arg_t) file_download_context_user );
    require_noerr_string( err, exit, "ERROR: Unable to start the file download thread" );

    return err;
    
exit: /* error happened, must release resource and set context to NULL */
    if ( file_download_context != NULL )
    {
        if ( file_download_context->http_req != NULL )
        {
            free( file_download_context->http_req );
        }

        if ( file_download_context->url != NULL )
        {
            free( file_download_context->url );
        }

        free( file_download_context );
    }
    *file_download_context_user = NULL;
    return err;
}

HTTP_FILE_DOWNLOAD_CONTROL_STATE_E http_file_download_get_state( FILE_DOWNLOAD_CONTEXT *file_download_context )
{
    if ( file_download_context != NULL && *file_download_context != NULL )
    {
        return (*file_download_context)->control_state;
    } else
    {
        return HTTP_FILE_DOWNLOAD_CONTROL_STATE_NONE;
    }
}

uint32_t http_file_download_get_total_file_len(FILE_DOWNLOAD_CONTEXT *file_download_context)
{
    if ( file_download_context != NULL && *file_download_context != NULL )
    {
        return (*file_download_context)->file_info.file_total_len;
    } else
    {
        return 0;
    }
}

uint32_t http_file_download_get_download_len(FILE_DOWNLOAD_CONTEXT *file_download_context)
{
    if ( file_download_context != NULL && *file_download_context != NULL )
    {
        return (*file_download_context)->file_info.download_len;
    } else
    {
        return 0;
    }
}

//pause file download
OSStatus http_file_download_pause( FILE_DOWNLOAD_CONTEXT *file_download_context )
{
    OSStatus err = kGeneralErr;

    require_string( file_download_context != NULL && *file_download_context != NULL, exit, "param error" );

    (*file_download_context)->control_state = HTTP_FILE_DOWNLOAD_CONTROL_STATE_PAUSE;

    err = kNoErr;
    exit:
    return err;
}

//continue file download
OSStatus http_file_download_continue( FILE_DOWNLOAD_CONTEXT *file_download_context )
{
    OSStatus err = kGeneralErr;

    require_string( file_download_context != NULL && *file_download_context != NULL, exit, "param error" );

    (*file_download_context)->control_state = HTTP_FILE_DOWNLOAD_CONTROL_STATE_RUN;

    err = kNoErr;
    exit:
    return err;
}
//stop file download
OSStatus http_file_download_stop( FILE_DOWNLOAD_CONTEXT *file_download_context, bool is_block)
{
    OSStatus err = kGeneralErr;

    require_action(file_download_context != NULL, exit, err = kGeneralErr);

    if((*file_download_context) == NULL)
    {
        app_log("file download may already finish!");
        return kNoErr;
    }

    (*file_download_context)->control_state = HTTP_FILE_DOWNLOAD_CONTROL_STATE_STOP;

    if(is_block == true)
    {
        while((*file_download_context) != NULL)
        {
            mico_rtos_thread_msleep(20);
            app_log("------waitting download stop------- state %d",
                (*file_download_context)->control_state);
            // yhb added, control_state may be changed by http download thread.
            (*file_download_context)->control_state = HTTP_FILE_DOWNLOAD_CONTROL_STATE_STOP;
        }
    }

    err = kNoErr;
    app_log("-------download stop success-------");

    exit:
    return err;
}

