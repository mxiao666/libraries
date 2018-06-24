/******************************************************************************
 * Copyright (C) 2014-2018 Zhifeng Gong <gozfree@163.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libraries; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "libsys.h"
#define uiBufferLen 1024
int main(int argc, char **argv)
{
    s32 iRet = 0;
    char pBuffer[uiBufferLen] = {0};
    OS_SafeSystem("./test_shell.sh",NULL,5,&iRet);
    printf("\n\texecl ./test_shell.sh script result:%ld \n",iRet);
    OS_GetStrValueByCmd("ls",pBuffer,uiBufferLen);
    printf("\n\texecl ls result:\n%s\n",pBuffer);
    return 0;
}