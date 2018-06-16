/***********************************************************************************
 * �� �� ��   : os_system.c
 * �� �� ��   : ¬���� 765442484@qqq.com
 * ��������   : 2018��4��1��
 * �ļ�����   : ϵͳ�ԵĹ��ܺ�����װ
 * ��Ȩ˵��   : Copyright (c) 2008-2018   xx xx xx xx �������޹�˾
 * ��    ��   :
 * �޸���־   :
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
 * �� �� ��  : OS_GetSystemLlitm
 * �� �� ��  : ¬���� 765442484@qqq.com
 * ��������  : 2018��4��1��
 * ��������  : ��ȡϵͳ˭ʱ�䣬���뼶
 * �������  : ��
 * �������  : ��
 * �� �� ֵ  : �뼶ʱ������
 * ���ù�ϵ  :
 * ��    ��  :

*****************************************************************************/
u64 OS_GetSystemLlitm()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}


/*****************************************************************************
 * �� �� ��  : OS_SafeSystem
 * �� �� ��  : ¬���� 765442484@qqq.com
 * ��������  : 2018��4��1��
 * ��������  : ����ʱ���Ƶ�ִ�нű�����
 * �������  : char *pcCmd       ��ִ������
               char *argv[]      ִ������ʱ��Ҫ�Ĳ���
               u32 uiTimeOut     ָ�������ִ��ʱ��
               s32 *piScriptRet  ����ִ���˳����
 * �������  : ��
 * �� �� ֵ  :
 * ���ù�ϵ  :
 * ��    ��  :

*****************************************************************************/
s32 OS_SafeSystem(char *pcCmd, char *argv[], u32 uiTimeOut, s32 *piScriptRet)
{
    pid_t tChildPid = 0;
    return OS_SafeSystemSub(pcCmd, argv, uiTimeOut, piScriptRet, &tChildPid);

}

