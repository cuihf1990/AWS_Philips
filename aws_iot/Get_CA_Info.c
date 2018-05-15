/*
 * Get_CA_Info.c
 *
 *  Created on: 2018年3月28日
 *      Author: Cuihf
 */
#include "mico.h"
#include "Get_CA_Info.h"
#include "Remote_Client.h"
#include "HTTPUtils.h"
#include "json_parser.h"
#include "device.h"

#define Get_CA_log(M, ...)  custom_log("APP", M, ##__VA_ARGS__)
static OSStatus ReceivedData( struct _HTTPHeader_t * inHeader, uint32_t inPos, uint8_t * inData, size_t inLen, void * inUserContext );
static void onClearData( struct _HTTPHeader_t * inHeader, void * inUserContext );
extern system_context_t* sys_context;
extern devcie_running_status_t  device_running_status;
mico_semaphore_t get_ca_sem = NULL;
int Get_Key_Step = 1;
int Http_Totoa_Length = 0;

void Get_CA_INFO()
{
	int err = 0;
    mico_rtos_init_semaphore( &get_ca_sem, 1 );

	err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "GET_CA_Thread", GET_CA_Thread,0x2000, (uint32_t)NULL);
	require_noerr_action( err, exit, Get_CA_log("ERROR: Unable to start the GET  CA  Thread.") );

    mico_rtos_get_semaphore( &get_ca_sem, MICO_WAIT_FOREVER );
	return ;

	exit:
	Get_CA_log("creat_thread failed");

}

void Get_CERT(mico_ssl_t client_ssl)
{
    char * Report_data = NULL;
    int len = 0;

    if(Report_data == NULL)
    	Report_data = malloc(MAX_TCP_LENGH);

    memset(Report_data,0,MAX_TCP_LENGH);

  //  Get_CA_log("URL = %s",sys_context->flashContentInRam.Cloud_info.certificate_url);

	sprintf(Report_data,HTTP_GET,device_running_status.certificate_url,CA_HOST);
	len = ssl_send(client_ssl,Report_data,strlen(Report_data));
	if(len <= 0)
		Get_CA_log("ssl send error");

	Get_CA_log("send CERT %s",Report_data);
	if(Report_data)  free(Report_data);
}

void Get_PRIVATEKEY(mico_ssl_t client_ssl)
{
    char * Report_data = NULL;
    int len = 0;

    if(Report_data == NULL)
    	Report_data = malloc(MAX_TCP_LENGH);

    memset(Report_data,0,MAX_TCP_LENGH);

   // Get_CA_log("URL = %s",sys_context->flashContentInRam.Cloud_info.privatekey_id);
	sprintf(Report_data,HTTP_GET,device_running_status.privatekey_url,CA_HOST);
	len = ssl_send(client_ssl,Report_data,strlen(Report_data));
	if(len <= 0)
		Get_CA_log("ssl send error");

	Get_CA_log("send privatekey %s",Report_data);
	if(Report_data)  free(Report_data);
}

