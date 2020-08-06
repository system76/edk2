/** @file
  FrontPage routines to handle the callbacks and browser calls

Copyright (c) 2004 - 2017, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2018 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Protocol/UsbIo.h>
#include <Register/Amd/Cpuid.h>
#include <Register/Intel/Cpuid.h>
#include <Register/Intel/Msr.h>

#include "FrontPage.h"
#include "FrontPageCustomizedUi.h"

#define MAX_STRING_LEN            200

EFI_GUID   mFrontPageGuid      = FRONT_PAGE_FORMSET_GUID;

BOOLEAN   mResetRequired  = FALSE;

EFI_FORM_BROWSER2_PROTOCOL      *gFormBrowser2;
BOOLEAN   mModeInitialized = FALSE;
//
// Boot video resolution and text mode.
//
UINT32    mBootHorizontalResolution    = 0;
UINT32    mBootVerticalResolution      = 0;
UINT32    mBootTextModeColumn          = 0;
UINT32    mBootTextModeRow             = 0;
//
// BIOS setup video resolution and text mode.
//
UINT32    mSetupTextModeColumn         = 0;
UINT32    mSetupTextModeRow            = 0;
UINT32    mSetupHorizontalResolution   = 0;
UINT32    mSetupVerticalResolution     = 0;

EFI_SYSTEM_TABLE * gSystemTable = NULL;

FRONT_PAGE_CALLBACK_DATA  gFrontPagePrivate = {
  FRONT_PAGE_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  {
    FakeExtractConfig,
    FakeRouteConfig,
    FrontPageCallback
  }
};

HII_VENDOR_DEVICE_PATH  mFrontPageHiiVendorDevicePath = {
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
    // {8E6D99EE-7531-48f8-8745-7F6144468FF2}
    //
    { 0x8e6d99ee, 0x7531, 0x48f8, { 0x87, 0x45, 0x7f, 0x61, 0x44, 0x46, 0x8f, 0xf2 } }
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

/**
  Update the information for the Front Page based on Smbios information.

**/
VOID
UpdateFrontPageStrings (
  VOID
  );

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
  @retval  EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
FakeExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Progress = Request;
  return EFI_NOT_FOUND;
}

/**
  This function processes the results of changes in configuration.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Configuration   A null-terminated Unicode string in <ConfigResp> format.
  @param Progress        A pointer to a string filled in with the offset of the most
                         recent '&' before the first failing name/value pair (or the
                         beginning of the string if the failure is in the first
                         name/value pair) or the terminating NULL if all was successful.

  @retval  EFI_SUCCESS            The Results is processed successfully.
  @retval  EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
FakeRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;

  return EFI_NOT_FOUND;
}

/**
  This function processes the results of changes in configuration.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_OUT_OF_RESOURCES  Not enough storage is available to hold the variable and its data.
  @retval  EFI_DEVICE_ERROR      The variable could not be saved.
  @retval  EFI_UNSUPPORTED       The specified Action is not supported by the callback.

**/
EFI_STATUS
EFIAPI
FrontPageCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  return UiFrontPageCallbackHandler (gFrontPagePrivate.HiiHandle, Action, QuestionId, Type, Value, ActionRequest);
}

/**

  Update the menus in the front page.

**/
VOID
UpdateFrontPageForm (
  VOID
  )
{
  VOID                        *StartOpCodeHandle;
  VOID                        *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL          *StartGuidLabel;
  EFI_IFR_GUID_LABEL          *EndGuidLabel;

  //
  // Allocate space for creation of UpdateData Buffer
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);
  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartGuidLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartGuidLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartGuidLabel->Number       = LABEL_FRONTPAGE_INFORMATION;
  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndGuidLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndGuidLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndGuidLabel->Number       = LABEL_END;

  //
  //Updata Front Page form
  //
  UiCustomizeFrontPage (
    gFrontPagePrivate.HiiHandle,
    StartOpCodeHandle
    );

  HiiUpdateForm (
    gFrontPagePrivate.HiiHandle,
    &mFrontPageGuid,
    FRONT_PAGE_FORM_ID,
    StartOpCodeHandle,
    EndOpCodeHandle
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
}

/**
  Initialize HII information for the FrontPage


  @retval  EFI_SUCCESS        The operation is successful.
  @retval  EFI_DEVICE_ERROR   If the dynamic opcode creation failed.

**/
EFI_STATUS
InitializeFrontPage (
  VOID
  )
{
  EFI_STATUS                  Status;
  //
  // Locate Hii relative protocols
  //
  Status = gBS->LocateProtocol (&gEfiFormBrowser2ProtocolGuid, NULL, (VOID **) &gFormBrowser2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  gFrontPagePrivate.DriverHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gFrontPagePrivate.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mFrontPageHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gFrontPagePrivate.ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  gFrontPagePrivate.HiiHandle = HiiAddPackages (
                                  &mFrontPageGuid,
                                  gFrontPagePrivate.DriverHandle,
                                  FrontPageVfrBin,
                                  UiAppStrings,
                                  NULL
                                  );
  ASSERT (gFrontPagePrivate.HiiHandle != NULL);

  //
  //Updata Front Page strings
  //
  UpdateFrontPageStrings ();

  //
  // Update front page menus.
  //
  UpdateFrontPageForm();

  return Status;
}

/**
  Call the browser and display the front page

  @return   Status code that will be returned by
            EFI_FORM_BROWSER2_PROTOCOL.SendForm ().

**/
EFI_STATUS
CallFrontPage (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_BROWSER_ACTION_REQUEST  ActionRequest;

  //
  // Begin waiting for USER INPUT
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_PC_INPUT_WAIT)
    );

  ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  Status = gFormBrowser2->SendForm (
                            gFormBrowser2,
                            &gFrontPagePrivate.HiiHandle,
                            1,
                            &mFrontPageGuid,
                            0,
                            NULL,
                            &ActionRequest
                            );
  //
  // Check whether user change any option setting which needs a reset to be effective
  //
  if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
    EnableResetRequired ();
  }

  return Status;
}

