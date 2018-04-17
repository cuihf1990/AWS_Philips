#ifndef _HTTP_FILE_DOWNLOAD_H_
#define _HTTP_FILE_DOWNLOAD_H_

#include "http_file_download_thread.h"

#define HTTP_FILE_DOWNLOAD_DEBUG_ENABLE     (1)

#define HTTP_THREAD_STACK_SIZE_SSL          (0x3000)
#define HTTP_THREAD_STACK_SIZE_NOSSL        (0x1000)

#define HTTP_DOWNLOAD_FILE_MAX_THREAD_NUM   (2)

typedef FILE_DOWNLOAD_CONTEXT_S*            FILE_DOWNLOAD_CONTEXT;

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
 * @brief  http file download pause
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
 * @brief  http file download stop
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
 * @brief  http file download get already download length
 * @param  file_download_context[in/out]: file download context point
 * @retval already download length of user file
 */
uint32_t http_file_download_get_download_len(FILE_DOWNLOAD_CONTEXT *file_download_context);

/**
 * @brief  http file download check control state
 * @param  file_download_context[in/out]: file download context point
 * @retval true:user set run, false:user set stop
 */
bool file_download_check_control_state(FILE_DOWNLOAD_CONTEXT_S *file_download_context);
#endif