void GET_CA_Thread()
{
		int err = -1;
		int len= -1;
		struct sockaddr_in addr;
		struct timeval t;
		fd_set Client_fds;
		int Remote_Client_fd = -1;
		struct hostent* hostent_content = NULL;
	    uint8_t * Tcp_Buffer = NULL;
	    char ** pptr = NULL;
	    struct in_addr in_addr;
	    char ipstr[16];
	    int ssl_errno = 0;
	    HTTPHeader_t *httpHeader = NULL;     ///http
	    http_context_t context = { NULL, 0 };
	    mico_ssl_t client_ssl = NULL; ///ssl

	    if(Tcp_Buffer == NULL)
	    	Tcp_Buffer = malloc(MAX_TCP_LENGH);

	    memset(Tcp_Buffer,0,MAX_TCP_LENGH);

	    while(1){
	    	if(Remote_Client_fd == -1){    ////未创建socket
	    		hostent_content = gethostbyname( (char *)CA_HOST);
	            if(hostent_content == NULL)
	            {
	            	msleep(500);
	            	return;
	            }
	            /*HTTPHeaderCreateWithCallback set some callback functions */
	            httpHeader = HTTPHeaderCreateWithCallback( HTTP_RESPONSE_BODY_MAX_LEN, ReceivedData, onClearData, &context );
	            if ( httpHeader == NULL )
	            {
	                mico_thread_msleep( 200 );
	                Get_CA_log("HTTPHeaderCreateWithCallback() error");
	                return;
	            }
	    		pptr=hostent_content->h_addr_list;
	    		in_addr.s_addr = *(uint32_t *)(*pptr);
	    	 	strcpy( ipstr, inet_ntoa(in_addr));
	    	 	Remote_Client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	    		addr.sin_family = AF_INET;
	    		addr.sin_addr.s_addr = inet_addr(ipstr);
	    	  	addr.sin_port = htons(CA_PORT);

	    	   	err = connect(Remote_Client_fd, (struct sockaddr *)&addr, sizeof(addr));
	            if(err != 0){
	            	SocketClose(Remote_Client_fd);
	            	msleep(200);
	            	return ;
	            }

	            ssl_set_client_version( TLS_V1_2_MODE );
	            client_ssl = ssl_connect( Remote_Client_fd, 0, NULL, &ssl_errno );
	            if(client_ssl != NULL)
	            	Get_CA_log("SSL connect success");

	            msleep(100);

	            Get_CA_log("Get_Key_Step = %d",Get_Key_Step);
	            if(Get_Key_Step == 1)
	            	Get_CERT(client_ssl);
	            else if(Get_Key_Step ==2)
	            	Get_PRIVATEKEY(client_ssl);

	    	}else {
	    	    FD_ZERO(&Client_fds);
	    	    FD_SET(Remote_Client_fd, &Client_fds);
	    	    t.tv_sec = 2;
	    	    t.tv_usec = 0;
	            select(Remote_Client_fd+1,&Client_fds, NULL, NULL, &t);
	            if (FD_ISSET(Remote_Client_fd, &Client_fds)) {
	                /*parse header*/
	                err = SocketReadHTTPSHeader( client_ssl, httpHeader );
	                switch ( err )
	                {
	                 case kNoErr:
	                    {
	                         if((httpHeader->statusCode == -1) || (httpHeader->statusCode >= 500))
	                         {
	                        	 Get_CA_log("[ERROR]fog http response error, code:%d", httpHeader->statusCode);
	                            goto exit; //断开重新连接
	                         }

	                         if((httpHeader->statusCode == 405) || (httpHeader->statusCode == 404))
	                         {
	                        	 Get_CA_log( "[FAILURE]fog http response fail, code:%d", httpHeader->statusCode);
	                        	 break;
	                         }
	                        //只有code正确才解析返回数据,错误情况下解析容易造成内存溢出
	                        if((httpHeader->statusCode == 200) || (httpHeader->statusCode == 201) || (httpHeader->statusCode == 202) || (httpHeader->statusCode == 400) || (httpHeader->statusCode == 401) || (httpHeader->statusCode == 403))
	                        {
	                          // PrintHTTPHeader( httpHeader );
	                        	Http_Totoa_Length = httpHeader->contentLength;

	                        	sys_context->flashContentInRam.Cloud_info.certificate_len = 0;
	                        	sys_context->flashContentInRam.Cloud_info.privatekey_len = 0;
	                        	err = SocketReadHTTPSBody( client_ssl, httpHeader );    /*get body data*/
	                            require_noerr( err, exit );
	                        }else
	                        {
	                        	Get_CA_log( "[ERR]fog http response ok, but code = %d !!!", httpHeader->statusCode);
	                            goto exit;
	                        }
	                        break;
	                    }
	                 case EWOULDBLOCK:
	                    {
	                        break;
	                    }
	                 case kNoSpaceErr:
	                    {
	                    	Get_CA_log("SocketReadHTTPSHeader kNoSpaceErr");
	                        goto exit;
	                        break;
	                    }
	                 case kConnectionErr:
	                    {
	                    	Get_CA_log("SocketReadHTTPSHeader kConnectionErr");
	                        goto exit;
	                        break;
	                    }
	                 default:
	                    {
	                    	Get_CA_log("ERROR: HTTP Header parse error: %d", err);
	                        goto exit;
	                        break;
	                    }
	                }
	            }
	    	}

	    	continue;

	 exit:
			if( client_ssl )
			{
				ssl_close( client_ssl );
				client_ssl = NULL;
			}
			SocketClose( &Remote_Client_fd );
			HTTPHeaderDestory( &httpHeader );

			if(Get_Key_Step == 3)
			{
				if(Tcp_Buffer) free(Tcp_Buffer);
				mico_rtos_set_semaphore( &get_ca_sem );
				mico_rtos_delete_thread(NULL);
			}
	    }
}

