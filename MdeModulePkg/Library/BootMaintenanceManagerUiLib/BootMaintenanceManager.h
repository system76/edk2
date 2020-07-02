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
// Enumeration type definition
//
typedef UINT8 BBS_TYPE;

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

#define BOOT_TIME_OUT_VAR_OFFSET        VAR_OFFSET (BootTimeOut)
#define BOOT_NEXT_VAR_OFFSET            VAR_OFFSET (BootNext)
#define DRV_ADD_HANDLE_DESC_VAR_OFFSET  VAR_OFFSET (DriverAddHandleDesc)
#define DRV_ADD_ACTIVE_VAR_OFFSET       VAR_OFFSET (DriverAddActive)
#define DRV_ADD_RECON_VAR_OFFSET        VAR_OFFSET (DriverAddForceReconnect)
#define BOOT_OPTION_ORDER_VAR_OFFSET    VAR_OFFSET (BootOptionOrder)
#define DRIVER_OPTION_ORDER_VAR_OFFSET  VAR_OFFSET (DriverOptionOrder)
#define BOOT_OPTION_DEL_VAR_OFFSET      VAR_OFFSET (BootOptionDel)
#define DRIVER_OPTION_DEL_VAR_OFFSET    VAR_OFFSET (DriverOptionDel)
#define DRIVER_ADD_OPTION_VAR_OFFSET    VAR_OFFSET (DriverAddHandleOptionalData)

#define BOOT_TIME_OUT_QUESTION_ID       QUESTION_ID (BootTimeOut)
#define BOOT_NEXT_QUESTION_ID           QUESTION_ID (BootNext)
#define DRV_ADD_HANDLE_DESC_QUESTION_ID QUESTION_ID (DriverAddHandleDesc)
#define DRV_ADD_ACTIVE_QUESTION_ID      QUESTION_ID (DriverAddActive)
#define DRV_ADD_RECON_QUESTION_ID       QUESTION_ID (DriverAddForceReconnect)
#define BOOT_OPTION_ORDER_QUESTION_ID   QUESTION_ID (BootOptionOrder)
#define DRIVER_OPTION_ORDER_QUESTION_ID QUESTION_ID (DriverOptionOrder)
#define BOOT_OPTION_DEL_QUESTION_ID     QUESTION_ID (BootOptionDel)
#define DRIVER_OPTION_DEL_QUESTION_ID   QUESTION_ID (DriverOptionDel)
#define DRIVER_ADD_OPTION_QUESTION_ID   QUESTION_ID (DriverAddHandleOptionalData)

#define STRING_DEPOSITORY_NUMBER        8

#define NONE_BOOTNEXT_VALUE             (0xFFFF + 1)

typedef struct {
  BOOLEAN                   IsBootNext;
  BOOLEAN                   Deleted;

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
  BBS_TYPE                       BbsType;

  //
  // BMM main formset callback data.
  //

  EFI_FORM_ID                    BmmCurrentPageId;
  EFI_FORM_ID                    BmmPreviousPageId;
  BOOLEAN                        BmmAskSaveOrNot;
  BMM_FAKE_NV_DATA               BmmFakeNvData;
  BMM_FAKE_NV_DATA               BmmOldFakeNVData;

} BMM_CALLBACK_DATA;

/**

  Find drivers that will be added as Driver#### variables from handles
  in current system environment
  All valid handles in the system except those consume SimpleFs, LoadFile
  are stored in DriverMenu for future use.

  @retval EFI_SUCCESS The function complets successfully.
  @return Other value if failed to build the DriverMenu.

**/
EFI_STATUS
BOpt_FindDrivers (
  VOID
  );

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

  Build up all DriverOptionMenu

  @param CallbackData The BMM context data.

  @return EFI_SUCESS The functin completes successfully.
  @retval EFI_OUT_OF_RESOURCES Not enough memory to compete the operation.


