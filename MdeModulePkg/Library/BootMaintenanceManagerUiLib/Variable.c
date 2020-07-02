/** @file
Variable operation that will be used by bootmaint

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BootMaintenanceManager.h"

/**
  Delete Boot Option that represent a Deleted state in BootOptionMenu.

  @retval EFI_SUCCESS   If all boot load option EFI Variables corresponding to
                        BM_LOAD_CONTEXT marked for deletion is deleted.
  @retval EFI_NOT_FOUND If can not find the boot option want to be deleted.
  @return Others        If failed to update the "BootOrder" variable after deletion.

**/
EFI_STATUS
Var_DelBootOption (
  VOID
  )
{
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  EFI_STATUS      Status;
  UINTN           Index;
  UINTN           Index2;

  Index2  = 0;
  for (Index = 0; Index < BootOptionMenu.MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (&BootOptionMenu, (Index - Index2));
    if (NULL == NewMenuEntry) {
      return EFI_NOT_FOUND;
    }

    NewLoadContext = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
    if (!NewLoadContext->Deleted) {
      continue;
    }

    Status = EfiBootManagerDeleteLoadOptionVariable (NewMenuEntry->OptionNumber,LoadOptionTypeBoot);
    if (EFI_ERROR (Status)) {
     return Status;
    }
    Index2++;
    //
    // If current Load Option is the same as BootNext,
    // must delete BootNext in order to make sure
    // there will be no panic on next boot
    //
    if (NewLoadContext->IsBootNext) {
      EfiLibDeleteVariable (L"BootNext", &gEfiGlobalVariableGuid);
    }

    RemoveEntryList (&NewMenuEntry->Link);
    BOpt_DestroyMenuEntry (NewMenuEntry);
    NewMenuEntry = NULL;
  }

  BootOptionMenu.MenuNumber -= Index2;

  return EFI_SUCCESS;
}

/**
  Delete Load Option that represent a Deleted state in DriverOptionMenu.

  @retval EFI_SUCCESS       Load Option is successfully updated.
  @retval EFI_NOT_FOUND     Fail to find the driver option want to be deleted.
  @return Other value than EFI_SUCCESS if failed to update "Driver Order" EFI
          Variable.

**/
EFI_STATUS
Var_DelDriverOption (
  VOID
  )
{
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  EFI_STATUS      Status;
  UINTN           Index;
  UINTN           Index2;

  Index2  = 0;
  for (Index = 0; Index < DriverOptionMenu.MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (&DriverOptionMenu, (Index - Index2));
    if (NULL == NewMenuEntry) {
      return EFI_NOT_FOUND;
    }

    NewLoadContext = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
    if (!NewLoadContext->Deleted) {
      continue;
    }
    Status = EfiBootManagerDeleteLoadOptionVariable (NewMenuEntry->OptionNumber,LoadOptionTypeDriver);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Index2++;

    RemoveEntryList (&NewMenuEntry->Link);
    BOpt_DestroyMenuEntry (NewMenuEntry);
    NewMenuEntry = NULL;
  }

  DriverOptionMenu.MenuNumber -= Index2;

  return EFI_SUCCESS;
}

