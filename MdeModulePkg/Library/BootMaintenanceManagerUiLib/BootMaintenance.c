/** @file
The functions for Boot Maintainence Main menu.

Copyright (c) 2004 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BootMaintenanceManager.h"

#define FRONT_PAGE_KEY_OFFSET          0x4000

HII_VENDOR_DEVICE_PATH  mBmmHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    //
    // {165A028F-0BB2-4b5f-8747-77592E3F6499}
    //
    { 0x165a028f, 0xbb2, 0x4b5f, { 0x87, 0x47, 0x77, 0x59, 0x2e, 0x3f, 0x64, 0x99 } }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

EFI_GUID mBootMaintGuid = BOOT_MAINT_FORMSET_GUID;
CHAR16  mBootMaintStorageName[] = L"BmmData";

EFI_STATUS
EFIAPI
ExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  );

EFI_STATUS
EFIAPI
RouteConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN CONST EFI_STRING                     Configuration,
  OUT EFI_STRING                          *Progress
  );

EFI_STATUS
EFIAPI
DriverCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL         *This,
  IN        EFI_BROWSER_ACTION                     Action,
  IN        EFI_QUESTION_ID                        QuestionId,
  IN        UINT8                                  Type,
  IN        EFI_IFR_TYPE_VALUE                     *Value,
  OUT       EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  );


STATIC BMM_CALLBACK_DATA mBootMaintenancePrivate = {
  BMM_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  {
    ExtractConfig,
    RouteConfig,
    DriverCallback
  }
};

///
/// Boot Option from variable Menu
///
BM_MENU_OPTION      BootOptionMenu = {
  BM_MENU_OPTION_SIGNATURE,
  {NULL},
  0
};

STATIC BMM_CALLBACK_DATA *mBmmCallbackInfo = &mBootMaintenancePrivate;
BOOLEAN  mAllMenuInit               = FALSE;

/**
  This function converts an input device structure to a Unicode string.

  @param DevPath      A pointer to the device path structure.

  @return             A new allocated Unicode string that represents the device path.

**/
CHAR16 *
UiDevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath
  )
{
  EFI_STATUS                       Status;
  CHAR16                           *ToText;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL *DevPathToText;

  if (DevPath == NULL) {
    return NULL;
  }

  Status = gBS->LocateProtocol (
                  &gEfiDevicePathToTextProtocolGuid,
                  NULL,
                  (VOID **) &DevPathToText
                  );
  ASSERT_EFI_ERROR (Status);
  ToText = DevPathToText->ConvertDevicePathToText (
                            DevPath,
                            FALSE,
                            TRUE
                            );
  ASSERT (ToText != NULL);
  return ToText;
}

/**
  Extract device path for given HII handle and class guid.

  @param Handle          The HII handle.

  @retval  NULL          Fail to get the device path string.
  @return  PathString    Get the device path string.

**/
CHAR16 *
BmmExtractDevicePathFromHiiHandle (
  IN      EFI_HII_HANDLE      Handle
  )
{
  EFI_STATUS                       Status;
  EFI_HANDLE                       DriverHandle;

  ASSERT (Handle != NULL);

  if (Handle == NULL) {
    return NULL;
  }

  Status = gHiiDatabase->GetPackageListHandle (gHiiDatabase, Handle, &DriverHandle);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Get device path string.
  //
  return ConvertDevicePathToText(DevicePathFromHandle (DriverHandle), FALSE, FALSE);

}

/**
  Converts the unicode character of the string from uppercase to lowercase.
  This is a internal function.

  @param ConfigString  String to be converted

**/
VOID
HiiToLower (
  IN EFI_STRING  ConfigString
  )
{
  EFI_STRING  String;
  BOOLEAN     Lower;

  ASSERT (ConfigString != NULL);

  //
  // Convert all hex digits in range [A-F] in the configuration header to [a-f]
  //
  for (String = ConfigString, Lower = FALSE; *String != L'\0'; String++) {
    if (*String == L'=') {
      Lower = TRUE;
    } else if (*String == L'&') {
      Lower = FALSE;
    } else if (Lower && *String >= L'A' && *String <= L'F') {
      *String = (CHAR16) (*String - L'A' + L'a');
    }
  }
}

