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
CreateUpdateData (
  VOID
  )
{
  //
  // Init OpCode Handle and Allocate space for creation of Buffer
  //
  mStartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (mStartOpCodeHandle != NULL);

  mEndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (mEndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  mStartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (mStartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  mStartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  mEndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (mEndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  mEndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  mEndLabel->Number       = LABEL_END;
}

/**
  Refresh the global UpdateData structure.

**/
VOID
RefreshUpdateData (
  VOID
  )
{
  //
  // Free current updated date
  //
  if (mStartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mStartOpCodeHandle);
  }

  //
  // Create new OpCode Handle
  //
  mStartOpCodeHandle = HiiAllocateOpCodeHandle ();

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  mStartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (mStartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  mStartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;

}

/**
  Add a "Go back to main page" tag in front of the form when there are no
  "Apply changes" and "Discard changes" tags in the end of the form.

  @param CallbackData    The BMM context data.

**/
VOID
UpdatePageStart (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  RefreshUpdateData ();
  mStartLabel->Number = CallbackData->BmmCurrentPageId;

  if (!(CallbackData->BmmAskSaveOrNot)) {
    //
    // Add a "Go back to main page" tag in front of the form when there are no
    // "Apply changes" and "Discard changes" tags in the end of the form.
    //
    HiiCreateGotoOpCode (
      mStartOpCodeHandle,
      FORM_MAIN_ID,
      STRING_TOKEN (STR_FORM_GOTO_MAIN),
      STRING_TOKEN (STR_FORM_GOTO_MAIN),
      0,
      FORM_MAIN_ID
      );
  }
}

/**
  Create the "Apply changes" and "Discard changes" tags. And
  ensure user can return to the main page.

  @param CallbackData    The BMM context data.

**/
VOID
UpdatePageEnd (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  //
  // Create the "Apply changes" and "Discard changes" tags.
  //
  if (CallbackData->BmmAskSaveOrNot) {
    HiiCreateSubTitleOpCode (
      mStartOpCodeHandle,
      STRING_TOKEN (STR_NULL_STRING),
      0,
      0,
      0
      );

    HiiCreateActionOpCode (
      mStartOpCodeHandle,
      KEY_VALUE_SAVE_AND_EXIT,
      STRING_TOKEN (STR_SAVE_AND_EXIT),
      STRING_TOKEN (STR_NULL_STRING),
      EFI_IFR_FLAG_CALLBACK,
      0
      );
  }

  //
  // Ensure user can return to the main page.
  //
  HiiCreateActionOpCode (
    mStartOpCodeHandle,
    KEY_VALUE_NO_SAVE_AND_EXIT,
    STRING_TOKEN (STR_NO_SAVE_AND_EXIT),
    STRING_TOKEN (STR_NULL_STRING),
    EFI_IFR_FLAG_CALLBACK,
    0
    );

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
  IN UINT16                           LabelId,
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  RefreshUpdateData ();

  //
  // Remove all op-codes from dynamic page
  //
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
  Create a list of boot option from global BootOptionMenu. It
  allow user to delete the boot option.

  @param CallbackData    The BMM context data.

**/
VOID
UpdateBootDelPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  UINT16          Index;

  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData);

  ASSERT (BootOptionMenu.MenuNumber <= (sizeof (CallbackData->BmmFakeNvData.BootOptionDel) / sizeof (CallbackData->BmmFakeNvData.BootOptionDel[0])));
  for (Index = 0; Index < BootOptionMenu.MenuNumber; Index++) {
    NewMenuEntry    = BOpt_GetMenuEntry (&BootOptionMenu, Index);
    NewLoadContext  = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
    if (NewLoadContext->IsLegacy) {
      continue;
    }

    NewLoadContext->Deleted = FALSE;

    if (CallbackData->BmmFakeNvData.BootOptionDel[Index] && !CallbackData->BmmFakeNvData.BootOptionDelMark[Index]) {
      //
      // CallbackData->BmmFakeNvData.BootOptionDel[Index] == TRUE means browser knows this boot option is selected
      // CallbackData->BmmFakeNvData.BootOptionDelMark[Index] = FALSE means BDS knows the selected boot option has
      // deleted, browser maintains old useless info. So clear this info here, and later update this info to browser
      // through HiiSetBrowserData function.
      //
      CallbackData->BmmFakeNvData.BootOptionDel[Index] = FALSE;
      CallbackData->BmmOldFakeNVData.BootOptionDel[Index] = FALSE;
    }

    HiiCreateCheckBoxOpCode (
      mStartOpCodeHandle,
      (EFI_QUESTION_ID) (BOOT_OPTION_DEL_QUESTION_ID + Index),
      VARSTORE_ID_BOOT_MAINT,
      (UINT16) (BOOT_OPTION_DEL_VAR_OFFSET + Index),
      NewMenuEntry->DisplayStringToken,
      NewMenuEntry->HelpStringToken,
      EFI_IFR_FLAG_CALLBACK,
      0,
      NULL
      );
  }
  UpdatePageEnd (CallbackData);
}

/**
  Update the page's NV Map if user has changed the order
  a list. This list can be Boot Order.

  @param UpdatePageId    The form ID to be updated.
  @param OptionMenu      The new list.
  @param CallbackData    The BMM context data.

**/
VOID
UpdateOrderPage (
  IN UINT16                           UpdatePageId,
  IN BM_MENU_OPTION                   *OptionMenu,
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  BM_MENU_ENTRY     *NewMenuEntry;
  UINT16            Index;
  UINT16            OptionIndex;
  VOID              *OptionsOpCodeHandle;
  BOOLEAN           BootOptionFound;
  UINT32            *OptionOrder;
  EFI_QUESTION_ID   QuestionId;
  UINT16            VarOffset;

  CallbackData->BmmAskSaveOrNot = TRUE;
  UpdatePageStart (CallbackData);

  OptionOrder = NULL;
  QuestionId = 0;
  VarOffset = 0;
  switch (UpdatePageId) {

  case FORM_BOOT_CHG_ID:
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
    QuestionId = BOOT_OPTION_ORDER_QUESTION_ID;
    VarOffset = BOOT_OPTION_ORDER_VAR_OFFSET;
    break;
  }
  ASSERT (OptionOrder != NULL);

  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  NewMenuEntry = NULL;
  for (OptionIndex = 0; (OptionOrder[OptionIndex] != 0 && OptionIndex < MAX_MENU_NUMBER); OptionIndex++) {
    BootOptionFound = FALSE;
    for (Index = 0; Index < OptionMenu->MenuNumber; Index++) {
      NewMenuEntry   = BOpt_GetMenuEntry (OptionMenu, Index);
      if ((UINT32) (NewMenuEntry->OptionNumber + 1) == OptionOrder[OptionIndex]) {
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
      0,                                           // Question flag
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

/**
Update add boot option page.

@param CallbackData    The BMM context data.
@param FormId             The form ID to be updated.
@param DevicePath       Device path.

**/
VOID
UpdateOptionPage(
  IN   BMM_CALLBACK_DATA        *CallbackData,
  IN   EFI_FORM_ID              FormId,
  IN   EFI_DEVICE_PATH_PROTOCOL *DevicePath
  )
{
  CHAR16                *String;
  EFI_STRING_ID         StringToken;

  String = NULL;

  if (DevicePath != NULL){
    String = ExtractFileNameFromDevicePath(DevicePath);
  }
  if (String == NULL) {
    String = HiiGetString (CallbackData->BmmHiiHandle, STRING_TOKEN (STR_NULL_STRING), NULL);
    ASSERT (String != NULL);
  }

  StringToken = HiiSetString (CallbackData->BmmHiiHandle, 0, String, NULL);
  FreePool (String);

  if(FormId == FORM_BOOT_ADD_ID){
    if (!CallbackData->BmmFakeNvData.BootOptionChanged) {
      ZeroMem (CallbackData->BmmFakeNvData.BootOptionalData, sizeof (CallbackData->BmmFakeNvData.BootOptionalData));
      ZeroMem (CallbackData->BmmFakeNvData.BootDescriptionData, sizeof (CallbackData->BmmFakeNvData.BootDescriptionData));
      ZeroMem (CallbackData->BmmOldFakeNVData.BootOptionalData, sizeof (CallbackData->BmmOldFakeNVData.BootOptionalData));
      ZeroMem (CallbackData->BmmOldFakeNVData.BootDescriptionData, sizeof (CallbackData->BmmOldFakeNVData.BootDescriptionData));
    }
  }

  RefreshUpdateData();
  mStartLabel->Number = FormId;

  HiiCreateSubTitleOpCode (
    mStartOpCodeHandle,
    StringToken,
    0,
    0,
    0
    );

  HiiUpdateForm (
    CallbackData->BmmHiiHandle,
    &mBootMaintGuid,
    FormId,
    mStartOpCodeHandle,// Label FormId
    mEndOpCodeHandle   // LABEL_END
    );
}

/**
  Dispatch the correct update page function to call based on
  the UpdatePageId.

  @param UpdatePageId    The form ID.
  @param CallbackData    The BMM context data.

**/
VOID
UpdatePageBody (
  IN UINT16                           UpdatePageId,
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  CleanUpPage (UpdatePageId, CallbackData);
  switch (UpdatePageId) {
  case FORM_BOOT_CHG_ID:
    UpdateOrderPage (UpdatePageId, &BootOptionMenu, CallbackData);
    break;

  default:
    break;
  }
}

/**
  Dispatch the display to the next page based on NewPageId.

  @param Private         The BMM context data.
  @param NewPageId       The original page ID.

**/
VOID
UpdatePageId (
  BMM_CALLBACK_DATA              *Private,
  UINT16                         NewPageId
  )
{
  if ((NewPageId == KEY_VALUE_SAVE_AND_EXIT) || (NewPageId == KEY_VALUE_NO_SAVE_AND_EXIT)) {
    //
    // Return to main page after "Save Changes" or "Discard Changes".
    //
    NewPageId = FORM_MAIN_ID;
  }

  if ((NewPageId > 0) && (NewPageId < MAXIMUM_FORM_ID)) {
    Private->BmmPreviousPageId  = Private->BmmCurrentPageId;
    Private->BmmCurrentPageId   = NewPageId;
  }
}
