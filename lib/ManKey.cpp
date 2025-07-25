#include "ManKey.h"
#include "stddef.h"


ManKeyPinRead*  ManKey::pinRead = NULL;
ManKeyOutEvent* ManKey::onEvent = NULL;
int  ManKey::M_nDevices = 0;
ManKey * ManKey::M_devices[CON_MAX_MAN_KEY_NUM];

static ManKeyEventCode buildManKeyEventCode(uint8_t evtCode ,uint8_t inx){
    ManKeyEventCode manKeyEventCode;
    manKeyEventCode.one.evtCode=evtCode;
    manKeyEventCode.one.inx=inx;
    manKeyEventCode.one.arg=0;
    return manKeyEventCode;
}

ManKey::ManKey(uint8_t inx){
    m_inx=inx;
    M_devices[M_nDevices++]= this;
}

void ManKey::OnTick(uint32_t ms)
{
    uint16_t keyPressState = pinRead(m_inx);
    switch (m_state)
    {
        case ManKeyState::STT_IDLE: // 缺省处于空闲状态
        default:
            if (keyPressState == 1)
            { // 一切从按下开始
                m_downMs = ms;
                onEvent(ms, buildManKeyEventCode(MAN_KEY_EVT_DOWN, m_inx));
                m_state = ManKeyState::STT_WAITING_CLICK_UP;
            }
            break;
        case ManKeyState::STT_WAITING_CLICK_UP:
            if (ms - m_downMs > 300)
            { // 超时转到长按状态，同时产生第一个按住事件
                onEvent(ms, buildManKeyEventCode(MAN_KEY_EVT_PRESSING, m_inx));
                m_pressingCount = 1;
                m_state = ManKeyState::STT_LONG_PRESSING;
            }
            else if (keyPressState == 0)
            { // 完成一次点击，但需要延迟区分CLICK还是DBL_CLICK
                m_state = ManKeyState::STT_WAITING_DBL_CLICK_DOWN;
            }
            break;
        case ManKeyState::STT_WAITING_DBL_CLICK_DOWN:
            if (ms - m_downMs > 400)
            { // 超出400毫秒没等到第二次按下，判断为CLICK结束，回归空闲
                onEvent(ms, buildManKeyEventCode(MAN_KEY_EVT_CLICK, m_inx));
                onEvent(ms, buildManKeyEventCode(MAN_KEY_EVT_UP, m_inx));
                m_state = ManKeyState::STT_IDLE;
            }
            else if (keyPressState == 1)
            {
                m_state = ManKeyState::STT_WAITING_DBL_CLICK_UP; // 遭遇再次按下，判断为DBL_CLICK，后续DBL_CLICK_UP
            }
            break;
        case ManKeyState::STT_WAITING_DBL_CLICK_UP:
            if (keyPressState == 0)
            { // 简单等到再次抬起，结束DBL_CLICK，回归空闲
                onEvent(ms, buildManKeyEventCode(MAN_KEY_EVT_DBL_CLICK, m_inx));
                onEvent(ms, buildManKeyEventCode(MAN_KEY_EVT_UP, m_inx));
                m_state = ManKeyState::STT_IDLE;
            }
            break;
        case ManKeyState::STT_LONG_PRESSING:
            if (keyPressState == 0)
            { // 长按期间等到抬起，结束LONG_CLICK，回归空闲
                onEvent(ms, buildManKeyEventCode(MAN_KEY_EVT_LONG_CLICK, m_inx));
                onEvent(ms, buildManKeyEventCode(MAN_KEY_EVT_UP, m_inx));
                m_state = ManKeyState::STT_IDLE;
            }
            else if (ms - m_downMs > 300 + m_pressingCount * 100)
            {
                onEvent(ms, buildManKeyEventCode(MAN_KEY_EVT_PRESSING, m_inx));
                m_pressingCount++;
            }
            break;
    }
}

void ManKey::Create(int nDevices){
    for (int i=0;i<nDevices;i++){
        M_devices[i]=new ManKey(i);
    }
}

void ManKey::OnTickAll(uint32_t ms) {
    for (int i=0;i<M_nDevices;i++){
        M_devices[i]->OnTick(ms);
    }
}