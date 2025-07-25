/*
 CON_AT_MSH=0: AT模式: 配合 Xmodem1K 更新固件
=>: AT+fun(a,b,c)\r\n
CON_AT_MSH=1: MSH模式: 调试
=>: fun a b \n
**/

#ifndef _AT_SHELL_H
#define _AT_SHELL_H

#include "stdint.h"
#include "string.h"
#include "stdbool.h"

//模式  0:AT    1:MSH
#define  CON_AT_MSH 1
//机器Hex通讯 AT+c(55,01 02)
#define  CON_AT_USE_CALLBACK 1
//数据监控
#define  CON_AT_USE_CycleMonitorData 1
//监控总数量
#define CON_CYCLE_MONITOR_DATA_PACK_NUM  10

//方法数
#define  CON_AT_METHOD_NUM     10//  10
#define  FINSH_CMD_SIZE       20  //20             //最长命令尺寸
#define  RT_FINSH_ARG_MAX      3// 6        //参数个数
#define  FINSH_HISTORY_LINES    3       //历史命令条数
#define  CON_AT_R_SUCCESS   0    // 成功
#define  CON_AT_R_ERR_ARG   1    // 参数错误
#define  CON_AT_R_ERR_NO_CMD   2  //无此命令
#define  CON_AT_R_ERR_EXEC_FAIL 3  //执行失败
#define  CON_AT_WRITE_TIMEOUT   100
//一种异步发送的实现
#define  CON_AT_USE_EXPORT    0
#define  CON_AT_OUT_BUF_SIZE     200
#define CON_METHOD_NAME_SIZE 8
#define CON_HELP_INFO_SIZE  20


typedef int (*ATServerFun)(int argc, char **argv);
typedef int (*ATWriteFun)(uint8_t *buf, uint32_t len,uint32_t timeout);

#if CON_AT_USE_CALLBACK == 1
typedef void (*ATCallBackFun)(uint32_t code, uint8_t *buf, uint32_t len);
#endif

typedef struct {
    char methodName[CON_METHOD_NAME_SIZE];
    char helpInfo[CON_HELP_INFO_SIZE];
    ATServerFun atFun;
    void *userData;
} AT_CMD_ENTRY_TypeDef;


typedef struct {
    char     tag[CON_CYCLE_MONITOR_DATA_PACK_NUM][10];
    uint8_t  buffer[CON_CYCLE_MONITOR_DATA_PACK_NUM][100];
    uint8_t  bufferLen[CON_CYCLE_MONITOR_DATA_PACK_NUM];
    uint8_t  curInx;
} AT_CycleMonitorDataTypeDef;
#ifdef __cplusplus


class AtShell {
private:
    AT_CMD_ENTRY_TypeDef *m_cmdList;
    uint32_t m_cmdSize;
    int m_cmdNum;
    char m_buf[FINSH_CMD_SIZE];
    uint32_t m_bufLen;
    uint32_t m_importMs;
    uint32_t m_lock;
    int m_argc;
    char m_method[CON_METHOD_NAME_SIZE];
    char *m_argv[RT_FINSH_ARG_MAX];
    ATWriteFun m_initWriteFun;
    ATWriteFun m_writeFun;
    virtual bool Parse(char *str);

public:
    AtShell();
    virtual ~AtShell();
    AT_CMD_ENTRY_TypeDef *ctx;
#if CON_AT_USE_CALLBACK == 1
    ATCallBackFun m_atCallBackFun;
#endif
#if CON_AT_USE_EXPORT == 1
    char m_out_buf[CON_AT_OUT_BUF_SIZE];
    uint32_t m_out_bufLen;
    virtual void Export();
#endif

    virtual void Init(ATWriteFun writeFun);
    virtual void SetWriteFun(ATWriteFun writeFun);
    virtual void ResetWriteFun();
    virtual char *GetBuf() { return m_buf; };
    virtual int GetCmdNum() { return m_cmdNum; };
    virtual bool Regist(AT_CMD_ENTRY_TypeDef cmd);
    virtual bool Regist(AT_CMD_ENTRY_TypeDef *cmdList, int cmdLen);
    virtual bool Exec(char *str);
    virtual int Import(uint8_t *buf, uint32_t len, uint32_t ms = 0);
    virtual int ImportForAt(uint8_t *buf, uint32_t len, uint32_t ms = 0);
    virtual int  AsyncPrintf(const char *format, ...);
    virtual int  Printf(const char *format, ...);
    virtual int Output(long nLevel, const char *pszFileName, int nLineNo, const char *pszFmt, ...);
    virtual int PrintfBs(uint8_t *buf, uint32_t len);
    virtual int Write(uint8_t *buf, uint32_t len,uint32_t timeout);
    virtual int Write(uint8_t data);
    virtual void AtCall(uint32_t code, uint8_t *buf, uint32_t len);
    virtual int Reply(uint8_t errCode);
    virtual void ShowVersion();
#if CON_AT_USE_CycleMonitorData==1
    AT_CycleMonitorDataTypeDef m_monitorData;
    virtual int MonitorDataPush(const char *tag, uint8_t * buffer, int len);
    virtual int MonitorDataViewInfo();
#endif

#if CON_AT_MSH == 1
    uint16_t m_currentHistory;
    uint16_t m_historyCount;
    char m_cmdHistory[FINSH_HISTORY_LINES][FINSH_CMD_SIZE];
    int m_stat;
    char m_line[FINSH_CMD_SIZE];
    uint8_t m_linePosition;
    uint8_t m_lineCurpos;