/**
  Remove the installed packages from the HiiDatabase.

**/
VOID
FreeFrontPage(
  VOID
  )
{
  EFI_STATUS                  Status;
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  gFrontPagePrivate.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mFrontPageHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gFrontPagePrivate.ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  HiiRemovePackages (gFrontPagePrivate.HiiHandle);
}

/**
  Convert Processor Frequency Data to a string.

  @param ProcessorFrequency The frequency data to process
  @param Base10Exponent     The exponent based on 10
  @param String             The string that is created

**/
VOID
ConvertProcessorToString (
  IN  UINT16                               ProcessorFrequency,
  IN  UINT16                               Base10Exponent,
  OUT CHAR16                               **String
  )
{
  CHAR16  *StringBuffer;
  UINTN   Index;
  UINTN   DestMax;
  UINT32  FreqMhz;

  if (Base10Exponent >= 6) {
    FreqMhz = ProcessorFrequency;
    for (Index = 0; Index < (UINT32) Base10Exponent - 6; Index++) {
      FreqMhz *= 10;
    }
  } else {
    FreqMhz = 0;
  }
  DestMax = 0x20 / sizeof (CHAR16);
  StringBuffer = AllocateZeroPool (0x20);
  ASSERT (StringBuffer != NULL);
  UnicodeValueToStringS (StringBuffer, sizeof (CHAR16) * DestMax, LEFT_JUSTIFY, FreqMhz / 1000, 3);
  Index = StrnLenS (StringBuffer, DestMax);
  StrCatS (StringBuffer, DestMax, L".");
  UnicodeValueToStringS (
    StringBuffer + Index + 1,
    sizeof (CHAR16) * (DestMax - (Index + 1)),
    PREFIX_ZERO,
    (FreqMhz % 1000) / 10,
    2
    );
  StrCatS (StringBuffer, DestMax, L" GHz");
  *String = (CHAR16 *) StringBuffer;
  return ;
}


/**
  Convert Memory Size to a string.

  @param MemorySize      The size of the memory to process
  @param String          The string that is created

**/
VOID
ConvertMemorySizeToString (
  IN  UINT32          MemorySize,
  OUT CHAR16          **String
  )
{
  CHAR16  *StringBuffer;

  StringBuffer = AllocateZeroPool (0x24);
  ASSERT (StringBuffer != NULL);
  UnicodeValueToStringS (StringBuffer, 0x24, LEFT_JUSTIFY, MemorySize, 10);
  StrCatS (StringBuffer, 0x24 / sizeof (CHAR16), L" MB RAM");

  *String = (CHAR16 *) StringBuffer;

  return ;
}

/**

  Acquire the string associated with the Index from smbios structure and return it.
  The caller is responsible for free the string buffer.

  @param    OptionalStrStart  The start position to search the string
  @param    Index             The index of the string to extract
  @param    String            The string that is extracted

  @retval   EFI_SUCCESS       The function returns EFI_SUCCESS always.

**/
EFI_STATUS
GetOptionalStringByIndex (
  IN      CHAR8                   *OptionalStrStart,
  IN      UINT8                   Index,
  OUT     CHAR16                  **String
  )
{
  UINTN          StrSize;

  if (Index == 0) {
    *String = AllocateZeroPool (sizeof (CHAR16));
    return EFI_SUCCESS;
  }

  StrSize = 0;
  do {
    Index--;
    OptionalStrStart += StrSize;
    StrSize           = AsciiStrSize (OptionalStrStart);
  } while (OptionalStrStart[StrSize] != 0 && Index != 0);

  if ((Index != 0) || (StrSize == 1)) {
    //
    // Meet the end of strings set but Index is non-zero, or
    // Find an empty string
    //
    *String = GetStringById (STRING_TOKEN (STR_MISSING_STRING));
  } else {
    *String = AllocatePool (StrSize * sizeof (CHAR16));
    AsciiStrToUnicodeStrS (OptionalStrStart, *String, StrSize);
  }

  return EFI_SUCCESS;
}


UINT16 SmbiosTableLength (SMBIOS_STRUCTURE_POINTER SmbiosTableN)
{
  CHAR8  *AChar;
  UINT16  Length;

  AChar = (CHAR8 *)(SmbiosTableN.Raw + SmbiosTableN.Hdr->Length);
  while ((*AChar != 0) || (*(AChar + 1) != 0)) {
    AChar ++; //stop at 00 - first 0
  }
  Length = (UINT16)((UINTN)AChar - (UINTN)SmbiosTableN.Raw + 2); //length includes 00
  return Length;
}

