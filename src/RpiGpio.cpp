#ifdef HAS_WIRING_PI
#include "RpiGpio.h"
#include <SDL.h>

namespace {
    //   

    int pinNumbers[6] = { 17, 27,22, 23, 24, 25 };
    SDL_Keycode pinsToKeycode[6] = { SDLK_q, SDLK_p, SDLK_a, SDLK_b, SDLK_t, SDLK_x };
}


void RpiGpio::init() {
    // BCM GPIO numbering rather than the WiringPi numbers
    if (wiringPiSetupGpio() == -1) {
        SDL_Log("Failed to start wriringPi.");
        return;
    }
    for (int pin = 0; pin < 6; ++pin) {
        pinMode(pinNumbers[pin], INPUT);
        pullUpDnControl(pinNumbers[pin], PUD_DOWN);
        stableValue[pin] = 0;
        lastRead[pin] = 0;
    }
}

void RpiGpio::pollAndDispatchKeyboardEvents() {
    for (int pin = 0; pin < 6; ++pin) {
        int current = digitalRead(pinNumbers[pin]);
        if (current == lastRead[pin] && current != stableValue[pin]) {
            stableValue[pin] = current;
            SDL_Event sdlevent;
            sdlevent.type = current == 1 ? SDL_KEYDOWN : SDL_KEYUP;
            sdlevent.key.keysym.sym = pinsToKeycode[pin];
            SDL_PushEvent(&sdlevent);
        }
        lastRead[pin] = current;
    }
}

#endif  // HAS_WIRING_PI

