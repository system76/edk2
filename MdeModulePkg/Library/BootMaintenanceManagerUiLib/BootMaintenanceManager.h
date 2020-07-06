/** @file
Header file for boot maintenance module.

Copyright (c) 2004 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _BOOT_MAINT_H_
#define _BOOT_MAINT_H_

#include "FormGuid.h"

#include <Guid/MdeModuleHii.h>
#include <Guid/FileSystemVolumeLabelInfo.h>
#include <Guid/GlobalVariable.h>
#include <Guid/HiiBootMaintenanceFormset.h>

#include <Protocol/LoadFile.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/FormBrowserEx2.h>

#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/FileExplorerLib.h>
#include "BootMaintenanceManagerCustomizedUi.h"

#pragma pack(1)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH             VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;
#pragma pack()

//
// Variable created with this flag will be "Efi:...."
//
#define VAR_FLAG  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE

extern EFI_GUID mBootMaintGuid;
extern CHAR16   mBootMaintStorageName[];
//
// These are the VFR compiler generated data representing our VFR data.
//
extern UINT8    BootMaintenanceManagerBin[];

//
// Callback function helper
//
#define BMM_CALLBACK_DATA_SIGNATURE     SIGNATURE_32 ('C', 'b', 'c', 'k')
#define BMM_CALLBACK_DATA_FROM_THIS(a)  CR (a, BMM_CALLBACK_DATA, BmmConfigAccess, BMM_CALLBACK_DATA_SIGNATURE)

//
// All of the signatures that will be used in list structure
//
#define BM_MENU_OPTION_SIGNATURE      SIGNATURE_32 ('m', 'e', 'n', 'u')
#define BM_LOAD_OPTION_SIGNATURE      SIGNATURE_32 ('l', 'o', 'a', 'd')
#define BM_FILE_OPTION_SIGNATURE      SIGNATURE_32 ('f', 'i', 'l', 'e')
#define BM_HANDLE_OPTION_SIGNATURE    SIGNATURE_32 ('h', 'n', 'd', 'l')
#define BM_MENU_ENTRY_SIGNATURE       SIGNATURE_32 ('e', 'n', 't', 'r')

#define BM_LOAD_CONTEXT_SELECT        0x0
#define BM_FILE_CONTEXT_SELECT        0x2
#define BM_HANDLE_CONTEXT_SELECT      0x3

//
// Buffer size for update data
//
#define UPDATE_DATA_SIZE        0x100000

//
// Namespace of callback keys used in display and file system navigation
//
#define MAX_BBS_OFFSET          0xE000
#define NET_OPTION_OFFSET       0xD800
#define BEV_OPTION_OFFSET       0xD000
#define FD_OPTION_OFFSET        0xC000
#define HD_OPTION_OFFSET        0xB000
#define CD_OPTION_OFFSET        0xA000
#define FILE_OPTION_OFFSET      0x8000
#define FILE_OPTION_MASK        0x7FFF
#define HANDLE_OPTION_OFFSET    0x7000
#define CONFIG_OPTION_OFFSET    0x1200
#define KEY_VALUE_OFFSET        0x1100
#define FORM_ID_OFFSET          0x1000

//
// VarOffset that will be used to create question
// all these values are computed from the structure
// defined below
//
#define VAR_OFFSET(Field)              ((UINT16) ((UINTN) &(((BMM_FAKE_NV_DATA *) 0)->Field)))

//
// Question Id of Zero is invalid, so add an offset to it
//
#define QUESTION_ID(Field)             (VAR_OFFSET (Field) + CONFIG_OPTION_OFFSET)

#define BOOT_OPTION_ORDER_VAR_OFFSET    VAR_OFFSET (BootOptionOrder)

#define BOOT_OPTION_ORDER_QUESTION_ID   QUESTION_ID (BootOptionOrder)

#define STRING_DEPOSITORY_NUMBER        8

typedef struct {
  BOOLEAN                   IsLegacy;

  UINT32                    Attributes;
  UINT16                    FilePathListLength;
  UINT16                    *Description;
  EFI_DEVICE_PATH_PROTOCOL  *FilePathList;
  UINT8                     *OptionalData;
} BM_LOAD_CONTEXT;

typedef struct {
  EFI_HANDLE                        Handle;
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
  EFI_FILE_HANDLE                   FHandle;
  UINT16                            *FileName;
  EFI_FILE_SYSTEM_VOLUME_LABEL      *Info;

  BOOLEAN                           IsRoot;
  BOOLEAN                           IsDir;
  BOOLEAN                           IsRemovableMedia;
  BOOLEAN                           IsLoadFile;
  BOOLEAN                           IsBootLegacy;
} BM_FILE_CONTEXT;

typedef struct {
  EFI_HANDLE                Handle;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
} BM_HANDLE_CONTEXT;

typedef struct {
  UINTN           Signature;
  LIST_ENTRY      Head;
  UINTN           MenuNumber;
} BM_MENU_OPTION;

typedef struct {
  UINTN           Signature;
  LIST_ENTRY      Link;
  UINTN           OptionNumber;
  UINT16          *DisplayString;
  UINT16          *HelpString;
  EFI_STRING_ID   DisplayStringToken;
  EFI_STRING_ID   HelpStringToken;
  UINTN           ContextSelection;
  VOID            *VariableContext;
} BM_MENU_ENTRY;

typedef struct {

  UINTN                          Signature;

  EFI_HII_HANDLE                 BmmHiiHandle;
  EFI_HANDLE                     BmmDriverHandle;
  ///
  /// Boot Maintenance  Manager Produced protocols
  ///
  EFI_HII_CONFIG_ACCESS_PROTOCOL BmmConfigAccess;
  EFI_FORM_BROWSER2_PROTOCOL     *FormBrowser2;

  BM_MENU_ENTRY                  *MenuEntry;
  BM_HANDLE_CONTEXT              *HandleContext;
  BM_FILE_CONTEXT                *FileContext;
  BM_LOAD_CONTEXT                *LoadContext;

  //
  // BMM main formset callback data.
  //

  EFI_FORM_ID                    BmmCurrentPageId;
  EFI_FORM_ID                    BmmPreviousPageId;
  BMM_FAKE_NV_DATA               BmmFakeNvData;
  BMM_FAKE_NV_DATA               BmmOldFakeNVData;

} BMM_CALLBACK_DATA;

/**

  Build the BootOptionMenu according to BootOrder Variable.
  This Routine will access the Boot#### to get EFI_LOAD_OPTION.

  @param CallbackData The BMM context data.

  @return The number of the Var Boot####.

**/
EFI_STATUS
BOpt_GetBootOptions (
  IN  BMM_CALLBACK_DATA         *CallbackData
  );

