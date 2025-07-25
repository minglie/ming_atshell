#include "AtShell.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"
#include <ctype.h>
#include <limits.h>

#define CON_AT_DELIMITER   "(,)"
#define CON_AT_DELIMITER_BANK   " "
#define FINSH_PROMPT        "$:"

static int __help(int argc, char **argv);


AtShell g_atShell;


#if CON_AT_MSH == 1

static int __clean(int argc, char **argv) {
    AT_printf("\033c");
    return 0;
}

#else
static int __at(int argc, char** argv) {
    at_reply(CON_AT_R_SUCCESS);
    return 0;
}
#endif


#if CON_AT_USE_CALLBACK == 1
static int __call(int argc, char **argv) {
    if (argc <= 2) {
        return 0;
    }
    char *hexStr = argv[2];
    uint32_t code = strtol(argv[1], NULL, 16);
    char *bs = g_atShell.GetBuf();
    int len = at_hexStringToByteArray(hexStr, (unsigned char *) bs);
    if (g_atShell.m_atCallBackFun != NULL) {
        g_atShell.m_atCallBackFun(code, (uint8_t *) bs, len);
    }
    return 0;
}

#endif

static AT_CMD_ENTRY_TypeDef s_ap_cmd_entry[CON_AT_METHOD_NUM] = {
        {AT_FUN(help), "list cmd", __help, 0},
#if CON_AT_USE_CALLBACK == 1
        {AT_FUN(c), "(55,01 02)", __call, 0},
#endif
#if CON_AT_MSH == 1
        {"clean", "clean screen", __clean, 0},
#else
        { "AT","",__at, 0 },
#endif
};

static int __help(int argc, char **argv) {
    int cmdInx = g_atShell.GetCmdNum();
    AT_printf("AtShell commands:\r\n");
    for (int i = 0; i < cmdInx; i++) {
        AT_printf("%2d.%-20s - %s \r\n", i, s_ap_cmd_entry[i].methodName, s_ap_cmd_entry[i].helpInfo);
    }
    return 0;
}


AtShell::AtShell() {
    m_cmdList = s_ap_cmd_entry;
    m_cmdSize = CON_AT_METHOD_NUM;
    m_cmdNum = 0;
    m_writeFun = NULL;
    m_bufLen = 0;
    m_importMs = 0;
#if CON_AT_USE_EXPORT == 1
    m_out_bufLen=0;
#endif
    for (size_t i = 0; i < m_cmdSize; i++) {
        if (m_cmdList[i].atFun == NULL) {
            m_cmdNum = i;
            break;
        }
    }
}


AtShell::~AtShell() {

}


bool AtShell::Regist(AT_CMD_ENTRY_TypeDef cmd) {
    if (m_cmdNum >= CON_AT_METHOD_NUM) {
        return false;
    }
    m_cmdList[m_cmdNum] = cmd;
    m_cmdNum++;
    return true;
}

bool AtShell::Regist(AT_CMD_ENTRY_TypeDef *cmdList, int cmdLen) {
    if (m_cmdNum + cmdLen > CON_AT_METHOD_NUM) {
        return false;
    }
    for (int i = 0; i < cmdLen; i++) {
        AtShell::Regist(cmdList[i]);
    }
    return true;
}

void AtShell::Init(ATWriteFun writeFun) {
    m_writeFun = writeFun;
    m_initWriteFun = writeFun;
}


void AtShell::SetWriteFun(ATWriteFun writeFun) {
    m_writeFun = writeFun;
}

void AtShell::ResetWriteFun() {
    m_writeFun = m_initWriteFun;
}