/*one request may receive multi reply*/
static OSStatus ReceivedData( struct _HTTPHeader_t * inHeader, uint32_t inPos, uint8_t * inData, size_t inLen, void * inUserContext )
{
    OSStatus err = kNoErr;
    http_context_t *context = inUserContext;

    if( inHeader->chunkedData == false){ //Extra data with a content length value
        if( inPos == 0 && context->content == NULL ){
            context->content = calloc( inHeader->contentLength + 1, sizeof(uint8_t) );
            require_action( context->content, exit, err = kNoMemoryErr );
            context->content_length = inHeader->contentLength;
        }
        memcpy( context->content + inPos, inData, inLen );
    }else{ //extra data use a chunked data protocol
        //app_log("This is a chunked data, %d", inLen);
        if( inPos == 0 ){
            context->content = calloc( inHeader->contentLength + 1, sizeof(uint8_t) );
            require_action( context->content, exit, err = kNoMemoryErr );
            context->content_length = inHeader->contentLength;
        }else{
            context->content_length += inLen;
            context->content = realloc(context->content, context->content_length + 1);
            require_action( context->content, exit, err = kNoMemoryErr );
        }
        memcpy( context->content + inPos, inData, inLen );
    }

    Get_CA_log("inLen  = %d, data = %s   %d",inLen,inData,inHeader->extraDataLen);

    Get_Key(inData,inLen);
    ////手动清楚buffer缓存

 exit:
    return err;
}

/* Called when HTTPHeaderClear is called */
static void onClearData( struct _HTTPHeader_t * inHeader, void * inUserContext )
{
    UNUSED_PARAMETER( inHeader );
    http_context_t *context = inUserContext;
    if( context->content ) {
        free( context->content );
        context->content = NULL;
    }
}



void Get_Key(uint8_t * inData, int Current_len)
{

	if(Current_len != 0){    /////如果不是空包
	if(strstr(inData,"CERTIFICATE") != NULL){
		if(sys_context->flashContentInRam.Cloud_info.certificate_len < Http_Totoa_Length){
		memcpy(sys_context->flashContentInRam.Cloud_info.certificate, inData,Current_len);
		sys_context->flashContentInRam.Cloud_info.certificate_len = Current_len;
		}
		if(sys_context->flashContentInRam.Cloud_info.certificate_len >= Http_Totoa_Length)
		Get_Key_Step = 2;
	//	Get_CA_log("inData = %s",sys_context->flashContentInRam.Cloud_info.certificate);
	}else if(strstr(inData,"PRIVATE") != NULL){
        if(Current_len < Http_Totoa_Length){
        Get_CA_log("cloud_info->privatekey_len = %d",sys_context->flashContentInRam.Cloud_info.privatekey_len);
        memcpy(sys_context->flashContentInRam.Cloud_info.privatekey+sys_context->flashContentInRam.Cloud_info.privatekey_len,inData,Current_len);
        sys_context->flashContentInRam.Cloud_info.privatekey_len += Current_len;
        }
        if(sys_context->flashContentInRam.Cloud_info.privatekey_len >= Http_Totoa_Length)
        	Get_Key_Step = 3;
		}
	}
}