SMBIOS_STRUCTURE_POINTER GetSmbiosTableFromType (
  SMBIOS_TABLE_ENTRY_POINT *SmbiosPoint, UINT8 SmbiosType, UINTN IndexTable)
{
  SMBIOS_STRUCTURE_POINTER SmbiosTableN;
  UINTN                    SmbiosTypeIndex;

  SmbiosTypeIndex = 0;
  SmbiosTableN.Raw = (UINT8 *)((UINTN)SmbiosPoint->TableAddress);
  if (SmbiosTableN.Raw == NULL) {
    return SmbiosTableN;
  }
  while ((SmbiosTypeIndex != IndexTable) || (SmbiosTableN.Hdr->Type != SmbiosType)) {
    if (SmbiosTableN.Hdr->Type == SMBIOS_TYPE_END_OF_TABLE) {
      SmbiosTableN.Raw = NULL;
      return SmbiosTableN;
    }
    if (SmbiosTableN.Hdr->Type == SmbiosType) {
      SmbiosTypeIndex++;
    }
    SmbiosTableN.Raw = (UINT8 *)(SmbiosTableN.Raw + SmbiosTableLength (SmbiosTableN));
  }
  return SmbiosTableN;
}

STATIC
VOID
WarnNoBootableMedia (
  VOID
  )
{
  CHAR16                        *String;
  EFI_STRING_ID                 Token;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption;
  UINTN                         BootOptionCount;
  UINTN                         Index;
  UINTN                         Count = 0;

  String = AllocateZeroPool (0x60);
  BootOption = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

  for (Index = 0; Index < BootOptionCount; Index++) {
    //
    // Don't count the hidden/inactive boot option
    //
    if (((BootOption[Index].Attributes & LOAD_OPTION_HIDDEN) != 0) || ((BootOption[Index].Attributes & LOAD_OPTION_ACTIVE) == 0)) {
      continue;
    }

    Count++;
  }

  EfiBootManagerFreeLoadOptions (BootOption, BootOptionCount);

  if (Count == 0) {
    StrCatS (String, 0x60 / sizeof (CHAR16), L"Warning: No bootable media found");
  } else {
    StrCatS (String, 0x60 / sizeof (CHAR16), L"");
  }

  Token = STRING_TOKEN (STR_NO_BOOTABLE_MEDIA);
  HiiSetString (gFrontPagePrivate.HiiHandle, Token, String, NULL);
  FreePool(String);
}

BOOLEAN
StandardSignatureIsAuthenticAMD (
  VOID
  )
{
  UINT32  RegEbx;
  UINT32  RegEcx;
  UINT32  RegEdx;

  AsmCpuid (CPUID_SIGNATURE, NULL, &RegEbx, &RegEcx, &RegEdx);
  return (RegEbx == CPUID_SIGNATURE_AUTHENTIC_AMD_EBX &&
          RegEcx == CPUID_SIGNATURE_AUTHENTIC_AMD_ECX &&
          RegEdx == CPUID_SIGNATURE_AUTHENTIC_AMD_EDX);
}

BOOLEAN
StandardSignatureIsGenuineIntel (
  VOID
  )
{
  UINT32  RegEbx;
  UINT32  RegEcx;
  UINT32  RegEdx;

  AsmCpuid (CPUID_SIGNATURE, NULL, &RegEbx, &RegEcx, &RegEdx);
  return (RegEbx == CPUID_SIGNATURE_GENUINE_INTEL_EBX &&
          RegEcx == CPUID_SIGNATURE_GENUINE_INTEL_ECX &&
          RegEdx == CPUID_SIGNATURE_GENUINE_INTEL_EDX);
}

typedef struct {
  CHAR8     Signature[8];
  UINT8     Checksum;
  CHAR8     OemId[6];
  UINT8     Revision;
  UINT32    RsdtAddress;
} ACPI_RSDP;

CHAR8 RSDP_SIGNATURE[8] = {'R', 'S', 'D', ' ', 'P', 'T', 'R', ' '};

typedef struct {
  CHAR8     Signature[4];
  UINT32    Length;
  UINT8     Revision;
  UINT8     Checksum;
  CHAR8     OemId[6];
  CHAR8     OemTableId[8];
  UINT32    OemRevision;
  UINT32    CreatorId;
  UINT32    CreatorRevision;
} ACPI_SDT_HEADER;

CHAR8 RSDT_SIGNATURE[4] = {'R', 'S', 'D', 'T'};

