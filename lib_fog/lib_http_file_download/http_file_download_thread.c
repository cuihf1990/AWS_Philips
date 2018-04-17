#include "mico.h"
#include "http_file_download_thread.h"
#include "HTTPUtils.h"
#include "url.h"

#if (HTTP_FILE_DOWNLOAD_THREAD_DEBUG_ENABLE == 1)
#define app_log(M, ...)             custom_log("DOWNLOAD_THREAD", M, ##__VA_ARGS__)
#elif (HTTP_FILE_DOWNLOAD_THREAD_DEBUG_ENABLE == 0)
#define app_log(M, ...)             {;}
#else
#error "HTTP_FILE_DOWNLOAD_THREAD_DEBUG_ENABLE not define!"
#endif

void http_file_download_thread( mico_thread_arg_t arg );
uint32_t http_file_download_get_thread_num(void);

static OSStatus http_generate_request( FILE_DOWNLOAD_CONTEXT_S *file_download_context );
static OSStatus socket_send( int fd, const char *inBuf, uint32_t inBufLen );
static OSStatus file_download_dns( FILE_DOWNLOAD_CONTEXT_S *file_download_context, const char * domain, uint8_t * addr, uint8_t addrLen );
static OSStatus onReceivedData( struct _HTTPHeader_t * inHeader, uint32_t inPos, uint8_t * inData, size_t inLen, void * inUserContext );

static OSStatus http_connect( FILE_DOWNLOAD_CONTEXT_S *file_download_context );
static OSStatus http_disconnect( FILE_DOWNLOAD_CONTEXT_S *file_download_context );
bool file_download_check_control_state(FILE_DOWNLOAD_CONTEXT_S *file_download_context);

static uint32_t download_file_thread_num = 0;

extern OSStatus http_file_download_start( FILE_DOWNLOAD_CONTEXT_S **file_download_context_user, const char *file_url, FILE_DOWNLOAD_STATE_CB download_state_cb, FILE_DOWNLOAD_DATA_CB download_data_cb, uint32_t user_args);
void* ssl_nonblock_connect(int fd, int calen, char*ca, int *errno, int timeout);

uint32_t http_file_download_get_thread_num(void)
{
    return download_file_thread_num;
}

static OSStatus http_process_redirect( FILE_DOWNLOAD_CONTEXT_S * file_download_context )
{
    OSStatus err = kGeneralErr;
    char *new_url_start = NULL, *new_url_end = NULL;
    uint32_t redirect_url_len = 0;

    require_string(file_download_context != NULL && file_download_context->httpHeader != NULL, exit, "param error");

    new_url_start = strstr(file_download_context->httpHeader->buf, (const char *)HTTP_REDIRECT_HEAD);
    require_string(new_url_start != NULL, exit, "get new url head error!");

    new_url_start = strstr(new_url_start, (const char *)"http");
    require_string(new_url_start != NULL, exit, "get new url head error!");

    new_url_end = strstr(new_url_start, (const char *)"\r\n");
    require_string(new_url_start != NULL, exit, "get new url end error!");

    require_string((HTTP_REDIRECT_URL_MAX_LEN + new_url_start) > new_url_end, exit, "new url len error");

    if(file_download_context->url != NULL)
    {
        free(file_download_context->url);
        file_download_context->url = NULL;
    }

    redirect_url_len = new_url_end - new_url_start + 3;

    file_download_context->url = malloc(redirect_url_len);
    require_string(file_download_context->url != NULL, exit, "malloc error");

    memset(file_download_context->url, 0, redirect_url_len);
    memcpy(file_download_context->url, new_url_start, new_url_end - new_url_start);

    file_download_context->is_redirect = true;
    file_download_context->is_success = true;

    err = kNoErr;

    exit:
    return err;
}

