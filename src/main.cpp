#include <Arduino.h>
#include "AtShell.h"
#include "Protothread.h"
//Atshell 是一个精简好用的嵌入端Shell,  支持tab命令补全, 历史命令, 根据命令 或 命令序号执行对应函数
//只有AtShell.h, AtShell.cpp 两个文件


//Atshell AT模式与 MSH 模式
//MSH模式   fun a b
//AT模式    AT+fun(a,b)



//ATShell 唯一依赖的函数
static int user_at_write(uint8_t* srcBuf, uint32_t toSendLen,uint32_t timeout) {
    return Serial.write(srcBuf, toSendLen);
}

//测试 函数
static int test_01(int argc, char** argv) {
    AT_println("hello test_01");
    return 0;
}


void setup() {
    Serial.begin(115200);
    //初始化AtShell
    at_init(user_at_write);
    //命令导出,控制台输入test01 则执行 test_01函数
    AT_SHELL_EXPORT(test01, 测试 ,test_01);
    //执行AtShell内置的help函数
    AT_EXEC("help");
    //启动PT协程
    Protothread::AllStart();
}
void loop() {
    static uint32_t s_ms_tick=0;
    static uint32_t s_ms_s=0;
    long ms= millis();
    if(ms-s_ms_tick>=PT_THREAD_TICK_MS){
        s_ms_tick=ms;
        s_ms_s=1;
    }
    if(s_ms_s){
        int len=  Serial.available();
        if(len>0) {
            Serial.readBytes((uint8_t *)AT_m_buf, len);
            if (len > 0) {
                //导入AtShell
                at_import((uint8_t *)AT_m_buf, len, millis());
            }
        }
        Protothread::OnTickAll();
        s_ms_s=0;
    }
}