STATIC ACPI_SDT_HEADER* FindAcpiTable(CHAR8 Name[4]) {
  UINTN                    Index;
  EFI_CONFIGURATION_TABLE* ConfigurationTable;
  UINTN                    RsdpPtr;
  ACPI_RSDP*               Rsdp;
  UINTN                    RsdtPtr;
  ACPI_SDT_HEADER*         Rsdt;
  UINTN                    TablePtr;
  ACPI_SDT_HEADER*         Table;

  DEBUG ((EFI_D_INFO, "FindAcpiTable: '%c%c%c%c'\n",
    Name[0],
    Name[1],
    Name[2],
    Name[3]
  ));

  if (gSystemTable == NULL) {
      DEBUG ((EFI_D_INFO, "  System Table missing\n"));
      return NULL;
  }

  // Search the table for an entry that matches the ACPI Table Guid
  for (Index = 0; Index < gSystemTable->NumberOfTableEntries; Index++) {
    if (CompareGuid (&gEfiAcpiTableGuid, &(gSystemTable->ConfigurationTable[Index].VendorGuid))) {
      ConfigurationTable = &gSystemTable->ConfigurationTable[Index];
      break;
    }
  }

  if (ConfigurationTable == NULL) {
      DEBUG ((EFI_D_INFO, "  ACPI Configuration Table missing\n"));
      return NULL;
  }

  RsdpPtr = (UINTN)ConfigurationTable->VendorTable;
  DEBUG ((EFI_D_INFO, "  RSDP 0x%x\n", RsdpPtr));
  Rsdp = (ACPI_RSDP*)RsdpPtr;
  DEBUG ((EFI_D_INFO, "    Signature: '%c%c%c%c%c%c%c%c'\n",
    Rsdp->Signature[0],
    Rsdp->Signature[1],
    Rsdp->Signature[2],
    Rsdp->Signature[3],
    Rsdp->Signature[4],
    Rsdp->Signature[5],
    Rsdp->Signature[6],
    Rsdp->Signature[7]
  ));
  if (CompareMem(Rsdp->Signature, RSDP_SIGNATURE, 8) != 0) {
      DEBUG ((EFI_D_INFO, "    RSDP invalid signature\n"));
      return NULL;
  }
  DEBUG ((EFI_D_INFO, "    Revision: 0x%x\n", Rsdp->Revision));

  RsdtPtr = (UINTN)Rsdp->RsdtAddress;
  DEBUG ((EFI_D_INFO, "  RSDT 0x%x\n", RsdpPtr));
  Rsdt = (ACPI_SDT_HEADER*)RsdtPtr;
  DEBUG ((EFI_D_INFO, "    Signature: '%c%c%c%c'\n",
    Rsdt->Signature[0],
    Rsdt->Signature[1],
    Rsdt->Signature[2],
    Rsdt->Signature[3]
  ));
  if (CompareMem(Rsdt->Signature, RSDT_SIGNATURE, 4) != 0) {
      DEBUG ((EFI_D_INFO, "    RSDT invalid signature\n"));
      return NULL;
  }
  DEBUG ((EFI_D_INFO, "    Revision: 0x%x\n", Rsdt->Revision));
  DEBUG ((EFI_D_INFO, "    Length: 0x%x\n", Rsdt->Length));

  for (Index = sizeof(ACPI_SDT_HEADER); Index < Rsdt->Length; Index += 4) {
      TablePtr = (UINTN)(*(UINT32*)(RsdtPtr + Index));
      DEBUG ((EFI_D_INFO, "  Table %d: 0x%x\n", Index, TablePtr));
      Table = (ACPI_SDT_HEADER*)TablePtr;
      DEBUG ((EFI_D_INFO, "    Signature: '%c%c%c%c'\n",
        Table->Signature[0],
        Table->Signature[1],
        Table->Signature[2],
        Table->Signature[3]
      ));
      DEBUG ((EFI_D_INFO, "    Revision: 0x%x\n", Table->Revision));
      DEBUG ((EFI_D_INFO, "    Length: 0x%x\n", Table->Length));

      if (CompareMem(Table->Signature, Name, 4) == 0) {
          DEBUG ((EFI_D_INFO, "  Match found\n"));
          return Table;
      }
  }

  DEBUG ((EFI_D_INFO, "  No match found\n"));
  return NULL;
}

STATIC VOID FirmwareConfigurationInformation(VOID) {
    EFI_STRING_ID Token;

    Token = STRING_TOKEN (STR_VIRTUALIZATION);
    if (StandardSignatureIsGenuineIntel()) {
        HiiSetString (gFrontPagePrivate.HiiHandle, Token, L"Intel Virtualization", NULL);

        Token = STRING_TOKEN (STR_VIRTUALIZATION_STATUS);
        CPUID_VERSION_INFO_ECX VersionInfoEcx;
        AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, &VersionInfoEcx.Uint32, NULL);
        if (VersionInfoEcx.Bits.VMX) {
            HiiSetString (gFrontPagePrivate.HiiHandle, Token, L"VT-x: Active", NULL);
        } else {
            HiiSetString (gFrontPagePrivate.HiiHandle, Token, L"VT-x: Deactivated", NULL);
        }

        Token = STRING_TOKEN (STR_IOMMU_STATUS);
        CHAR8 TableName[4] = {'D', 'M', 'A', 'R'};
        if (FindAcpiTable(TableName)) {
            HiiSetString (gFrontPagePrivate.HiiHandle, Token, L"VT-d: Active", NULL);
        } else {
            HiiSetString (gFrontPagePrivate.HiiHandle, Token, L"VT-d: Deactivated", NULL);
        }

        Token = STRING_TOKEN(STR_ME_STATUS);
        //TODO: proper test for ME
        BOOLEAN me_active = FALSE;
        if (!me_active) {
            HiiSetString (gFrontPagePrivate.HiiHandle, Token, L"The Intel Management Engine is disabled at runtime to increase security.", NULL);
        }
    } else if (StandardSignatureIsAuthenticAMD()) {
        //TODO: verify AMD tests

        HiiSetString (gFrontPagePrivate.HiiHandle, Token, L"AMD Virtualization", NULL);

        Token = STRING_TOKEN (STR_VIRTUALIZATION_STATUS);
        CPUID_AMD_EXTENDED_CPU_SIG_ECX AmdExtendedCpuSigEcx;
        AsmCpuid (CPUID_EXTENDED_CPU_SIG, NULL, NULL, &AmdExtendedCpuSigEcx.Uint32, NULL);
        if (AmdExtendedCpuSigEcx.Bits.SVM) {
            HiiSetString (gFrontPagePrivate.HiiHandle, Token, L"AMD-V: Active", NULL);
        } else {
            HiiSetString (gFrontPagePrivate.HiiHandle, Token, L"AMD-V: Deactivated", NULL);
        }

        Token = STRING_TOKEN (STR_IOMMU_STATUS);
        //TODO: proper test for AMD IOMMU
        BOOLEAN iommu_active = FALSE;
        if (iommu_active) {
            HiiSetString (gFrontPagePrivate.HiiHandle, Token, L"AMD-Vi: Active", NULL);
        } else {
            HiiSetString (gFrontPagePrivate.HiiHandle, Token, L"AMD-Vi: Deactivated", NULL);
        }
    }

    Token = STRING_TOKEN (STR_TPM_STATUS);
    CHAR8 TableName[4] = {'T', 'P', 'M', '2'};
    if (FindAcpiTable(TableName)) {
        HiiSetString (gFrontPagePrivate.HiiHandle, Token, L"Trusted Platform Module: Active", NULL);
    } else {
        HiiSetString (gFrontPagePrivate.HiiHandle, Token, L"Trusted Platform Module: Deactivated", NULL);
    }
}

