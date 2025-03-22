#include <Arduino.h>
#include "no_os_delay.h"

void no_os_udelay(uint32_t usecs) {
    delayMicroseconds(usecs);
}

void no_os_mdelay(uint32_t msecs) {
    delay(msecs);
}

struct no_os_time no_os_get_time(void) {
    struct no_os_time t;
    unsigned long micros_now = micros();

    t.s = micros_now / 1000000;
    t.us = micros_now % 1000000;

    return t;
}