bool AtShell::Parse(char *str) {
    if (m_buf != str) {
        strcpy(m_buf, str);
    }
    m_argv[0] = m_buf;
    char *token;
    char delimiter[16] = CON_AT_DELIMITER;
    if (strstr((char *) str, "AT") != (char *) str) {
        sprintf(delimiter, CON_AT_DELIMITER_BANK);
    }
    token = strtok(m_buf, delimiter);
    int i = 0;
    while (token != NULL) {
        if (i == 0) {
            snprintf(m_method, sizeof(m_method), "%s", token);
            snprintf(m_argv[0], sizeof(m_method), "%s", token);
            m_argv[1] = &m_buf[strlen(m_argv[0]) + 1];
        } else if (i < RT_FINSH_ARG_MAX) {
            sprintf(m_argv[i], "%s", token);
            m_argv[i + 1] = m_argv[i] + strlen(m_argv[i]) + 1;
        }
        token = strtok(NULL, delimiter);
        i++;
        m_argc = i;
    }
    return true;
}


bool AtShell::Exec(char *str) {
    m_lock = 1;
    bool r = Parse(str);
    if (r == false) {
        m_lock = 0;
        return r;
    }
    bool cmd_ret = false;
    //methodName match
    for (int i = 0; i < m_cmdNum; i++) {
        if (strcmp(m_cmdList[i].methodName, m_method) == 0) {
            this->ctx = &m_cmdList[i];
            m_cmdList[i].atFun(m_argc, m_argv);
            cmd_ret = true;
            break;
        }
    }
    //inx match
    if (!cmd_ret) {
        int inx = strtol(m_method, NULL, 10);
        if ('0' <= m_method[0] && m_method[0] <= '9' && inx < m_cmdNum) {
            this->ctx = &m_cmdList[inx];
            m_cmdList[inx].atFun(m_argc, m_argv);
            cmd_ret = true;
        }
    }
    if (!cmd_ret) {
        char *tcmd;
        tcmd = str;
        while (*tcmd != ' ' && *tcmd != '\0') {
            tcmd++;
        }
        *tcmd = '\0';
        this->Printf("%s: command not found.\n", str);
    }
    m_bufLen = 0;
    m_lock = 0;
    return cmd_ret;
}

int AtShell::Import(uint8_t *buf, uint32_t len, uint32_t ms) {
#if CON_AT_MSH == 1
    return this->ImportForMsh(buf, len, ms);
#else
    return  this->ImportForAt(buf, len, ms);
#endif
}



int AtShell::ImportForAt(uint8_t *buf, uint32_t len, uint32_t ms) {
    if (m_lock == 1 || len == 0 || len > sizeof(m_buf)) {
        return 0;
    }
    if (buf == NULL || ms - m_importMs > 10 || m_bufLen + len >= sizeof(m_buf)) {
        m_bufLen = 0;
    }
    if (buf != NULL) {
        memcpy(m_buf + m_bufLen, buf, len);
    }
    m_bufLen = m_bufLen + len;
    m_importMs = ms;
    if (m_bufLen < 3) {
        return 0;
    }
    if (m_buf[m_bufLen - 2] == 0x0d && m_buf[m_bufLen - 1] == 0x0a) {
        if (strstr((char *) m_buf, "AT") == (char *) m_buf) {
            m_buf[m_bufLen - 1] = 0;
            m_buf[m_bufLen - 2] = 0;
        } else {
            m_buf[m_bufLen - 1] = 0;
            m_buf[m_bufLen - 2] = 0;
        }
        this->Exec(m_buf);
        return m_bufLen;
    }
    return 0;
}


int AtShell::Printf(const char *format, ...) {
    char log_buf[256];
    va_list args;
    va_start(args, format);
    vsprintf(log_buf, format, args);
    uint32_t len = strlen(log_buf);
    m_writeFun((uint8_t *) log_buf, len,CON_AT_WRITE_TIMEOUT);
    va_end(args);
    return len;
}


int AtShell::Output(long nLevel, const char *pszFileName, int nLineNo, const char *format, ...) {
    char log_buf[256];
    snprintf(log_buf, sizeof(log_buf), "[%s:%d]", pszFileName, nLineNo);
    uint32_t len = strlen(log_buf);
    m_writeFun((uint8_t *) log_buf, len,CON_AT_WRITE_TIMEOUT);
    va_list args;
    va_start(args, format);
    vsprintf(log_buf, format, args);
    len = strlen(log_buf);
    m_writeFun((uint8_t *) log_buf, len,CON_AT_WRITE_TIMEOUT);
    va_end(args);
    return len;
}

