#include "RingController.h"

RingController::RingController(uint16_t pixCount, uint16_t nRings, uint8_t pixPin) {
    pixelCount = pixCount;
    numRings = nRings;
    pixelPin = pixPin;
    //ring = new NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod>(pixelCount * nRings, pixelPin);
    ring = new Adafruit_NeoPixel(pixelCount * nRings, pixelPin, NEO_GRBW + NEO_KHZ800);
    //animator = new NeoPixelAnimator(pixelCount * nRings);
    animator = NULL;
    ring->begin();
    ring->show();
}

void RingController::allOn(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    for(int i = 0; i < pixelCount * numRings; i++){
        ring->setPixelColor(i, r, g, b, w);
    }
    ring->show();   
}

void RingController::allOff(){
    for(int i = 0; i < pixelCount * numRings; i++){
        ring->setPixelColor(i, 0, 0, 0, 0);
    }
    ring->show();
}

void RingController::setPixel(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    ring->setPixelColor(n, r, g, b, w);
    ring->show();
}

void RingController::setRing(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w){
    for(uint16_t i = 0 + (pixelCount * n); i < ((pixelCount * n) + pixelCount); i++) {
        ring->setPixelColor(i, r, g, b, w);
    }
    ring->show();
}

void RingController::startPixelAnimation(uint16_t t, uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w, AnimEaseFunction e){
    if(animator == NULL) {
        //Serial2.println("Creating animator.");
        animator = new NeoPixelAnimator(pixelCount * numRings);
        timeOfLastAnimationEnd = 0;
        animatorStartTime = 0;
    }
    RgbwColor targetColor(r, g, b, w);
    uint32_t c = ring->getPixelColor(n);
    RgbwColor originalColor;
    originalColor.W = (c >> 24) & 0xFF;
    originalColor.R = (c >> 16) & 0xFF;
    originalColor.G = (c >> 8) & 0xFF;
    originalColor.B = c & 0xFF;
    AnimEaseFunction easing = e;
    AnimUpdateCallback animUpdate = [=](const AnimationParam& param) {
        float progress = easing(param.progress);
        RgbwColor updatedColor = RgbwColor::LinearBlend(originalColor, targetColor, progress);
        if(updatedColor.R == 128){
            updatedColor.R = 127;
        }
        if(updatedColor.G == 128){
            updatedColor.G = 127;
        }
        if(updatedColor.B == 128){
            updatedColor.B = 127;
        }
        if(updatedColor.W == 128){
            updatedColor.W = 127;
        }
        ring->setPixelColor(n, updatedColor.R, updatedColor.G, updatedColor.B, updatedColor.W);
    };
    animator->StartAnimation(n, t, animUpdate);
    timeOfLastAnimationEnd += (t + 100);
    bAnimating = true;
}

void RingController::fadeOn(uint16_t ring, uint16_t t, uint8_t r, uint8_t g, uint8_t b, uint8_t w){
    //animator->StopAll();
    
    /*
    Serial2.print("Fading on ");
    Serial2.print(ring, DEC);
    Serial2.print(" to ");
    Serial2.print(r, DEC);
    Serial2.print(", ");
    Serial2.print(g, DEC);
    Serial2.print(", ");
    Serial2.print(b, DEC);
    Serial2.print(", ");
    Serial2.print(w, DEC);
    Serial2.print(": ");
    Serial2.println(t, DEC);
    */

    //delay(50);

    for(uint16_t i = 0 + (pixelCount * ring); i < ((pixelCount * ring) + pixelCount); i++) {
        startPixelAnimation(t, i, r, g, b, w, NeoEase::CubicIn);
    }
}

void RingController::fadeOff(uint16_t ring, uint16_t t){
    //  animator->StopAll();
    for(uint16_t i = 0 + (pixelCount * ring); i < ((pixelCount * ring) + pixelCount); i++) {
        startPixelAnimation(t, i, 0, 0, 0, 0, NeoEase::CubicIn);
    }
}

void RingController::swirlOn(uint16_t ring, uint16_t t, uint8_t r, uint8_t g, uint8_t b, uint8_t w){
    //animator->StopAll();
    uint16_t timeSlice = t / pixelCount;
    for(uint16_t i = 0 + (pixelCount * ring); i < ((pixelCount * ring) + pixelCount); i++) {
        startPixelAnimation((timeSlice * i + timeSlice), i, r, g, b, w, NeoEase::CubicIn);
    }
}

void RingController::swirlOff(uint16_t ring, uint16_t t){
    //animator->StopAll();
    if(ring == 255) {
        uint16_t timeSlice = t / (pixelCount * numRings);
        for(uint16_t i = 0; i < (pixelCount * numRings); i++) {
            startPixelAnimation((timeSlice * i + timeSlice), i, 0, 0, 0, 0, NeoEase::CubicOut);
        }
    } else {
        uint16_t timeSlice = t / pixelCount;
        for(uint16_t i = 0 + (pixelCount * ring); i < ((pixelCount * ring) + pixelCount); i++) {
            startPixelAnimation((timeSlice * i + timeSlice), i, 0, 0, 0, 0, NeoEase::CubicOut);
        }
    }
}