VOID WebcamStatus(VOID) {
  EFI_STATUS                    Status;
  UINTN                         UsbIoHandleCount;
  EFI_HANDLE                    *UsbIoBuffer;
  UINTN                         Index;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  EFI_USB_DEVICE_DESCRIPTOR     DevDesc;
  EFI_USB_INTERFACE_DESCRIPTOR  IntfDesc;
  UINTN                         Webcams;
  EFI_STRING_ID                 Token;

  //
  // Get all Usb IO handles in system
  //
  UsbIoHandleCount = 0;
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiUsbIoProtocolGuid, NULL, &UsbIoHandleCount, &UsbIoBuffer);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_INFO, "Failed to read UsbIo handles: 0x%x\n", Status));
    return;
  }

  Webcams = 0;
  for (Index = 0; Index < UsbIoHandleCount; Index++) {
    DEBUG ((EFI_D_INFO, "UsbIo Handle %d\n", Index));

    //
    // Get the child Usb IO interface
    //
    Status = gBS->HandleProtocol(
                     UsbIoBuffer[Index],
                     &gEfiUsbIoProtocolGuid,
                     (VOID **) &UsbIo
                     );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_INFO, "  Failed to find UsbIo protocol\n"));
      continue;
    }

    Status = UsbIo->UsbGetDeviceDescriptor (UsbIo, &DevDesc);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_INFO, "  Failed to get device descriptor\n"));
      continue;
    }

    DEBUG ((EFI_D_INFO, "  ID: 0x%04X:0x%04X\n", DevDesc.IdVendor, DevDesc.IdProduct));
    DEBUG ((EFI_D_INFO, "  DeviceClass: %d\n", DevDesc.DeviceClass));
    DEBUG ((EFI_D_INFO, "  DeviceSubClass: %d\n", DevDesc.DeviceSubClass));
    DEBUG ((EFI_D_INFO, "  DeviceProtocol: %d\n", DevDesc.DeviceProtocol));

    Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &IntfDesc);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_INFO, "  Failed to get interface descriptor\n"));
      continue;
    }

    DEBUG ((EFI_D_INFO, "  Interface: %d\n", IntfDesc.InterfaceNumber));
    DEBUG ((EFI_D_INFO, "  InterfaceClass: %d\n", IntfDesc.InterfaceClass));
    DEBUG ((EFI_D_INFO, "  InterfaceSubClass: %d\n", IntfDesc.InterfaceSubClass));
    DEBUG ((EFI_D_INFO, "  InterfaceProtocol: %d\n", IntfDesc.InterfaceProtocol));

    //TODO: make sure this picks up all of our laptop webcams and nothing else
    UINT32 Id = (((UINT32)DevDesc.IdVendor) << 16) | ((UINT32)DevDesc.IdProduct);
    switch (Id) {
        case 0x04f2b5a7: // Chicony Camera (bonw13, serw10, oryp4)
        case 0x04f2b649: // Chicony Camera (galp4)
        case 0x04f2b685: // Chicony Camera (bonw14, darp6, gaze14, gaze15, lemp9, serw12)
        case 0x59869102: // Acer BisonCam (addw1, addw2, oryp6)
            Webcams++;
            break;
    }
  }

  FreePool (UsbIoBuffer);

  //TODO: logic for not showing the warning on desktops
  Token = STRING_TOKEN (STR_WEBCAM_STATUS);
  if (Webcams == 0) {
    HiiSetString (gFrontPagePrivate.HiiHandle, Token, L"Info: Webcam Module Disconnected", NULL);
  }
}