/**
  Free resources allocated in Allocate Rountine.

  @param FreeMenu        Menu to be freed

**/
VOID
BOpt_FreeMenu (
  BM_MENU_OPTION        *FreeMenu
  );

/**
  Create a menu entry give a Menu type.

  @param MenuType        The Menu type to be created.


  @retval NULL           If failed to create the menu.
  @return                The menu.

**/
BM_MENU_ENTRY                     *
BOpt_CreateMenuEntry (
  UINTN           MenuType
  );

/**
  Free up all resource allocated for a BM_MENU_ENTRY.

  @param MenuEntry   A pointer to BM_MENU_ENTRY.

**/
VOID
BOpt_DestroyMenuEntry (
  BM_MENU_ENTRY         *MenuEntry
  );

/**
  Get the Menu Entry from the list in Menu Entry List.

  If MenuNumber is great or equal to the number of Menu
  Entry in the list, then ASSERT.

  @param MenuOption      The Menu Entry List to read the menu entry.
  @param MenuNumber      The index of Menu Entry.

  @return The Menu Entry.

**/
BM_MENU_ENTRY                     *
BOpt_GetMenuEntry (
  BM_MENU_OPTION      *MenuOption,
  UINTN               MenuNumber
  );

/**
  Get option number according to Boot#### and BootOrder variable.
  The value is saved as #### + 1.

  @param CallbackData    The BMM context data.
**/
VOID
GetBootOrder (
  IN  BMM_CALLBACK_DATA    *CallbackData
  );

/**
  This function update the "BootOrder" EFI Variable based on BMM Formset's NV map. It then refresh
  BootOptionMenu with the new "BootOrder" list.

  @param CallbackData           The BMM context data.

  @retval EFI_SUCCESS           The function complete successfully.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory to complete the function.
  @return not The EFI variable can not be saved. See gRT->SetVariable for detail return information.

**/
EFI_STATUS
Var_UpdateBootOrder (
  IN BMM_CALLBACK_DATA            *CallbackData
  );

