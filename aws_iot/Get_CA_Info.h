/*
 * Get_CA_Info.h
 *
 *  Created on: 2018��3��28��
 *      Author: Cuihf
 */

#ifndef AWS_IOT_GET_CA_INFO_H_
#define AWS_IOT_GET_CA_INFO_H_

#define CA_PORT  443


#ifndef FRANKFURT
#define CA_HOST "s3.cn-north-1.amazonaws.com.cn"
#else
#define CA_HOST "fogphilips.s3.amazonaws.com"
#endif

void GET_CA_Thread();
void Get_CA_INFO();

#endif /* AWS_IOT_GET_CA_INFO_H_ */
