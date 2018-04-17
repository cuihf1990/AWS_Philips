/**
 ******************************************************************************
 * @file    fog_notify.c
 * @author  dingqq
 * @version V1.0.0
 * @date    2018年3月24日
 * @brief   This file provides xxx functions.
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2018 MXCHIP Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is furnished
 *  to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 */
#include "mico.h"
#include "fog.h"
#include "fog_notify.h"

#define notify_log(M, ...) custom_log("notify", M, ##__VA_ARGS__)

static bool fog_notifing = false;

static void udp_notify_thread( uint32_t arg )
{
    int i, ret = 0;
    int fd;
    fd_set writefds;
    struct timeval t;
    struct sockaddr_in s_addr;
    char buf[256] = { 0 };

    sprintf( buf, FOG_NOTIFY, fog_context_get( )->config.activate_confiog.device_id );

    fd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if ( fd < 0 )
    {
        notify_log( "CREATE UDP SOCKET ERROR!!!\n" );
        goto exit;
    }

    notify_log( "notify ip:%s", fog_context_get()->status.activate_status.aws_notify_ip );

    memset( &s_addr, 0, sizeof(struct sockaddr_in) );
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr( fog_context_get( )->status.activate_status.aws_notify_ip );
    s_addr.sin_port = htons( FOG_NOTIFY_PORT );

    for ( i = 0; i < 10; i++ )
    {
        FD_ZERO( &writefds );
        FD_SET( fd, &writefds );
        t.tv_sec = 0;
        t.tv_usec = 1000;
        select( fd + 1, NULL, &writefds, NULL, &t );
        if ( FD_ISSET( fd, &writefds ) )
        {
            ret = sendto( fd, buf, strlen( buf ), 0, (struct sockaddr *) &s_addr, sizeof(s_addr) );
            if ( ret > 0 )
            {
                i++;
            }
        }
        mico_rtos_thread_msleep( 20 );
    }

    exit:
    close( fd );
    fog_notifing = false;
    mico_rtos_delete_thread( NULL );
}

OSStatus fog_activation_notify( void )
{
    OSStatus err = kNoErr;

    if ( fog_notifing == true )
    {
        return err = kGeneralErr;
    }

    if ( strlen( fog_context_get( )->status.activate_status.aws_notify_ip ) < 4 )
    {
        return kNoErr;
    }
    fog_notifing = true;
    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "notify", udp_notify_thread, 0x500, 0 );
    require_noerr_string( err, exit, "ERROR: Unable to start the notify thread." );
    exit:
    return err;
}
