/**
******************************************************************************
* @file    fog_local.h
* @author  dingqq
* @version V1.0.0
* @date    2018年3月20日
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
#ifndef FOG_LOCAL_H_
#define FOG_LOCAL_H_


#ifdef __cplusplus
extern "C"
{
#endif


#ifdef __cplusplus
}
#endif

#define MULTICAST_ADDR      "224.0.1.187"
#define COAP_PORT           5683

#define DEV_INFO_URI        "/sys/dev/info"
#define DEV_STATUS_URL      "/sys/dev/status"
#define DEV_CONTROL         "/sys/dev/control"

#define COAP_RESP_OK        "{\"status\":\"success\"}"

OSStatus coap_server_start( void );

void fog_locol_report( char *data, uint32_t len );

#endif /* LIB_FOG_SRC_FOG_LOCAL_H_ */