static OSStatus file_download_dns( FILE_DOWNLOAD_CONTEXT_S *file_download_context, const char * domain, uint8_t * addr, uint8_t addrLen )
{
    OSStatus err = kGeneralErr;
    struct hostent* host = NULL;
    struct in_addr in_addr;
    char **pptr = NULL;
    char *ip_addr = NULL;
    uint8_t retry = 0;

    require_action_string( domain != NULL && addr != NULL, exit, err = kGeneralErr, "param error" );
    require_action_string(addr != NULL && addrLen >= 16, exit, err = kGeneralErr, "param error");

    start:
    require_action_string(file_download_check_control_state(file_download_context) == true, exit, err = kGeneralErr, "user set stop download!");

    host = gethostbyname( domain );
    require_action_string(host != NULL && host->h_addr_list != NULL, exit_with_retry, err = kGeneralErr, "gethostbyname() error");

    pptr = host->h_addr_list;
    in_addr.s_addr = *(uint32_t *) (*pptr);
    ip_addr = inet_ntoa( in_addr );
    memset( addr, 0, addrLen );
    memcpy( addr, ip_addr, strlen( ip_addr ) );

    err = kNoErr;

    exit_with_retry:
    retry ++;
    if( retry < HTTP_DNS_RETRY_MAX_TIMES && err != kNoErr)
    {
        app_log("dns error, retry = %d", retry);
        goto start;
    }

    exit:
    return err;
}

//set tcp keep_alive param
// static int user_set_tcp_keepalive( int socket, int send_timeout, int recv_timeout, int idle, int interval, int count )
// {
//     int retVal = 0, opt = 0;

//     retVal = setsockopt( socket, SOL_SOCKET, SO_SNDTIMEO, (char *) &send_timeout, sizeof(int) );
//     require_string( retVal >= 0, exit, "SO_SNDTIMEO setsockopt error!" );

//     app_log("setsockopt SO_SNDTIMEO=%d ms ok.", send_timeout);

//     retVal = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&recv_timeout,sizeof(int));
//     require_string(retVal >= 0, exit, "SO_RCVTIMEO setsockopt error!");

//     app_log("setsockopt SO_RCVTIMEO=%d ms ok.", recv_timeout);

//     // set keepalive
//     opt = 1;
//     retVal = setsockopt( socket, SOL_SOCKET, SO_KEEPALIVE, (void *) &opt, sizeof(opt) );
//     require_string( retVal >= 0, exit, "SO_KEEPALIVE setsockopt error!" );

//     opt = idle;
//     retVal = setsockopt( socket, IPPROTO_TCP, TCP_KEEPIDLE, (void *) &opt, sizeof(opt) );
//     require_string( retVal >= 0, exit, "TCP_KEEPIDLE setsockopt error!" );

//     opt = interval;
//     retVal = setsockopt( socket, IPPROTO_TCP, TCP_KEEPINTVL, (void *) &opt, sizeof(opt) );
//     require_string( retVal >= 0, exit, "TCP_KEEPINTVL setsockopt error!" );

//     opt = count;
//     retVal = setsockopt( socket, IPPROTO_TCP, TCP_KEEPCNT, (void *) &opt, sizeof(opt) );
//     require_string( retVal >= 0, exit, "TCP_KEEPCNT setsockopt error!" );

//     app_log("set tcp keepalive: idle=%d, interval=%d, cnt=%d.", idle, interval, count);

//     exit:
//     return retVal;
// }

static OSStatus socket_send( int fd, const char *inBuf, uint32_t inBufLen )
{
    OSStatus err = kParamErr;
    uint32_t writeResult = 0;
    int selectResult = 0;
    uint32_t numWritten = 0;
    fd_set writeSet;
    struct timeval t;

    require( fd >= 0, exit );
    require( inBuf, exit );
    require( inBufLen, exit );

    err = kNotWritableErr;

    FD_ZERO( &writeSet );
    FD_SET( fd, &writeSet );

    t.tv_sec = 2;
    t.tv_usec = 0;
    numWritten = 0;

    do
    {
        selectResult = select( fd + 1, NULL, &writeSet, NULL, &t );
        require( selectResult >= 1, exit );

        writeResult = write( fd, (void *) (inBuf + numWritten), (inBufLen - numWritten) );
        require( writeResult > 0, exit );

        numWritten += writeResult;

    } while ( numWritten < inBufLen );

    require_action( numWritten == inBufLen, exit, app_log("ERROR: Did not write all the bytes in the buffer. BufLen: %ld, Bytes Written: %ld", inBufLen, numWritten ); err = kUnderrunErr; );

    err = kNoErr;

    exit:
    return err;
}