**/
EFI_STATUS
BOpt_GetDriverOptions (
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

  Get the Option Number that has not been allocated for use.

  @param Type  The type of Option.

  @return The available Option Number.

**/
UINT16
BOpt_GetOptionNumber (
  CHAR16        *Type
  );

/**

  Get the Option Number for Boot#### that does not used.

  @return The available Option Number.

**/
UINT16
BOpt_GetBootOptionNumber (
  VOID
  );

/**

Get the Option Number for Driver#### that does not used.

@return The unused Option Number.

**/
UINT16
BOpt_GetDriverOptionNumber (
  VOID
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
  Get driver option order from globalc DriverOptionMenu.

  @param CallbackData    The BMM context data.

**/
VOID
GetDriverOrder (
  IN  BMM_CALLBACK_DATA    *CallbackData
  );

//
// Variable operation by menu selection
//
/**
  This function create a currently loaded Boot Option from
  the BMM. It then appends this Boot Option to the end of
  the "BootOrder" list. It also append this Boot Opotion to the end
  of BootOptionMenu.

  @param CallbackData           The BMM context data.

  @retval EFI_OUT_OF_RESOURCES  If not enought memory to complete the operation.
  @retval EFI_SUCCESS           If function completes successfully.

**/
EFI_STATUS
Var_UpdateBootOption (
  IN  BMM_CALLBACK_DATA                   *CallbackData
  );

/**
  Delete Boot Option that represent a Deleted state in BootOptionMenu.

  @retval EFI_SUCCESS   If all boot load option EFI Variables corresponding to
                        BM_LOAD_CONTEXT marked for deletion is deleted
  @return Others        If failed to update the "BootOrder" variable after deletion.

**/
EFI_STATUS
Var_DelBootOption (
  VOID
  );

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

  @retval EFI_OUT_OF_RESOURCES If not enought memory to complete the operation.
  @retval EFI_SUCCESS          If function completes successfully.

**/
EFI_STATUS
Var_UpdateDriverOption (
  IN  BMM_CALLBACK_DATA         *CallbackData,
  IN  EFI_HII_HANDLE            HiiHandle,
  IN  UINT16                    *DescriptionData,
  IN  UINT16                    *OptionalData,
  IN  UINT8                     ForceReconnect
  );

/**
  Delete Load Option that represent a Deleted state in DriverOptionMenu.

  @retval EFI_SUCCESS Load Option is successfully updated.
  @return Other value than EFI_SUCCESS if failed to update "Driver Order" EFI
          Variable.

**/
EFI_STATUS
Var_DelDriverOption (
  VOID
  );

/**
  This function update the "BootNext" EFI Variable. If there is no "BootNex" specified in BMM,
  this EFI Variable is deleted.
  It also update the BMM context data specified the "BootNext" value.

  @param CallbackData    The BMM context data.

  @retval EFI_SUCCESS    The function complete successfully.
  @return The EFI variable can not be saved. See gRT->SetVariable for detail return information.

**/
EFI_STATUS
Var_UpdateBootNext (
  IN BMM_CALLBACK_DATA            *CallbackData
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
  Create a lit of boot option from global BootOptionMenu. It
  allow user to delete the boot option.

  @param CallbackData    The BMM context data.

**/
VOID
UpdateBootDelPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  Create a lit of driver option from global DriverMenu.

  @param CallbackData    The BMM context data.
**/
VOID
UpdateDrvAddHandlePage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  Create a lit of driver option from global DriverOptionMenu. It
  allow user to delete the driver option.

  @param CallbackData    The BMM context data.
**/
VOID
UpdateDrvDelPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  );

/**
  Prepare the page to allow user to add description for a Driver Option.

  @param CallbackData    The BMM context data.
**/
VOID
UpdateDriverAddHandleDescPage (
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
 Update add boot/driver option page.

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
  Function deletes the variable specified by VarName and VarGuid.


  @param VarName            A Null-terminated Unicode string that is
                            the name of the vendor's variable.

  @param VarGuid            A unique identifier for the vendor.

  @retval  EFI_SUCCESS           The variable was found and removed
  @retval  EFI_UNSUPPORTED       The variable store was inaccessible
  @retval  EFI_OUT_OF_RESOURCES  The temporary buffer was not available
  @retval  EFI_NOT_FOUND         The variable was not found

**/
EFI_STATUS
EfiLibDeleteVariable (
  IN CHAR16   *VarName,
  IN EFI_GUID *VarGuid
  );

/**
  Function is used to determine the number of device path instances
  that exist in a device path.


  @param DevicePath      A pointer to a device path data structure.

  @return This function counts and returns the number of device path instances
          in DevicePath.

**/
UINTN
EfiDevicePathInstanceCount (
  IN EFI_DEVICE_PATH_PROTOCOL      *DevicePath
  );

/**
  Get a string from the Data Hub record based on
  a device path.

  @param DevPath         The device Path.

  @return A string located from the Data Hub records based on
          the device path.
  @retval NULL  If failed to get the String from Data Hub.

**/
UINT16 *
EfiLibStrFromDatahub (
  IN EFI_DEVICE_PATH_PROTOCOL                 *DevPath
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
  Discard all changes done to the BMM pages such as Boot Order change,
  Driver order change.

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
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  );

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
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN CONST EFI_STRING                     Configuration,
  OUT EFI_STRING                          *Progress
  );

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
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL         *This,
  IN        EFI_BROWSER_ACTION                     Action,
  IN        EFI_QUESTION_ID                        QuestionId,
  IN        UINT8                                  Type,
  IN        EFI_IFR_TYPE_VALUE                     *Value,
  OUT       EFI_BROWSER_ACTION_REQUEST             *ActionRequest
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
extern BM_MENU_OPTION             DriverOptionMenu;
extern BM_MENU_OPTION             DriverMenu;

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
