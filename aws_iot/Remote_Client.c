/*
 * Remote_Client.c
 *
 *  Created on: 2018年3月27日
 *      Author: Cuihf
 */
#include "mico.h"
#include "Remote_Client.h"
#include "HTTPUtils.h"
#include "json_parser.h"

#define remote_log(M, ...) //custom_log("APP", M, ##__VA_ARGS__)


static OSStatus ReceivedData( struct _HTTPHeader_t * inHeader, uint32_t inPos, uint8_t * inData, size_t inLen, void * inUserContext );
static void ClearData( struct _HTTPHeader_t * inHeader, void * inUserContext );

extern mico_semaphore_t login_sem;
extern system_context_t* sys_context;
uint8_t Get_Info_Step = 1;

void Start_Login()
{
   int err = -1;

   err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "Remote_Client_Thread", Remote_Client_Thread,0x2000, (uint32_t)NULL);
   require_noerr_action( err, exit, remote_log("ERROR: Unable to start the Remote Client Thread.") );

   return ;

   exit:
   remote_log("creat_thread failed");
}

void Get_CERT_URL(mico_ssl_t client_ssl)
{
    char * Report_data = NULL;
    int len = 0;
    char  URL_Address[200] = {0};

    if(Report_data == NULL)
    	Report_data = malloc(MAX_TCP_LENGH);

    memset(Report_data,0,MAX_TCP_LENGH);


    sprintf(URL_Address,GET_CA_URL,sys_context->flashContentInRam.Cloud_info.mac_address);

	sprintf(Report_data,HTTP_GET,URL_Address,HOST);

	remote_log("report = %s",Report_data);

	len = ssl_send(client_ssl,Report_data,strlen(Report_data));
	if(len <= 0)
		remote_log("ssl send error");

	if(Report_data)  free(Report_data);
	if(URL_Address)  free(URL_Address);
}

void Get_MQTT(mico_ssl_t client_ssl)
{
    char * Report_data = NULL;
    int len = 0;
    char  URL_Address[200] = {0};

    if(Report_data == NULL)
    	Report_data = malloc(MAX_TCP_LENGH);

    memset(Report_data,0,MAX_TCP_LENGH);

    sprintf(URL_Address,CA_REGISTER,sys_context->flashContentInRam.Cloud_info.mac_address);

	sprintf(Report_data,HTTP_GET,URL_Address,HOST);
	len = ssl_send(client_ssl,Report_data,strlen(Report_data));
	if(len <= 0)
		remote_log("ssl send error");

	remote_log("send MQTT %s",Report_data);
	if(Report_data)  free(Report_data);
}

