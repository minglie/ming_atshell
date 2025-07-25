#include "Protothread.h"

int  Protothread::M_nspt = 0;
int  Protothread::M_npt = 0;


uint32_t  Protothread::M_ms_tick = 0;

Protothread * Protothread::M_pts[PT_MAX_THREAD_NUM];

Protothread::Protothread(): _ptLine(0){
    m_delay = 0;
    m_indata=0;
    m_outdata=0;
    m_state=0;
    m_run=0;
    M_pts[Protothread::M_nspt++]= this;
}

void Protothread::PtOsDelay(uint32_t tick) {
    m_delay=tick;
}

void Protothread::PtOsDelayMs(uint32_t ms) {
    m_delay=ms/PT_THREAD_TICK_MS;
}

void Protothread::PtOsDelayResume() {
    m_delay=0;
}

void Protothread::Init() {

}

void Protothread::OnTick(){
    if (m_delay > 0)
        m_delay--;
    if(m_delay==0){
        this->Run();
    }
}

uint32_t Protothread::GetDelay(){
    return m_delay;
}

uint32_t Protothread::GetInData(){
    return m_indata;
}
void Protothread::SetInData(Protothread * pt,uint32_t indata){
    pt->m_indata=indata;
}

uint32_t Protothread::PopInData(){
    uint32_t v = m_indata;
    m_indata = 0;
    return v;
}

bool Protothread::PushIndata(Protothread * pt,uint32_t indata){
    if (pt->m_indata == 0) {
        pt->m_indata = indata;
        return true;
    }
    return false;
}

uint32_t Protothread::GetOutData(Protothread * pt){
    return pt->m_outdata;
}

void Protothread::SetOutData(uint32_t outdata){
    m_outdata=outdata;
}

uint32_t  Protothread::PopOutData(Protothread * pt){
    uint32_t v=pt->m_outdata;
    pt->m_outdata=0;
    return v;
}

bool  Protothread::PushOutData(uint32_t outdata){
    if(m_outdata==0) {
        m_outdata=outdata;
        return true;
    }
    return false;
}

unsigned int Protothread::Start() {
    this->Init();
    m_id=Protothread::M_npt++;
    M_pts[m_id]= this;
    return m_id;
}

bool Protothread::Run() {
    if(m_run!=0){
        m_run(this);
    }
    return true;
}

Protothread* Protothread::Create(PtRunFun run) {
    Protothread* pt= new Protothread();
    pt->m_run=run;
    return pt;
}



void Protothread::AllStart() {
    for (int i=0;i<M_nspt;i++){
        M_pts[i]->Start();
    }
}


void Protothread::OnTickAll() {
    for (int i=0;i<M_npt;i++){
        M_pts[i]->OnTick();
    }
}


int Protothread::PollAndRun(uint32_t tsMs) {
    #if PT_THREAD_TICK_MS==1
        Protothread::OnTickAll();
        return 1;
    #else
        if(tsMs-Protothread::M_ms_tick>=PT_THREAD_TICK_MS){
            Protothread::M_ms_tick=tsMs;
            Protothread::OnTickAll();
            return 1;
        }
        return 0;
    #endif
}

void  Protothread::Notify(Protothread * target,ProtothreadNotifyEvent evt){
    Protothread::OnRecvNotify(this,target,evt);
    if(target== nullptr){
          for(int i=0;i<Protothread::M_npt;i++){
              Protothread::M_pts[i]->OnRecvNotify(target,evt);
          }
          return;
      } else{
          target->OnRecvNotify(target,evt);
      }
}


void  Protothread::OnRecvNotify(Protothread * source,ProtothreadNotifyEvent evt){
    // nothing
}

void  Protothread::OnRecvNotify(Protothread * source, Protothread * target,ProtothreadNotifyEvent evt){
   // nothing
}