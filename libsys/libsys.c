/***********************************************************************************
 * 文 件 名   : os_system.c
 * 负 责 人   : 卢美宏 765442484@qqq.com
 * 创建日期   : 2018年4月1日
 * 文件描述   : 系统性的功能函数封装
 * 版权说明   : Copyright (c) 2008-2018   xx xx xx xx 技术有限公司
 * 其    他   :
 * 修改日志   :
***********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "libsys.h"


#include "string.h"
#include<sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <fcntl.h>
#include "stdarg.h"
#include "unistd.h"


/*****************************************************************************
 * 函 数 名  : OS_GetSystemLlitm
 * 负 责 人  : 卢美宏 765442484@qqq.com
 * 创建日期  : 2018年4月1日
 * 函数功能  : 获取系统谁时间，毫秒级
 * 输入参数  : 无
 * 输出参数  : 无
 * 返 回 值  : 秒级时间整数
 * 调用关系  :
 * 其    它  :

*****************************************************************************/
u64 OS_GetSystemLlitm()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}


/*****************************************************************************
 * 函 数 名  : OS_SafeSystem
 * 负 责 人  : 卢美宏 765442484@qqq.com
 * 创建日期  : 2018年4月1日
 * 函数功能  : 带超时机制的执行脚本命令
 * 输入参数  : char *pcCmd       待执行命令
               char *argv[]      执行命令时需要的参数
               u32 uiTimeOut     指定命令最长执行时间
               s32 *piScriptRet  命令执行退出结果
 * 输出参数  : 无
 * 返 回 值  :
 * 调用关系  :
 * 其    它  :

*****************************************************************************/
s32 OS_SafeSystem(char *pcCmd, char *argv[], u32 uiTimeOut, s32 *piScriptRet)
{
    pid_t tChildPid = 0;
    return OS_SafeSystemSub(pcCmd, argv, uiTimeOut, piScriptRet, &tChildPid);

}

/*****************************************************************************
 * 函 数 名  : OS_SafeSystemSub
 * 负 责 人  : 卢美宏 765442484@qqq.com
 * 创建日期  : 2018年6月16日
 * 函数功能  : 带超时机制的执行脚本命令，同时可以获知子进程的pid
 * 输入参数  : char *pcCmd        待执行命令
               char *argv[]       命令执行参数
               u32 uiTimeOut      命令执行超时时间
               s32 *piScriptRet   命令自行退出状态
               pid_t *ptChildPid  子线程id
 * 输出参数  : 无
 * 返 回 值  : 执行结果
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
s32 OS_SafeSystemSub(char *pcCmd, char *argv[], u32 uiTimeOut, s32 *piScriptRet, pid_t *ptChildPid)
{

    s32 iRet = RET_ERR;
    pid_t tChildPid = 0;
    struct rlimit stFileLimit;
    u32 uiFileIndex = 0;
    if(NULL == pcCmd || NULL == piScriptRet || NULL == ptChildPid)
    {
        return RET_ERR;
    }

    if(0 > (tChildPid = vfork()))
    {
        OS_LOG(LOG_ERROR, "Creat Child fail.");
        return RET_ERR;
    }
    if(tChildPid == 0)
    {
        //恢复三种信号源
        (void)signal(SIGQUIT, SIG_DFL);
        (void)signal(SIGCHLD, SIG_DFL);
        (void)signal(SIGINT, SIG_DFL);
        setpgid(0, 0); //设置组便于一次kill
        //获取进程能打开最大文件数并关闭
        if(0 == getrlimit(RLIMIT_NOFILE, &stFileLimit))
        {
            for(uiFileIndex = STDOUT_FILENOED + 1; uiFileIndex < stFileLimit.rlim_max; ++uiFileIndex)
            {
                close((int32_t)uiFileIndex);
            }
            if(NULL == argv)
            {
                (void)execl("/bin/sh", "sh", "-c", pcCmd, (char*)0);
            }
            else
            {
                (void)execv(pcCmd, argv);
            }

        }
        _exit(127);
    }
    else
    {
        iRet = OS_WaitChild(tChildPid, NULL, uiTimeOut, piScriptRet, NULL, 0);
        if(RET_OK != iRet)
        {
            OS_LOG(LOG_ERROR, "Execl shell cmd(%s) fail,iRet(%ld) iScriptRet(%ld).", pcCmd, iRet, *piScriptRet);
            return RET_ERR;
        }
        else
        {
            return RET_OK;
        }
    }
}

/*****************************************************************************
 * 函 数 名  : OS_WaitChild
 * 负 责 人  : 卢美宏 765442484@qqq.com
 * 创建日期  : 2018年4月1日
 * 函数功能  : 等待子进程执行退出，带超时机制，同时可以获知脚本执行输出结果
 * 输入参数  : pid_t uiChildPid  子线程id
               int *piFd         文件描述符
               u32 uiTimeout     等待子线程最大执行时间
               s32 *iScriptRet   子线程退出状态
               char *pOutBuf     子线程输出缓存
               u64 uiOutBufLen   缓存大小
 * 输出参数  : 无
 * 返 回 值  : 执行结果
 * 调用关系  :
 * 其    它  :

*****************************************************************************/
s32 OS_WaitChild(pid_t uiChildPid, int *piFd, u32 uiTimeout, s32 *iScriptRet, char *pOutBuf, u64 uiOutBufLen)
{
    s32 iRet  = RET_ERR;
    s32 iStat = 0;
    u64 uiBeginTime = 0;
    u64 uiEndTime   = 0;
    u64 uiTime      = 0;
    u32 iOffset     = 0;
    pid_t iStopPid  = 0;
    uiTime = (uiTimeout == 0) ? INVALUE_INAVLE : uiTimeout * 1000;
    if(NULL == piFd || NULL == iScriptRet || NULL == pOutBuf)
    {
        return RET_ERR;
    }
    uiBeginTime = OS_GetSystemLlitm();
    for(;;)
    {

        if(NULL != piFd && (iOffset < uiOutBufLen))
        {
            iOffset += OS_CheckReadBuf(*piFd, pOutBuf + iOffset, uiOutBufLen - iOffset);
        }

        iStopPid = wait4(uiChildPid, (int *)&iStat, WNOHANG, 0);
        uiEndTime = OS_GetSystemLlitm();

        //wait4异常 或者等待超时
        if((0 != iStopPid) || ((uiEndTime - uiBeginTime) > uiTime))
        {
            break;
        }
        usleep(200);
    }

    if(0 > iStopPid)
    {
        iRet = OS_Kill(uiChildPid);
        OS_LOG(LOG_WARN, "wait child exit fail,iRet(%ld) .", iRet);
        return RET_INDIDE_ERR;
    }
    else if(0 == iStopPid)
    {
        iRet = OS_Kill(uiChildPid);
        OS_LOG(LOG_WARN, "Wait Child Timeout(%llu) but used(%llu),iRet(%ld).", uiTime, uiEndTime - uiBeginTime, iRet);
        return RET_TIMEOUT;
    }
    else if(iStopPid == uiChildPid)
    {
        return OS_GetExitStatus(iStat, iScriptRet);
    }
    else
    {
        return RET_ERR;
    }

}

