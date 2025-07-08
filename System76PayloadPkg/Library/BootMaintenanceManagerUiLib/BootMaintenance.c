/** @file
The functions for Boot Maintainence Main menu.

Copyright (c) 2004 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BootMaintenanceManager.h"

#define FRONT_PAGE_KEY_OFFSET  0x4000

HII_VENDOR_DEVICE_PATH  mBmmHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    //
    // {165A028F-0BB2-4b5f-8747-77592E3F6499}
    //
    { 0x165a028f, 0xbb2, 0x4b5f, { 0x87, 0x47, 0x77, 0x59, 0x2e, 0x3f, 0x64, 0x99 }
    }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(END_DEVICE_PATH_LENGTH),
      (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

EFI_GUID  mBootMaintGuid = BOOT_MAINT_FORMSET_GUID;

CHAR16             mBootMaintStorageName[] = L"BmmData";
BMM_CALLBACK_DATA  gBootMaintenancePrivate = {
  BMM_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  {
    BootMaintExtractConfig,
    BootMaintRouteConfig,
    BootMaintCallback
  }
};

BMM_CALLBACK_DATA  *mBmmCallbackInfo  = &gBootMaintenancePrivate;
BOOLEAN            mAllMenuInit       = FALSE;

/**
  This function converts an input device structure to a Unicode string.

  @param DevPath      A pointer to the device path structure.

  @return             A new allocated Unicode string that represents the device path.

**/
CHAR16 *
UiDevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevPath
  )
{
  EFI_STATUS                        Status;
  CHAR16                            *ToText;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL  *DevPathToText;

  if (DevPath == NULL) {
    return NULL;
  }

  Status = gBS->LocateProtocol (
                  &gEfiDevicePathToTextProtocolGuid,
                  NULL,
                  (VOID **)&DevPathToText
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
    } else if (Lower && (*String >= L'A') && (*String <= L'F')) {
      *String = (CHAR16)(*String - L'A' + L'a');
    }
  }
}

/**
  Update the progress string through the offset value.

  @param Offset           The offset value
  @param Configuration    Point to the configuration string.

**/
EFI_STRING
UpdateProgress (
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
  Length = StrLen (L"&OFFSET=") + 4 + 1;

  StringPtr = AllocateZeroPool (Length * sizeof (CHAR16));

  if (StringPtr == NULL) {
    return NULL;
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
BootMaintExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Request,
  OUT EFI_STRING                            *Progress,
  OUT EFI_STRING                            *Results
  )
{
  EFI_STATUS         Status;
  UINTN              BufferSize;
  BMM_CALLBACK_DATA  *Private;
  EFI_STRING         ConfigRequestHdr;
  EFI_STRING         ConfigRequest;
  BOOLEAN            AllocatedRequest;
  UINTN              Size;

  if ((Progress == NULL) || (Results == NULL)) {
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
  BufferSize    = sizeof (BMM_FAKE_NV_DATA);
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&mBootMaintGuid, mBootMaintStorageName, Private->BmmDriverHandle);
    Size             = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest    = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }

  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *)&Private->BmmFakeNvData,
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
BootMaintRouteConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                           *Progress
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *ConfigRouting;
  BMM_FAKE_NV_DATA                 *NewBmmData;
  BMM_FAKE_NV_DATA                 *OldBmmData;
  BMM_CALLBACK_DATA                *Private;
  UINTN                            Offset;

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
                            (UINT8 *)NewBmmData,
                            &BufferSize,
                            Progress
                            );
  ASSERT_EFI_ERROR (Status);

  // Compare new and old BMM configuration data and only do action for modified item to
  // avoid setting unnecessary non-volatile variable
  if (CompareMem (NewBmmData->BootOptionOrder, OldBmmData->BootOptionOrder, sizeof (NewBmmData->BootOptionOrder)) != 0) {
    Status = Var_UpdateBootOrder (Private);
    if (EFI_ERROR (Status)) {
      Offset = OFFSET_OF (BMM_FAKE_NV_DATA, BootOptionOrder);
      goto Exit;
    }
  }

  // After user do the save action, need to update OldBmmData.
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
  Discard all changes done to the BMM pages such as Boot Order change,
  Driver order change.

  @param Private            The BMM context data.
  @param CurrentFakeNVMap   The current Fack NV Map.

