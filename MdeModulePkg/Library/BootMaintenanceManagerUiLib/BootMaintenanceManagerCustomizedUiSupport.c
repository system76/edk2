/** @file
The functions for Boot Maintainence Main menu.

Copyright (c) 2004 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "BootMaintenanceManager.h"
#include "BootMaintenanceManagerCustomizedUiSupport.h"

/**
  Create the dynamic item to allow user to set the "BootNext" vaule.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateBootNextMenu(
  IN EFI_HII_HANDLE              HiiHandle,
  IN VOID                        *StartOpCodeHandle
  )
{
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  UINT16          Index;
  VOID            *OptionsOpCodeHandle;
  UINT32          BootNextIndex;

  if (BootOptionMenu.MenuNumber == 0) {
    return;
  }

  BootNextIndex = NONE_BOOTNEXT_VALUE;

  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  for (Index = 0; Index < BootOptionMenu.MenuNumber; Index++) {
    NewMenuEntry    = BOpt_GetMenuEntry (&BootOptionMenu, Index);
    NewLoadContext  = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;

    if (NewLoadContext->IsBootNext) {
      HiiCreateOneOfOptionOpCode (
        OptionsOpCodeHandle,
        NewMenuEntry->DisplayStringToken,
        EFI_IFR_OPTION_DEFAULT,
        EFI_IFR_TYPE_NUM_SIZE_32,
        Index
        );
      BootNextIndex = Index;
    } else {
      HiiCreateOneOfOptionOpCode (
        OptionsOpCodeHandle,
        NewMenuEntry->DisplayStringToken,
        0,
        EFI_IFR_TYPE_NUM_SIZE_32,
        Index
        );
    }
  }

  if (BootNextIndex == NONE_BOOTNEXT_VALUE) {
    HiiCreateOneOfOptionOpCode (
      OptionsOpCodeHandle,
      STRING_TOKEN (STR_NONE),
      EFI_IFR_OPTION_DEFAULT,
      EFI_IFR_TYPE_NUM_SIZE_32,
      NONE_BOOTNEXT_VALUE
      );
  } else {
    HiiCreateOneOfOptionOpCode (
      OptionsOpCodeHandle,
      STRING_TOKEN (STR_NONE),
      0,
      EFI_IFR_TYPE_NUM_SIZE_32,
      NONE_BOOTNEXT_VALUE
      );
  }

  HiiCreateOneOfOpCode (
    StartOpCodeHandle,
    (EFI_QUESTION_ID) BOOT_NEXT_QUESTION_ID,
    VARSTORE_ID_BOOT_MAINT,
    BOOT_NEXT_VAR_OFFSET,
    STRING_TOKEN (STR_BOOT_NEXT),
    STRING_TOKEN (STR_BOOT_NEXT_HELP),
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    OptionsOpCodeHandle,
    NULL
    );

  HiiFreeOpCodeHandle (OptionsOpCodeHandle);

}

/**
  Create Time Out Menu in the page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateTimeOutMenu (
  IN EFI_HII_HANDLE              HiiHandle,
  IN VOID                        *StartOpCodeHandle
  )
{
  HiiCreateNumericOpCode (
    StartOpCodeHandle,
    (EFI_QUESTION_ID) FORM_TIME_OUT_ID,
    VARSTORE_ID_BOOT_MAINT,
    BOOT_TIME_OUT_VAR_OFFSET,
    STRING_TOKEN(STR_NUM_AUTO_BOOT),
    STRING_TOKEN(STR_HLP_AUTO_BOOT),
    EFI_IFR_FLAG_CALLBACK,
    EFI_IFR_NUMERIC_SIZE_2 | EFI_IFR_DISPLAY_UINT_DEC,
    0,
    65535,
    0,
    NULL
    );
}

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

/**
  Create empty line menu in the front page.

  @param    HiiHandle           The hii handle for the Uiapp driver.
  @param    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateEmptyLine (
  IN EFI_HII_HANDLE              HiiHandle,
  IN VOID                        *StartOpCodeHandle
  )
{
  HiiCreateSubTitleOpCode (StartOpCodeHandle, STRING_TOKEN (STR_NULL_STRING), 0, 0, 0);
}