/*****************************************************************************
 * 函 数 名  : OS_CheckReadBuf
 * 负 责 人  : 卢美宏 765442484@qqq.com
 * 创建日期  : 2018年4月1日
 * 函数功能  : 通过句柄变化获知是否可以读数据
 * 输入参数  : s32 v_uiFd       文件描述符
               char *pOutBuf    输出缓存地址
               u32 uiOutBufLen  输出缓存的大小
 * 输出参数  : 无
 * 返 回 值  : 执行结果
 * 调用关系  :
 * 其    它  :

*****************************************************************************/
s32 OS_CheckReadBuf(s32 v_uiFd, char *pOutBuf, u32 uiOutBufLen)
{
    s32 iRet = RET_ERR;
    u32 iReadLen = 0;
    struct timeval tv;
    fd_set fdset;
    tv.tv_usec = TV_USEC_VALUE;
    tv.tv_sec = TV_USEC_VALUE / 1000;
    if(NULL == pOutBuf)
    {
        return 0;
    }
    FD_ZERO(&fdset);
    FD_SET(v_uiFd, &fdset);

    iRet = select(v_uiFd + 1, &fdset, NULL, NULL, &tv);
    //select error
    if(0 == iRet)
    {
        //OS_LOG(LOG_DEBUG,"Select function execl fail.");
        return 0;
    }
    //timeout
    if(0 > iRet)
    {
        //OS_LOG(LOG_DEBUG,"Select function execl timeout.");
        return 0;
    }
    //read is true
    if(!FD_ISSET(v_uiFd, &fdset))
    {
        //OS_LOG(LOG_DEBUG,"Select function execl really read.");
        return 0;
    }

    iReadLen = read(v_uiFd, pOutBuf, uiOutBufLen);
    return (iReadLen ? iReadLen : 0);

}