**/
STATIC
VOID
DiscardChangeHandler (
  IN  BMM_CALLBACK_DATA  *Private,
  IN  BMM_FAKE_NV_DATA   *CurrentFakeNVMap
  )
{
  CopyMem (CurrentFakeNVMap->BootOptionOrder, Private->BmmOldFakeNVData.BootOptionOrder, sizeof (CurrentFakeNVMap->BootOptionOrder));
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
BootMaintCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN        EFI_BROWSER_ACTION              Action,
  IN        EFI_QUESTION_ID                 QuestionId,
  IN        UINT8                           Type,
  IN        EFI_IFR_TYPE_VALUE              *Value,
  OUT       EFI_BROWSER_ACTION_REQUEST      *ActionRequest
  )
{
  BMM_CALLBACK_DATA         *Private;
  BMM_FAKE_NV_DATA          *CurrentFakeNVMap;

  if ((Action != EFI_BROWSER_ACTION_CHANGING) && (Action != EFI_BROWSER_ACTION_CHANGED) && (Action != EFI_BROWSER_ACTION_FORM_OPEN)) {
    // Do nothing for other UEFI Action. Only do call back when data is changed or the form is open.
    return EFI_UNSUPPORTED;
  }

  Private = BMM_CALLBACK_DATA_FROM_THIS (This);

  if (Action == EFI_BROWSER_ACTION_FORM_OPEN) {
    DEBUG ((DEBUG_INFO, "EFI_BROWSER_ACTION_FORM_OPEN: 0x%0X\n", QuestionId));
    if (QuestionId == KEY_VALUE_TRIGGER_FORM_OPEN_ACTION) {
      CleanUpPage (FORM_BOOT_CHG_ID, Private);
      UpdateBootOrderPage (&BootOptionMenu, Private);

      EfiBootManagerRefreshAllBootOption ();
      BOpt_GetBootOptions (Private);
    }
  }

  // Retrieve uncommitted data from Form Browser
  CurrentFakeNVMap = &Private->BmmFakeNvData;
  HiiGetBrowserData (&mBootMaintGuid, mBootMaintStorageName, sizeof (BMM_FAKE_NV_DATA), (UINT8 *)CurrentFakeNVMap);

  if (Action == EFI_BROWSER_ACTION_CHANGING) {
    DEBUG ((DEBUG_INFO, "EFI_BROWSER_ACTION_CHANGING: 0x%0X\n", QuestionId));
    if (Value == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    if (QuestionId < CONFIG_OPTION_OFFSET) {
      switch (QuestionId) {
        case FORM_BOOT_CHG_ID:
          CleanUpPage (FORM_BOOT_CHG_ID, Private);
          UpdateBootOrderPage (&BootOptionMenu, Private);
          break;

        default:
          break;
      }
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGED) {
    DEBUG ((DEBUG_INFO, "EFI_BROWSER_ACTION_CHANGED: 0x%0X\n", QuestionId));
    if ((Value == NULL) || (ActionRequest == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    switch (QuestionId) {
      case BOOT_OPTION_ORDER_QUESTION_ID:
        // Save BootOrder on list update
        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
        break;

      case KEY_VALUE_SAVE_AND_EXIT:
        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_SUBMIT_EXIT;
        break;

      case KEY_VALUE_NO_SAVE_AND_EXIT:
        DiscardChangeHandler (Private, CurrentFakeNVMap);
        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD_EXIT;
        break;

      default:
        break;
    }
  }

  // Pass changed uncommitted data back to Form Browser
  HiiSetBrowserData (&mBootMaintGuid, mBootMaintStorageName, sizeof (BMM_FAKE_NV_DATA), (UINT8 *)CurrentFakeNVMap, NULL);

  DEBUG ((DEBUG_INFO, "%a complete: %r\n", __func__, EFI_SUCCESS));
  return EFI_SUCCESS;
}

/**
   Create dynamic code for BMM and initialize all of BMM configuration data in BmmFakeNvData and
   BmmOldFakeNVData member in BMM context data.

  @param CallbackData    The BMM context data.

**/
STATIC
VOID
InitializeBmmConfig (
  IN  BMM_CALLBACK_DATA  *CallbackData
  )
{
  ASSERT (CallbackData != NULL);

  // Initialize data which located in Boot Options Menu
  GetBootOrder (CallbackData);

  // Backup Initialize BMM configuartion data to BmmOldFakeNVData
  CopyMem (&CallbackData->BmmOldFakeNVData, &CallbackData->BmmFakeNvData, sizeof (BMM_FAKE_NV_DATA));
}

/**
  Initialized all Menu Option List.

  @param CallbackData    The BMM context data.

**/
STATIC
VOID
InitAllMenu (
  IN  BMM_CALLBACK_DATA  *CallbackData
  )
{
  InitializeListHead (&BootOptionMenu.Head);
  BOpt_GetBootOptions (CallbackData);
  mAllMenuInit = TRUE;
}

/**
  Free up all Menu Option list.

**/
STATIC
VOID
FreeAllMenu (
  VOID
  )
{
  if (!mAllMenuInit) {
    return;
  }

  BOpt_FreeMenu (&BootOptionMenu);
  mAllMenuInit = FALSE;
}

/**
 * Remove the F9 and F10 hotkeys.
 */
STATIC
EFI_STATUS
UnregisterHotKeys (
  VOID
  )
{
  EFI_STATUS Status;
  EFI_INPUT_KEY HotKey;
  EDKII_FORM_BROWSER_EXTENSION2_PROTOCOL *FormBrowserEx2;
  Status = gBS->LocateProtocol (&gEdkiiFormBrowserEx2ProtocolGuid, NULL, (VOID **)&FormBrowserEx2);
  if (!EFI_ERROR (Status)) {
    HotKey.UnicodeChar = CHAR_NULL;
    HotKey.ScanCode = SCAN_F9;
    FormBrowserEx2->RegisterHotKey(&HotKey, BROWSER_ACTION_UNREGISTER, 0, NULL);
    HotKey.ScanCode = SCAN_F10;
    FormBrowserEx2->RegisterHotKey(&HotKey, BROWSER_ACTION_UNREGISTER, 0, NULL);
  }
  return Status;
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
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )

{
  EFI_STATUS  Status;
  UINT8       *Ptr;

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
  // Create LoadOption in BmmCallbackInfo for Driver Callback
  //
  Ptr = AllocateZeroPool (sizeof (BM_LOAD_CONTEXT) + sizeof (BM_MENU_ENTRY));
  ASSERT (Ptr != NULL);

  //
  // Initialize Bmm callback data.
  //
  mBmmCallbackInfo->LoadContext = (BM_LOAD_CONTEXT *)Ptr;
  Ptr                          += sizeof (BM_LOAD_CONTEXT);

  mBmmCallbackInfo->MenuEntry = (BM_MENU_ENTRY *)Ptr;

  mBmmCallbackInfo->BmmPreviousPageId = FORM_BOOT_CHG_ID;
  mBmmCallbackInfo->BmmCurrentPageId  = FORM_BOOT_CHG_ID;

  InitAllMenu (mBmmCallbackInfo);

  CreateUpdateData ();
  //
  // Update boot maintenance manager page
  //
  InitializeBmmConfig (mBmmCallbackInfo);

  UnregisterHotKeys();

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
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )

{
  if (mStartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mStartOpCodeHandle);
  }

  if (mEndOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mEndOpCodeHandle);
  }

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
