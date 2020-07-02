/** @file
The functions for Boot Maintainence Main menu.

Copyright (c) 2004 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "BootMaintenanceManager.h"
#include "BootMaintenanceManagerCustomizedUiSupport.h"

/**
  Create Boot Option menu in the page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateBootOptionMenu (
  IN EFI_HII_HANDLE              HiiHandle,
  IN VOID                        *StartOpCodeHandle
  )
{
  HiiCreateGotoOpCode (
    StartOpCodeHandle,
    FORM_BOOT_CHG_ID,
    STRING_TOKEN (STR_CHANGE_ORDER),
    STRING_TOKEN (STR_NULL_STRING),
    EFI_IFR_FLAG_CALLBACK,
    FORM_BOOT_CHG_ID
    );
}

/**
  Create Boot From File menu in the page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateBootFromFileMenu (
  IN EFI_HII_HANDLE              HiiHandle,
  IN VOID                        *StartOpCodeHandle
  )
{
  HiiCreateGotoOpCode (
    StartOpCodeHandle,
    FORM_MAIN_ID,
    STRING_TOKEN (STR_BOOT_FROM_FILE),
    STRING_TOKEN (STR_BOOT_FROM_FILE_HELP),
    EFI_IFR_FLAG_CALLBACK,
    KEY_VALUE_BOOT_FROM_FILE
    );
}
