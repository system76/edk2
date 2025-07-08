/** @file
Dynamically update the pages.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BootMaintenanceManager.h"

/**
 Create the global UpdateData structure.

**/
VOID
CreateUpdateData (VOID)
{
  // Init OpCode Handle and Allocate space for creation of Buffer
  mStartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (mStartOpCodeHandle != NULL);

  mEndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (mEndOpCodeHandle != NULL);

  // Create Hii Extend Label OpCode as the start opcode
  mStartLabel               = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (mStartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  mStartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;

  // Create Hii Extend Label OpCode as the end opcode
  mEndLabel               = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (mEndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  mEndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  mEndLabel->Number       = LABEL_END;
}

/**
  Refresh the global UpdateData structure.

**/
STATIC
VOID
RefreshUpdateData (VOID)
{
  // Free current updated date
  if (mStartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mStartOpCodeHandle);
  }

  // Create new OpCode Handle
  mStartOpCodeHandle = HiiAllocateOpCodeHandle ();

  // Create Hii Extend Label OpCode as the start opcode
  mStartLabel               = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (mStartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  mStartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
}

/**
  Create the "Apply changes" and "Discard changes" tags. And
  ensure user can return to the main page.

  @param CallbackData    The BMM context data.

**/
VOID
UpdatePageEnd (
  IN BMM_CALLBACK_DATA  *CallbackData
  )
{
  HiiUpdateForm (
    CallbackData->BmmHiiHandle,
    &mBootMaintGuid,
    CallbackData->BmmCurrentPageId,
    mStartOpCodeHandle, // Label CallbackData->BmmCurrentPageId
    mEndOpCodeHandle    // LABEL_END
    );
}

/**
  Clean up the dynamic opcode at label and form specified by both LabelId.

  @param LabelId         It is both the Form ID and Label ID for opcode deletion.
  @param CallbackData    The BMM context data.

**/
VOID
CleanUpPage (
  IN UINT16             LabelId,
  IN BMM_CALLBACK_DATA  *CallbackData
  )
{
  RefreshUpdateData ();

  // Remove all op-codes from dynamic page
  mStartLabel->Number = LabelId;
  HiiUpdateForm (
    CallbackData->BmmHiiHandle,
    &mBootMaintGuid,
    LabelId,
    mStartOpCodeHandle, // Label LabelId
    mEndOpCodeHandle    // LABEL_END
    );
}

/**
  Update the page's NV Map if user has changed the order
  a list. This list can be Boot Order or Driver Order.

  @param OptionMenu      The new list.
  @param CallbackData    The BMM context data.

**/
VOID
UpdateBootOrderPage (
  IN BM_MENU_OPTION     *OptionMenu,
  IN BMM_CALLBACK_DATA  *CallbackData
  )
{
  BM_MENU_ENTRY    *NewMenuEntry;
  UINT16           Index;
  UINT16           OptionIndex;
  VOID             *OptionsOpCodeHandle;
  BOOLEAN          BootOptionFound;
  UINT32           *OptionOrder;
  EFI_QUESTION_ID  QuestionId;
  UINT16           VarOffset;

  RefreshUpdateData ();
  mStartLabel->Number = CallbackData->BmmCurrentPageId;

  OptionOrder = NULL;
  QuestionId  = 0;
  VarOffset   = 0;

  //
  // If the BootOptionOrder in the BmmFakeNvData are same with the date in the BmmOldFakeNVData,
  // means all Boot Options has been save in BootOptionMenu, we can get the date from the menu.
  // else means browser maintains some uncommitted date which are not saved in BootOptionMenu,
  // so we should not get the data from BootOptionMenu to show it.
  //
  if (CompareMem (CallbackData->BmmFakeNvData.BootOptionOrder, CallbackData->BmmOldFakeNVData.BootOptionOrder, sizeof (CallbackData->BmmFakeNvData.BootOptionOrder)) == 0) {
    GetBootOrder (CallbackData);
  }

  OptionOrder = CallbackData->BmmFakeNvData.BootOptionOrder;
  QuestionId  = BOOT_OPTION_ORDER_QUESTION_ID;
  VarOffset   = BOOT_OPTION_ORDER_VAR_OFFSET;

  ASSERT (OptionOrder != NULL);

  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  NewMenuEntry = NULL;
  for (OptionIndex = 0; (OptionIndex < MAX_MENU_NUMBER && OptionOrder[OptionIndex] != 0); OptionIndex++) {
    BootOptionFound = FALSE;
    for (Index = 0; Index < OptionMenu->MenuNumber; Index++) {
      NewMenuEntry = BOpt_GetMenuEntry (OptionMenu, Index);
      if ((UINT32)(NewMenuEntry->OptionNumber + 1) == OptionOrder[OptionIndex]) {
        BootOptionFound = TRUE;
        break;
      }
    }

    if (BootOptionFound) {
      HiiCreateOneOfOptionOpCode (
        OptionsOpCodeHandle,
        NewMenuEntry->DisplayStringToken,
        0,
        EFI_IFR_TYPE_NUM_SIZE_32,
        OptionOrder[OptionIndex]
        );
    }
  }

  if (OptionMenu->MenuNumber > 0) {
    HiiCreateOrderedListOpCode (
      mStartOpCodeHandle,                          // Container for dynamic created opcodes
      QuestionId,                                  // Question ID
      VARSTORE_ID_BOOT_MAINT,                      // VarStore ID
      VarOffset,                                   // Offset in Buffer Storage
      STRING_TOKEN (STR_CHANGE_ORDER),             // Question prompt text
      STRING_TOKEN (STR_CHANGE_ORDER),             // Question help text
      EFI_IFR_FLAG_CALLBACK,                       // Question flag
      0,                                           // Ordered list flag, e.g. EFI_IFR_UNIQUE_SET
      EFI_IFR_TYPE_NUM_SIZE_32,                    // Data type of Question value
      100,                                         // Maximum container
      OptionsOpCodeHandle,                         // Option Opcode list
      NULL                                         // Default Opcode is NULL
      );
  }

  HiiFreeOpCodeHandle (OptionsOpCodeHandle);

  UpdatePageEnd (CallbackData);
}
