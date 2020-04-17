/** @file
  System76 EC logging

  Copyright (c) 2020 System76, Inc.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/System76EcLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>

#define SYSTEM76_EC_BASE 0xE00
#define CMD_PRINT 0x4

/**
  @param  Offset    The offset into the SFMI RAM window to read from.

  @return The value read.
**/
STATIC
UINT8
System76EcReadByte (
  UINT8   Offset
  )
{
  return IoRead8 (SYSTEM76_EC_BASE + Offset);
}

/**
  @param  Offset    The offset into the SFMI RAM window to write to.
  @param  Value     The value to write.

  @return The value written back to the I/O port.
**/
STATIC
UINT8
System76EcWriteByte (
  UINT8   Offset,
  UINT8   Value
  )
{
  return IoWrite8 (SYSTEM76_EC_BASE + Offset, Value);
}

/**
  Write data to the embedded controller using SMFI command interface.

  @param  Buffer            Pointer to the data buffer to be written.
  @param  NumberOfBytes     Number of bytes to write.

  @return -1 if the command failed, else the number of data bytes written.
**/
INTN
System76EcWrite (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  )
{
  UINTN Index;
  UINTN Index2;
  UINTN Timeout;

  if (Buffer == NULL) {
      return 0;
  }

  for (Index = 0; Index < NumberOfBytes;) {
    if (System76EcReadByte (0) == 0) {

      // Can write 256 bytes of data at a time
      for (Index2 = 0; (Index2 < NumberOfBytes) && ((Index2 + 4) < 256); Index++, Index2++) {
        System76EcWriteByte ((Index + 4), Buffer[Index]);
      }
      // Flags
      System76EcWriteByte (2, 0);
      // Length
      System76EcWriteByte (3, Index2);
      // Command
      System76EcWriteByte (0, CMD_PRINT);

      // Wait for command completion, for up to 1 second
      for (Timeout = 1000000; Timeout > 0; Timeout--) {
        if (System76EcReadByte (0) == 0)
          break;
        MicroSecondDelay (1);
      }
      if (Timeout == 0) {
        // Error: Timeout occured
        return -1;
      }
      if (System76EcReadByte (1) != 0) {
        // Error: Command failed
        return -1;
      }
    } else {
      // Error: Command already running
      return -1;
    }
  }

  return Index;
}