int AtShell::PrintfBs(uint8_t *buf, uint32_t len) {
    char log_buf[256];
    if (len == 0 || len > 80) {
        return 0;
    }
    for (size_t i = 0; i < len; i++) {
        sprintf(&log_buf[3 * i], "%02X ", buf[i]);
    }
    log_buf[3 * len - 1] = '\n';
    this->Write((uint8_t *) log_buf, 3 * len,CON_AT_WRITE_TIMEOUT);
    return 3 * len;
}


int AtShell::Write(uint8_t *buf, uint32_t len,uint32_t timeout) {
    return m_writeFun(buf, len,timeout);
}

int AtShell::Write(uint8_t data) {
    uint8_t temp[1] = {data};
    return m_writeFun(temp, 1,CON_AT_WRITE_TIMEOUT);
}


void AtShell::AtCall(uint32_t code, uint8_t *buf, uint32_t len) {
    char log_buf[256];
    if (len == 0 || len > 80) {
        return;
    }
    for (size_t i = 0; i < len; i++) {
        sprintf(&log_buf[3 * i], "%02X ", buf[i]);
    }
    log_buf[3 * len - 1] = 0;
    this->Printf("AT+c(%d,%s)\r\n", code, log_buf);
}


int AtShell::Reply(uint8_t errCode) {
    if (errCode) {
        this->Printf("NG:%d\r\n", errCode);
        return errCode;
    }
    this->Printf("OK\r\n");
    return errCode;
}


void AtShell::ShowVersion() {
    this->Printf(" \\ | /\r\n");
    this->Printf("- AT_SHELL - %s  msh:%d \r\n", __DATE__, CON_AT_MSH);
    this->Printf(" / | \\\r\n");
#if CON_AT_MSH == 1
    char s[1] = {""};
    ShellAutoComplete(s);
#endif
}


#if CON_AT_MSH == 1
#if 0
/*
void msh_split_test(){
    char* argv[RT_FINSH_ARG_MAX];
    char test_cmd[100] = { "funcname arg0 \"arg1 arg1\"" };
    msh_split(test_cmd, sizeof(test_cmd), argv);
}
*/
static int msh_split(char* cmd, uint32_t length, char* argv[RT_FINSH_ARG_MAX])
{
    char* ptr;
    uint32_t position;
    uint32_t argc;
    uint32_t i;
    /* strim the beginning of command */
    while (*cmd == ' ' || *cmd == '\t')
    {
        cmd++;
        length--;
    }
    if (length == 0)
        return 0;
    ptr = cmd;
    position = 0; argc = 0;
    while (position < length)
    {
        /* strip bank and tab */
        while ((*ptr == ' ' || *ptr == '\t') && position < length)
        {
            *ptr = '\0';
            ptr++; position++;
        }

        if (argc >= RT_FINSH_ARG_MAX)
        {
            rt_kprintf("Too many args ! We only Use:\n");
            for (i = 0; i < argc; i++)
            {
                rt_kprintf("%s ", argv[i]);
            }
            rt_kprintf("\n");
            break;
        }

        if (position >= length) break;

        /* handle string */
        if (*ptr == '"')
        {
            ptr++; position++;
            argv[argc] = ptr; argc++;

            /* skip this string */
            while (*ptr != '"' && position < length)
            {
                if (*ptr == '\\')
                {
                    if (*(ptr + 1) == '"')
                    {
                        ptr++; position++;
                    }
                }
                ptr++; position++;
            }
            if (position >= length) break;

            /* skip '"' */
            *ptr = '\0'; ptr++; position++;
        }
        else
        {
            argv[argc] = ptr;
            argc++;
            while ((*ptr != ' ' && *ptr != '\t') && position < length)
            {
                ptr++; position++;
            }
            if (position >= length) break;
        }
    }
    return argc;
}
#endif


