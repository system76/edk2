/** @file
  Provide boot option support for Application "BootMaint"

  Include file system navigation, system handle selection

  Boot option manipulation

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BootMaintenanceManager.h"

///
/// Define the maximum characters that will be accepted.
///
#define MAX_CHAR  480

/**
  Create a menu entry by given menu type.

  @param MenuType        The Menu type to be created.

  @retval NULL           If failed to create the menu.
  @return the new menu entry.

**/
BM_MENU_ENTRY *
BOpt_CreateMenuEntry (
  UINTN  MenuType
  )
{
  BM_MENU_ENTRY  *MenuEntry;
  UINTN          ContextSize;

  //
  // Get context size according to menu type
  //
  switch (MenuType) {
    case BM_LOAD_CONTEXT_SELECT:
      ContextSize = sizeof (BM_LOAD_CONTEXT);
      break;

    default:
      ContextSize = 0;
      break;
  }

  if (ContextSize == 0) {
    return NULL;
  }

  //
  // Create new menu entry
  //
  MenuEntry = AllocateZeroPool (sizeof (BM_MENU_ENTRY));
  if (MenuEntry == NULL) {
    return NULL;
  }

  MenuEntry->VariableContext = AllocateZeroPool (ContextSize);
  if (MenuEntry->VariableContext == NULL) {
    FreePool (MenuEntry);
    return NULL;
  }

  MenuEntry->Signature        = BM_MENU_ENTRY_SIGNATURE;
  MenuEntry->ContextSelection = MenuType;
  return MenuEntry;
}

/**
  Free up all resource allocated for a BM_MENU_ENTRY.

  @param MenuEntry   A pointer to BM_MENU_ENTRY.

**/
STATIC
VOID
BOpt_DestroyMenuEntry (
  BM_MENU_ENTRY  *MenuEntry
  )
{
  BM_LOAD_CONTEXT      *LoadContext;

  //
  //  Select by the type in Menu entry for current context type
  //
  switch (MenuEntry->ContextSelection) {
    case BM_LOAD_CONTEXT_SELECT:
      LoadContext = (BM_LOAD_CONTEXT *)MenuEntry->VariableContext;
      FreePool (LoadContext->FilePathList);

      FreePool (LoadContext);
      break;

    default:
      break;
  }

  FreePool (MenuEntry->DisplayString);
  if (MenuEntry->HelpString != NULL) {
    FreePool (MenuEntry->HelpString);
  }

  FreePool (MenuEntry);
}

/**
  Get the Menu Entry from the list in Menu Entry List.

  If MenuNumber is great or equal to the number of Menu
  Entry in the list, then ASSERT.

  @param MenuOption      The Menu Entry List to read the menu entry.
  @param MenuNumber      The index of Menu Entry.

  @return The Menu Entry.

**/
BM_MENU_ENTRY *
BOpt_GetMenuEntry (
  BM_MENU_OPTION  *MenuOption,
  UINTN           MenuNumber
  )
{
  BM_MENU_ENTRY  *NewMenuEntry;
  UINTN          Index;
  LIST_ENTRY     *List;

  ASSERT (MenuNumber < MenuOption->MenuNumber);

  List = MenuOption->Head.ForwardLink;
  for (Index = 0; Index < MenuNumber; Index++) {
    List = List->ForwardLink;
  }

  NewMenuEntry = CR (List, BM_MENU_ENTRY, Link, BM_MENU_ENTRY_SIGNATURE);

  return NewMenuEntry;
}

/**
  Free resources allocated in Allocate Rountine.

  @param FreeMenu        Menu to be freed
**/
VOID
BOpt_FreeMenu (
  BM_MENU_OPTION  *FreeMenu
  )
{
  BM_MENU_ENTRY  *MenuEntry;

  while (!IsListEmpty (&FreeMenu->Head)) {
    MenuEntry = CR (
                  FreeMenu->Head.ForwardLink,
                  BM_MENU_ENTRY,
                  Link,
                  BM_MENU_ENTRY_SIGNATURE
                  );
    RemoveEntryList (&MenuEntry->Link);
    BOpt_DestroyMenuEntry (MenuEntry);
  }

  FreeMenu->MenuNumber = 0;
}