/**

  Update the information for the Front Page based on Smbios information.

**/
VOID
UpdateFrontPageStrings (
  VOID
  )
{
  UINT8                             StrIndex;
  UINT8                             Str2Index;
  CHAR16                            *NewString;
  CHAR16                            *NewString2;
  CHAR16                            *NewString3;
  EFI_STATUS                        Status;
  EFI_STRING_ID                     TokenToUpdate;
  EFI_PHYSICAL_ADDRESS              *Table;
  SMBIOS_TABLE_ENTRY_POINT          *EntryPoint;
  SMBIOS_STRUCTURE_POINTER          SmbiosTable;

  FirmwareConfigurationInformation();
  WarnNoBootableMedia ();
  WebcamStatus();

  Status = EfiGetSystemConfigurationTable (&gEfiSmbiosTableGuid, (VOID **)  &Table);
  if (EFI_ERROR (Status) || Table == NULL) {
      return;
  }

  EntryPoint = (SMBIOS_TABLE_ENTRY_POINT*)Table;

  SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_BIOS_INFORMATION , 0);
  if (SmbiosTable.Raw != NULL) {
    NewString2 = AllocateZeroPool (0xC0);

    StrIndex = SmbiosTable.Type0->BiosVersion;
    GetOptionalStringByIndex ((CHAR8*)((UINT8*)SmbiosTable.Raw + SmbiosTable.Hdr->Length), StrIndex, &NewString);
    StrCatS (NewString2, 0x80 / sizeof (CHAR16), L"Version: ");
    StrCatS (NewString2, 0x80 / sizeof (CHAR16), NewString);
    TokenToUpdate = STRING_TOKEN (STR_FRONT_PAGE_BIOS_VERSION);
    HiiSetString (gFrontPagePrivate.HiiHandle, TokenToUpdate, NewString2, NULL);
    FreePool (NewString);
    FreePool (NewString2);
  }

  SmbiosTable = GetSmbiosTableFromType (EntryPoint, SMBIOS_TYPE_SYSTEM_INFORMATION , 0);
  if (SmbiosTable.Raw != NULL) {
    NewString3 = AllocateZeroPool (0x60);

    StrIndex = SmbiosTable.Type1->ProductName;
    Str2Index = SmbiosTable.Type1->Manufacturer;
    GetOptionalStringByIndex ((CHAR8*)((UINT8*)SmbiosTable.Raw + SmbiosTable.Hdr->Length), StrIndex, &NewString);
    GetOptionalStringByIndex ((CHAR8*)((UINT8*)SmbiosTable.Raw + SmbiosTable.Hdr->Length), Str2Index, &NewString2);
    StrCatS (NewString3, 0x60 / sizeof (CHAR16), NewString2);
    StrCatS (NewString3, 0x60 / sizeof (CHAR16), L" ");
    StrCatS (NewString3, 0x60 / sizeof (CHAR16), NewString);
    TokenToUpdate = STRING_TOKEN (STR_FRONT_PAGE_TITLE);
    HiiSetString (gFrontPagePrivate.HiiHandle, TokenToUpdate, NewString3, NULL);
    FreePool (NewString);

    NewString3 = AllocateZeroPool (0x60);

    StrIndex = SmbiosTable.Type1->Version;
    GetOptionalStringByIndex ((CHAR8*)((UINT8*)SmbiosTable.Raw + SmbiosTable.Hdr->Length), StrIndex, &NewString);
    StrCatS (NewString3, 0x60 / sizeof (CHAR16), L"Model: ");
    StrCatS (NewString3, 0x60 / sizeof (CHAR16), NewString);
    TokenToUpdate = STRING_TOKEN (STR_FRONT_PAGE_COMPUTER_MODEL);
    HiiSetString (gFrontPagePrivate.HiiHandle, TokenToUpdate, NewString3, NULL);
    FreePool (NewString);
    FreePool (NewString2);
    FreePool (NewString3);
  }
}