//generate http request
static OSStatus http_generate_request( FILE_DOWNLOAD_CONTEXT_S *file_download_context )
{
    OSStatus err = kGeneralErr;
    url_field_t *url_parse_p = NULL;

    require_string( file_download_context != NULL, exit, "param error" );
    require_string( file_download_context->url != NULL && file_download_context->http_req != NULL, exit, "param error" );

    url_parse_p = url_parse( file_download_context->url );
    require_string( url_parse_p != NULL, exit, "url_parse error!" );

    //url_field_print( url_parse_p );

    require_string( url_parse_p->host != NULL && url_parse_p->schema != NULL, exit, "get url_path error!" );

    memset( file_download_context->ip_info.host, 0, HTTP_HOST_BUFF_SIZE );

    require_string( url_parse_p->host_type != HOST_IPV6, exit, "host type error" );
    memcpy( file_download_context->ip_info.host, url_parse_p->host, strlen( url_parse_p->host ) );

    if ( url_parse_p->port != NULL )
    {
        file_download_context->ip_info.port = atoi( (const char *) (url_parse_p->port) );
        if ( strcmp( url_parse_p->schema, "http" ) == 0 )
        {
            file_download_context->ip_info.serurity_type = HTTP_SECURITY_HTTP;
        } else if ( strcmp( url_parse_p->schema, "https" ) == 0 )
        {
            file_download_context->ip_info.serurity_type = HTTP_SECURITY_HTTPS;
        }else
        {
            file_download_context->ip_info.serurity_type = HTTP_SECURITY_NONE;
            goto exit;
        }
    }else
    {
        if ( strcmp( url_parse_p->schema, "http" ) == 0 )
        {
            file_download_context->ip_info.serurity_type = HTTP_SECURITY_HTTP;
            file_download_context->ip_info.port = HTTP_PORT_DEFAULT_NOSSL;
        } else if ( strcmp( url_parse_p->schema, "https" ) == 0 )
        {
            file_download_context->ip_info.serurity_type = HTTP_SECURITY_HTTPS;
            file_download_context->ip_info.port = HTTP_PORT_DEFAULT_SSL;
        } else
        {
            file_download_context->ip_info.serurity_type = HTTP_SECURITY_NONE;
            goto exit;
        }
    }

    if ( url_parse_p->path != NULL )
    {
        file_download_context->http_req_uri_p = strstr( file_download_context->url, url_parse_p->path );
        require_string( file_download_context->http_req_uri_p != NULL, exit, "get uri error!" );
    } else
    {
        if ( url_parse_p->query_num != 0 ) //no path and have query
        {
            file_download_context->http_req_uri_p = strchr( file_download_context->url, '?' );
            require_string( file_download_context->http_req_uri_p != NULL, exit, "get url_path error!" );
        } else
        {
            file_download_context->http_req_uri_p = NULL;
        }
    }

    memset( file_download_context->http_req, 0, file_download_context->http_req_len );
    if ( file_download_context->http_req_uri_p == NULL )
    {
        sprintf( file_download_context->http_req, HTTP_FILE_DOWNLOAD_REQUEST_STR_NOURI, file_download_context->ip_info.host, file_download_context->file_info.download_len );
    } else
    {
        sprintf( file_download_context->http_req, HTTP_FILE_DOWNLOAD_REQUEST_STR, file_download_context->http_req_uri_p, file_download_context->ip_info.host, file_download_context->file_info.download_len );
    }

    //app_log("http file download:\r\n%s", file_download_context->http_req);

    err = kNoErr;

    exit:
    if ( url_parse_p != NULL )
    {
        url_free( url_parse_p );
    }

    return err;
}

