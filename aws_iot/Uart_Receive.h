/*
 * Uart_Rcveice.h
 *
 *  Created on: 2018Äê3ÔÂ19ÈÕ
 *      Author: Cuihf
 */

#ifndef AWS_IOT_UART_RECEIVE_H_
#define AWS_IOT_UART_RECEIVE_H_



#define UART_BUFFER_LENGTH 1024*2

#define MAX_QUEUE_LENGTH 1024
#define MAX_QUEUE_NUM 8

#define Pre_Filter_timer_TIMMEOUT 60*60*24
#define Filter1_Warning_TIMMEOUT  60*60*24
#define Filter2_Warning_TIMMEOUT  60*60*24

void Uart_Init();
void  uartRecv_thread();
int uart_data_process(uint8_t * input_buffer , int data_len);
int get_uart_data(uint8_t *input_buffer, int recv_length);
void Process_notify(uint8_t Error_Data);

#endif /* AWS_IOT_UART_RECEIVE_H_ */
