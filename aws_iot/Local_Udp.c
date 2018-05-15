/*
 * Local_Udp.c
 *
 *  Created on: 2018Äê4ÔÂ4ÈÕ
 *      Author: Cuihf
 */
#include "mico.h"
#include "Local_Udp.h"
#include "Device.h"

#define local_udp(M, ...)  //custom_log("APP", M, ##__VA_ARGS__)

extern system_context_t* sys_context;

char *Udp_Send2APP_Success = "{\"Status\":\"Device_Discovery\",\"Device_id\":\"%s\"}";

char *Udp_Send2APP_fail = "{\"Status\":\"Device_Failed\",\"Device_id\":\"%s\"}";

void start_local_udp()
{
	int err = -1;

	err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "Local_Udp_Thread", Local_Udp_Thread,0x500, (uint32_t)NULL);
	require_noerr_action( err, exit, local_udp("ERROR: Unable to start the Local_Udp_Thread.") );
	return ;

	exit:
	local_udp("creat_thread failed");
}


void Local_Udp_Thread()
{
    OSStatus err;
    struct sockaddr_in addr;
    fd_set readfds;
    int udp_fd = -1;
    struct timeval timeout;
    socklen_t addrLen = sizeof(addr);
    int len ;
    uint8_t *buf = NULL;
    uint8_t *Udp_Send = NULL;
    int Send_Count = 0;

    if(Udp_Send == NULL)
    	Udp_Send = malloc(512);
    require_action( Udp_Send, exit, err = kNoMemoryErr );

    if(buf == NULL)
    	buf = malloc( 1024 );
    require_action( buf, exit, err = kNoMemoryErr );

    /*Establish a UDP port to receive any data sent to this port*/
    udp_fd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    require_action( IsValidSocket( udp_fd ), exit, err = kNoResourcesErr );

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons( LOCAL_UDP_PORT );

    err = bind( udp_fd, (struct sockaddr *) &addr, sizeof(addr) );
    require_noerr( err, exit );

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if(sys_context->flashContentInRam.Cloud_info.device_id[0] != '\0')
    	sprintf(Udp_Send,Udp_Send2APP_Success,sys_context->flashContentInRam.Cloud_info.device_id);
    else
    	sprintf(Udp_Send,Udp_Send2APP_fail,sys_context->flashContentInRam.Cloud_info.device_id);

    local_udp("Start UDP broadcast mode, data: %s", Udp_Send);

    while ( 1 )
    {
        FD_ZERO( &readfds );
        FD_SET( udp_fd, &readfds );

        require_action( select(udp_fd + 1, &readfds, NULL, NULL, &timeout) >= 0, exit,
                        err = kConnectionErr );

        /*Read data from udp and send data back */
        if ( FD_ISSET( udp_fd, &readfds ) )
        {
            len = recvfrom( udp_fd, buf, 1024, 0, (struct sockaddr *)&addr, &addrLen );
            require_action( len >= 0, exit, err = kConnectionErr );
            local_udp("recv data");
            if(strcmp(buf,"finished")){
               goto exit;
            }
        }else {
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = INADDR_BROADCAST;
			addr.sin_port = htons( 8000 );
			/*the receiver should bind at port=8000*/
			err = sendto( udp_fd, Udp_Send, strlen( Udp_Send ), 0, (struct sockaddr *) &addr, sizeof(addr) );
			local_udp("err = %d , data = %s",err, Udp_Send);
			Send_Count++;
			if(Send_Count >= MAX_SEND_TIME)
				goto exit;
        }
    }

    exit:
    if ( err != kNoErr )
    	local_udp("UDP thread exit with err: %d", err);
    mico_rtos_delete_thread( NULL );
}


