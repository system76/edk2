/** @file
  System76 EC logging

  Copyright (c) 2020 System76, Inc.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SYSTEM76_EC_LIB__
#define __SYSTEM76_EC_LIB__

// From coreboot/src/include/console/system76_ec.h {
void system76_ec_init(void);
void system76_ec_flush(void);
void system76_ec_print(UINT8 byte);
// } From coreboot/src/include/console/system76_ec.h

#endif /* __SYSTEM76_EC_LIB__ */
