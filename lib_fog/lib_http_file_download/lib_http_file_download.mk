#
#  UNPUBLISHED PROPRIETARY SOURCE CODE
#  Copyright (c) 2016 MXCHIP Inc.
#
#  The contents of this file may not be disclosed to third parties, copied or
#  duplicated in any form, in whole or in part, without the prior written
#  permission of MXCHIP Corporation.
#

NAME := Lib_http_file_download

$(NAME)_SOURCES :=	http_file_download.c \
					http_file_download_thread.c
				
GLOBAL_INCLUDES += 	.

$(NAME)_COMPONENTS += utilities/url