/**
  Update the progress string through the offset value.

  @param Offset           The offset value
  @param Configuration    Point to the configuration string.

**/
EFI_STRING
UpdateProgress(
  IN  UINTN       Offset,
  IN  EFI_STRING  Configuration
)
{
  UINTN       Length;
  EFI_STRING  StringPtr;
  EFI_STRING  ReturnString;

  StringPtr    = NULL;
  ReturnString = NULL;

  //
  // &OFFSET=XXXX followed by a Null-terminator.
  // Length = StrLen (L"&OFFSET=") + 4 + 1
  //
  Length    = StrLen (L"&OFFSET=") + 4 + 1;

  StringPtr = AllocateZeroPool (Length * sizeof (CHAR16));

  if (StringPtr == NULL) {
    return  NULL;
  }

  UnicodeSPrint (
    StringPtr,
    (8 + 4 + 1) * sizeof (CHAR16),
    L"&OFFSET=%04x",
    Offset
    );

  ReturnString = StrStr (Configuration, StringPtr);

  if (ReturnString == NULL) {
    //
    // If doesn't find the string in Configuration, convert the string to lower case then search again.
    //
    HiiToLower (StringPtr);
    ReturnString = StrStr (Configuration, StringPtr);
  }

  FreePool (StringPtr);

  return ReturnString;
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
STATIC
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

STATIC
VOID
UpdateBootOrderList(
  IN BMM_CALLBACK_DATA *CallbackData
  )
{
  EFI_IFR_GUID_LABEL  *StartLabel;
  EFI_IFR_GUID_LABEL  *EndLabel;
  VOID                *StartOpCodeHandle;
  VOID                *EndOpCodeHandle;

  BM_MENU_ENTRY     *NewMenuEntry;
  UINT16            Index;
  UINT16            OptionIndex;
  VOID              *OptionsOpCodeHandle;
  BOOLEAN           BootOptionFound;
  UINT32            *OptionOrder = NULL;

  StartOpCodeHandle = HiiAllocateOpCodeHandle();
  ASSERT(StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle();
  ASSERT(EndOpCodeHandle != NULL);

  // Create Hii Extend Label OpCode as the start opcode
  StartLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode(StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number = FORM_BOOT_CHG_ID;

  // Create Hii Extend Label OpCode as the end opcode
  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number = LABEL_END;

  HiiUpdateForm(
    CallbackData->BmmHiiHandle,
    &mBootMaintGuid,
    FORM_BOOT_CHG_ID,
    StartOpCodeHandle,  // FORM_BOOT_CHG_ID
    EndOpCodeHandle     // LABEL_END
    );

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
  ASSERT (OptionOrder != NULL);

  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  NewMenuEntry = NULL;
  for (OptionIndex = 0; (OptionOrder[OptionIndex] != 0 && OptionIndex < MAX_MENU_NUMBER); OptionIndex++) {
    BootOptionFound = FALSE;
    for (Index = 0; Index < BootOptionMenu.MenuNumber; Index++) {
      NewMenuEntry   = BOpt_GetMenuEntry (&BootOptionMenu, Index);
      if ((UINT32) (NewMenuEntry->OptionNumber + 1) == OptionOrder[OptionIndex]) {
        BootOptionFound = TRUE;
        break;
      }
    }
    if (BootOptionFound) {
      HiiCreateOneOfOptionOpCode (
        OptionsOpCodeHandle,
        NewMenuEntry->DisplayStringToken,
        EFI_IFR_FLAG_CALLBACK,
        EFI_IFR_TYPE_NUM_SIZE_32,
        OptionOrder[OptionIndex]
        );
    }
  }

  if (BootOptionMenu.MenuNumber > 0) {
    HiiCreateOrderedListOpCode (
      StartOpCodeHandle,                           // Container for dynamic created opcodes
      BOOT_OPTION_ORDER_QUESTION_ID,               // Question ID
      VARSTORE_ID_BOOT_MAINT,                      // VarStore ID
      BOOT_OPTION_ORDER_VAR_OFFSET,                // Offset in Buffer Storage
      STRING_TOKEN (STR_CHANGE_BOOT_ORDER),        // Question prompt text
      STRING_TOKEN (STR_CHANGE_BOOT_ORDER),        // Question help text
      EFI_IFR_FLAG_CALLBACK,                       // Question flag
      EFI_IFR_UNIQUE_SET,                          // Ordered list flag, e.g. EFI_IFR_UNIQUE_SET
      EFI_IFR_TYPE_NUM_SIZE_32,                    // Data type of Question value
      MAX_MENU_NUMBER,                             // Maximum container
      OptionsOpCodeHandle,                         // Option Opcode list
      NULL                                         // Default Opcode is NULL
      );
  }

  HiiUpdateForm(
    CallbackData->BmmHiiHandle,
    &mBootMaintGuid,
    FORM_BOOT_CHG_ID,
    StartOpCodeHandle,  // FORM_BOOT_CHG_ID
    EndOpCodeHandle     // LABEL_END
    );

  HiiFreeOpCodeHandle(OptionsOpCodeHandle);
  HiiFreeOpCodeHandle(EndOpCodeHandle);
  HiiFreeOpCodeHandle(StartOpCodeHandle);

}

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Request         A null-terminated Unicode string in <ConfigRequest> format.
  @param Progress        On return, points to a character in the Request string.
                         Points to the string's null terminator if request was successful.
                         Points to the most recent '&' before the first failing name/value
                         pair (or the beginning of the string if the failure is in the
                         first name/value pair) if the request was not successful.
  @param Results         A null-terminated Unicode string in <ConfigAltResp> format which
                         has all values filled in for the names in the Request string.
                         String to be allocated by the called function.

  @retval  EFI_SUCCESS            The Results is filled with the requested values.
  @retval  EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval  EFI_INVALID_PARAMETER  Request is NULL, illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
ExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS         Status;
  UINTN              BufferSize;
  BMM_CALLBACK_DATA  *Private;
  EFI_STRING                       ConfigRequestHdr;
  EFI_STRING                       ConfigRequest;
  BOOLEAN                          AllocatedRequest;
  UINTN                            Size;

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &mBootMaintGuid, mBootMaintStorageName)) {
    return EFI_NOT_FOUND;
  }

  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  AllocatedRequest = FALSE;
  Size             = 0;

  Private = BMM_CALLBACK_DATA_FROM_THIS (This);
  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  BufferSize = sizeof (BMM_FAKE_NV_DATA);
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&mBootMaintGuid, mBootMaintStorageName, Private->BmmDriverHandle);
    Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }

  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *) &Private->BmmFakeNvData,
                                BufferSize,
                                Results,
                                Progress
                                );
  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
    ConfigRequest = NULL;
  }
  //
  // Set Progress string to the original request string.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return Status;
}