/*****************************************************************************
 * �� �� ��  : OS_SafeSystemSub
 * �� �� ��  : ¬���� 765442484@qqq.com
 * ��������  : 2018��6��16��
 * ��������  : ����ʱ���Ƶ�ִ�нű����ͬʱ���Ի�֪�ӽ��̵�pid
 * �������  : char *pcCmd        ��ִ������
               char *argv[]       ����ִ�в���
               u32 uiTimeOut      ����ִ�г�ʱʱ��
               s32 *piScriptRet   ���������˳�״̬
               pid_t *ptChildPid  ���߳�id
 * �������  : ��
 * �� �� ֵ  : ִ�н��
 * ���ù�ϵ  : 
 * ��    ��  : 

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
        //�ָ������ź�Դ
        (void)signal(SIGQUIT, SIG_DFL);
        (void)signal(SIGCHLD, SIG_DFL);
        (void)signal(SIGINT, SIG_DFL);
        setpgid(0, 0); //���������һ��kill
        //��ȡ�����ܴ�����ļ������ر�
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
 * �� �� ��  : OS_WaitChild
 * �� �� ��  : ¬���� 765442484@qqq.com
 * ��������  : 2018��4��1��
 * ��������  : �ȴ��ӽ���ִ���˳�������ʱ���ƣ�ͬʱ���Ի�֪�ű�ִ��������
 * �������  : pid_t uiChildPid  ���߳�id
               int *piFd         �ļ�������
               u32 uiTimeout     �ȴ����߳����ִ��ʱ��
               s32 *iScriptRet   ���߳��˳�״̬
               char *pOutBuf     ���߳��������
               u64 uiOutBufLen   �����С
 * �������  : ��
 * �� �� ֵ  : ִ�н��
 * ���ù�ϵ  :
 * ��    ��  :

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

        //wait4�쳣 ���ߵȴ���ʱ
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
 * �� �� ��  : OS_CheckReadBuf
 * �� �� ��  : ¬���� 765442484@qqq.com
 * ��������  : 2018��4��1��
 * ��������  : ͨ������仯��֪�Ƿ���Զ�����
 * �������  : s32 v_uiFd       �ļ�������
               char *pOutBuf    ��������ַ
               u32 uiOutBufLen  �������Ĵ�С
 * �������  : ��
 * �� �� ֵ  : ִ�н��
 * ���ù�ϵ  :
 * ��    ��  :

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
 * �� �� ��  : OS_Kill
 * �� �� ��  : ¬���� 765442484@qqq.com
 * ��������  : 2018��6��16��
 * ��������  : killָ���Ľ���
 * �������  : pid_t uiChildPid  ��Ҫɱ���Ľ���id
 * �������  : ��
 * �� �� ֵ  : ִ�н��
 * ���ù�ϵ  : 
 * ��    ��  : 

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
 * �� �� ��  : OS_GetProcessStatus
 * �� �� ��  : ¬���� 765442484@qqq.com
 * ��������  : 2018��4��1��
 * ��������  : ��ȡ�ӽ����˳���״̬D
 * �������  : pid_t uiPid     ���߳�pid
               char *v_Status  ������߳�ִ��״̬
 * �������  : ��
 * �� �� ֵ  : ִ�н��
 * ���ù�ϵ  :
 * ��    ��  :

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
 * �� �� ��  : OS_GetExitStatus
 * �� �� ��  : ¬���� 765442484@qqq.com
 * ��������  : 2018��4��1��
 * ��������  :  ��ȡ�ӽ����˳��ĵ�״̬��
 * �������  : s32 iStatus       �߳��˳�״̬
               s32 *v_ScriptRet  ���״̬
 * �������  : ��
 * �� �� ֵ  : ִ�н��
 * ���ù�ϵ  :
 * ��    ��  :

*****************************************************************************/
s32 OS_GetExitStatus(s32 iStatus, s32 *v_ScriptRet)
{
    if(NULL == v_ScriptRet)
    {
        return RET_ERR;
    }
    //�����˳�
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
 * �� �� ��  : OS_GetStrValueByCmd
 * �� �� ��  : ¬���� 765442484@qqq.com
 * ��������  : 2018��4��1��
 * ��������  : ��ȡ�ű�ִ�к�������
 * �������  : const char * pacCmd  ��ִ������
               char *pBuffer        ����ִ�������������ַ
               u64 uiBufferLen      �����ַ��С����
 * �������  : ��
 * �� �� ֵ  :
 * ���ù�ϵ  :
 * ��    ��  :

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
 * �� �� ��  : OS_ReadBufByCmd
 * �� �� ��  : ¬���� 765442484@qqq.com
 * ��������  : 2018��4��1��
 * ��������  :  ��ȡbuffer������
 * �������  : const char * pacCmd  ��ִ������
               u32 uiTimeout        ����ִ�г�ʱ����
               char *pBuffer        ���������������ַ
               u64 uiBufferLen      �����ַ��С����
 * �������  : ��
 * �� �� ֵ  : ִ�н��
 * ���ù�ϵ  :
 * ��    ��  :

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
        //�ָ������ź�Դ
        (void)signal(SIGQUIT, SIG_DFL);
        (void)signal(SIGCHLD, SIG_DFL);
        (void)signal(SIGINT, SIG_DFL);
        setpgid(0, 0); //���������һ��kill
        //��ȡ�����ܴ�����ļ������ر�
        if(0 == getrlimit(RLIMIT_NOFILE, &stFileLimit))
        {
            for(uiFileIndex = STDOUT_FILENOED + 1; uiFileIndex < stFileLimit.rlim_max; ++uiFileIndex)
            {
                //�ر��ӽ��̵ķ�д��
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
 * �� �� ��  : Debug_Log
 * �� �� ��  : ¬���� 765442484@qqq.com
 * ��������  : 2018��4��1��
 * ��������  : ����д��־�ӿ�
 * �������  : int iModePid            ģ���id
               unsigned char loglevel  ��־�ȼ�
               const char *pFunction   ������־�ӿں���
               int uLine               ������
               const char *pFileName   ���ú��������ļ�
               char *fromat            �����ʽ
               ...                     ��������
 * �������  : ��
 * �� �� ֵ  :
 * ���ù�ϵ  :
 * ��    ��  :

*****************************************************************************/
int Debug_Log(int iModePid, unsigned char loglevel, const char *pFunction, int uLine, const char *pFileName, char *fromat, ...)
{
    //���ڴ�ʵ�������־����
    return RET_OK;
}


