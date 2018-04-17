/*
 * Uart_Rcveive.c
 *
 *  Created on: 2018年3月19日
 *      Author: Cuihf
 */
#include "MICO.h"
#include "platform.h"
#include "Uart_Receive.h"
#include "Device.h"

#define uart_log(M, ...) custom_log("APP", M, ##__VA_ARGS__)
#define uart_log_trace() custom_log_trace("APP")

volatile ring_buffer_t  rx_buffer;
volatile uint8_t    rx_data[UART_BUFFER_LENGTH];

extern uart_cmd_t uartcmd;
extern uint8_t Aws_Mqtt_satus;
mico_queue_t Uart_push_queue = NULL;

mico_timer_t Pre_Filter_timer;
mico_timer_t Filter1_Warning;
mico_timer_t Filter2_Warning;

uint8_t Old_Pre_Filter,Old_Filter1_Warning,Old_Filter2_Warning = 0;

void _Pre_Filter_timer (void* arg)
{
  uartcmd.Notify = Old_Pre_Filter;
  uart_log("post notify  Old_Pre_Filter");
  uartcmd.Notify = 0;
}

void _Filter1_Warning (void* arg)
{
  uartcmd.Notify = Old_Filter1_Warning;
  uart_log("post notify Old_Filter1_Warning");
  uartcmd.Notify = 0;
}

void _Filter2_Warning (void* arg)
{
  uartcmd.Notify = Old_Filter2_Warning;
  uart_log("post notify Old_Filter2_Warning");
  uartcmd.Notify = 0;
}

void Process_notify(uint8_t Error_Data)
{
  if((Error_Data&0x01)!= 0 && Old_Pre_Filter!=(Error_Data&0x01) && uartcmd.Notify == 0){
    uartcmd.Notify = 1;
    Old_Pre_Filter = (Error_Data&0x01);
    uart_log("post notify  Old_Pre_Filter");
    mico_reload_timer (&Pre_Filter_timer);
  }
  if((Error_Data&0x04) != 0&& Old_Filter1_Warning!=(Error_Data&0x04) && uartcmd.Notify == 0 ){
    uartcmd.Notify = 2;
    Old_Filter1_Warning = (Error_Data&0x04) ;
    uart_log("post notify Old_Filter1_Warning");
    mico_reload_timer (&Filter1_Warning);
  }
  if((Error_Data&0x10) != 0 && Old_Filter2_Warning!=(Error_Data&0x10) && uartcmd.Notify == 0){
    uartcmd.Notify = 3;
    Old_Filter2_Warning = (Error_Data&0x10) ;
    uart_log("post notify Old_Filter2_Warning");
    mico_reload_timer (&Filter2_Warning);
  }
}

void Uart_Init()
{
   OSStatus err = kNoErr;
//////////////////uart config
   mico_uart_config_t uart_config;
   uart_config.baud_rate = 115200;
   uart_config.data_width = DATA_WIDTH_8BIT;
   uart_config.parity = NO_PARITY;
   uart_config.stop_bits = STOP_BITS_1;
   uart_config.flow_control = FLOW_CONTROL_DISABLED;
   uart_config.flags = UART_WAKEUP_DISABLE;
   ring_buffer_init( (ring_buffer_t *) &rx_buffer, (uint8_t *) rx_data, UART_BUFFER_LENGTH );
   MicoUartInitialize( UART_FOR_APP, &uart_config, (ring_buffer_t *) &rx_buffer );

   err = mico_rtos_init_queue( &Uart_push_queue, "uart_push_queue", MAX_QUEUE_LENGTH , MAX_QUEUE_NUM);  //只容纳一个成员 传递的只是地址
   require_noerr( err, exit );

   err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "UART Recv", uartRecv_thread,0x800, (uint32_t)NULL);
   require_noerr_action( err, exit, uart_log("ERROR: Unable to start the uart recv thread.") );

   return ;

   exit:
   uart_log("creat_thread failed");
}

void  uartRecv_thread()
{
    int recv_len = -1;
    uint8_t buffer_data[1024] = {0};

    mico_init_timer (&Pre_Filter_timer,Pre_Filter_timer_TIMMEOUT*1000, _Pre_Filter_timer, NULL);
    mico_start_timer (&Pre_Filter_timer);
    mico_init_timer (&Filter1_Warning,Filter1_Warning_TIMMEOUT*1000, _Filter1_Warning, NULL);
    mico_start_timer (&Filter1_Warning);
    mico_init_timer (&Filter2_Warning,Filter2_Warning_TIMMEOUT*1000, _Filter2_Warning, NULL);
    mico_start_timer (&Filter2_Warning);

    while(1){
    	recv_len = get_uart_data(buffer_data,1024);
    	if(recv_len <= 0)
    		continue;
    	if(Aws_Mqtt_satus > 0 )
    		uart_data_process(buffer_data , recv_len);
    }
}

int get_uart_data(uint8_t *input_buffer, int recv_length)
{
  OSStatus err = kNoErr;
  int datalen = -1;
  uint8_t * p ;

  while(1){
	    p = input_buffer;
	    err = MicoUartRecv(UART_FOR_APP, p, 1, MICO_WAIT_FOREVER);
	    require_noerr(err, exit);
	    require(*p == 0xFe, exit);
	    p++;
	    err = MicoUartRecv(UART_FOR_APP, p, 1, 500);
	    require_noerr(err, exit);
	    require(*p == 0xFF, exit);
	    p++;
	    err = MicoUartRecv(UART_FOR_APP, p, 3, 500);
	    require_noerr(err, exit);
	    datalen = p[1];
	    datalen = datalen<<8;
	    datalen += p[2];
	    p+=3;
	    err = MicoUartRecv(UART_FOR_APP, p, datalen+2, 1000);
	    require_noerr(err, exit);

	    return datalen+7;
  }

  exit:
  uart_log(" %02x",*p);
  return -1;
}


