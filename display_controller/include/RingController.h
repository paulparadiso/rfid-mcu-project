#include <Arduino.h>
#include "Adafruit_NeoPixel.h"
//#include <NeoPixelBus.h>
//#include "internal/RgbwColor.h"
#include "NeoPixelAnimator.h"
#include "RgbwColor.h"
#include <vector>

#define LIGHT_EFFECT_ALL_ON 0x00
#define LIGHT_EFFECT_ALL_OFF 0x01
#define LIGHT_EFFECT_PIXEL_ON 0x02
#define LIGHT_EFFECT_PIXEL_OFF 0x03
#define LIGHT_EFFECT_PIXEL_FADE_ON 0x04
#define LIGHT_EFFECT_PIXEL_FADE_OFF 0x05
#define LIGHT_EFFECT_RING_ON 0x06
#define LIGHT_EFFECT_RING_OFF 0x07
#define LIGHT_EFFECT_RING_FADE_ON 0x08
#define LIGHT_EFFECT_RING_FADE_OFF 0x09
#define LIGHT_EFFECT_RING_SWIRL_ON 0x0A
#define LIGHT_EFFECT_RING_SWIRL_OFF 0x0B

#define MAX_TIMED_RING_COMMANDS 256

enum class LightingEventType {
    SET_PIXEL,
    SET_RING,
    ALL_ON,
    ALL_OFF,
    FADE_ON,
    FADE_OFF,
    SWIRL_ON,
    SWIRL_OFF
};

class TimedRingCommand {

public:

    TimedRingCommand(){}
    TimedRingCommand(unsigned long rT, uint16_t c, uint16_t fT, uint8_t l, uint8_t r, uint8_t g, uint8_t b, uint8_t w):
        runTime(rT),
        command(c),
        fadeTime(fT),
        location(l),
        redValue(r),
        greenValue(g),
        blueValue(b),
        whiteValue(w),
        ran(false)
    {}

    unsigned long runTime;
    uint16_t command;
    uint16_t fadeTime;
    uint8_t location;
    uint8_t redValue;
    uint8_t greenValue;
    uint8_t blueValue;
    uint8_t whiteValue;
    bool ran;
};

struct RingCommandNode {
    TimedRingCommand trc;
    RingCommandNode* next;
};

class RingCommandList {

public:

    RingCommandList() {
        currentCmd = 0;
    }

    void add(unsigned long rT, uint16_t c, uint16_t fT, uint8_t l, uint8_t rV, uint16_t gV, uint8_t bV, uint8_t wV) {
        cmds[currentCmd].runTime = rT;
        cmds[currentCmd].fadeTime = fT;
        cmds[currentCmd].command = c;
        cmds[currentCmd].location = l;
        cmds[currentCmd].redValue = rV;
        cmds[currentCmd].greenValue = gV;
        cmds[currentCmd].blueValue = bV;
        cmds[currentCmd].whiteValue = wV;
        cmds[currentCmd].ran = false;
        currentCmd = (currentCmd + 1) % MAX_TIMED_RING_COMMANDS;
    }

    TimedRingCommand* start() {
        return &cmds[0];
    }

private:
    TimedRingCommand cmds[MAX_TIMED_RING_COMMANDS];
    uint32_t currentCmd = 0;
};

class RingController {

public:

    RingController(uint16_t pixCount, uint16_t nRings, uint8_t pixPin);
    void allOn(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
    void allOff();
    void setPixel(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
    void setRing(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
    void fadeOn(uint16_t ring, uint16_t t, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
    void fadeOff(uint16_t ring, uint16_t t);
    void swirlOn(uint16_t ring, uint16_t t, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
    void swirlOff(uint16_t ring, uint16_t t);
    void chaseOn(uint16_t t, uint8_t r, uint8_t g, uint8_t b);
    void chaseOff(uint16_t t);
    void addEvent(unsigned long rT, uint16_t c, uint16_t fT, uint8_t l, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
    void update();
    bool shouldShow();
    void show();

private:
    //NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod>* ring;
    Adafruit_NeoPixel* ring;
    NeoPixelAnimator* animator;
    void startPixelAnimation(uint16_t t, uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w, AnimEaseFunction e);
    void checkRingCommands();
    void runRingCommand(TimedRingCommand* trc);
    uint16_t pixelCount;
    uint16_t numRings;
    uint8_t pixelPin;
    uint8_t animationChannels;
    int fadeStart;
    int fadeTarget;
    RingCommandList rcl;
    uint32_t timeOfLastAnimationEnd = 0;
    uint32_t animatorStartTime = 0;
    bool bAnimating = false;
    bool bShouldShow = false;
};