/**
  This function create a currently loaded Drive Option from
  the BMM. It then appends this Driver Option to the end of
  the "DriverOrder" list. It append this Driver Opotion to the end
  of DriverOptionMenu.

  @param CallbackData    The BMM context data.
  @param HiiHandle       The HII handle associated with the BMM formset.
  @param DescriptionData The description of this driver option.
  @param OptionalData    The optional load option.
  @param ForceReconnect  If to force reconnect.

  @retval other                Contain some errors when excuting this function.See function
                               EfiBootManagerInitializeLoadOption/EfiBootManagerAddLoadOptionVariabl
                               for detail return information.
  @retval EFI_SUCCESS          If function completes successfully.

**/
EFI_STATUS
Var_UpdateDriverOption (
  IN  BMM_CALLBACK_DATA         *CallbackData,
  IN  EFI_HII_HANDLE            HiiHandle,
  IN  UINT16                    *DescriptionData,
  IN  UINT16                    *OptionalData,
  IN  UINT8                     ForceReconnect
  )
{
  UINT16          Index;
  UINT16          DriverString[12];
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  BOOLEAN         OptionalDataExist;
  EFI_STATUS      Status;
  EFI_BOOT_MANAGER_LOAD_OPTION  LoadOption;
  UINT8                         *OptionalDesData;
  UINT32                        OptionalDataSize;

  OptionalDataExist = FALSE;
  OptionalDesData = NULL;
  OptionalDataSize = 0;

  Index             = BOpt_GetDriverOptionNumber ();
  UnicodeSPrint (
    DriverString,
    sizeof (DriverString),
    L"Driver%04x",
    Index
    );

  if (*DescriptionData == 0x0000) {
    StrCpyS (DescriptionData, MAX_MENU_NUMBER, DriverString);
  }

  if (*OptionalData != 0x0000) {
    OptionalDataExist = TRUE;
    OptionalDesData = (UINT8 *)OptionalData;
    OptionalDataSize = (UINT32)StrSize (OptionalData);
  }

  NewMenuEntry = BOpt_CreateMenuEntry (BM_LOAD_CONTEXT_SELECT);
  if (NULL == NewMenuEntry) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EfiBootManagerInitializeLoadOption (
             &LoadOption,
             Index,
             LoadOptionTypeDriver,
             LOAD_OPTION_ACTIVE | (ForceReconnect << 1),
             DescriptionData,
             CallbackData->LoadContext->FilePathList,
             OptionalDesData,
             OptionalDataSize
           );
  if (EFI_ERROR (Status)){
    return Status;
  }

  Status = EfiBootManagerAddLoadOptionVariable (&LoadOption,(UINTN) -1 );
  if (EFI_ERROR (Status)) {
    EfiBootManagerFreeLoadOption(&LoadOption);
    return Status;
  }

  NewLoadContext                  = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
  NewLoadContext->Deleted         = FALSE;
  NewLoadContext->Attributes = LoadOption.Attributes;
  NewLoadContext->FilePathListLength = (UINT16)GetDevicePathSize (LoadOption.FilePath);

  NewLoadContext->Description = AllocateZeroPool (StrSize (DescriptionData));
  ASSERT (NewLoadContext->Description != NULL);
  NewMenuEntry->DisplayString = NewLoadContext->Description;
  CopyMem (
    NewLoadContext->Description,
    LoadOption.Description,
    StrSize (DescriptionData)
    );

  NewLoadContext->FilePathList = AllocateZeroPool (GetDevicePathSize (CallbackData->LoadContext->FilePathList));
  ASSERT (NewLoadContext->FilePathList != NULL);
  CopyMem (
    NewLoadContext->FilePathList,
    LoadOption.FilePath,
    GetDevicePathSize (CallbackData->LoadContext->FilePathList)
    );

  NewMenuEntry->HelpString    = UiDevicePathToStr (NewLoadContext->FilePathList);
  NewMenuEntry->OptionNumber  = Index;
  NewMenuEntry->DisplayStringToken = HiiSetString (HiiHandle, 0, NewMenuEntry->DisplayString, NULL);
  NewMenuEntry->HelpStringToken = HiiSetString (HiiHandle, 0, NewMenuEntry->HelpString, NULL);

  if (OptionalDataExist) {
    NewLoadContext->OptionalData = AllocateZeroPool (LoadOption.OptionalDataSize);
    ASSERT (NewLoadContext->OptionalData != NULL);
    CopyMem (
      NewLoadContext->OptionalData,
      LoadOption.OptionalData,
      LoadOption.OptionalDataSize
      );
  }

  InsertTailList (&DriverOptionMenu.Head, &NewMenuEntry->Link);
  DriverOptionMenu.MenuNumber++;

  EfiBootManagerFreeLoadOption(&LoadOption);

  return EFI_SUCCESS;
}

