/** @file
  This library class defines a set of interfaces to be used by customize Ui module

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __BOOT_MAINTENANCE_MANAGER_UI_LIB_H__
#define __BOOT_MAINTENANCE_MANAGER_UI_LIB_H__

/**
  Create Boot Option menu in the page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateBootOptionMenu (
  IN EFI_HII_HANDLE              HiiHandle,
  IN VOID                        *StartOpCodeHandle
  );

/**
  Create Boot From File menu in the page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateBootFromFileMenu (
  IN EFI_HII_HANDLE              HiiHandle,
  IN VOID                        *StartOpCodeHandle
  );

#endif