/*****************************************************************************
 * 函 数 名  : OS_Kill
 * 负 责 人  : 卢美宏 765442484@qqq.com
 * 创建日期  : 2018年6月16日
 * 函数功能  : kill指定的进程
 * 输入参数  : pid_t uiChildPid  需要杀死的进程id
 * 输出参数  : 无
 * 返 回 值  : 执行结果
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
s32 OS_Kill(pid_t uiChildPid)
{
    s32 iRet = RET_ERR;
    pid_t tKillPid = 0;
    char sChildStat = 0;

    (void)killpg(uiChildPid, SIGTERM);
    usleep(200);
    (void)killpg(uiChildPid, SIGKILL);

    tKillPid = wait4(uiChildPid, NULL, WNOHANG, 0);
    if(tKillPid == uiChildPid)
    {
        return RET_OK;
    }

    iRet = OS_GetProcessStatus(uiChildPid, &sChildStat);
    if(RET_OK != iRet)
    {
        tKillPid = wait4(uiChildPid, NULL, WNOHANG, 0);
        if(tKillPid == uiChildPid)
        {
            return RET_OK;
        }
        else
        {
            OS_LOG(LOG_WARN, "kill childPid(%d) fail.", uiChildPid);
            return RET_INDIDE_ERR;
        }
    }
    else if('D' != sChildStat)
    {
        return RET_STAT_D;
    }
    else
    {
        (void)wait4(uiChildPid, NULL, 0, 0);
        return RET_OK;
    }
}

/*****************************************************************************
 * 函 数 名  : OS_GetProcessStatus
 * 负 责 人  : 卢美宏 765442484@qqq.com
 * 创建日期  : 2018年4月1日
 * 函数功能  : 获取子进程退出的状态D
 * 输入参数  : pid_t uiPid     子线程pid
               char *v_Status  输出子线程执行状态
 * 输出参数  : 无
 * 返 回 值  : 执行结果
 * 调用关系  :
 * 其    它  :

*****************************************************************************/
s32 OS_GetProcessStatus(pid_t uiPid, char *v_Status)
{
    s32 iRet = RET_ERR;
    s32 iFd  = RET_ERR;
    char acStatPath[STAT_PATH_LEN] = {0};
    char acStatBuffer[STAT_BUFFER_LEN] = {0};
    char *pcEedBracket = NULL;
    if(NULL == v_Status)
    {
        return RET_ERR;
    }
    iRet = snprintf(acStatPath, STAT_PATH_LEN -1, "/proc/%d/stat", uiPid);
    if(RET_ERR == iRet)
        return RET_INDIDE_ERR;

    iFd = open(acStatPath, O_RDONLY, NULL);

    if(0 >= iFd)
        return RET_INDIDE_ERR;

    iRet = read(iFd, acStatBuffer, STAT_BUFFER_LEN - 1);
    close(iFd);

    if(RET_OK != iRet)
        return RET_INDIDE_ERR;

    pcEedBracket = strstr(acStatBuffer, ")");
    if(NULL == pcEedBracket)
        return RET_INDIDE_ERR;

    *v_Status = *(pcEedBracket + 2);
    return RET_OK;
}