static void *rt_memmove(void *dest, const void *src, uint32_t n) {
    char *tmp = (char *) dest, *s = (char *) src;

    if (s < tmp && tmp < s + n) {
        tmp += n;
        s += n;

        while (n--)
            *(--tmp) = *(--s);
    } else {
        while (n--)
            *tmp++ = *s++;
    }

    return dest;
}

static char *rt_strncpy(char *dst, const char *src, uint32_t n) {
    if (n != 0) {
        char *d = dst;
        const char *s = src;

        do {
            if ((*d++ = *s++) == 0) {
                /* NUL pad the remaining n-1 bytes */
                while (--n != 0)
                    *d++ = 0;
                break;
            }
        } while (--n != 0);
    }

    return (dst);
}

static int str_common(const char *str1, const char *str2) {
    const char *str = str1;
    while ((*str != 0) && (*str2 != 0) && (*str == *str2)) {
        str++;
        str2++;
    }
    return (str - str1);
}


int AtShell::ImportForMsh(uint8_t *buf, uint32_t len, uint32_t ms) {
    for (size_t i = 0; i < len; i++) {
        MshAddChar(buf[i]);
    }
    return len;
}


void AtShell::ShellAutoComplete(char *prefix) {
    this->Printf("\r\n");
    MshAutoComplete(prefix);
    this->Printf("%s%s", FINSH_PROMPT, prefix);
}


void AtShell::MshAutoComplete(char *prefix) {
    int length, min_length;
    const char *name_ptr, *cmd_name;
    int index;
    min_length = 0;
    name_ptr = 0;
    if (*prefix == '\0') {
        return;
    }
    for (index = 0; index < g_atShell.GetCmdNum(); index++) {
        /* skip finsh shell function */
        cmd_name = s_ap_cmd_entry[index].methodName;
        if (strncmp(prefix, cmd_name, strlen(prefix)) == 0) {
            if (min_length == 0) {
                /* set name_ptr */
                name_ptr = cmd_name;
                /* set initial length */
                min_length = strlen(name_ptr);
            }
            length = str_common(name_ptr, cmd_name);
            if (length < min_length)
                min_length = length;

            this->Printf("%s\r\n", cmd_name);
        }
    }


    /* auto complete string */
    if (name_ptr != NULL) {
        rt_strncpy(prefix, name_ptr, min_length);
    }
    return;
}

