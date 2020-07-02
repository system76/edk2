/** @file
  System76 EC logging

  Copyright (c) 2020 System76, Inc.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/System76EcLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>

// From coreboot/src/drivers/system76_ec/system76_ec.c {
#define SYSTEM76_EC_BASE 0x0E00

static inline UINT8 system76_ec_read(UINT8 addr) {
    return IoRead8(SYSTEM76_EC_BASE + (UINT16)addr);
}

static inline void system76_ec_write(UINT8 addr, UINT8 data) {
    IoWrite8(SYSTEM76_EC_BASE + (UINT16)addr, data);
}

void system76_ec_init(void) {
    // Clear entire command region
    for (int i = 0; i < 256; i++) {
        system76_ec_write((UINT8)i, 0);
    }
}

void system76_ec_flush(void) {
    // Send command
    system76_ec_write(0, 4);

    // Wait for command completion, for up to 10 milliseconds
    int timeout;
    for (timeout = 10000; timeout > 0; timeout--) {
        if (system76_ec_read(0) == 0) break;
        MicroSecondDelay(1);
    }

    // Clear length
    system76_ec_write(3, 0);
}

void system76_ec_print(UINT8 byte) {
    // Read length
    UINT8 len = system76_ec_read(3);
    // Write data at offset
    system76_ec_write(len + 4, byte);
    // Update length
    system76_ec_write(3, len + 1);

    // If we hit the end of the buffer, or were given a newline, flush
    if (byte == '\n' || len >= 128) {
        system76_ec_flush();
    }
}
// } From coreboot/src/drivers/system76_ec/system76_ec.c