    virtual int ImportForMsh(uint8_t *buf, uint32_t len, uint32_t ms = 0);
    virtual void MshAddChar(char ch);
    virtual void ShellPushHistory();
    virtual void ShellAutoComplete(char *prefix);
    virtual void MshAutoComplete(char *prefix);
    virtual bool ShellHandleHistory();
#endif
};

extern AtShell g_atShell;

enum AT_LOG_LEVEL {
    AT_LOG_LEVEL_TRACE,
    AT_LOG_LEVEL_DEBUG,
    AT_LOG_LEVEL_INFO,
    AT_LOG_LEVEL_WARNING,
    AT_LOG_LEVEL_ERROR
};

extern AtShell g_atShell;

#define AT_m_buf  g_atShell.GetBuf()

#if CON_AT_MSH == 1
#define AT_FUN(fun)    #fun
#else
#define AT_FUN(fun)    "AT+"#fun
#endif
#define CONCAT(a, b) a ## b
#define AT_FILE_NAME(x)   strrchr(x,'\\')?strrchr(x,'\\')+1:x
#define AT_info(...)      g_atShell.Output(AT_LOG_LEVEL_INFO,__func__, __LINE__,__VA_ARGS__)
#define AT_debug(...)     g_atShell.Output(AT_LOG_LEVEL_DEBUG,AT_FILE_NAME(__FILE__), __LINE__,__VA_ARGS__)
#define AT_debug1(...)    g_atShell.Output(AT_LOG_LEVEL_DEBUG,__func__,__LINE__,__VA_ARGS__)
#define AT_error(...)     g_atShell.Output(AT_LOG_LEVEL_ERROR,__PRETTY_FUNCTION__,__LINE__,__VA_ARGS__)
#define AT_printf(format, ...)  g_atShell.Printf(format,##__VA_ARGS__)
#define AT_println(format, ...)  g_atShell.Printf(format,##__VA_ARGS__);AT_printf("\r\n")
#define AT_aprintf(format, ...)    g_atShell.AsyncPrintf(format,##__VA_ARGS__)
#define AT_aprintln(format, ...)  g_atShell.AsyncPrintf(format,##__VA_ARGS__);AT_aprintf("\r\n")

#define AT_printfBs(buf, len)  g_atShell.PrintfBs(buf,len)
#define rt_kprintf                  AT_printf
#define ATX_info(w, ...)             g_atShell.SetWriteFun(w);AT_info(__VA_ARGS__);g_atShell.ResetWriteFun();
#define ATX_debug(w, ...)            g_atShell.SetWriteFun(w);AT_debug(__VA_ARGS__);g_atShell.ResetWriteFun();
#define ATX_error(w, ...)            g_atShell.SetWriteFun(w);AT_error(__VA_ARGS__);g_atShell.ResetWriteFun();
#define ATX_printf(w, ...)           g_atShell.SetWriteFun(w);AT_printf(__VA_ARGS__);g_atShell.ResetWriteFun();
#define ATX_printfBs(w, buf, len)     g_atShell.SetWriteFun(w);g_atShell.PrintfBs(buf,len);g_atShell.ResetWriteFun();
#endif

#if CON_AT_USE_CALLBACK == 1
#define AT_SET_CALL_BACK(fun)          g_atShell.m_atCallBackFun=fun
#endif


#define AT_SHELL_EXPORT(cmdName, desc, fun, ...)  AT_CMD_ENTRY_TypeDef fun##entrycmd={AT_FUN(cmdName),#desc,fun,__VA_ARGS__}; at_register(fun##entrycmd)
#define AT_EXEC(cmd)    g_atShell.Exec(cmd)


#ifdef __cplusplus
extern "C" {
#endif
void at_init(ATWriteFun writeFun);
int  at_import(uint8_t *buf, uint32_t len, uint32_t ms);
#if CON_AT_USE_EXPORT == 1
void  at_export();
#endif
#if CON_AT_USE_CycleMonitorData == 1
void  at_monitor_init();
void  at_monitor_push(const char *tag, uint8_t * buffer, int len);
void  at_monitor_viewInfo();
#endif
bool at_try_import(uint8_t *buf, uint32_t len, uint32_t ms);
bool at_register(AT_CMD_ENTRY_TypeDef cmd);
bool at_register_many(AT_CMD_ENTRY_TypeDef *cmdList, int cmdLen);
int  at_write(uint8_t *buf, uint32_t len,uint32_t timeout);
int  at_awrite(uint8_t *buf, uint32_t len);
int  at_printf(const char *format, ...);
int  at_aprintf(const char *format, ...);
int  at_reply(uint8_t errCode);
int  at_hexStringToByteArray(const char *hexStr, unsigned char *bs);
long at_str_to_int(char* str);
void at_show_version();
#ifdef __cplusplus
}
#endif

#endif





/*
*
#include <iostream>
#include "AtShell.h"
#include "stdio.h"
int shell_write(uint8_t* buf, uint32_t len,uint32_t timeout) {
	return  printf("%s", buf);
}
static int test01(int argc, char** argv) {
	AT_printf("argc %d:\r\n", argc);
	return 0;
}
int main() {
	at_init(shell_write);
	AT_SHELL_EXPORT(test01, "", test01);
	while (1) {
		char a = getchar();
		uint8_t bs[1] = { a };
		at_import(bs, 1, 0);
	}
	return 0;
}
**/