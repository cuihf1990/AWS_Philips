/*
 * Devcie.h
 *
 *  Created on: 2018Äê3ÔÂ20ÈÕ
 *      Author: Cuihf
 */

#ifndef AWS_IOT_MAIN_H_
#define AWS_IOT_MAIN_H_

#define MAC_TOPIC_LENGTH 100

typedef struct {
 int  type;
 char * name;
}data_process_t;

enum{
  Switch = 1,
  WindSpeed,
  Countdown,
  childLock,
  UILight,
  AQILight,
  WorkMode
};

data_process_t data[8] = {
    {Switch,"Switch"},
    {WindSpeed,"WindSpeed"},
    {Countdown,"Countdown"},
    {childLock,"childLock"},
    {UILight,"UILight"},
    {AQILight,"AQILight"},
    {WorkMode,"WorkMode"},
    {0,NULL}
};


char * Shadow_Update = "{ \
    \"state\" : {\
        \"reported\" : { %s \
         } \
}";

char * Shadow_Delete = "";

char * Shadow_Get = "";


#endif /* AWS_IOT_MAIN_H_ */