/**
  This function create a currently loaded Boot Option from
  the BMM. It then appends this Boot Option to the end of
  the "BootOrder" list. It also append this Boot Opotion to the end
  of BootOptionMenu.

  @param CallbackData    The BMM context data.

  @retval other                Contain some errors when excuting this function. See function
                               EfiBootManagerInitializeLoadOption/EfiBootManagerAddLoadOptionVariabl
                               for detail return information.
  @retval EFI_SUCCESS          If function completes successfully.

**/
EFI_STATUS
Var_UpdateBootOption (
  IN  BMM_CALLBACK_DATA              *CallbackData
  )
{
  UINT16          BootString[10];
  UINT16          Index;
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  BOOLEAN         OptionalDataExist;
  EFI_STATUS      Status;
  BMM_FAKE_NV_DATA  *NvRamMap;
  EFI_BOOT_MANAGER_LOAD_OPTION  LoadOption;
  UINT8                         *OptionalData;
  UINT32                        OptionalDataSize;

  OptionalDataExist = FALSE;
  NvRamMap = &CallbackData->BmmFakeNvData;
  OptionalData = NULL;
  OptionalDataSize = 0;

  Index = BOpt_GetBootOptionNumber () ;
  UnicodeSPrint (BootString, sizeof (BootString), L"Boot%04x", Index);

  if (NvRamMap->BootDescriptionData[0] == 0x0000) {
    StrCpyS (NvRamMap->BootDescriptionData, sizeof (NvRamMap->BootDescriptionData) / sizeof (NvRamMap->BootDescriptionData[0]), BootString);
  }

  if (NvRamMap->BootOptionalData[0] != 0x0000) {
    OptionalDataExist = TRUE;
    OptionalData = (UINT8 *)NvRamMap->BootOptionalData;
    OptionalDataSize = (UINT32)StrSize (NvRamMap->BootOptionalData);
  }

  NewMenuEntry = BOpt_CreateMenuEntry (BM_LOAD_CONTEXT_SELECT);
  if (NULL == NewMenuEntry) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EfiBootManagerInitializeLoadOption (
             &LoadOption,
             Index,
             LoadOptionTypeBoot,
             LOAD_OPTION_ACTIVE,
             NvRamMap->BootDescriptionData,
             CallbackData->LoadContext->FilePathList,
             OptionalData,
             OptionalDataSize
           );
  if (EFI_ERROR (Status)){
    return Status;
  }

  Status = EfiBootManagerAddLoadOptionVariable (&LoadOption,(UINTN) -1 );
  if (EFI_ERROR (Status)) {
    EfiBootManagerFreeLoadOption(&LoadOption);
    return Status;
  }

  NewLoadContext                  = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
  NewLoadContext->Deleted         = FALSE;
  NewLoadContext->Attributes = LoadOption.Attributes;
  NewLoadContext->FilePathListLength = (UINT16) GetDevicePathSize (LoadOption.FilePath);

  NewLoadContext->Description = AllocateZeroPool (StrSize (NvRamMap->BootDescriptionData));
  ASSERT (NewLoadContext->Description != NULL);

  NewMenuEntry->DisplayString = NewLoadContext->Description;

  CopyMem (
    NewLoadContext->Description,
    LoadOption.Description,
    StrSize (NvRamMap->BootDescriptionData)
    );

  NewLoadContext->FilePathList = AllocateZeroPool (GetDevicePathSize (CallbackData->LoadContext->FilePathList));
  ASSERT (NewLoadContext->FilePathList != NULL);
  CopyMem (
    NewLoadContext->FilePathList,
    LoadOption.FilePath,
    GetDevicePathSize (CallbackData->LoadContext->FilePathList)
    );

  NewMenuEntry->HelpString    = UiDevicePathToStr (NewLoadContext->FilePathList);
  NewMenuEntry->OptionNumber  = Index;
  NewMenuEntry->DisplayStringToken = HiiSetString (CallbackData->BmmHiiHandle, 0, NewMenuEntry->DisplayString, NULL);
  NewMenuEntry->HelpStringToken = HiiSetString (CallbackData->BmmHiiHandle, 0, NewMenuEntry->HelpString, NULL);

  if (OptionalDataExist) {
    NewLoadContext->OptionalData = AllocateZeroPool (LoadOption.OptionalDataSize);
    ASSERT (NewLoadContext->OptionalData != NULL);
    CopyMem (
      NewLoadContext->OptionalData,
      LoadOption.OptionalData,
      LoadOption.OptionalDataSize
      );
  }

  InsertTailList (&BootOptionMenu.Head, &NewMenuEntry->Link);
  BootOptionMenu.MenuNumber++;

  EfiBootManagerFreeLoadOption(&LoadOption);

  return EFI_SUCCESS;
}

/**
  This function update the "BootNext" EFI Variable. If there is
  no "BootNext" specified in BMM, this EFI Variable is deleted.
  It also update the BMM context data specified the "BootNext"
  vaule.

  @param CallbackData    The BMM context data.

  @retval EFI_SUCCESS    The function complete successfully.
  @return                The EFI variable can be saved. See gRT->SetVariable
                         for detail return information.

**/
EFI_STATUS
Var_UpdateBootNext (
  IN BMM_CALLBACK_DATA            *CallbackData
  )
{
  BM_MENU_ENTRY     *NewMenuEntry;
  BM_LOAD_CONTEXT   *NewLoadContext;
  BMM_FAKE_NV_DATA  *CurrentFakeNVMap;
  UINT16            Index;
  EFI_STATUS        Status;

  Status            = EFI_SUCCESS;
  CurrentFakeNVMap  = &CallbackData->BmmFakeNvData;
  for (Index = 0; Index < BootOptionMenu.MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (&BootOptionMenu, Index);
    ASSERT (NULL != NewMenuEntry);

    NewLoadContext              = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
    NewLoadContext->IsBootNext  = FALSE;
  }

  if (CurrentFakeNVMap->BootNext == NONE_BOOTNEXT_VALUE) {
    EfiLibDeleteVariable (L"BootNext", &gEfiGlobalVariableGuid);
    return EFI_SUCCESS;
  }

  NewMenuEntry = BOpt_GetMenuEntry (
                  &BootOptionMenu,
                  CurrentFakeNVMap->BootNext
                  );
  ASSERT (NewMenuEntry != NULL);

  NewLoadContext = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
  Status = gRT->SetVariable (
                  L"BootNext",
                  &gEfiGlobalVariableGuid,
                  VAR_FLAG,
                  sizeof (UINT16),
                  &NewMenuEntry->OptionNumber
                  );
  NewLoadContext->IsBootNext              = TRUE;
  CallbackData->BmmOldFakeNVData.BootNext = CurrentFakeNVMap->BootNext;
  return Status;
}