/**

  Build the BootOptionMenu according to BootOrder Variable.
  This Routine will access the Boot#### to get EFI_LOAD_OPTION.

  @param CallbackData The BMM context data.

  @return EFI_NOT_FOUND Fail to find "BootOrder" variable.
  @return EFI_SUCESS    Success build boot option menu.

**/
EFI_STATUS
BOpt_GetBootOptions (
  IN  BMM_CALLBACK_DATA  *CallbackData
  )
{
  UINTN                         Index;
  UINT16                        BootString[10];
  UINT8                         *LoadOptionFromVar = NULL;
  UINTN                         BootOptionSize;
  UINT16                        *BootOrderList = NULL;
  UINTN                         BootOrderListSize = 0;
  BM_MENU_ENTRY                 *NewMenuEntry;
  BM_LOAD_CONTEXT               *NewLoadContext;
  UINT8                         *LoadOptionPtr;
  UINTN                         StringSize;
  UINTN                         MenuCount = 0;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption;
  UINTN                         BootOptionCount;

  BOpt_FreeMenu (&BootOptionMenu);
  InitializeListHead (&BootOptionMenu.Head);

  //
  // Get the BootOrder from the Var
  //
  GetEfiGlobalVariable2 (L"BootOrder", (VOID **)&BootOrderList, &BootOrderListSize);
  if (BootOrderList == NULL) {
    return EFI_NOT_FOUND;
  }

  BootOption = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);
  for (Index = 0; Index < BootOrderListSize / sizeof (UINT16); Index++) {
    // Don't display the hidden/inactive boot option
    if (((BootOption[Index].Attributes & LOAD_OPTION_HIDDEN) != 0) || ((BootOption[Index].Attributes & LOAD_OPTION_ACTIVE) == 0)) {
      continue;
    }

    // Get all loadoptions from the VAR
    UnicodeSPrint (BootString, sizeof (BootString), L"Boot%04x", BootOrderList[Index]);
    GetEfiGlobalVariable2 (BootString, (VOID **)&LoadOptionFromVar, &BootOptionSize);
    if (LoadOptionFromVar == NULL) {
      continue;
    }

    NewMenuEntry = BOpt_CreateMenuEntry (BM_LOAD_CONTEXT_SELECT);
    ASSERT (NULL != NewMenuEntry);

    NewLoadContext = (BM_LOAD_CONTEXT *)NewMenuEntry->VariableContext;

    LoadOptionPtr = LoadOptionFromVar;

    NewMenuEntry->OptionNumber = BootOrderList[Index];

    //
    // LoadOption is a pointer type of UINT8
    // for easy use with following LOAD_OPTION
    // embedded in this struct
    //

    NewLoadContext->Attributes = *(UINT32 *)LoadOptionPtr;

    LoadOptionPtr += sizeof (UINT32);

    NewLoadContext->FilePathListLength = *(UINT16 *)LoadOptionPtr;
    LoadOptionPtr += sizeof (UINT16);

    StringSize = StrSize ((UINT16 *)LoadOptionPtr);

    NewLoadContext->Description = AllocateZeroPool (StrSize ((UINT16 *)LoadOptionPtr));
    ASSERT (NewLoadContext->Description != NULL);
    StrCpyS (NewLoadContext->Description, StrSize ((UINT16 *)LoadOptionPtr) / sizeof (UINT16), (UINT16 *)LoadOptionPtr);

    ASSERT (NewLoadContext->Description != NULL);
    NewMenuEntry->DisplayString      = NewLoadContext->Description;
    NewMenuEntry->DisplayStringToken = HiiSetString (CallbackData->BmmHiiHandle, 0, NewMenuEntry->DisplayString, NULL);

    LoadOptionPtr += StringSize;

    NewLoadContext->FilePathList = AllocateZeroPool (NewLoadContext->FilePathListLength);
    ASSERT (NewLoadContext->FilePathList != NULL);
    CopyMem (
      NewLoadContext->FilePathList,
      (EFI_DEVICE_PATH_PROTOCOL *)LoadOptionPtr,
      NewLoadContext->FilePathListLength
      );

    NewMenuEntry->HelpString      = UiDevicePathToStr (NewLoadContext->FilePathList);
    NewMenuEntry->HelpStringToken = HiiSetString (CallbackData->BmmHiiHandle, 0, NewMenuEntry->HelpString, NULL);

    LoadOptionPtr += NewLoadContext->FilePathListLength;

    InsertTailList (&BootOptionMenu.Head, &NewMenuEntry->Link);
    MenuCount++;
    FreePool (LoadOptionFromVar);
  }

  EfiBootManagerFreeLoadOptions (BootOption, BootOptionCount);

  if (BootOrderList != NULL) {
    FreePool (BootOrderList);
  }

  BootOptionMenu.MenuNumber = MenuCount;
  return EFI_SUCCESS;
}