mico_ssl_t client_ssl = NULL; ///ssl
void Remote_Client_Thread()
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

    if(Tcp_Buffer == NULL)
    	Tcp_Buffer = malloc(MAX_TCP_LENGH);

    memset(Tcp_Buffer,0,MAX_TCP_LENGH);

    while(1){
    	if(Remote_Client_fd == -1){    ////未创建socket
    		hostent_content = gethostbyname( (char *)HOST);
            if(hostent_content == NULL)
            {
            	msleep(500);
            	return;
            }
            /*HTTPHeaderCreateWithCallback set some callback functions */
            httpHeader = HTTPHeaderCreateWithCallback( HTTP_RESPONSE_BODY_MAX_LEN, ReceivedData, NULL, &context );
            if ( httpHeader == NULL )
            {
                mico_thread_msleep( 200 );
                remote_log("HTTPHeaderCreateWithCallback() error");
                return;
            }
    		pptr=hostent_content->h_addr_list;
    		in_addr.s_addr = *(uint32_t *)(*pptr);
    	 	strcpy( ipstr, inet_ntoa(in_addr));
    	 	Remote_Client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    		addr.sin_family = AF_INET;
    		addr.sin_addr.s_addr = inet_addr(ipstr);
    	  	addr.sin_port = htons(443);

    	   	err = connect(Remote_Client_fd, (struct sockaddr *)&addr, sizeof(addr));
            if(err != 0){
            	SocketClose(Remote_Client_fd);
            	msleep(200);
            	return ;
            }

            ssl_set_client_version( TLS_V1_2_MODE );
            client_ssl = ssl_connect( Remote_Client_fd, 0, NULL, &ssl_errno );
            if(client_ssl != NULL)
            	remote_log("SSL connect success");

            if(Get_Info_Step == 1)
            	Get_CERT_URL(client_ssl);
            else if(Get_Info_Step == 2)
            	Get_MQTT(client_ssl);

    	}else {
    	    FD_ZERO(&Client_fds);
    	    FD_SET(Remote_Client_fd, &Client_fds);
    	    t.tv_sec = 2;
    	    t.tv_usec = 0;
            select(Remote_Client_fd+1,&Client_fds, NULL, NULL, &t);
            if (FD_ISSET(Remote_Client_fd, &Client_fds)) {
                /*parse header*/
                err = SocketReadHTTPSHeader( client_ssl, httpHeader );
                remote_log("err = %d",err);
                switch ( err )
                {
                 case kNoErr:
                    {
                         if((httpHeader->statusCode == -1) || (httpHeader->statusCode >= 500))
                         {
                        	remote_log("[ERROR]fog http response error, code:%d", httpHeader->statusCode);
                            goto exit; //断开重新连接
                         }

                         if((httpHeader->statusCode == 405) || (httpHeader->statusCode == 404))
                         {
                        	remote_log( "[FAILURE]fog http response fail, code:%d", httpHeader->statusCode);
                        	Get_info_Success(0);
                        	break;
                         }
                        //只有code正确才解析返回数据,错误情况下解析容易造成内存溢出
                        if((httpHeader->statusCode == 200) || (httpHeader->statusCode == 201) || (httpHeader->statusCode == 202) || (httpHeader->statusCode == 400) || (httpHeader->statusCode == 401) || (httpHeader->statusCode == 403))
                        {
                            //PrintHTTPHeader( httpHeader );
                            err = SocketReadHTTPSBody( client_ssl, httpHeader );    /*get body data*/
                            require_noerr( err, exit );

                            goto exit;

                        }else
                        {
                        	remote_log( "[ERR]fog http response ok, but code = %d !!!", httpHeader->statusCode);
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
                    	remote_log("SocketReadHTTPSHeader kNoSpaceErr");
                        goto exit;
                        break;
                    }
                 case kConnectionErr:
                    {
                    	remote_log("SocketReadHTTPSHeader kConnectionErr");
                        goto exit;
                        break;
                    }
                 default:
                    {
                    	remote_log("ERROR: HTTP Header parse error: %d", err);
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

		if(Get_Info_Step == 2)
		{
			if(Tcp_Buffer)  free(Tcp_Buffer);
			Get_info_Success(1);
			mico_rtos_set_semaphore( &login_sem );
			mico_rtos_delete_thread(NULL);
		}
		Get_Info_Step =2;
    }
}


void Cloud_Data_Process(uint8_t * Tcp_Buffer , int Buffer_Length)
{
	  OSStatus err = kNoErr;
	  jsontok_t json_tokens[10];    // Define an array of JSON tokens
	  jobj_t jobj;

      remote_log("recv = %s",Tcp_Buffer);
      err = json_init( &jobj, json_tokens, 10, (char *) Tcp_Buffer, strlen( Tcp_Buffer ) );
      if(err != 0) remote_log("parse error!");

      err = json_get_val_str( &jobj, "certificate_id", sys_context->flashContentInRam.Cloud_info.certificate_id, sizeof(sys_context->flashContentInRam.Cloud_info.certificate_id) );
      if(err != 0) remote_log("have no certificate_id");

      err = json_get_val_str( &jobj, "certificate_url", sys_context->flashContentInRam.Cloud_info.certificate_url, sizeof(sys_context->flashContentInRam.Cloud_info.certificate_url) );
      if(err != 0) remote_log("have no certificate_url");

      err = json_get_val_str( &jobj, "privatekey_url", sys_context->flashContentInRam.Cloud_info.privatekey_id, sizeof(sys_context->flashContentInRam.Cloud_info.privatekey_id) );
      if(err != 0) remote_log("have no privatekey_ur");

      err = json_get_val_int( &jobj, "mqtt_port", &sys_context->flashContentInRam.Cloud_info.mqtt_port);
      if(err != 0) remote_log("have no mqtt_port");

      err = json_get_val_str( &jobj, "mqtt_host", sys_context->flashContentInRam.Cloud_info.mqtt_host, sizeof(sys_context->flashContentInRam.Cloud_info.mqtt_host) );
      if(err != 0) remote_log("have no mqtt_host");

      err = json_get_val_str( &jobj, "device_id", sys_context->flashContentInRam.Cloud_info.device_id, sizeof(sys_context->flashContentInRam.Cloud_info.device_id) );
      if(err != 0) remote_log("have no device_id");


   return ;
}

void Save_cloud_data(int num , char * data)
{
  switch(num){
  case CERT_ID:
	  memset(sys_context->flashContentInRam.Cloud_info.certificate_id , 0 ,INFO_MAX_DATA);
	  strcpy(sys_context->flashContentInRam.Cloud_info.certificate_id,data);
	  break;
  case CERT_URL:
	  memset(sys_context->flashContentInRam.Cloud_info.certificate_url , 0 ,INFO_MAX_DATA);
	  strcpy(sys_context->flashContentInRam.Cloud_info.certificate_url,data);
	  break;
  case PRIVA_ID:
	  memset(sys_context->flashContentInRam.Cloud_info.privatekey_id, 0 ,INFO_MAX_DATA);
	  strcpy(sys_context->flashContentInRam.Cloud_info.privatekey_id,data);
	  break;
  case MQTT_PORT:
	  sys_context->flashContentInRam.Cloud_info.mqtt_port = atoi(data);
	  break;
  case MQTT_HOST:
	  memset(sys_context->flashContentInRam.Cloud_info.mqtt_host , 0 ,INFO_MAX_DATA);
	  strcpy(sys_context->flashContentInRam.Cloud_info.mqtt_host,data);
	  break;
  case DEVICE_ID:
	  memset(sys_context->flashContentInRam.Cloud_info.device_id , 0 ,INFO_MAX_DATA);
	  strcpy(sys_context->flashContentInRam.Cloud_info.device_id,data);
	  break;
  default:
	break;
  }
}


/*one request may receive multi reply*/
static OSStatus ReceivedData( struct _HTTPHeader_t * inHeader, uint32_t inPos, uint8_t * inData, size_t inLen, void * inUserContext )
{
    OSStatus err = kNoErr;
    http_context_t *context = inUserContext;
    remote_log("have get data = %s",inData);
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

     Cloud_Data_Process(inData,inLen);

 exit:
    return err;
}

/* Called when HTTPHeaderClear is called */
static void ClearData( struct _HTTPHeader_t * inHeader, void * inUserContext )
{
    UNUSED_PARAMETER( inHeader );
    http_context_t *context = inUserContext;
    if( context->content ) {
        free( context->content );
        context->content = NULL;
    }
}
