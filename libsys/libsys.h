/***********************************************************************************
 * 文 件 名   : os_system.h
 * 负 责 人   : 卢美宏 765442484@qqq.com
 * 创建日期   : 2018年4月1日
 * 文件描述   :  系统性的功能函数封装头文件
 * 版权说明   : Copyright (c) 2008-2018   xx xx xx xx 技术有限公司
 * 其    他   :
 * 修改日志   :
***********************************************************************************/

#ifndef LIBSYS_H
#define LIBSYS_H

#ifdef __cplusplus
extern "C" {
#endif

#define s32 long int
#define u32 unsigned int
#define s64 long long int
#define u64 unsigned long long int

#define RET_OK 0
#define RET_ERR -1
#define RET_VOID

#define RET_EXCEPTIONAL 128//异常退出
#define RET_TIMEOUT     129//超时退出
#define RET_INDIDE_ERR  130//内部错误
#define RET_STAT_D      131//进程D状态

#define STAT_PATH_LEN 64
#define STAT_BUFFER_LEN 288
#define TV_USEC_VALUE 200
#define INVALUE_INAVLE  0xffffffffffffffffUL
#define MAX_MODE_ALL_NUMBER  12
typedef enum
{
    LOG_LIMIT = -1,
    LOG_ERROR = 0,
    LOG_WARN  = 1,
    LOG_NOTE  = 2,
    LOG_INFO  = 3,
    LOG_DEBUG = 4,
    LOG_BUTT  = 255
} LOGLEVEL;
//日志函数
int Debug_Log(int iModePid, unsigned char loglevel, const char *pFunction, int uLine, const char *pFileName, char *fromat, ...);

/*****************************************************************************
 * 函 数 名  : OS_LOG
 * 负 责 人  : 卢美宏 765442484@qqq.com
 * 创建日期  : 2018年6月16日
 * 函数功能  : 输出日志
 * 输入参数  : loglevel  日志等级
               fmt       需要输出的文本格式
               args...   可变参
 * 输出参数  : 无
 * 返 回 值  : 
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
#define OS_LOG(loglevel,fmt, args...)\
            Debug_Log(MAX_MODE_ALL_NUMBER,loglevel,__func__,__LINE__,__FILE__,fmt,##args)

/*通用断言定义宏*/
#define OS_ASSERT(expr,ret) \
do{\
    if(!(expr)) \
    {\
        OS_LOG(LOG_ERROR,"Assertion fallure("#expr") is false,return("#ret").");\
        return ret;\
    }\
  }while(0)
/*通用断言定义宏*/
#define OS_ASSERT_NO_LOG(expr,ret) \
do{\
    if(!(expr)) \
    {\
        return ret;\
    }\
  }while(0)

/*空指针断言定义宏*/
#define OS_ASSERT_PTR(expr,ret)     OS_ASSERT(expr,ret)
#define OS_ASSERT_PTR_RET_VOID(expr)  OS_ASSERT(expr,RET_VOID)
#define OS_ASSERT_PTR_RET_NO_LOG(expr,ret)  OS_ASSERT_NO_LOG(expr,ret)
#define OS_ASSERT_PTR_RET_VOID_AND_NO_LOG(expr)  OS_ASSERT_NO_LOG(expr,RET_VOID)

/*内存释放定义宏*/
#define OS_FREE_PERT(expr)\
do{\
    if(NULL != (expr))\
        free((expr));\
    (expr) = NULL;\
}while(0)

#define STDOUT_FILENOED 2
#define PATH_NAME_MAX_LEN 256
#define OM_CMD_EXCE_TIME 5

s32 OS_SafeSystem(char *pcCmd, char *argv[], u32 uiTimeOut, s32 *piScriptRet);
s32 OS_SafeSystemSub(char *pcCmd, char *argv[], u32 uiTimeOut, s32 *piScriptRet, pid_t *ptChildPid);
s32 OS_WaitChild(pid_t uiChildPid, int *piFd, u32 uiTimeout, s32 *iScriptRet, char *pOutBuf, u64 uiOutBufLen);
s32 OS_CheckReadBuf(s32 v_uiFd, char *pOutBuf, u32 uiOutBufLen);
s32 OS_Kill(pid_t uiChildPid);
s32 OS_GetProcessStatus(pid_t uiPid, char *v_Status);
s32 OS_GetExitStatus(s32 iStatus, s32 *v_ScriptRet);
u64 OS_GetSystemLlitm();
s32 OS_ReadBufByCmd(const char * pacCmd, u32 uiTimeout, char *pBuffer, u64 uiBufferLen);
s32 OS_GetStrValueByCmd(const char * pacCmd, char *pBuffer, u64 uiBufferLen);

#ifdef __cplusplus
}
#endif
#endif