/**
  This function update the "BootOrder" EFI Variable based on
  BMM Formset's NV map. It then refresh BootOptionMenu
  with the new "BootOrder" list.

  @param CallbackData    The BMM context data.

  @retval EFI_SUCCESS             The function complete successfully.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to complete the function.
  @return The EFI variable can not be saved. See gRT->SetVariable for detail return information.

**/
EFI_STATUS
Var_UpdateBootOrder (
  IN BMM_CALLBACK_DATA            *CallbackData
  )
{
  EFI_STATUS  Status;
  UINT16      Index;
  UINT16      OrderIndex;
  UINT16      *BootOrder;
  UINTN       BootOrderSize;
  UINT16      OptionNumber;

  //
  // First check whether BootOrder is present in current configuration
  //
  GetEfiGlobalVariable2 (L"BootOrder", (VOID **) &BootOrder, &BootOrderSize);
  if (BootOrder == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ASSERT (BootOptionMenu.MenuNumber <= (sizeof (CallbackData->BmmFakeNvData.BootOptionOrder) / sizeof (CallbackData->BmmFakeNvData.BootOptionOrder[0])));

  //
  // OptionOrder is subset of BootOrder
  //
  for (OrderIndex = 0; (OrderIndex < BootOptionMenu.MenuNumber) && (CallbackData->BmmFakeNvData.BootOptionOrder[OrderIndex] != 0); OrderIndex++) {
    for (Index = OrderIndex; Index < BootOrderSize / sizeof (UINT16); Index++) {
      if ((BootOrder[Index] == (UINT16) (CallbackData->BmmFakeNvData.BootOptionOrder[OrderIndex] - 1)) && (OrderIndex != Index)) {
        OptionNumber = BootOrder[Index];
        CopyMem (&BootOrder[OrderIndex + 1], &BootOrder[OrderIndex], (Index - OrderIndex) * sizeof (UINT16));
        BootOrder[OrderIndex] = OptionNumber;
      }
    }
  }

  Status = gRT->SetVariable (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  VAR_FLAG,
                  BootOrderSize,
                  BootOrder
                  );
  FreePool (BootOrder);

  BOpt_FreeMenu (&BootOptionMenu);
  BOpt_GetBootOptions (CallbackData);

  return Status;

}

/**
  This function update the "DriverOrder" EFI Variable based on
  BMM Formset's NV map. It then refresh DriverOptionMenu
  with the new "DriverOrder" list.

  @param CallbackData    The BMM context data.

  @retval EFI_SUCCESS           The function complete successfully.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory to complete the function.
  @return The EFI variable can not be saved. See gRT->SetVariable for detail return information.

**/
EFI_STATUS
Var_UpdateDriverOrder (
  IN BMM_CALLBACK_DATA            *CallbackData
  )
{
  EFI_STATUS  Status;
  UINT16      Index;
  UINT16      *DriverOrderList;
  UINT16      *NewDriverOrderList;
  UINTN       DriverOrderListSize;

  DriverOrderList     = NULL;
  DriverOrderListSize = 0;

  //
  // First check whether DriverOrder is present in current configuration
  //
  GetEfiGlobalVariable2 (L"DriverOrder", (VOID **) &DriverOrderList, &DriverOrderListSize);
  NewDriverOrderList = AllocateZeroPool (DriverOrderListSize);

  if (NewDriverOrderList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // If exists, delete it to hold new DriverOrder
  //
  if (DriverOrderList != NULL) {
    EfiLibDeleteVariable (L"DriverOrder", &gEfiGlobalVariableGuid);
    FreePool (DriverOrderList);
  }

  ASSERT (DriverOptionMenu.MenuNumber <= (sizeof (CallbackData->BmmFakeNvData.DriverOptionOrder) / sizeof (CallbackData->BmmFakeNvData.DriverOptionOrder[0])));
  for (Index = 0; Index < DriverOptionMenu.MenuNumber; Index++) {
    NewDriverOrderList[Index] = (UINT16) (CallbackData->BmmFakeNvData.DriverOptionOrder[Index] - 1);
  }

  Status = gRT->SetVariable (
                  L"DriverOrder",
                  &gEfiGlobalVariableGuid,
                  VAR_FLAG,
                  DriverOrderListSize,
                  NewDriverOrderList
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BOpt_FreeMenu (&DriverOptionMenu);
  BOpt_GetDriverOptions (CallbackData);
  return EFI_SUCCESS;
}