void AtShell::MshAddChar(char ch) {
    enum input_stat {
        WAIT_NORMAL = 0,
        WAIT_SPEC_KEY,
        WAIT_FUNC_KEY,
    };
    /*
     * handle control key
     * up key  : 0x1b 0x5b 0x41
     * down key: 0x1b 0x5b 0x42
     * right key:0x1b 0x5b 0x43
     * left key: 0x1b 0x5b 0x44
     */
    if (ch == 0x1b) {
        m_stat = WAIT_SPEC_KEY;
        return;
    } else if (m_stat == WAIT_SPEC_KEY) {
        if (ch == 0x5b) {
            m_stat = WAIT_FUNC_KEY;
            return;
        }
        m_stat = WAIT_NORMAL;
    } else if (m_stat == WAIT_FUNC_KEY) {
        m_stat = WAIT_NORMAL;
        if (ch == 0x41) /* up key */
        {
            /* prev history */
            if (m_currentHistory > 0)
                m_currentHistory--;
            else {
                m_currentHistory = 0;
                return;
            }

            /* copy the history command */
            memcpy(m_line, &m_cmdHistory[m_currentHistory][0], FINSH_CMD_SIZE);
            m_lineCurpos = m_linePosition = strlen(m_line);
            ShellHandleHistory();
            return;
        } else if (ch == 0x42) {
            /* next history */
            if (m_currentHistory < m_historyCount - 1)
                m_currentHistory++;
            else {
                /* set to the end of history */
                if (m_historyCount != 0)
                    m_currentHistory = m_historyCount - 1;
                else
                    return;
            }

            memcpy(m_line, &m_cmdHistory[m_currentHistory][0],
                   FINSH_CMD_SIZE);
            m_lineCurpos = m_linePosition = strlen(m_line);
            ShellHandleHistory();
            return;
        } else if (ch == 0x44) /* left key */
        {
            if (m_lineCurpos) {
                this->Printf("\b");
                m_lineCurpos--;
            }

            return;
        } else if (ch == 0x43) /* right key */
        {
            if (m_lineCurpos < m_linePosition) {
                this->Printf("%c", m_line[m_lineCurpos]);
                m_lineCurpos++;
            }

            return;
        }
    }
    /* received null or error */
    if (ch == '\0' || ch == 0xFF) return;
        /* handle tab key */
    else if (ch == '\t') {
        int i;
        /* move the cursor to the beginning of line */
        for (i = 0; i < m_lineCurpos; i++)
            this->Printf("\b");

        /* auto complete */
        ShellAutoComplete(&m_line[0]);
        /* re-calculate position */
        m_lineCurpos = m_linePosition = strlen(m_line);
        return;
    }
        /* handle backspace key */
    else if (ch == 0x7f || ch == 0x08) {
        /* note that line_curpos >= 0 */
        if (m_lineCurpos == 0)
            return;

        m_linePosition--;
        m_lineCurpos--;
        if (m_linePosition > m_lineCurpos) {
            int i;
            rt_memmove(&m_line[m_lineCurpos],
                       &m_line[m_lineCurpos + 1],
                       m_linePosition - m_lineCurpos);
            m_line[m_linePosition] = 0;
            this->Printf("\b%s  \b", &m_line[m_lineCurpos]);

            /* move the cursor to the origin position */
            for (i = m_lineCurpos; i <= m_linePosition; i++)
                this->Printf("\b");
        } else {
            this->Printf("\b \b");
            m_line[m_linePosition] = 0;
        }

        return;
    }

    /* handle end of line, break */
    if (ch == '\r' || ch == '\n') {
        ShellPushHistory();

        this->Printf("\r\n");
        // printf("%s", line);
        m_line[m_linePosition] = '\0';
        g_atShell.Exec(m_line);
        this->Printf("\r\n%s", FINSH_PROMPT);
        memset(m_line, 0, sizeof(m_line));
        m_lineCurpos = m_linePosition = 0;
        return;
    }

    /* it's a large line, discard it */
    if (m_linePosition >= FINSH_CMD_SIZE)
        m_linePosition = 0;

    /* normal character */
    if (m_lineCurpos < m_linePosition) {
        int i;
        rt_memmove(&m_line[m_lineCurpos + 1], &m_line[m_lineCurpos], m_linePosition - m_lineCurpos);
        m_line[m_lineCurpos] = ch;
        this->Printf("%s", &m_line[m_lineCurpos]);
        /* move the cursor to new position */
        for (i = m_lineCurpos; i < m_linePosition; i++)
            this->Printf("\b");
    } else {
        m_line[m_linePosition] = ch;
        this->Printf("%c", ch);
    }

    ch = 0;
    m_linePosition++;
    m_lineCurpos++;
    if (m_linePosition >= FINSH_CMD_SIZE) {
        /* clear command line */
        m_linePosition = 0;
        m_lineCurpos = 0;
    }
}