/*****************************************************************************
 * 函 数 名  : OS_GetExitStatus
 * 负 责 人  : 卢美宏 765442484@qqq.com
 * 创建日期  : 2018年4月1日
 * 函数功能  :  获取子进程退出的的状态码
 * 输入参数  : s32 iStatus       线程退出状态
               s32 *v_ScriptRet  输出状态
 * 输出参数  : 无
 * 返 回 值  : 执行结果
 * 调用关系  :
 * 其    它  :

*****************************************************************************/
s32 OS_GetExitStatus(s32 iStatus, s32 *v_ScriptRet)
{
    if(NULL == v_ScriptRet)
    {
        return RET_ERR;
    }
    //正常退出
    if(WIFEXITED(iStatus))
    {
        if(NULL != v_ScriptRet)
        {
            *v_ScriptRet = WEXITSTATUS(iStatus);
        }
        return RET_OK;
    }

    if(WIFSIGNALED(iStatus))
    {
        OS_LOG(LOG_WARN, "Get pid exit status fail.");
    }
    return RET_EXCEPTIONAL;

}
/*****************************************************************************
 * 函 数 名  : OS_GetStrValueByCmd
 * 负 责 人  : 卢美宏 765442484@qqq.com
 * 创建日期  : 2018年4月1日
 * 函数功能  : 获取脚本执行后输出结果
 * 输入参数  : const char * pacCmd  待执行命令
               char *pBuffer        命令执行输出结果缓存地址
               u64 uiBufferLen      缓存地址大小限制
 * 输出参数  : 无
 * 返 回 值  :
 * 调用关系  :
 * 其    它  :

*****************************************************************************/
s32 OS_GetStrValueByCmd(const char * pacCmd, char *pBuffer, u64 uiBufferLen)
{
    s32 iRet = RET_ERR;
    u64 uiLen = 0;
    if(NULL == pacCmd || NULL == pBuffer || (0 >= uiBufferLen))
    {
        return RET_ERR;
    }
    iRet = OS_ReadBufByCmd(pacCmd, OM_CMD_EXCE_TIME, pBuffer, uiBufferLen);
    if(RET_OK != iRet)
    {
        OS_LOG(LOG_WARN, "Get cmd execl result fail.");
        return RET_ERR;
    }
    uiLen = strlen(pBuffer);
    if(0 == uiLen)
    {
        OS_LOG(LOG_WARN, "Get cmd execl result is null.");
        return RET_ERR;
    }
    else
    {
        pBuffer[uiLen - 1] = '\0';
        return RET_OK;
    }
}
/*****************************************************************************
 * 函 数 名  : OS_ReadBufByCmd
 * 负 责 人  : 卢美宏 765442484@qqq.com
 * 创建日期  : 2018年4月1日
 * 函数功能  :  读取buffer中数据
 * 输入参数  : const char * pacCmd  待执行命令
               u32 uiTimeout        命令执行超时限制
               char *pBuffer        命令输出结果缓存地址
               u64 uiBufferLen      缓存地址大小限制
 * 输出参数  : 无
 * 返 回 值  : 执行结果
 * 调用关系  :
 * 其    它  :

*****************************************************************************/
s32 OS_ReadBufByCmd(const char * pacCmd, u32 uiTimeout, char *pBuffer, u64 uiBufferLen)
{

    s32 iRet = RET_ERR;
    s32 iScriptRet  =  RET_ERR;
    pid_t iChildPid = 0;
    u32 uiFileIndex = 0;
    int pdes[2] = {0};
    struct rlimit stFileLimit;

    if(0 > pipe(pdes))
    {
        OS_LOG(LOG_ERROR, "Creat pipe fail.");
        return RET_ERR;
    }

    if(0 > (iChildPid = vfork()))
    {
        close(pdes[0]);
        close(pdes[1]);
        OS_LOG(LOG_ERROR, "Creat Child fail.");
        return RET_ERR;
    }

    if(iChildPid == 0)
    {
        //恢复三种信号源
        (void)signal(SIGQUIT, SIG_DFL);
        (void)signal(SIGCHLD, SIG_DFL);
        (void)signal(SIGINT, SIG_DFL);
        setpgid(0, 0); //设置组便于一次kill
        //获取进程能打开最大文件数并关闭
        if(0 == getrlimit(RLIMIT_NOFILE, &stFileLimit))
        {
            for(uiFileIndex = STDOUT_FILENOED + 1; uiFileIndex < stFileLimit.rlim_max; ++uiFileIndex)
            {
                //关闭子进程的非写端
                if(uiFileIndex != pdes[1])
                    close((int32_t)uiFileIndex);
            }
            dup2(pdes[1], STDOUT_FILENO);
            close(pdes[1]);
            (void)execl("/bin/sh", "sh", "-c", pacCmd, (char*)0);
        }
        _exit(127);
    }
    else
    {
        close(pdes[1]);
        iRet = OS_WaitChild(iChildPid, &pdes[0], uiTimeout, &iScriptRet, pBuffer, uiBufferLen);
        close(pdes[0]);
        if(RET_OK != iRet || RET_OK != iScriptRet)
        {
            OS_LOG(LOG_ERROR, "Execl shell cmd(%s) fail,iRet(%ld) iScriptRet(%ld).", pacCmd, iRet, iScriptRet);
            return RET_ERR;
        }
        return RET_OK;
    }
}



/*****************************************************************************
 * 函 数 名  : Debug_Log
 * 负 责 人  : 卢美宏 765442484@qqq.com
 * 创建日期  : 2018年4月1日
 * 函数功能  : 对外写日志接口
 * 输入参数  : int iModePid            模块的id
               unsigned char loglevel  日志等级
               const char *pFunction   调用日志接口函数
               int uLine               调用行
               const char *pFileName   调用函数所在文件
               char *fromat            输出格式
               ...                     不定参数
 * 输出参数  : 无
 * 返 回 值  :
 * 调用关系  :
 * 其    它  :

*****************************************************************************/
int Debug_Log(int iModePid, unsigned char loglevel, const char *pFunction, int uLine, const char *pFileName, char *fromat, ...)
{
    //请在此实现你的日志函数
    return RET_OK;
}