/**

  Get the Option Number that has not been allocated for use.

  @param Type  The type of Option.

  @return The available Option Number.

**/
STATIC
UINT16
BOpt_GetOptionNumber (
  CHAR16  *Type
  )
{
  UINT16  *OrderList;
  UINTN   OrderListSize;
  UINTN   Index;
  CHAR16  StrTemp[20];
  UINT16  *OptionBuffer;
  UINT16  OptionNumber;
  UINTN   OptionSize;

  OrderListSize = 0;
  OrderList     = NULL;
  OptionNumber  = 0;
  Index         = 0;

  UnicodeSPrint (StrTemp, sizeof (StrTemp), L"%sOrder", Type);

  GetEfiGlobalVariable2 (StrTemp, (VOID **)&OrderList, &OrderListSize);
  for (OptionNumber = 0; ; OptionNumber++) {
    if (OrderList != NULL) {
      for (Index = 0; Index < OrderListSize / sizeof (UINT16); Index++) {
        if (OptionNumber == OrderList[Index]) {
          break;
        }
      }
    }

    if (Index < OrderListSize / sizeof (UINT16)) {
      //
      // The OptionNumber occurs in the OrderList, continue to use next one
      //
      continue;
    }

    UnicodeSPrint (StrTemp, sizeof (StrTemp), L"%s%04x", Type, (UINTN)OptionNumber);
    DEBUG ((DEBUG_ERROR, "Option = %s\n", StrTemp));
    GetEfiGlobalVariable2 (StrTemp, (VOID **)&OptionBuffer, &OptionSize);
    if (NULL == OptionBuffer) {
      //
      // The Boot[OptionNumber] / Driver[OptionNumber] NOT occurs, we found it
      //
      break;
    }
  }

  return OptionNumber;
}

/**

  Get the Option Number for Boot#### that does not used.

  @return The available Option Number.

**/
UINT16
BOpt_GetBootOptionNumber (
  VOID
  )
{
  return BOpt_GetOptionNumber (L"Boot");
}

/**
  Get option number according to Boot#### and BootOrder variable.
  The value is saved as #### + 1.

  @param CallbackData    The BMM context data.
**/
VOID
GetBootOrder (
  IN  BMM_CALLBACK_DATA  *CallbackData
  )
{
  BMM_FAKE_NV_DATA  *BmmConfig;
  UINT16            Index;
  UINT16            OptionOrderIndex;
  BM_MENU_ENTRY     *NewMenuEntry;

  ASSERT (CallbackData != NULL);

  BmmConfig  = &CallbackData->BmmFakeNvData;
  ZeroMem (BmmConfig->BootOptionOrder, sizeof (BmmConfig->BootOptionOrder));

  for (Index = 0, OptionOrderIndex = 0; ((Index < BootOptionMenu.MenuNumber) &&
                                         (OptionOrderIndex < (sizeof (BmmConfig->BootOptionOrder) / sizeof (BmmConfig->BootOptionOrder[0]))));
       Index++)
  {
    NewMenuEntry   = BOpt_GetMenuEntry (&BootOptionMenu, Index);
    BmmConfig->BootOptionOrder[OptionOrderIndex++] = (UINT32)(NewMenuEntry->OptionNumber + 1);
  }
}