/**
  This function will change video resolution and text mode
  according to defined setup mode or defined boot mode

  @param  IsSetupMode   Indicate mode is changed to setup mode or boot mode.

  @retval  EFI_SUCCESS  Mode is changed successfully.
  @retval  Others             Mode failed to be changed.

**/
EFI_STATUS
UiSetConsoleMode (
  BOOLEAN  IsSetupMode
  )
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *GraphicsOutput;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL       *SimpleTextOut;
  UINTN                                 SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;
  UINT32                                MaxGopMode;
  UINT32                                MaxTextMode;
  UINT32                                ModeNumber;
  UINT32                                NewHorizontalResolution;
  UINT32                                NewVerticalResolution;
  UINT32                                NewColumns;
  UINT32                                NewRows;
  UINTN                                 HandleCount;
  EFI_HANDLE                            *HandleBuffer;
  EFI_STATUS                            Status;
  UINTN                                 Index;
  UINTN                                 CurrentColumn;
  UINTN                                 CurrentRow;

  MaxGopMode  = 0;
  MaxTextMode = 0;

  //
  // Get current video resolution and text mode
  //
  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID**)&GraphicsOutput
                  );
  if (EFI_ERROR (Status)) {
    GraphicsOutput = NULL;
  }

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiSimpleTextOutProtocolGuid,
                  (VOID**)&SimpleTextOut
                  );
  if (EFI_ERROR (Status)) {
    SimpleTextOut = NULL;
  }

  if ((GraphicsOutput == NULL) || (SimpleTextOut == NULL)) {
    return EFI_UNSUPPORTED;
  }

  if (IsSetupMode) {
    //
    // The required resolution and text mode is setup mode.
    //
    NewHorizontalResolution = mSetupHorizontalResolution;
    NewVerticalResolution   = mSetupVerticalResolution;
    NewColumns              = mSetupTextModeColumn;
    NewRows                 = mSetupTextModeRow;
  } else {
    //
    // The required resolution and text mode is boot mode.
    //
    NewHorizontalResolution = mBootHorizontalResolution;
    NewVerticalResolution   = mBootVerticalResolution;
    NewColumns              = mBootTextModeColumn;
    NewRows                 = mBootTextModeRow;
  }

  if (GraphicsOutput != NULL) {
    MaxGopMode  = GraphicsOutput->Mode->MaxMode;
  }

  if (SimpleTextOut != NULL) {
    MaxTextMode = SimpleTextOut->Mode->MaxMode;
  }

  //
  // 1. If current video resolution is same with required video resolution,
  //    video resolution need not be changed.
  //    1.1. If current text mode is same with required text mode, text mode need not be changed.
  //    1.2. If current text mode is different from required text mode, text mode need be changed.
  // 2. If current video resolution is different from required video resolution, we need restart whole console drivers.
  //
  for (ModeNumber = 0; ModeNumber < MaxGopMode; ModeNumber++) {
    Status = GraphicsOutput->QueryMode (
                       GraphicsOutput,
                       ModeNumber,
                       &SizeOfInfo,
                       &Info
                       );
    if (!EFI_ERROR (Status)) {
      if ((Info->HorizontalResolution == NewHorizontalResolution) &&
          (Info->VerticalResolution == NewVerticalResolution)) {
        if ((GraphicsOutput->Mode->Info->HorizontalResolution == NewHorizontalResolution) &&
            (GraphicsOutput->Mode->Info->VerticalResolution == NewVerticalResolution)) {
          //
          // Current resolution is same with required resolution, check if text mode need be set
          //
          Status = SimpleTextOut->QueryMode (SimpleTextOut, SimpleTextOut->Mode->Mode, &CurrentColumn, &CurrentRow);
          ASSERT_EFI_ERROR (Status);
          if (CurrentColumn == NewColumns && CurrentRow == NewRows) {
            //
            // If current text mode is same with required text mode. Do nothing
            //
            FreePool (Info);
            return EFI_SUCCESS;
          } else {
            //
            // If current text mode is different from required text mode.  Set new video mode
            //
            for (Index = 0; Index < MaxTextMode; Index++) {
              Status = SimpleTextOut->QueryMode (SimpleTextOut, Index, &CurrentColumn, &CurrentRow);
              if (!EFI_ERROR(Status)) {
                if ((CurrentColumn == NewColumns) && (CurrentRow == NewRows)) {
                  //
                  // Required text mode is supported, set it.
                  //
                  Status = SimpleTextOut->SetMode (SimpleTextOut, Index);
                  ASSERT_EFI_ERROR (Status);
                  //
                  // Update text mode PCD.
                  //
                  Status = PcdSet32S (PcdConOutColumn, mSetupTextModeColumn);
                  ASSERT_EFI_ERROR (Status);
                  Status = PcdSet32S (PcdConOutRow, mSetupTextModeRow);
                  ASSERT_EFI_ERROR (Status);
                  FreePool (Info);
                  return EFI_SUCCESS;
                }
              }
            }
            if (Index == MaxTextMode) {
              //
              // If required text mode is not supported, return error.
              //
              FreePool (Info);
              return EFI_UNSUPPORTED;
            }
          }
        } else {
          //
          // If current video resolution is not same with the new one, set new video resolution.
          // In this case, the driver which produces simple text out need be restarted.
          //
          Status = GraphicsOutput->SetMode (GraphicsOutput, ModeNumber);
          if (!EFI_ERROR (Status)) {
            FreePool (Info);
            break;
          }
        }
      }
      FreePool (Info);
    }
  }

  if (ModeNumber == MaxGopMode) {
    //
    // If the resolution is not supported, return error.
    //
    return EFI_UNSUPPORTED;
  }

  //
  // Set PCD to Inform GraphicsConsole to change video resolution.
  // Set PCD to Inform Consplitter to change text mode.
  //
  Status = PcdSet32S (PcdVideoHorizontalResolution, NewHorizontalResolution);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdVideoVerticalResolution, NewVerticalResolution);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdConOutColumn, NewColumns);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdConOutRow, NewRows);
  ASSERT_EFI_ERROR (Status);

  //
  // Video mode is changed, so restart graphics console driver and higher level driver.
  // Reconnect graphics console driver and higher level driver.
  // Locate all the handles with GOP protocol and reconnect it.
  //
  Status = gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiSimpleTextOutProtocolGuid,
                   NULL,
                   &HandleCount,
                   &HandleBuffer
                   );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleCount; Index++) {
      gBS->DisconnectController (HandleBuffer[Index], NULL, NULL);
    }
    for (Index = 0; Index < HandleCount; Index++) {
      gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
    }
    if (HandleBuffer != NULL) {
      FreePool (HandleBuffer);
    }
  }

  return EFI_SUCCESS;
}

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the image goes into a library that calls this
  function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeUserInterface (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HII_HANDLE                     HiiHandle;
  EFI_STATUS                         Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL       *GraphicsOutput;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *SimpleTextOut;
  UINTN                              BootTextColumn;
  UINTN                              BootTextRow;

  gSystemTable = SystemTable;

  if (!mModeInitialized) {
    //
    // After the console is ready, get current video resolution
    // and text mode before launching setup at first time.
    //
    Status = gBS->HandleProtocol (
                    gST->ConsoleOutHandle,
                    &gEfiGraphicsOutputProtocolGuid,
                    (VOID**)&GraphicsOutput
                    );
    if (EFI_ERROR (Status)) {
      GraphicsOutput = NULL;
    }

    Status = gBS->HandleProtocol (
                    gST->ConsoleOutHandle,
                    &gEfiSimpleTextOutProtocolGuid,
                    (VOID**)&SimpleTextOut
                    );
    if (EFI_ERROR (Status)) {
      SimpleTextOut = NULL;
    }

    if (GraphicsOutput != NULL) {
      //
      // Get current video resolution and text mode.
      //
      mBootHorizontalResolution = GraphicsOutput->Mode->Info->HorizontalResolution;
      mBootVerticalResolution   = GraphicsOutput->Mode->Info->VerticalResolution;
    }

    if (SimpleTextOut != NULL) {
      Status = SimpleTextOut->QueryMode (
                                SimpleTextOut,
                                SimpleTextOut->Mode->Mode,
                                &BootTextColumn,
                                &BootTextRow
                                );
      mBootTextModeColumn = (UINT32)BootTextColumn;
      mBootTextModeRow    = (UINT32)BootTextRow;
    }

    //
    // Get user defined text mode for setup.
    //
    mSetupHorizontalResolution = PcdGet32 (PcdSetupVideoHorizontalResolution);
    mSetupVerticalResolution   = PcdGet32 (PcdSetupVideoVerticalResolution);
    mSetupTextModeColumn       = PcdGet32 (PcdSetupConOutColumn);
    mSetupTextModeRow          = PcdGet32 (PcdSetupConOutRow);

    mModeInitialized           = TRUE;
  }

  gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);
  gST->ConOut->ClearScreen (gST->ConOut);

  //
  // Install customized fonts needed by Front Page
  //
  HiiHandle = ExportFonts ();
  ASSERT (HiiHandle != NULL);

  InitializeStringSupport ();

  UiSetConsoleMode (TRUE);
  UiEntry (FALSE);
  UiSetConsoleMode (FALSE);

  UninitializeStringSupport ();
  HiiRemovePackages (HiiHandle);

  return EFI_SUCCESS;
}