//
// Following are page create and refresh functions
//
/**
 Create the global UpdateData structure.

**/
VOID
CreateUpdateData (
  VOID
  );

/**
  Refresh the global UpdateData structure.

**/
VOID
RefreshUpdateData (
  VOID
  );

/**
  Clean up the dynamic opcode at label and form specified by
  both LabelId.

  @param LabelId         It is both the Form ID and Label ID for
                         opcode deletion.
  @param CallbackData    The BMM context data.

**/
VOID
CleanUpPage (
  IN UINT16                           LabelId,
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  Dispatch the correct update page function to call based on the UpdatePageId.

  @param UpdatePageId    The form ID.
  @param CallbackData    The BMM context data.
**/
VOID
UpdatePageBody (
  IN UINT16                           UpdatePageId,
  IN BMM_CALLBACK_DATA                *CallbackData
  );

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
  );

/**
  Get the index number (#### in Boot####) for the boot option pointed to a BBS legacy device type
  specified by DeviceType.

  @param DeviceType      The legacy device type. It can be floppy, network, harddisk, cdrom,
                         etc.
  @param OptionIndex     Returns the index number (#### in Boot####).
  @param OptionSize      Return the size of the Boot### variable.

**/
VOID *
GetLegacyBootOptionVar (
  IN  UINTN                            DeviceType,
  OUT UINTN                            *OptionIndex,
  OUT UINTN                            *OptionSize
  );

/**
  Discard all changes done to the BMM pages such as Boot Order change.

  @param Private         The BMM context data.
  @param CurrentFakeNVMap The current Fack NV Map.

**/
VOID
DiscardChangeHandler (
  IN  BMM_CALLBACK_DATA               *Private,
  IN  BMM_FAKE_NV_DATA                *CurrentFakeNVMap
  );


/**
  This function is to clean some useless data before submit changes.

  @param Private            The BMM context data.

**/
VOID
CleanUselessBeforeSubmit (
  IN  BMM_CALLBACK_DATA               *Private
  );

/**
  Dispatch the display to the next page based on NewPageId.

  @param Private         The BMM context data.
  @param NewPageId       The original page ID.

**/
VOID
UpdatePageId (
  BMM_CALLBACK_DATA              *Private,
  UINT16                         NewPageId
  );

/**
  Remove the installed BootMaint and FileExplorer HiiPackages.

**/
VOID
FreeBMPackage(
  VOID
  );

/**
  Install BootMaint and FileExplorer HiiPackages.

**/
VOID
InitBootMaintenance(
  VOID
  );

/**
  This function converts an input device structure to a Unicode string.

  @param DevPath       A pointer to the device path structure.

  @return              A new allocated Unicode string that represents the device path.

**/
CHAR16 *
UiDevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath
  );

/**
  Extract filename from device path. The returned buffer is allocated using AllocateCopyPool.
  The caller is responsible for freeing the allocated buffer using FreePool().

  @param DevicePath      Device path.

  @return                A new allocated string that represents the file name.

**/
CHAR16 *
ExtractFileNameFromDevicePath (
  IN   EFI_DEVICE_PATH_PROTOCOL *DevicePath
  );

/**
  Create boot option base on the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.

**/
BOOLEAN
EFIAPI
CreateBootOptionFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL    *FilePath
  );

/**
  Create driver option base on the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.
**/
BOOLEAN
EFIAPI
CreateDriverOptionFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL    *FilePath
  );

/**
  Boot the file specified by the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.

**/
BOOLEAN
EFIAPI
BootFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL    *FilePath
  );

//
// Global variable in this program (defined in data.c)
//
extern BM_MENU_OPTION             BootOptionMenu;

//
// Shared IFR form update data
//
extern VOID                        *mStartOpCodeHandle;
extern VOID                        *mEndOpCodeHandle;
extern EFI_IFR_GUID_LABEL          *mStartLabel;
extern EFI_IFR_GUID_LABEL          *mEndLabel;
extern BMM_CALLBACK_DATA           gBootMaintenancePrivate;
extern BMM_CALLBACK_DATA           *mBmmCallbackInfo;

#endif
