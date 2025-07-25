#ifndef __man_key_h
#define __man_key_h
#include "stdint.h"

#define  CON_MAX_MAN_KEY_NUM  1

//按键编码
typedef	union
{
    struct {
        uint16_t arg;
        uint8_t	inx;
        uint8_t	evtCode;
    }one;
    uint32_t	all;
} ManKeyEventCode;

//按键值,按下为1,未按为0
typedef int(ManKeyPinRead)(uint8_t id);
//发出事件
typedef void (ManKeyOutEvent)(uint32_t ms, ManKeyEventCode manKeyEventCode);


#define MAN_KEY_EVT_GROUP_KEY	    0x00
#define MAN_KEY_EVT_DOWN	    (MAN_KEY_EVT_GROUP_KEY|0x00)
#define MAN_KEY_EVT_UP		    (MAN_KEY_EVT_GROUP_KEY|0x01)
#define MAN_KEY_EVT_CLICK		(MAN_KEY_EVT_GROUP_KEY|0x02)
#define MAN_KEY_EVT_DBL_CLICK	(MAN_KEY_EVT_GROUP_KEY|0x03)
#define MAN_KEY_EVT_PRESSING	(MAN_KEY_EVT_GROUP_KEY|0x04)
#define MAN_KEY_EVT_LONG_CLICK	(MAN_KEY_EVT_GROUP_KEY|0x05)


enum class ManKeyState{
    // 内部状态定义
    STT_IDLE= 0,
    //等单击松开
    STT_WAITING_CLICK_UP= 1,
    //等双击按下
    STT_WAITING_DBL_CLICK_DOWN =2,
    //等双击松开
    STT_WAITING_DBL_CLICK_UP= 3,
    //长按中
    STT_LONG_PRESSING= 10
};


class ManKey {
public:
    static int	  M_nDevices;
    static ManKey* M_devices[CON_MAX_MAN_KEY_NUM];
    static ManKeyPinRead*   pinRead;
    static ManKeyOutEvent*  onEvent;
    static void  Create(int nDevices);
    static void  OnTickAll(uint32_t ms);
    virtual void OnTick(uint32_t ms);
    ManKey(uint8_t inx);
    virtual ~ManKey() {}
protected:
    uint8_t		m_inx;
    ManKeyState	m_state;
    uint32_t	m_downMs;
    uint32_t	m_pressingCount;
};

#endif