/**
  This function applies changes in a driver's configuration.
  Input is a Configuration, which has the routing data for this
  driver followed by name / value configuration pairs. The driver
  must apply those pairs to its configurable storage. If the
  driver's configuration is stored in a linear block of data
  and the driver's name / value pairs are in <BlockConfig>
  format, it may use the ConfigToBlock helper function (above) to
  simplify the job. Currently not implemented.

  @param[in]  This                Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration       A null-terminated Unicode string in
                                  <ConfigString> format.
  @param[out] Progress            A pointer to a string filled in with the
                                  offset of the most recent '&' before the
                                  first failing name / value pair (or the
                                  beginn ing of the string if the failure
                                  is in the first name / value pair) or
                                  the terminating NULL if all was
                                  successful.

  @retval EFI_SUCCESS             The results have been distributed or are
                                  awaiting distribution.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the
                                  parts of the results that must be
                                  stored awaiting possible future
                                  protocols.
  @retval EFI_INVALID_PARAMETERS  Passing in a NULL for the
                                  Results parameter would result
                                  in this type of error.
  @retval EFI_NOT_FOUND           Target for the specified routing data
                                  was not found.
**/
EFI_STATUS
EFIAPI
RouteConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN CONST EFI_STRING                     Configuration,
  OUT EFI_STRING                          *Progress
  )
{
  EFI_STATUS                      Status;
  UINTN                           BufferSize;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *ConfigRouting;
  BMM_FAKE_NV_DATA                *NewBmmData;
  BMM_FAKE_NV_DATA                *OldBmmData;
  BMM_CALLBACK_DATA               *Private;
  UINTN                           Offset;

  if (Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Progress = Configuration;

  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check routing data in <ConfigHdr>.
  // Note: there is no name for Name/Value storage, only GUID will be checked
  //
  if (!HiiIsConfigHdrMatch (Configuration, &mBootMaintGuid, mBootMaintStorageName)) {
    return EFI_NOT_FOUND;
  }

  Status = gBS->LocateProtocol (
                  &gEfiHiiConfigRoutingProtocolGuid,
                  NULL,
                  (VOID **)&ConfigRouting
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private = BMM_CALLBACK_DATA_FROM_THIS (This);
  //
  // Get Buffer Storage data from EFI variable
  //
  BufferSize = sizeof (BMM_FAKE_NV_DATA);
  OldBmmData = &Private->BmmOldFakeNVData;
  NewBmmData = &Private->BmmFakeNvData;
  Offset     = 0;
  //
  // Convert <ConfigResp> to buffer data by helper function ConfigToBlock()
  //
  Status = ConfigRouting->ConfigToBlock (
                            ConfigRouting,
                            Configuration,
                            (UINT8 *) NewBmmData,
                            &BufferSize,
                            Progress
                            );
  ASSERT_EFI_ERROR (Status);
  //
  // Compare new and old BMM configuration data and only do action for modified item to
  // avoid setting unnecessary non-volatile variable
  //

  if (CompareMem (NewBmmData->BootOptionOrder, OldBmmData->BootOptionOrder, sizeof (NewBmmData->BootOptionOrder)) != 0) {
    Status = Var_UpdateBootOrder (Private);
    if (EFI_ERROR (Status)) {
      Offset = OFFSET_OF (BMM_FAKE_NV_DATA, BootOptionOrder);
      goto Exit;
    }
  }

  //
  // After user do the save action, need to update OldBmmData.
  //
  CopyMem (OldBmmData, NewBmmData, sizeof (BMM_FAKE_NV_DATA));

  return EFI_SUCCESS;

Exit:
  //
  // Fail to save the data, update the progress string.
  //
  *Progress = UpdateProgress (Offset, Configuration);
  if (Status == EFI_OUT_OF_RESOURCES) {
    return Status;
  } else {
    return EFI_NOT_FOUND;
  }
}

/**
  This function processes the results of changes in configuration.


  @param This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action             Specifies the type of action taken by the browser.
  @param QuestionId         A unique value which is sent to the original exporting driver
                            so that it can identify the type of data to expect.
  @param Type               The type of value for the question.
  @param Value              A pointer to the data being sent to the original exporting driver.
  @param ActionRequest      On return, points to the action requested by the callback function.

  @retval EFI_SUCCESS           The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES  Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR      The variable could not be saved.
  @retval EFI_UNSUPPORTED       The specified Action is not supported by the callback.
  @retval EFI_INVALID_PARAMETER The parameter of Value or ActionRequest is invalid.
**/
EFI_STATUS
EFIAPI
DriverCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL         *This,
  IN        EFI_BROWSER_ACTION                     Action,
  IN        EFI_QUESTION_ID                        QuestionId,
  IN        UINT8                                  Type,
  IN        EFI_IFR_TYPE_VALUE                     *Value,
  OUT       EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  BMM_CALLBACK_DATA *Private = BMM_CALLBACK_DATA_FROM_THIS(This);
  BMM_FAKE_NV_DATA  *CurrentFakeNVMap = &Private->BmmFakeNvData;
  EFI_STATUS Status = EFI_UNSUPPORTED;

  // Retrieve uncommitted data from Form Browser
  HiiGetBrowserData (&mBootMaintGuid, mBootMaintStorageName, sizeof (BMM_FAKE_NV_DATA), (UINT8 *) CurrentFakeNVMap);

  switch (Action) {
  case EFI_BROWSER_ACTION_FORM_OPEN:
    // Dispatch the display to the next page
    UpdateBootOrderList(Private);
    Status = EFI_SUCCESS;
    break;

  case EFI_BROWSER_ACTION_CHANGED:
    switch (QuestionId) {
    case FORM_BOOT_CHG_ID:
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
      Status = EFI_SUCCESS;
      break;

    default:
      break;
    }

    break;

  default:
    break;
  }

  if (Status == EFI_SUCCESS) {
    // Pass changed uncommitted data back to Form Browser
    HiiSetBrowserData (&mBootMaintGuid, mBootMaintStorageName, sizeof(BMM_FAKE_NV_DATA), (UINT8 *)CurrentFakeNVMap, NULL);
  }

  return Status;
}

/**
   Create dynamic code for BMM and initialize all of BMM configuration data in BmmFakeNvData and
   BmmOldFakeNVData member in BMM context data.

  @param CallbackData    The BMM context data.

**/
VOID
InitializeBmmConfig (
  IN  BMM_CALLBACK_DATA    *CallbackData
  )
{
  ASSERT (CallbackData != NULL);

  //
  // Initialize data which located in Boot Options Menu
  //
  GetBootOrder (CallbackData);

  //
  // Backup Initialize BMM configuartion data to BmmOldFakeNVData
  //
  CopyMem (&CallbackData->BmmOldFakeNVData, &CallbackData->BmmFakeNvData, sizeof (BMM_FAKE_NV_DATA));
}

STATIC
VOID
InitAllMenu (
  IN  BMM_CALLBACK_DATA    *CallbackData
  )
{
  InitializeListHead (&BootOptionMenu.Head);
  BOpt_GetBootOptions (CallbackData);
  mAllMenuInit = TRUE;
}

STATIC
VOID
FreeAllMenu (VOID)
{
  if (!mAllMenuInit) {
    return;
  }
  BOpt_FreeMenu (&BootOptionMenu);
  mAllMenuInit = FALSE;
}

/**

  Install Boot Maintenance Manager Menu driver.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCEESS  Install Boot manager menu success.
  @retval  Other        Return error status.

**/
EFI_STATUS
EFIAPI
BootMaintenanceManagerUiLibConstructor (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )

{
  EFI_STATUS               Status;
  UINT8                    *Ptr;

  Status = EFI_SUCCESS;

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mBmmCallbackInfo->BmmDriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mBmmHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &mBmmCallbackInfo->BmmConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Post our Boot Maint VFR binary to the HII database.
  //
  mBmmCallbackInfo->BmmHiiHandle = HiiAddPackages (
                                    &mBootMaintGuid,
                                    mBmmCallbackInfo->BmmDriverHandle,
                                    BootMaintenanceManagerBin,
                                    BootMaintenanceManagerUiLibStrings,
                                    NULL
                                    );
  ASSERT (mBmmCallbackInfo->BmmHiiHandle != NULL);

  //
  // Locate Formbrowser2 protocol
  //
  Status = gBS->LocateProtocol (&gEfiFormBrowser2ProtocolGuid, NULL, (VOID **) &mBmmCallbackInfo->FormBrowser2);
  ASSERT_EFI_ERROR (Status);

  //
  // Create LoadOption in BmmCallbackInfo for Driver Callback
  //
  Ptr = AllocateZeroPool (sizeof (BM_LOAD_CONTEXT) + sizeof (BM_FILE_CONTEXT) + sizeof (BM_HANDLE_CONTEXT) + sizeof (BM_MENU_ENTRY));
  ASSERT (Ptr != NULL);

  //
  // Initialize Bmm callback data.
  //
  mBmmCallbackInfo->LoadContext = (BM_LOAD_CONTEXT *) Ptr;
  Ptr += sizeof (BM_LOAD_CONTEXT);

  mBmmCallbackInfo->FileContext = (BM_FILE_CONTEXT *) Ptr;
  Ptr += sizeof (BM_FILE_CONTEXT);

  mBmmCallbackInfo->HandleContext = (BM_HANDLE_CONTEXT *) Ptr;
  Ptr += sizeof (BM_HANDLE_CONTEXT);

  mBmmCallbackInfo->MenuEntry     = (BM_MENU_ENTRY *) Ptr;

  InitAllMenu (mBmmCallbackInfo);

  // Update boot maintenance manager page
  InitializeBmmConfig(mBmmCallbackInfo);

  return EFI_SUCCESS;
}

/**
  Unloads the application and its installed protocol.

  @param ImageHandle       Handle that identifies the image to be unloaded.
  @param  SystemTable      The system table.

  @retval EFI_SUCCESS      The image has been unloaded.

**/
EFI_STATUS
EFIAPI
BootMaintenanceManagerUiLibDestructor (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )

{
  FreeAllMenu ();

  //
  // Remove our IFR data from HII database
  //
  HiiRemovePackages (mBmmCallbackInfo->BmmHiiHandle);

  gBS->UninstallMultipleProtocolInterfaces (
         mBmmCallbackInfo->BmmDriverHandle,
         &gEfiDevicePathProtocolGuid,
         &mBmmHiiVendorDevicePath,
         &gEfiHiiConfigAccessProtocolGuid,
         &mBmmCallbackInfo->BmmConfigAccess,
         NULL
         );

  FreePool (mBmmCallbackInfo->LoadContext);
  mBmmCallbackInfo->BmmDriverHandle = NULL;

  return EFI_SUCCESS;
}

