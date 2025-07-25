#include <Arduino.h>
#include "AtShell.h"
#include "Protothread.h"
#include "ManKey.h"

#define  KEY0   0
#define  LED1   26
#define  LED2   27

class LedPt;
class KeyPt;

extern KeyPt keyPt;
extern LedPt ledPt;


static int keyPinRead(uint8_t id) {
    return !digitalRead(KEY0);
}

static void onKeyEvent(uint32_t ms, ManKeyEventCode manKeyEventCode) {
    switch (manKeyEventCode.one.evtCode) {
        case MAN_KEY_EVT_DOWN:
            Serial.println("按下");
            Protothread::PushIndata((Protothread *) &ledPt, 1);
            break;
        case MAN_KEY_EVT_UP    :
            Serial.println("抬起");
            break;
        case MAN_KEY_EVT_CLICK        : {

            Serial.println("单击");
            break;
        }
        case MAN_KEY_EVT_DBL_CLICK:
            Serial.println("双击");
            break;
        case MAN_KEY_EVT_PRESSING:
            Serial.println("短按");
            break;
        case MAN_KEY_EVT_LONG_CLICK    :
            Serial.println("长按");
            break;
    }

}


class KeyPt : public Protothread {
    void Init() {
        AT_println("KeyPt init");
        pinMode(KEY0, INPUT_PULLUP);
        ManKey::Create(1);
        ManKey::pinRead = keyPinRead;
        ManKey::onEvent = onKeyEvent;
    }

    bool Run() {
        PtOsDelayMs(10);
        ManKey::OnTickAll(millis());
        return 0;
    }
};

class LedPt : public Protothread {
    void Init() {
        AT_println("LedPt init");
        pinMode(LED1, OUTPUT);
        pinMode(LED2, OUTPUT);
    }

    bool Run() {
        WHILE(1) {
                PT_WAIT_UNTIL(PopInData());
                digitalWrite(LED2, !digitalRead(LED2));
            }
        PT_END();
    }

};


KeyPt keyPt;
LedPt ledPt;

