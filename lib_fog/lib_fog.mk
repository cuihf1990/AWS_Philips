############################################################################### 
#
#  The MIT License
#  Copyright (c) 2016 MXCHIP Inc.
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy 
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights 
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is furnished
#  to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
#  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
############################################################################### 


NAME := Lib_fog

$(NAME)_SOURCES := 	src/fog_act.c \
					src/fog_cloud.c \
					src/fog_task.c \
					src/fog_notify.c \
					src/fog_wifi.c \
					src/fog_wifi_config.c \
					src/fog_product.c \
					src/fog_ota.c \
					src/fog_time.c \
					src/fog_parser.c \
					src/fog_tools.c

GLOBAL_INCLUDES += src

GLOBAL_INCLUDES += .

$(NAME)_COMPONENTS := 	lib_fog/lib_http_short_connection \
						lib_fog/lib_http_file_download 

						
ifdef LOCAL_CONTRAL_ENABLE
$(NAME)_SOURCES += src/fog_local.c
$(NAME)_COMPONENTS += lib_fog/lib_linkcoap
GLOBAL_DEFINES += LOCAL_CONTROL_MODE_ENABLE
endif

ifeq ($(HOST_ARCH),ARM968E-S)
GLOBAL_DEFINES += MOC108
endif




