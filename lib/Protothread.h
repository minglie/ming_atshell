#ifndef __PROTOTHREAD_H__
#define __PROTOTHREAD_H__
#include "stdint.h"
#define  PT_MAX_THREAD_NUM      5
#define  PT_THREAD_TICK_MS      10
/**
class LEDFlasher : public Protothread
{
    void Init(){
        AT_println("dd");
    }
    bool Run() {
        WHILE(1){
            AT_println("dd");
            PT_DELAY_MS(1000);
        }
        PT_END();
    }

    bool _Run() {
        AT_println("d33d");
        PtOsDelayMs(1000);
        return true;
    }
};

 void let_test(Protothread * pt) {
    LED_TOGGLE();
    PT_OS_DELAY_MS(10);
}

Protothread::Create(let_test);
LEDFlasher ledFlasher;
ledFlasher.Start();
**/


typedef struct {
    uint32_t	 code;
    uint32_t	 ms;
    void *       args;
} ProtothreadNotifyEvent;


class Protothread;
typedef  void (*PtRunFun)(Protothread * pt);
class Protothread
{
public:
    static Protothread* M_pts[PT_MAX_THREAD_NUM];
    //for start
    static int M_nspt;
    static int M_npt;
    //for cycle
    static uint32_t M_ms_tick;
    Protothread();
    virtual ~Protothread() { }
    virtual const char* Name(void) 	{ return("");}
    void Restart() { _ptLine = 0; }
    void Stop() { _ptLine = LineNumberInvalid; }
    bool IsRunning() { return _ptLine != LineNumberInvalid; }
    void PtOsDelay(uint32_t tick);
    void PtOsDelayMs(uint32_t ms);
    void PtOsDelayResume();
    virtual void OnTick();
    virtual void Init();
    virtual bool Run();
    virtual unsigned int Start();
    virtual void   Notify(Protothread * target,ProtothreadNotifyEvent evt);
    virtual void   OnRecvNotify(Protothread * source,ProtothreadNotifyEvent evt);
    static  void   OnRecvNotify(Protothread * source, Protothread * target,ProtothreadNotifyEvent evt);
    static void  AllStart();
    static void  OnTickAll();
    static int   PollAndRun(uint32_t tsMs);
    static Protothread* Create(PtRunFun run);
protected:
    PtRunFun m_run;
    unsigned int  m_id;
    uint32_t m_delay;
    typedef unsigned short LineNumber;
    static const LineNumber LineNumberInvalid = (LineNumber)(-1);
    LineNumber _ptLine;
public:
    uint32_t  m_state;
    uint32_t  m_indata;
    uint32_t  m_outdata;
    uint32_t GetDelay();
    uint32_t GetInData();
    void SetOutData(uint32_t outdata);
    uint32_t PopInData();
    bool  PushOutData(uint32_t outdata);
    static void SetInData(Protothread * pt,uint32_t indata);
    static bool PushIndata(Protothread * pt,uint32_t indata);
    static uint32_t GetOutData(Protothread * pt);
    static uint32_t  PopOutData(Protothread * pt);
};

#define PT_BEGIN() bool ptYielded = true; (void) ptYielded; switch (_ptLine) { case 0:
#define PT_END() default: ; } Stop(); return false;
#define PT_WAIT_UNTIL(condition) \
    do { _ptLine = __LINE__; case __LINE__: \
    if (!(condition)) return true; } while (0)
#define PT_WAIT_WHILE(condition) PT_WAIT_UNTIL(!(condition))
#define PT_WAIT_THREAD(child) PT_WAIT_WHILE((child).Run())
#define PT_SPAWN(child) \
    do { (child).Restart(); PT_WAIT_THREAD(child); } while (0)

#define PT_RESTART() do { Restart(); return true; } while (0)

#define PT_EXIT() do { Stop(); return false; } while (0)

#define PT_YIELD() \
    do { ptYielded = false; _ptLine = __LINE__; case __LINE__: \
    if (!ptYielded) return true; } while (0)

#define PT_YIELD_UNTIL(condition) \
    do { ptYielded = false; _ptLine = __LINE__; case __LINE__: \
    if (!ptYielded || !(condition)) return true; } while (0)

#define PT_DELAY(v)				\
  do {						\
    m_delay = v;			\
    PT_WAIT_UNTIL(m_delay == 0);		\
  } while(0)

#define PT_DELAY_MS(v)				\
  do {						\
    m_delay = v/PT_THREAD_TICK_MS;			\
    PT_WAIT_UNTIL(m_delay == 0);		\
  } while(0)

#define WHILE(a)   PT_BEGIN(); \
  while(1)

#define PT_OS_DELAY(tick)        pt->PtOsDelay(tick)
#define PT_OS_DELAY_MS(ms)       pt->PtOsDelayMs(ms)
//free time slice
#define PT_FREE_TIME_SLICE(tsMs)   PT_THREAD_TICK_MS-(tsMs-Protothread::M_ms_tick)
#endif