void RingController::chaseOn(uint16_t t, uint8_t r, uint8_t g, uint8_t b) {
    uint16_t timeSlice = t / (pixelCount * numRings);
    for(uint16_t i = 0; i < (pixelCount * numRings); i++) {
        startPixelAnimation((timeSlice * i + timeSlice), i, r, g, b, 0, NeoEase::CubicIn);
    }
}

void RingController::chaseOff(uint16_t t) {
    uint16_t timeSlice = t / (pixelCount * numRings);
    for(uint16_t i = 0; i < (pixelCount * numRings); i++) {
        startPixelAnimation((timeSlice * i + timeSlice), i, 0, 0, 0, 0, NeoEase::CubicOut);
    }   
}

void RingController::addEvent(unsigned long rT, uint16_t c, uint16_t fT, uint8_t l, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    rcl.add(millis() + rT, c, fT, l, r, g, b, w);
}

void RingController::checkRingCommands() {
    /*
    RingCommandNode * n = rcl.start();
    while(n != NULL) {
        if((n->trc.runTime < millis()) && (n->trc.ran == false)) {
            runRingCommand(&(n->trc));
            RingCommandNode* tmp = n;
            n = n->next;
            rcl.remove(tmp);
            //delete t;
        } else {
            n = n->next;
        }
    }
    */
    TimedRingCommand* trc = rcl.start();
    for(uint32_t i = 0; i < MAX_TIMED_RING_COMMANDS; i++) {
        if(trc[i].runTime < millis() && (trc[i].ran == false)) {
            runRingCommand(&trc[i]);
        }
    }
}

void RingController::runRingCommand(TimedRingCommand* trc) { 
    switch (trc->command) {
        case LIGHT_EFFECT_ALL_ON:
            allOn(trc->redValue, trc->greenValue, trc->blueValue, trc->whiteValue);
            break;
        case LIGHT_EFFECT_ALL_OFF:
            allOff();
            break;
        case LIGHT_EFFECT_PIXEL_ON:
            setPixel(trc->location, trc->redValue, trc->greenValue, trc->blueValue, trc->whiteValue);
            break;
        case LIGHT_EFFECT_PIXEL_OFF:
            allOff();
            break;
        case LIGHT_EFFECT_PIXEL_FADE_ON:
            allOff();
            break;
        case LIGHT_EFFECT_PIXEL_FADE_OFF:
            allOff();
            break;
        case LIGHT_EFFECT_RING_ON:
            setRing(trc->location, trc->redValue, trc->greenValue, trc->blueValue, trc->whiteValue);
            break;
        case LIGHT_EFFECT_RING_OFF:
            allOff();
            break;
        case LIGHT_EFFECT_RING_FADE_ON:
            if(trc->location == 255) {
                for(uint16_t i = 0; i < numRings; i++) {
                    fadeOn(i, trc->fadeTime, trc->redValue, trc->greenValue, trc->blueValue, trc->whiteValue);
                } 
            } else {
                fadeOn(trc->location, trc->fadeTime, trc->redValue, trc->greenValue, trc->blueValue, trc->whiteValue);
                //setRing(trc->location, trc->redValue, trc->greenValue, trc->blueValue, trc->whiteValue);
            }
            break;
        case LIGHT_EFFECT_RING_FADE_OFF:
            if (trc->location == 255) {
                for(uint16_t i = 0; i <  numRings; i++) {
                    fadeOff(i, trc->fadeTime);
                } 
            } else {
                fadeOff(trc->location, trc->fadeTime);
            }
            break;
        case LIGHT_EFFECT_RING_SWIRL_ON:
            if(trc->location == 255) {
                for(uint16_t i = 0; i < numRings; i++) {    
                    swirlOn(trc->location, trc->fadeTime, trc->redValue, trc->greenValue, trc->blueValue, trc->whiteValue);
                }
            } else {
                swirlOn(trc->location, trc->fadeTime, trc->redValue, trc->greenValue, trc->blueValue, trc->whiteValue);
            }
            break;
        case LIGHT_EFFECT_RING_SWIRL_OFF:
            if(trc->location == 255) {
                for (uint16_t i = 0; i < numRings; i++) {
                    swirlOff(trc->location, trc->fadeTime);
                }
            } else {
                swirlOff(trc->location, trc->fadeTime);
            }
            break;
        default:
            break;
    }

    trc->ran = true;
}

void RingController::update(){
    checkRingCommands();
    if(animator != NULL) {
        if (animator->IsAnimating()) {
            //Serial2.println("Updating animator.");
            animator->UpdateAnimations();
            bShouldShow = true;
        } else {
        //if(bAnimating && (millis() > (animatorStartTime + timeOfLastAnimationEnd))) {
            animator->StopAll();
            delete animator;
            //Serial2.println("Destroying animator.");
            animator = NULL;
            bAnimating = false;
        }
    }
}

bool RingController::shouldShow() {
    return bShouldShow;
}

void RingController::show() {
    ring->show();
    bShouldShow = false;
}