void AtShell::ShellPushHistory() {
    if (m_linePosition != 0) {
        /* push history */
        if (m_historyCount >= FINSH_HISTORY_LINES) {
            /* if current cmd is same as last cmd, don't push */
            if (memcmp(&m_cmdHistory[FINSH_HISTORY_LINES - 1], m_line, FINSH_CMD_SIZE)) {
                /* move history */
                int index;
                for (index = 0; index < FINSH_HISTORY_LINES - 1; index++) {
                    memcpy(&m_cmdHistory[index][0],
                           &m_cmdHistory[index + 1][0], FINSH_CMD_SIZE);
                }
                memset(&m_cmdHistory[index][0], 0, FINSH_CMD_SIZE);
                memcpy(&m_cmdHistory[index][0], m_line, m_linePosition);
                m_historyCount = FINSH_HISTORY_LINES;
            }
        } else {
            /* if current cmd is same as last cmd, don't push */
            if (m_historyCount == 0 || memcmp(&m_cmdHistory[m_historyCount - 1], m_line, FINSH_CMD_SIZE)) {
                m_currentHistory = m_historyCount;
                memset(&m_cmdHistory[m_historyCount][0], 0, FINSH_CMD_SIZE);
                memcpy(&m_cmdHistory[m_historyCount][0], m_line, m_linePosition);
                m_historyCount++;
            }
        }
    }
    m_currentHistory = m_historyCount;
}


bool AtShell::ShellHandleHistory() {
#if defined(_WIN32)
    int i;
    this->Printf("\r");

    for (i = 0; i <= 60; i++)
      this->Write(' ');
      this->Printf("\r");

#else
    rt_kprintf("\033[2K\r");
#endif
    this->Printf("%s%s", FINSH_PROMPT, m_line);
    return 0;
}

#endif


#if CON_AT_USE_EXPORT == 1
void AtShell::Export(){
    if(m_out_bufLen>0){
        m_writeFun((uint8_t *) m_out_buf, m_out_bufLen,CON_AT_WRITE_TIMEOUT);
    }
    m_out_bufLen=0;
}

#endif

#if CON_AT_USE_EXPORT == 1
int AtShell::AsyncPrintf(const char *format, ...){
    char log_buf[256];
    va_list args;
    va_start(args, format);
    vsprintf(log_buf, format, args);
    uint32_t len = strlen(log_buf);
    if(m_out_bufLen+len < CON_AT_OUT_BUF_SIZE){
        memcpy(m_out_buf + m_out_bufLen, log_buf, len);
        m_out_bufLen=m_out_bufLen+len;
    } else{
        AT_error("error \r\n");
    }
    va_end(args);
    return len;
}
#else
int AtShell::AsyncPrintf(const char *format, ...){
    char log_buf[256];
    va_list args;
    va_start(args, format);
    vsprintf(log_buf, format, args);
    uint32_t len = strlen(log_buf);
    m_writeFun((uint8_t *) log_buf, len,0);
    va_end(args);
    return len;
}
#endif

#if CON_AT_USE_CycleMonitorData==1
static uint8_t _sum(uint8_t * buffer, int len){
    uint8_t sum = 0;
    for (int i = 0; i < len; i++) {
        sum += buffer[i];
    }
    return(sum);
}
int AtShell::MonitorDataPush(const char *tag, uint8_t * buffer, int len){
    if(m_monitorData.curInx==CON_CYCLE_MONITOR_DATA_PACK_NUM){
        m_monitorData.curInx=0;
    }
    sprintf(m_monitorData.tag[m_monitorData.curInx], "%s", tag);
    memcpy(m_monitorData.buffer[m_monitorData.curInx],buffer,len);
    m_monitorData.bufferLen[m_monitorData.curInx]=len;
    m_monitorData.curInx++;
    return 0;
}

int AtShell::MonitorDataViewInfo(){
    int inx=m_monitorData.curInx-1;
    for(int i=0;i<CON_CYCLE_MONITOR_DATA_PACK_NUM;i++){
        if(inx==-1){
            inx=9;
        }
        this->Printf("%s:%3d.%3d@",m_monitorData.tag[inx],m_monitorData.bufferLen[inx],_sum(m_monitorData.buffer[inx],m_monitorData.bufferLen[inx]));
        AT_printfBs( m_monitorData.buffer[inx],m_monitorData.bufferLen[inx]);
        this->Printf("\r\n");
        inx--;
    }
    return 0;
}
#endif



