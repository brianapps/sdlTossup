#ifndef RPIGPIO_H_
#define RPIGPIO_H

#ifdef HAS_WIRING_PI
#include <wiringPi.h>

/*
bool initGpio() {
    if (wiringPiSetup() == -1) {
        std::cout << "Failed to start\n";
        return false;
    }
    for (int pin = 0; pin < 5; ++pin) {
        pinMode(pin, INPUT);
        pullUpDnControl(pin, PUD_DOWN);
    }
    return true;
}



int main() {
    if (!initGpio())
        return 1;

    int values[5] = { 0 };
    int lastRead[5] = { 0 };

    while (true) {
        delay(1);
        for (int pin = 0; pin < 5; ++pin) {
            int current = digitalRead(pin);
            if (current == lastRead[pin] && current != values[pin]) {
                values[pin] = current;
                std::cout << pin << ":" << current << std::endl;
            }
            lastRead[pin] = current;
        }

*/

// Manage any switches wired to the raspberry pi gpio pins and convert them into keyboard messages

class RpiGpio {
protected:
    int lastRead[6];
    int stableValue[6];
public:
    void init();
    void pollAndDispatchKeyboardEvents();
};

#else

class RpiGpio {
public:
    void init() {}
    void pollAndDispatchKeyboardEvents() {}
};

#endif  // HAS_WIRING_PI


#endif  // RPIGPIO_H