/**
  This function is the main entry of the UI entry.
  The function will present the main menu of the system UI.

  @param ConnectAllHappened Caller passes the value to UI to avoid unnecessary connect-all.

**/
VOID
EFIAPI
UiEntry (
  IN BOOLEAN                      ConnectAllHappened
  )
{
  EFI_STATUS                    Status;
  EFI_BOOT_LOGO_PROTOCOL        *BootLogo;

  //
  // Enter Setup page.
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_PC_USER_SETUP)
    );

  //
  // Indicate if the connect all has been performed before.
  // If has not been performed before, do here.
  //
  if (!ConnectAllHappened) {
    EfiBootManagerConnectAll ();
  }

  //
  // The boot option enumeration time is acceptable in Ui driver
  //
  EfiBootManagerRefreshAllBootOption ();

  //
  // Boot Logo is corrupted, report it using Boot Logo protocol.
  //
  Status = gBS->LocateProtocol (&gEfiBootLogoProtocolGuid, NULL, (VOID **) &BootLogo);
  if (!EFI_ERROR (Status) && (BootLogo != NULL)) {
    BootLogo->SetBootLogo (BootLogo, NULL, 0, 0, 0, 0);
  }

  InitializeFrontPage ();

  CallFrontPage ();

  FreeFrontPage ();

  //
  //Will leave browser, check any reset required change is applied? if yes, reset system
  //
  SetupResetReminder ();
}

//
//  Following are BDS Lib functions which contain all the code about setup browser reset reminder feature.
//  Setup Browser reset reminder feature is that an reset reminder will be given before user leaves the setup browser  if
//  user change any option setting which needs a reset to be effective, and  the reset will be applied according to  the user selection.
//

/**
  Record the info that  a reset is required.
  A  module boolean variable is used to record whether a reset is required.

**/
VOID
EFIAPI
EnableResetRequired (
  VOID
  )
{
  mResetRequired = TRUE;
}

/**
  Check if  user changed any option setting which needs a system reset to be effective.

**/
BOOLEAN
EFIAPI
IsResetRequired (
  VOID
  )
{
  return mResetRequired;
}


/**
  Check whether a reset is needed, and finish the reset reminder feature.
  If a reset is needed, Popup a menu to notice user, and finish the feature
  according to the user selection.

**/
VOID
EFIAPI
SetupResetReminder (
  VOID
  )
{
  EFI_INPUT_KEY                 Key;
  CHAR16                        *StringBuffer1;
  CHAR16                        *StringBuffer2;

  //
  //check any reset required change is applied? if yes, reset system
  //
  if (IsResetRequired ()) {

    StringBuffer1 = AllocateZeroPool (MAX_STRING_LEN * sizeof (CHAR16));
    ASSERT (StringBuffer1 != NULL);
    StringBuffer2 = AllocateZeroPool (MAX_STRING_LEN * sizeof (CHAR16));
    ASSERT (StringBuffer2 != NULL);
    StrCpyS (StringBuffer1, MAX_STRING_LEN, L"Configuration changed. Reset to apply it Now.");
    StrCpyS (StringBuffer2, MAX_STRING_LEN, L"Press ENTER to reset");
    //
    // Popup a menu to notice user
    //
    do {
      CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, StringBuffer1, StringBuffer2, NULL);
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

    FreePool (StringBuffer1);
    FreePool (StringBuffer2);

    gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
  }
}