void at_init(ATWriteFun writeFun) {
    g_atShell.Init(writeFun);
}


int at_import(uint8_t *buf, uint32_t len, uint32_t ms) {
    return g_atShell.Import(buf, len, ms);
}
bool at_try_import(uint8_t *buf, uint32_t len, uint32_t ms) {
    if (CON_AT_MSH || strstr((char*)buf, "AT") == (char*)buf || strstr((char*)buf, "\r\n") == (char*)buf) {
        g_atShell.Import(buf, len, ms);
        return true;
    }
    return false;
}

#if CON_AT_USE_EXPORT == 1
void at_export() {
    g_atShell.Export();
}
#endif

bool at_register(AT_CMD_ENTRY_TypeDef cmd) {
    return g_atShell.Regist(cmd);
}

bool at_register_many(AT_CMD_ENTRY_TypeDef *cmdList, int cmdLen) {
    return g_atShell.Regist(cmdList, cmdLen);
}



int at_write(uint8_t *buf, uint32_t len,uint32_t timeout){
    return  g_atShell.Write(buf,len,timeout);
}

int at_printf(const char *format, ...) {
    char log_buf[256];
    va_list args;
    va_start(args, format);
    vsprintf(log_buf, format, args);
    uint32_t len = strlen(log_buf);
    g_atShell.Write((uint8_t *) log_buf, len,CON_AT_WRITE_TIMEOUT);
    va_end(args);
    return len;
}



int at_awrite(uint8_t *buf, uint32_t len){
#if CON_AT_USE_EXPORT == 1
    if(g_atShell.m_out_bufLen+len < CON_AT_OUT_BUF_SIZE){
            memcpy(g_atShell.m_out_buf + g_atShell.m_out_bufLen, buf, len);
            g_atShell.m_out_bufLen=g_atShell.m_out_bufLen+len;
            return len;
        }
        return 0;
#else
    return g_atShell.Write(buf,len,0);;
#endif
}


int at_aprintf(const char *format, ...) {
    char log_buf[256];
    va_list args;
    va_start(args, format);
    vsprintf(log_buf, format, args);
    uint32_t len = strlen(log_buf);
    at_awrite((uint8_t *) log_buf, len);
    va_end(args);
    return len;
}





int at_reply(uint8_t errCode) {
    return g_atShell.Reply(errCode);
}


void at_show_version() {
    g_atShell.ShowVersion();
}


int at_hexStringToByteArray(const char *hexStr, unsigned char *bs) {
    char token[3];
    int i = 0;
    long int byteValue;
    const int byteArrayLen = (strlen(hexStr) + 1) / 3;
    while (*hexStr && i < byteArrayLen) {
        if (isspace(*hexStr)) {
            hexStr++;
            continue;
        }

        if (!isxdigit(*hexStr)) {
            return i;
        }
        token[0] = *hexStr++;
        token[1] = *hexStr++;
        token[2] = '\0';
        byteValue = strtol(token, NULL, 16);
        bs[i] = (unsigned char) byteValue;
        i++;
    }
    return i;
}

long at_str_to_int(char* str) {
    char* endptr;
    int base = 10;
    if (str == 0) {
        return 0;
    }
    if (str[0] == '0') {
        if (str[1] == 'x' || str[1] == 'X') {
            base = 16;
            str += 2;
        }
        else {
            base = 8;
        }
    }
    long result = strtol(str, &endptr, base);
    if (*endptr != '\0') {

        return 0;
    }
    return result;
}

#if CON_AT_USE_CycleMonitorData == 1

void  at_monitor_init(){
    g_atShell.m_monitorData.curInx=0;
}
void  at_monitor_push(const char *tag, uint8_t * buffer, int len){
    g_atShell.MonitorDataPush(tag, buffer, len);
}
void  at_monitor_viewInfo(){
    g_atShell.MonitorDataViewInfo();
}

#endif