//dns socket_create connect
static OSStatus http_connect( FILE_DOWNLOAD_CONTEXT_S *file_download_context )
{
    OSStatus err = kGeneralErr;
    struct sockaddr_in addr;
    int ssl_errno = 0;
    // int ret = 0;

    require_string( file_download_context != NULL, exit, "param error" );

    err = file_download_dns(file_download_context, (const char *) file_download_context->ip_info.host, (uint8_t *) file_download_context->ip_info.ip, HTTP_IP_BUFF_SIZE );
    require_action( err == kNoErr, exit, app_log("usergethostbyname() error") );
    app_log("[DNS]host:%s, ip: %s", file_download_context->ip_info.host, file_download_context->ip_info.ip);

    /*HTTPHeaderCreateWithCallback set some callback functions */
    file_download_context->httpHeader = HTTPHeaderCreateWithCallback( HTTP_DOWNLOAD_HEAD_MAX_LEN, onReceivedData, NULL, (void *) file_download_context );
    require_action( file_download_context->httpHeader, exit, err = kNoMemoryErr );

    file_download_context->ip_info.http_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    require_action( IsValidSocket( file_download_context->ip_info.http_fd ), exit, err = kNoResourcesErr );

    if(file_download_context->ip_info.serurity_type == HTTP_SECURITY_HTTPS)
    {
//        ret = user_set_tcp_keepalive( file_download_context->ip_info.http_fd, HTTP_SEND_TIME_OUT, HTTP_RECV_TIME_OUT, HTTP_KEEP_IDLE_TIME, HTTP_KEEP_INTVL_TIME, HTTP_KEEP_COUNT );
//        require_string( ret >= 0, exit, "set tcp keep alive error!" );
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr( file_download_context->ip_info.ip );
    addr.sin_port = htons( file_download_context->ip_info.port );

    err = connect( file_download_context->ip_info.http_fd, (struct sockaddr *) &addr, sizeof(addr) );
    require_action( err == kNoErr, exit, err = kGeneralErr );

    if ( file_download_context->ip_info.serurity_type == HTTP_SECURITY_HTTPS )
    {
        ssl_set_client_version( TLS_V1_2_MODE );
        //file_download_context->ip_info.http_ssl = ssl_nonblock_connect( file_download_context->ip_info.http_fd, 0, NULL, &ssl_errno, FILE_DOWNLOAD_SSL_CONNECT_TIMEOUT);
        file_download_context->ip_info.http_ssl = ssl_connect( file_download_context->ip_info.http_fd, 0, NULL, &ssl_errno);
        require_action( file_download_context->ip_info.http_ssl != NULL, exit, {err = kGeneralErr; app_log("ssl_connnect error, errno = %d", ssl_errno);} );
        app_log("#####FILE_DOWNLOAD CONNECT SSL#####:num_of_chunks:%d, free:%d", MicoGetMemoryInfo()->num_of_chunks, MicoGetMemoryInfo()->free_memory);
    }else
    {
        app_log("#####FILE_DOWNLOAD CONNECT#####:num_of_chunks:%d, free:%d", MicoGetMemoryInfo()->num_of_chunks, MicoGetMemoryInfo()->free_memory);
    }

    err = kNoErr;

    exit:
    return err;
}

//http disconnect
static OSStatus http_disconnect( FILE_DOWNLOAD_CONTEXT_S *file_download_context )
{
    OSStatus err = kGeneralErr;

    UNUSED_PARAMETER( err );

    require_string( file_download_context != NULL, exit, "param error" );

    if ( file_download_context->ip_info.serurity_type == HTTP_SECURITY_HTTPS )
    {
        //close ssl
        if ( file_download_context->ip_info.http_ssl != NULL )
        {
            ssl_close( file_download_context->ip_info.http_ssl );
            file_download_context->ip_info.http_ssl = NULL;
        }
    }else if ( file_download_context->ip_info.serurity_type == HTTP_SECURITY_HTTP )
    {
        if ( file_download_context->ip_info.http_fd >= 0 )
        {
            close( file_download_context->ip_info.http_fd );
        }
    }else
    {
        app_log("error http type!");
    }

    //destory http header
    HTTPHeaderDestory( &file_download_context->httpHeader );

    err = kNoErr;
    app_log("#####FILE_DOWNLOAD DISCONNECT#####:num_of_chunks:%d, free:%d, fd:%d", MicoGetMemoryInfo()->num_of_chunks, MicoGetMemoryInfo()->free_memory, file_download_context->ip_info.http_fd);

    exit:
    return err;
}

//http send request
static OSStatus http_send_req( FILE_DOWNLOAD_CONTEXT_S *file_download_context )
{
    OSStatus err = kGeneralErr;
    int ret = 0;

    if ( file_download_context->ip_info.serurity_type == HTTP_SECURITY_HTTPS )
    {
        ret = ssl_send( file_download_context->ip_info.http_ssl, file_download_context->http_req, strlen( (const char *) file_download_context->http_req ) ); /* Send HTTP Request */
        if ( ret > 0 )
        {
            app_log("file download ssl_send success [%d] [%d]", strlen((const char *)file_download_context->http_req) ,ret);
            app_log("\r\n%s", file_download_context->http_req);
            err = kNoErr;
        } else
        {
            app_log("file download ssl_send error, ret = %d", ret);
            err = kGeneralErr;
            goto exit;
        }
    } else if ( file_download_context->ip_info.serurity_type == HTTP_SECURITY_HTTP )
    {
        err = socket_send( file_download_context->ip_info.http_fd, file_download_context->http_req, strlen( (const char *) file_download_context->http_req ) );
        require_noerr_string( err, exit, "socket send error!" );
    }

    exit:
    return err;
}

//http file download thread
void http_file_download_thread( mico_thread_arg_t arg )
{
    OSStatus err = kGeneralErr;
    FILE_DOWNLOAD_CONTEXT_S **file_download_context_user = (FILE_DOWNLOAD_CONTEXT_S **) arg;
    FILE_DOWNLOAD_CONTEXT_S *file_download_context = *file_download_context_user ;
    fd_set readfds;
    int ret = 0;
    struct timeval t = { 0, HTTP_SELECT_YIELD_TMIE * 1000 };

    download_file_thread_num ++;

    require_string( file_download_context != NULL, exit, "param error" );

    require_action_string(file_download_check_control_state(file_download_context) == true, exit, err = kGeneralErr, "user set stop download!");

    (file_download_context->download_state_cb)(file_download_context, HTTP_FILE_DOWNLOAD_STATE_START, file_download_context->retry_count, file_download_context->user_args);

    FILE_DOWNLOAD_START:
    file_download_context->is_redirect = false;
    file_download_context->is_success = false;
    file_download_context->control_state = HTTP_FILE_DOWNLOAD_CONTROL_STATE_RUN;

    //url parse ->domain, port is_ssl
    err = http_generate_request( file_download_context );
    require_noerr_string( err, exit, "http_url_parse() error" );

    require_action_string(file_download_check_control_state(file_download_context) == true, exit, err = kGeneralErr, "user set stop download!");

    //socket create, connect
    err = http_connect( file_download_context );
    require_noerr_string( err, exit, "http_connect() error" );

    //http send
    err = http_send_req( file_download_context );
    require_noerr_string( err, exit, "http_send_req() error" );

    require_action_string(file_download_check_control_state(file_download_context) == true, exit, err = kGeneralErr, "user set stop download!");

    //select
    FD_ZERO( &readfds );
    FD_SET( file_download_context->ip_info.http_fd, &readfds );

    ret = select( file_download_context->ip_info.http_fd + 1, &readfds, NULL, NULL, &t );
    require_action( ret > 0, exit, app_log("select error! ret = %d", ret) );

    require_action_string(file_download_check_control_state(file_download_context) == true, exit, err = kGeneralErr, "user set stop download!");

    if ( FD_ISSET( file_download_context->ip_info.http_fd, &readfds ) )
    {
        require_action_string(file_download_check_control_state(file_download_context) == true, exit, err = kGeneralErr, "user set stop download!");

        /*parse header*/
        if(file_download_context->ip_info.serurity_type == HTTP_SECURITY_HTTP)
        {
            err = SocketReadHTTPHeader( file_download_context->ip_info.http_fd, file_download_context->httpHeader );
        }else if(file_download_context->ip_info.serurity_type == HTTP_SECURITY_HTTPS)
        {
            err = SocketReadHTTPSHeader( file_download_context->ip_info.http_ssl, file_download_context->httpHeader );
        }

        switch ( err )
        {
            case kNoErr:
                {
                    //TODO redirt problem
                    if ( (file_download_context->httpHeader->statusCode == 200) || (file_download_context->httpHeader->statusCode == 206) )
                    {
                        //PrintHTTPHeader( file_download_context->httpHeader );
                        if(file_download_context->ip_info.serurity_type == HTTP_SECURITY_HTTP)
                        {
                            err = SocketReadHTTPBody( file_download_context->ip_info.http_fd, file_download_context->httpHeader ); /*get body data*/
                            require_noerr_action( err, exit, app_log("============err = %d", err));
                        }else if(file_download_context->ip_info.serurity_type == HTTP_SECURITY_HTTPS)
                        {
                            err = SocketReadHTTPSBody( file_download_context->ip_info.http_ssl, file_download_context->httpHeader ); /*get body data*/
                            require_noerr( err, exit );
                        }
                    } else if(file_download_context->httpHeader->statusCode == 301 || file_download_context->httpHeader->statusCode == 302)
                    {
                        app_log( "get http redirect!!! statusCode = %d", file_download_context->httpHeader->statusCode);
                        err = http_process_redirect(file_download_context);
                        require_noerr( err, exit );
                    }else
                    {
                        app_log( "[ERROR]http response error, statusCode = %d !!!", file_download_context->httpHeader->statusCode);
                        goto exit;
                    }

                    if(file_download_context->file_info.download_len == file_download_context->file_info.file_total_len && \
                        file_download_context->file_info.file_total_len != 0)
                    {
                        file_download_context->is_success = true;
                    }

                    break;
                }
            case EWOULDBLOCK:
                {
                    app_log("SocketReadHTTPSHeader EWOULDBLOCK");
                    break;
                }
            case kNoSpaceErr:
                {
                    app_log("SocketReadHTTPSHeader kNoSpaceErr");
                    break;
                }
            case kConnectionErr:
                {
                    app_log("SocketReadHTTPSHeader kConnectionErr");
                    break;
                }
            default:
                {
                    app_log("ERROR: HTTP Header parse error: %d", err);
                    break;
                }
        }
    }else
    {
        app_log("[ERROR]http_fd not in readfds!");
    }

    exit:
    //TODO
    http_disconnect( file_download_context );

    //process redirect
    if(file_download_context->is_redirect == true)
    {
        app_log("file download get redirct url, new url:%s", file_download_context->url);
    }else
    {
        if(file_download_context->is_success == false)
        {
            mico_thread_msleep(100);

            file_download_context->retry_count ++;
            if(file_download_context->retry_count <= DOWNLOAD_FILE_MAX_RETRY)
            {
                require_action_string(file_download_check_control_state(file_download_context) == true, EXIT_THREAD, err = kGeneralErr, "user set stop download!");

                app_log("file download failed, retry = %d, control state = %d", file_download_context->retry_count, file_download_context->control_state);
                (file_download_context->download_state_cb)(file_download_context, HTTP_FILE_DOWNLOAD_STATE_FAILED_AND_RETRY, file_download_context->retry_count, file_download_context->user_args);

                goto FILE_DOWNLOAD_START;
            }
        }
    }

  EXIT_THREAD:
    if ( file_download_context->is_redirect == true )
    {
        *file_download_context_user = NULL;

        err = http_file_download_start(file_download_context_user, file_download_context->url, file_download_context->download_state_cb, file_download_context->download_data_cb, file_download_context->user_args);
        if(err != kNoErr)
        {
            app_log("redirect create file download thread error!");
        }
    }else
    {
        if(file_download_context->is_success == false)
        {
            app_log("file download failed!");
            (file_download_context->download_state_cb)(file_download_context, HTTP_FILE_DOWNLOAD_STATE_FAILED, 0, file_download_context->user_args);
        }else
        {
            app_log("file download success!");
            (file_download_context->download_state_cb)(file_download_context, HTTP_FILE_DOWNLOAD_STATE_SUCCESS, 0, file_download_context->user_args);
        }
    }

    if ( file_download_context != NULL )
    {
        if ( file_download_context->http_req != NULL )
        {
            free( file_download_context->http_req );
            file_download_context->http_req = NULL;
        }

        if ( file_download_context->url != NULL )
        {
            free( file_download_context->url );
            file_download_context->url = NULL;
        }

        if(file_download_context->is_redirect == false)
        {
            *file_download_context_user = NULL;
        }

        free( file_download_context );
        file_download_context = NULL;
    }

    download_file_thread_num --;

    app_log("download_file_thread_num = %ld", download_file_thread_num);
    mico_rtos_delete_thread( NULL );
    return;
}

/*one request may receive multi reply*/
static OSStatus onReceivedData( struct _HTTPHeader_t * inHeader, uint32_t inPos, uint8_t * inData, size_t inLen, void * inUserContext )
{
    OSStatus err = kGeneralErr;
    bool download_state = 0;
    uint32_t old_progress = 0, progress = 0;
    FILE_DOWNLOAD_CONTEXT_S *file_download_context = (FILE_DOWNLOAD_CONTEXT_S *) (inUserContext);
    uint32_t index = 0;

    if(inLen == 0)
    {
        return kNoErr;
    }

    require_string(file_download_context != NULL && inHeader != NULL, exit, "param error!");

    if(file_download_check_control_state(file_download_context) != true)
    {
        http_disconnect( file_download_context );
        err = kGeneralErr;
        app_log("user set stop download!");
        goto exit;
    }

    require_action_string(inHeader->statusCode == 200 || inHeader->statusCode == 206, exit, err = kGeneralErr, "status code error");

    if ( (inHeader->statusCode == 200 || inHeader->statusCode == 206) && file_download_context->file_info.file_total_len == 0) //if http disconected, the file len is the remaining length
    {
        //get the first length!!!
        file_download_context->file_info.file_total_len = (uint32_t) (file_download_context->httpHeader->contentLength);  //file_len
        app_log("get file total len:%ld.", file_download_context->file_info.file_total_len);
    }

    //app_log("total_file_len:%ld, inPos:%ld, inLen:%d, download_len:%ld", file_download_context->file_info.file_total_len, inPos, inLen, file_download_context->file_info.download_len);
    if(file_download_context->file_info.file_total_len < file_download_context->file_info.download_len)
    {
        http_disconnect( file_download_context );
        err = kGeneralErr;
        app_log("internal error!");
        goto exit;
    }

    //user state callback
    old_progress = (file_download_context->file_info.download_len * 100)/file_download_context->file_info.file_total_len;

    //TODO
    //user data callback, if write data error, disconnect the socket
    while(1)
    {
        if((index + HTTP_RECV_CALLBACK_MAX_PACKET_LEN) < inLen)
        {
            download_state = (file_download_context->download_data_cb)(file_download_context, (const char *)inData + index, HTTP_RECV_CALLBACK_MAX_PACKET_LEN, file_download_context->user_args);
            if(download_state == false)
            {
                app_log("download_data_cb return false");
                file_download_context->control_state = HTTP_FILE_DOWNLOAD_CONTROL_STATE_STOP;
                http_disconnect( file_download_context );
                err = kGeneralErr;
                goto exit;
            }
            index = index + HTTP_RECV_CALLBACK_MAX_PACKET_LEN;
            //app_log("index:%ld", index);
        }else
        {
            download_state = (file_download_context->download_data_cb)(file_download_context, (const char *)inData + index, inLen - index, file_download_context->user_args);
            if(download_state == false)
            {
                app_log("download_data_cb return false");
                file_download_context->control_state = HTTP_FILE_DOWNLOAD_CONTROL_STATE_STOP;
                http_disconnect( file_download_context );
                err = kGeneralErr;
                goto exit;
            }
            index = inLen;
            //app_log("index_end:%ld", index);
            break;
        }
    }

    file_download_context->file_info.download_len += inLen;

    progress = (file_download_context->file_info.download_len * 100)/file_download_context->file_info.file_total_len;

    if((progress % 2) == 0 && (old_progress != progress))
    {
        (file_download_context->download_state_cb)(file_download_context, HTTP_FILE_DOWNLOAD_STATE_LOADING, progress, file_download_context->user_args);
    }

    err = kNoErr;

    exit:
    if(err != kNoErr)
    {
        app_log("onReceivedData err = %d", err);
    }

    return err;
}

/**
 * @brief  http file downlaod check control state
 * @param  file_download_context[in/out]: file download context point
 * @retval true:user set run, false:user set stop
 */
bool file_download_check_control_state(FILE_DOWNLOAD_CONTEXT_S *file_download_context)
{
    while(1)
    {
        if(file_download_context->control_state == HTTP_FILE_DOWNLOAD_CONTROL_STATE_PAUSE)
        {
            mico_rtos_thread_msleep(10);
            continue;
        }else if(file_download_context->control_state == HTTP_FILE_DOWNLOAD_CONTROL_STATE_RUN )
        {
            return true;
        }else if(file_download_context->control_state == HTTP_FILE_DOWNLOAD_CONTROL_STATE_STOP)
        {
            return false;
        }
    }
}

