/** @file
  System76 EC logging

  Copyright (c) 2020 System76, Inc.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SYSTEM76_EC_LIB__
#define __SYSTEM76_EC_LIB__

/**
  Write data to the embedded controller using SMFI command interface.

  @param  Buffer            Pointer to the data buffer to be written.
  @param  NumberOfBytes     Number of bytes to write.

  @return -1 if the command failed, else the number of data bytes written.
**/
INTN
System76EcWrite (
  IN UINT8    *Buffer,
  IN UINTN    NumberOfBytes
);

#endif /* __SYSTEM76_EC_LIB__ */
