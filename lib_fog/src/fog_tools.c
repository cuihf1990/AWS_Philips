/**
 ******************************************************************************
 * @file    fog_tools.c
 * @author  dingqq
 * @version V1.0.0
 * @date    2018年3月16日
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

#include "fog_tools.h"

float constrain( float value, float min, float max )
{
    if ( value >= max )
        return max;
    if ( value <= min )
        return min;
    return value;
}

uint8_t a2x( char ch )
{
    switch ( ch )
    {
        case '1':
            return 1;
        case '2':
            return 2;
        case '3':
            return 3;
        case '4':
            return 4;
        case '5':
            return 5;
        case '6':
            return 6;
        case '7':
            return 7;
        case '8':
            return 8;
        case '9':
            return 9;
        case 'A':
            case 'a':
            return 10;
        case 'B':
            case 'b':
            return 11;
        case 'C':
            case 'c':
            return 12;
        case 'D':
            case 'd':
            return 13;
        case 'E':
            case 'e':
            return 14;
        case 'F':
            case 'f':
            return 15;
        default:
            break;
            ;
    }

    return 0;
}
