## @file
# Bootloader Payload Package
#
# Provides drivers and definitions to create uefi payload for bootloaders.
#
# Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                       = UefiPayloadPkg
  PLATFORM_GUID                       = F71608AB-D63D-4491-B744-A99998C8CD96
  PLATFORM_VERSION                    = 0.1
  DSC_SPECIFICATION                   = 0x00010005
  SUPPORTED_ARCHITECTURES             = IA32|X64
  BUILD_TARGETS                       = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER                    = DEFAULT
  OUTPUT_DIRECTORY                    = Build/UefiPayloadPkgX64
  FLASH_DEFINITION                    = UefiPayloadPkg/UefiPayloadPkg.fdf
  PCD_DYNAMIC_AS_DYNAMICEX            = TRUE

  DEFINE SOURCE_DEBUG_ENABLE          = FALSE
  DEFINE PS2_KEYBOARD_ENABLE          = TRUE
  DEFINE UNIVERSAL_PAYLOAD            = FALSE

  DEFINE PLATFORM_BOOT_TIMEOUT        = 2

  #
  # Send logs to System76 EC
  #
  DEFINE SYSTEM76_EC_LOGGING          = FALSE

  #
  # SBL:      UEFI payload for Slim Bootloader
  # COREBOOT: UEFI payload for coreboot
  #
  DEFINE   BOOTLOADER = COREBOOT

  #
  # CPU options
  #
  DEFINE MAX_LOGICAL_PROCESSORS       = 64

  #
  # PCI options
  #
  DEFINE PCIE_BASE_SUPPORT            = TRUE

  #
  # Serial port set up
  #
  DEFINE BAUD_RATE                    = 115200
  DEFINE SERIAL_CLOCK_RATE            = 1843200
  DEFINE SERIAL_LINE_CONTROL          = 3 # 8-bits, no parity
  DEFINE SERIAL_HARDWARE_FLOW_CONTROL = FALSE
  DEFINE SERIAL_DETECT_CABLE          = FALSE
  DEFINE SERIAL_FIFO_CONTROL          = 7 # Enable FIFO
  DEFINE SERIAL_EXTENDED_TX_FIFO_SIZE = 16
  DEFINE UART_DEFAULT_BAUD_RATE       = $(BAUD_RATE)
  DEFINE UART_DEFAULT_DATA_BITS       = 8
  DEFINE UART_DEFAULT_PARITY          = 1
  DEFINE UART_DEFAULT_STOP_BITS       = 1
  DEFINE DEFAULT_TERMINAL_TYPE        = 0

  # Enabling the serial terminal will slow down the boot menu redering!
  DEFINE DISABLE_SERIAL_TERMINAL      = FALSE

  #
  #  typedef struct {
  #    UINT16  VendorId;          ///< Vendor ID to match the PCI device.  The value 0xFFFF terminates the list of entries.
  #    UINT16  DeviceId;          ///< Device ID to match the PCI device
  #    UINT32  ClockRate;         ///< UART clock rate.  Set to 0 for default clock rate of 1843200 Hz
  #    UINT64  Offset;            ///< The byte offset into to the BAR
  #    UINT8   BarIndex;          ///< Which BAR to get the UART base address
  #    UINT8   RegisterStride;    ///< UART register stride in bytes.  Set to 0 for default register stride of 1 byte.
  #    UINT16  ReceiveFifoDepth;  ///< UART receive FIFO depth in bytes. Set to 0 for a default FIFO depth of 16 bytes.
  #    UINT16  TransmitFifoDepth; ///< UART transmit FIFO depth in bytes. Set to 0 for a default FIFO depth of 16 bytes.
  #    UINT8   Reserved[2];
  #  } PCI_SERIAL_PARAMETER;
  #
  # Vendor FFFF Device 0000 Prog Interface 1, BAR #0, Offset 0, Stride = 1, Clock 1843200 (0x1c2000)
  #
  #                                       [Vendor]   [Device]  [----ClockRate---]  [------------Offset-----------] [Bar] [Stride] [RxFifo] [TxFifo]   [Rsvd]   [Vendor]
  DEFINE PCI_SERIAL_PARAMETERS        = {0xff,0xff, 0x00,0x00, 0x0,0x20,0x1c,0x00, 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0, 0x00,    0x01, 0x0,0x0, 0x0,0x0, 0x0,0x0, 0xff,0xff}

  #
  # Shell options: [BUILD_SHELL, MIN_BIN, NONE, UEFI_BIN]
  #
  DEFINE SHELL_TYPE                   = BUILD_SHELL

  DEFINE EMU_VARIABLE_ENABLE   = TRUE
  DEFINE DISABLE_RESET_SYSTEM  = FALSE

  # Dfine the maximum size of the capsule image without a reset flag that the platform can support.
  DEFINE MAX_SIZE_NON_POPULATE_CAPSULE = 0xa00000

  # Define RTC related register.
  DEFINE RTC_INDEX_REGISTER = 0x70
  DEFINE RTC_TARGET_REGISTER = 0x71

  DEFINE SERIAL_DRIVER_ENABLE = TRUE
  #
  # Security options:
  #
  DEFINE SECURE_BOOT_ENABLE           = FALSE
  DEFINE TPM_ENABLE                   = FALSE

[BuildOptions]
  *_*_*_CC_FLAGS                 = -D DISABLE_NEW_DEPRECATED_INTERFACES
  GCC:*_UNIXGCC_*_CC_FLAGS       = -DMDEPKG_NDEBUG
  GCC:RELEASE_*_*_CC_FLAGS       = -DMDEPKG_NDEBUG
  INTEL:RELEASE_*_*_CC_FLAGS     = /D MDEPKG_NDEBUG
  MSFT:RELEASE_*_*_CC_FLAGS      = /D MDEPKG_NDEBUG

[BuildOptions.common.EDKII.DXE_RUNTIME_DRIVER]
  GCC:*_*_*_DLINK_FLAGS      = -z common-page-size=0x1000
  XCODE:*_*_*_DLINK_FLAGS    = -seg1addr 0x1000 -segalign 0x1000
  XCODE:*_*_*_MTOC_FLAGS     = -align 0x1000
  CLANGPDB:*_*_*_DLINK_FLAGS = /ALIGN:4096
  MSFT:*_*_*_DLINK_FLAGS     = /ALIGN:4096

################################################################################
#
# SKU Identification section - list of all SKU IDs supported by this Platform.
#
################################################################################
[SkuIds]
  0|DEFAULT

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  #
  # Entry point
  #
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf

  #
  # Basic
  #
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
!if $(PCIE_BASE_SUPPORT) == FALSE
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
!else
  PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
!endif
  PciSegmentLib|MdePkg/Library/PciSegmentLibSegmentInfo/BasePciSegmentLibSegmentInfo.inf
  PciSegmentInfoLib|UefiPayloadPkg/Library/PciSegmentInfoLibAcpiBoardInfo/PciSegmentInfoLibAcpiBoardInfo.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  DxeHobListLib|UefiPayloadPkg/Library/DxeHobListLib/DxeHobListLib.inf
!if $(SECURE_BOOT_ENABLE) == TRUE
  SecureBootVariableLib|SecurityPkg/Library/SecureBootVariableLib/SecureBootVariableLib.inf
  SecureBootVariableProvisionLib|SecurityPkg/Library/SecureBootVariableProvisionLib/SecureBootVariableProvisionLib.inf
!endif

!if $(UNIVERSAL_PAYLOAD) == TRUE
  HobLib|UefiPayloadPkg/Library/DxeHobLib/DxeHobLib.inf
!else
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
!endif

  #
  # UEFI & PI
  #
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiCpuLib|UefiCpuPkg/Library/BaseUefiCpuLib/BaseUefiCpuLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf

  #
  # Generic Modules
  #
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
  SecurityManagementLib|MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  BootLogoLib|MdeModulePkg/Library/BootLogoLib/BootLogoLib.inf
  CustomizedDisplayLib|MdeModulePkg/Library/CustomizedDisplayLib/CustomizedDisplayLib.inf
  FrameBufferBltLib|MdeModulePkg/Library/FrameBufferBltLib/FrameBufferBltLib.inf

  #
  # CPU
  #
  MtrrLib|UefiCpuPkg/Library/MtrrLib/MtrrLib.inf
  LocalApicLib|UefiCpuPkg/Library/BaseXApicX2ApicLib/BaseXApicX2ApicLib.inf
  MicrocodeLib|UefiCpuPkg/Library/MicrocodeLib/MicrocodeLib.inf

  #
  # Platform
  #
  TimerLib|UefiPayloadPkg/Library/AcpiTimerLib/AcpiTimerLib.inf
  ResetSystemLib|UefiPayloadPkg/Library/ResetSystemLib/ResetSystemLib.inf
!if $(SYSTEM76_EC_LOGGING) == TRUE
  SerialPortLib|UefiPayloadPkg/Library/System76EcLib/System76EcLib.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf
!else
    SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
    !if $(UNIVERSAL_PAYLOAD) == TRUE
      PlatformHookLib|UefiPayloadPkg/Library/UniversalPayloadPlatformHookLib/PlatformHookLib.inf
    !else
      PlatformHookLib|UefiPayloadPkg/Library/PlatformHookLib/PlatformHookLib.inf
    !endif
!endif
  PlatformBootManagerLib|UefiPayloadPkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf
  IoApicLib|PcAtChipsetPkg/Library/BaseIoApicLib/BaseIoApicLib.inf

  #
  # Misc
  #
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
!if $(SOURCE_DEBUG_ENABLE) == TRUE
  PeCoffExtraActionLib|SourceLevelDebugPkg/Library/PeCoffExtraActionLibDebug/PeCoffExtraActionLibDebug.inf
  DebugCommunicationLib|SourceLevelDebugPkg/Library/DebugCommunicationLibSerialPort/DebugCommunicationLibSerialPort.inf
!else
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
!endif
  PlatformSupportLib|UefiPayloadPkg/Library/PlatformSupportLibNull/PlatformSupportLibNull.inf
!if $(UNIVERSAL_PAYLOAD) == FALSE
  !if $(BOOTLOADER) == "COREBOOT"
    BlParseLib|UefiPayloadPkg/Library/CbParseLib/CbParseLib.inf
  !else
    BlParseLib|UefiPayloadPkg/Library/SblParseLib/SblParseLib.inf
  !endif
!endif

  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  LockBoxLib|MdeModulePkg/Library/LockBoxNullLib/LockBoxNullLib.inf
  FileExplorerLib|MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf
  AuthVariableLib|MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
  VarCheckLib|MdeModulePkg/Library/VarCheckLib/VarCheckLib.inf
  VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLib.inf
  VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf
!if $(BOOTLOADER) == "COREBOOT"
  SmmStoreLib|UefiPayloadPkg/Library/CbSMMStoreLib/CbSMMStoreLib.inf
!else
  SmmStoreLib|UefiPayloadPkg/Library/SblSMMStoreLib/SblSMMStoreLib.inf
!endif

[LibraryClasses.common]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf

!if $(TPM_ENABLE) == TRUE
  Tpm12CommandLib|SecurityPkg/Library/Tpm12CommandLib/Tpm12CommandLib.inf
  Tpm2CommandLib|SecurityPkg/Library/Tpm2CommandLib/Tpm2CommandLib.inf
  Tcg2PhysicalPresenceLib|OvmfPkg/Library/Tcg2PhysicalPresenceLibQemu/DxeTcg2PhysicalPresenceLib.inf
  Tcg2PhysicalPresencePlatformLib|UefiPayloadPkg/Library/Tcg2PhysicalPresencePlatformLibUefipayload/DxeTcg2PhysicalPresencePlatformLib.inf
  Tcg2PpVendorLib|SecurityPkg/Library/Tcg2PpVendorLibNull/Tcg2PpVendorLibNull.inf
  TpmMeasurementLib|SecurityPkg/Library/DxeTpmMeasurementLib/DxeTpmMeasurementLib.inf
!else
  TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
  Tcg2PhysicalPresenceLib|OvmfPkg/Library/Tcg2PhysicalPresenceLibNull/DxeTcg2PhysicalPresenceLib.inf
!endif

[LibraryClasses.common.SEC]
  HobLib|UefiPayloadPkg/Library/PayloadEntryHobLib/HobLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  DxeHobListLib|UefiPayloadPkg/Library/DxeHobListLibNull/DxeHobListLibNull.inf

[LibraryClasses.common.DXE_CORE]
  DxeHobListLib|UefiPayloadPkg/Library/DxeHobListLibNull/DxeHobListLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
!if $(SOURCE_DEBUG_ENABLE)
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/DxeDebugAgentLib.inf
!endif
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/DxeCpuExceptionHandlerLib.inf
  VmgExitLib|UefiCpuPkg/Library/VmgExitLibNull/VmgExitLibNull.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  RngLib|MdePkg/Library/BaseRngLibTimerLib/BaseRngLibTimerLib.inf

!if $(SECURE_BOOT_ENABLE) == TRUE
  AuthVariableLib|SecurityPkg/Library/AuthVariableLib/AuthVariableLib.inf
  # re-use the UserPhysicalPresent() dummy implementation from the ovmf tree
  PlatformSecureLib|OvmfPkg/Library/PlatformSecureLib/PlatformSecureLib.inf
!else
  AuthVariableLib|MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
!endif
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf

[LibraryClasses.common.DXE_DRIVER]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
!if $(SOURCE_DEBUG_ENABLE)
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/DxeDebugAgentLib.inf
!endif
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/DxeCpuExceptionHandlerLib.inf
  MpInitLib|UefiCpuPkg/Library/MpInitLib/DxeMpInitLib.inf
  VmgExitLib|UefiCpuPkg/Library/VmgExitLibNull/VmgExitLibNull.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  RngLib|MdePkg/Library/BaseRngLibTimerLib/BaseRngLibTimerLib.inf

!if $(SECURE_BOOT_ENABLE) == TRUE
  AuthVariableLib|SecurityPkg/Library/AuthVariableLib/AuthVariableLib.inf
  # re-use the UserPhysicalPresent() dummy implementation from the ovmf tree
  PlatformSecureLib|OvmfPkg/Library/PlatformSecureLib/PlatformSecureLib.inf
!else
  AuthVariableLib|MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
!endif
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
!if $(TPM_ENABLE) == TRUE
  Tpm12DeviceLib|SecurityPkg/Library/Tpm12DeviceLibTcg/Tpm12DeviceLibTcg.inf
  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibTcg2/Tpm2DeviceLibTcg2.inf
!endif

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/RuntimeDxeReportStatusCodeLib/RuntimeDxeReportStatusCodeLib.inf
  VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLibRuntimeDxe.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/RuntimeCryptLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  RngLib|MdePkg/Library/BaseRngLibTimerLib/BaseRngLibTimerLib.inf

!if $(SECURE_BOOT_ENABLE) == TRUE
  AuthVariableLib|SecurityPkg/Library/AuthVariableLib/AuthVariableLib.inf
  # re-use the UserPhysicalPresent() dummy implementation from the ovmf tree
  PlatformSecureLib|OvmfPkg/Library/PlatformSecureLib/PlatformSecureLib.inf
!else
  AuthVariableLib|MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
!endif

[LibraryClasses.common.UEFI_DRIVER,LibraryClasses.common.UEFI_APPLICATION]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform.
#
################################################################################
[PcdsFeatureFlag]
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSwitchToLongMode|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutGopSupport|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutUgaSupport|FALSE
  ## This PCD specified whether ACPI SDT protocol is installed.
  gEfiMdeModulePkgTokenSpaceGuid.PcdInstallAcpiSdtProtocol|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdHiiOsRuntimeSupport|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPciDegradeResourceForOptionRom|FALSE

[PcdsFixedAtBuild]
  # UEFI spec: Minimal value is 0x8000!
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x8000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxAuthVariableSize|0x8800
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxHardwareErrorVariableSize|0x8000
  gEfiMdeModulePkgTokenSpaceGuid.PcdVariableStoreSize|0x10000
  gEfiMdeModulePkgTokenSpaceGuid.PcdVpdBaseAddress|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseMemory|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdUse1GPageTable|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange|FALSE

  gUefiPayloadPkgTokenSpaceGuid.PcdPcdDriverFile|{ 0x57, 0x72, 0xcf, 0x80, 0xab, 0x87, 0xf9, 0x47, 0xa3, 0xfe, 0xD5, 0x0B, 0x76, 0xd8, 0x95, 0x41 }

!if $(SOURCE_DEBUG_ENABLE)
  gEfiSourceLevelDebugPkgTokenSpaceGuid.PcdDebugLoadImageMethod|0x2
!endif
  # Disable MTRR programming
  gUefiCpuPkgTokenSpaceGuid.PcdCpuDisableMtrrProgramming|TRUE

[PcdsPatchableInModule.common]
  gEfiMdeModulePkgTokenSpaceGuid.PcdBootManagerMenuFile|{ 0x21, 0xaa, 0x2c, 0x46, 0x14, 0x76, 0x03, 0x45, 0x83, 0x6e, 0x8a, 0xb6, 0xf4, 0x66, 0x23, 0x31 }
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x7
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x8000004F
!if $(TARGET) == DEBUG
  !if $(SOURCE_DEBUG_ENABLE)
    gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x17
  !else
    gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2B
  !endif
!endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxSizeNonPopulateCapsule|$(MAX_SIZE_NON_POPULATE_CAPSULE)
  gPcAtChipsetPkgTokenSpaceGuid.PcdRtcIndexRegister|$(RTC_INDEX_REGISTER)
  gPcAtChipsetPkgTokenSpaceGuid.PcdRtcTargetRegister|$(RTC_TARGET_REGISTER)
  #
  # The following parameters are set by Library/PlatformHookLib
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseMmio|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialBaudRate|$(BAUD_RATE)
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterStride|1

  #
  # Enable these parameters to be set on the command line
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialClockRate|$(SERIAL_CLOCK_RATE)
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialLineControl|$(SERIAL_LINE_CONTROL)
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseHardwareFlowControl|$(SERIAL_HARDWARE_FLOW_CONTROL)
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialDetectCable|$(SERIAL_DETECT_CABLE)
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialFifoControl|$(SERIAL_FIFO_CONTROL)
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialExtendedTxFifoSize|$(SERIAL_EXTENDED_TX_FIFO_SIZE)

  gEfiMdeModulePkgTokenSpaceGuid.PcdPciSerialParameters|$(PCI_SERIAL_PARAMETERS)

  gUefiCpuPkgTokenSpaceGuid.PcdCpuMaxLogicalProcessorNumber|$(MAX_LOGICAL_PROCESSORS)
  gUefiCpuPkgTokenSpaceGuid.PcdCpuNumberOfReservedVariableMtrrs|0

################################################################################
#
# Pcd DynamicEx Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsDynamicHii]
  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut|L"Timeout"|gEfiGlobalVariableGuid|0x0|$(PLATFORM_BOOT_TIMEOUT)

[PcdsDynamicExDefault]
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultBaudRate|$(UART_DEFAULT_BAUD_RATE)
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultDataBits|$(UART_DEFAULT_DATA_BITS)
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultParity|$(UART_DEFAULT_PARITY)
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultStopBits|$(UART_DEFAULT_STOP_BITS)
  gEfiMdePkgTokenSpaceGuid.PcdDefaultTerminalType|$(DEFAULT_TERMINAL_TYPE)
  gEfiMdeModulePkgTokenSpaceGuid.PcdAriSupport
  gEfiMdeModulePkgTokenSpaceGuid.PcdMrIovSupport
  gEfiMdeModulePkgTokenSpaceGuid.PcdSrIovSupport
  gEfiMdeModulePkgTokenSpaceGuid.PcdSrIovSystemPageSize
  gUefiCpuPkgTokenSpaceGuid.PcdCpuApInitTimeOutInMicroSeconds
  gUefiCpuPkgTokenSpaceGuid.PcdCpuApLoopMode
  gUefiCpuPkgTokenSpaceGuid.PcdCpuMicrocodePatchAddress
  gUefiCpuPkgTokenSpaceGuid.PcdCpuMicrocodePatchRegionSize
!if $(TARGET) == DEBUG
  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseSerial|TRUE
!else
  gEfiMdeModulePkgTokenSpaceGuid.PcdStatusCodeUseSerial|FALSE
!endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvStoreReserved|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable|FALSE

  ## This PCD defines the video horizontal resolution.
  #  This PCD could be set to 0 then video resolution could be at highest resolution.
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoHorizontalResolution|0
  ## This PCD defines the video vertical resolution.
  #  This PCD could be set to 0 then video resolution could be at highest resolution.
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoVerticalResolution|0

  ## The PCD is used to specify the video horizontal resolution of text setup.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoHorizontalResolution|0
  ## The PCD is used to specify the video vertical resolution of text setup.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoVerticalResolution|0

  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow|31
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn|100
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseSize|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdGhcbBase|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdTestKeyUsed|FALSE
  gUefiCpuPkgTokenSpaceGuid.PcdSevEsIsEnabled|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdPciDisableBusEnumeration|TRUE

  ## Patched by BlSupportDxe
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmInstanceGuid|{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmInitializationPolicy|0
  ## Match the hash algorithms listed in Tcg2Dxe
  gEfiSecurityPkgTokenSpaceGuid.PcdTcg2HashAlgorithmBitmap|0x1F

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform.
#
################################################################################

!if "IA32" in "$(ARCH)"
  [Components.IA32]
  !if $(UNIVERSAL_PAYLOAD) == TRUE
    UefiPayloadPkg/UefiPayloadEntry/UniversalPayloadEntry.inf
  !else
    UefiPayloadPkg/UefiPayloadEntry/UefiPayloadEntry.inf
  !endif
!else
  [Components.X64]
  !if $(UNIVERSAL_PAYLOAD) == TRUE
    UefiPayloadPkg/UefiPayloadEntry/UniversalPayloadEntry.inf
  !else
    UefiPayloadPkg/UefiPayloadEntry/UefiPayloadEntry.inf
  !endif
!endif

[Components.X64]
  #
  # DXE Core
  #
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  }

  #
  # Components that produce the architectural protocols
  #
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf {
    <LibraryClasses>
!if $(SECURE_BOOT_ENABLE) == TRUE
      NULL|SecurityPkg/Library/DxeImageVerificationLib/DxeImageVerificationLib.inf
!endif
!if $(TPM_ENABLE) == TRUE
      NULL|SecurityPkg/Library/DxeTpmMeasureBootLib/DxeTpmMeasureBootLib.inf
      NULL|SecurityPkg/Library/DxeTpm2MeasureBootLib/DxeTpm2MeasureBootLib.inf
!endif
  }

!if $(SECURE_BOOT_ENABLE) == TRUE
  SecurityPkg/VariableAuthenticated/SecureBootConfigDxe/SecureBootConfigDxe.inf
  SecurityPkg/VariableAuthenticated/SecureBootDefaultKeysDxe/SecureBootDefaultKeysDxe.inf
  UefiPayloadPkg/SecureBootEnrollDefaultKeys/SecureBootSetup.inf
!endif

  UefiCpuPkg/CpuDxe/CpuDxe.inf
  MdeModulePkg/Universal/BdsDxe/BdsDxe.inf
  MdeModulePkg/Logo/LogoDxe.inf
  MdeModulePkg/Application/UiApp/UiApp.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/BootManagerUiLib/BootManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootMaintenanceManagerUiLib/BootMaintenanceManagerUiLib.inf
  }

  PcAtChipsetPkg/HpetTimerDxe/HpetTimerDxe.inf
  MdeModulePkg/Universal/Metronome/Metronome.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
!if $(DISABLE_RESET_SYSTEM) == FALSE
  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
!endif
  PcAtChipsetPkg/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf
!if $(EMU_VARIABLE_ENABLE) == TRUE
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
  }
!endif

  #
  # Following are the DXE drivers
  #
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

  MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf
  UefiCpuPkg/CpuIo2Dxe/CpuIo2Dxe.inf
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/MemoryTest/NullMemoryTestDxe/NullMemoryTestDxe.inf
  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf

  UefiPayloadPkg/BlSupportDxe/BlSupportDxe.inf

  #
  # SMBIOS Support
  #
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf

  #
  # ACPI Support
  #
  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf

  #
  # PCI Support
  #
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf
  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf {
    <LibraryClasses>
      PciHostBridgeLib|UefiPayloadPkg/Library/PciHostBridgeLib/PciHostBridgeLib.inf
  }

  #
  # SCSI/ATA/IDE/DISK Support
  #
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  FatPkg/EnhancedFatDxe/Fat.inf
  MdeModulePkg/Bus/Pci/SataControllerDxe/SataControllerDxe.inf
  MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBusDxe.inf
  MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.inf
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf
  MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf

  #
  # SD/eMMC Support
  #
  MdeModulePkg/Bus/Pci/SdMmcPciHcDxe/SdMmcPciHcDxe.inf
  MdeModulePkg/Bus/Sd/EmmcDxe/EmmcDxe.inf
  MdeModulePkg/Bus/Sd/SdDxe/SdDxe.inf

  #
  # Usb Support
  #
  MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
  MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
  MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf
  MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf

  #
  # ISA Support
  #
!if $(SERIAL_DRIVER_ENABLE) == TRUE
  MdeModulePkg/Universal/SerialDxe/SerialDxe.inf
!endif
!if $(PS2_KEYBOARD_ENABLE) == TRUE
  OvmfPkg/SioBusDxe/SioBusDxe.inf
  MdeModulePkg/Bus/Isa/Ps2KeyboardDxe/Ps2KeyboardDxe.inf
!endif

  #
  # Console Support
  #
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
!if $(DISABLE_SERIAL_TERMINAL) == FALSE
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
!endif
  UefiPayloadPkg/GraphicsOutputDxe/GraphicsOutputDxe.inf
  UefiPayloadPkg/PciPlatformDxe/PciPlatformDxe.inf

  #
  # Random Number Generator
  #
  SecurityPkg/RandomNumberGenerator/RngDxe/RngDxe.inf {
      <LibraryClasses>
      RngLib|UefiPayloadPkg/Library/BaseRngLib/BaseRngLib.inf
  }

  #
  # SMMSTORE
  #
  UefiPayloadPkg/BlSMMStoreDxe/BlSMMStoreDxe.inf

!if $(TPM_ENABLE) == TRUE
  SecurityPkg/Tcg/Tcg2Dxe/Tcg2Dxe.inf {
    <LibraryClasses>
      Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibRouter/Tpm2DeviceLibRouterDxe.inf
      NULL|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2InstanceLibDTpm.inf
      HashLib|SecurityPkg/Library/HashLibBaseCryptoRouter/HashLibBaseCryptoRouterDxe.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha1/HashInstanceLibSha1.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha256/HashInstanceLibSha256.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha384/HashInstanceLibSha384.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha512/HashInstanceLibSha512.inf
      NULL|SecurityPkg/Library/HashInstanceLibSm3/HashInstanceLibSm3.inf
  }
  SecurityPkg/Tcg/Tcg2Config/Tcg2ConfigDxe.inf {
    <LibraryClasses>
    Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibRouter/Tpm2DeviceLibRouterDxe.inf
  }
  SecurityPkg/Tcg/TcgDxe/TcgDxe.inf {
    <LibraryClasses>
      Tpm12DeviceLib|SecurityPkg/Library/Tpm12DeviceLibDTpm/Tpm12DeviceLibDTpm.inf
  }
!endif

  #------------------------------
  #  Build the shell
  #------------------------------

!if $(SHELL_TYPE) == BUILD_SHELL

  #
  # Shell Lib
  #
[LibraryClasses]
  BcfgCommandLib|ShellPkg/Library/UefiShellBcfgCommandLib/UefiShellBcfgCommandLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  !include NetworkPkg/NetworkLibs.dsc.inc

[Components.X64]
  ShellPkg/DynamicCommand/TftpDynamicCommand/TftpDynamicCommand.inf {
    <PcdsFixedAtBuild>
      ## This flag is used to control initialization of the shell library
      #  This should be FALSE for compiling the dynamic command.
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE
  }
  ShellPkg/DynamicCommand/DpDynamicCommand/DpDynamicCommand.inf {
    <PcdsFixedAtBuild>
      ## This flag is used to control initialization of the shell library
      #  This should be FALSE for compiling the dynamic command.
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE
  }
  ShellPkg/Application/Shell/Shell.inf {
    <PcdsFixedAtBuild>
      ## This flag is used to control initialization of the shell library
      #  This should be FALSE for compiling the shell application itself only.
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE

    #------------------------------
    #  Basic commands
    #------------------------------

    <LibraryClasses>
      NULL|ShellPkg/Library/UefiShellLevel1CommandsLib/UefiShellLevel1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel2CommandsLib/UefiShellLevel2CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel3CommandsLib/UefiShellLevel3CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDriver1CommandsLib/UefiShellDriver1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellInstall1CommandsLib/UefiShellInstall1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDebug1CommandsLib/UefiShellDebug1CommandsLib.inf

    #------------------------------
    #  Networking commands
    #------------------------------

    <LibraryClasses>
      NULL|ShellPkg/Library/UefiShellNetwork1CommandsLib/UefiShellNetwork1CommandsLib.inf

    #------------------------------
    #  Support libraries
    #------------------------------

    <LibraryClasses>
      DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
      DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
      HandleParsingLib|ShellPkg/Library/UefiHandleParsingLib/UefiHandleParsingLib.inf
      OrderedCollectionLib|MdePkg/Library/BaseOrderedCollectionRedBlackTreeLib/BaseOrderedCollectionRedBlackTreeLib.inf
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
      ShellCEntryLib|ShellPkg/Library/UefiShellCEntryLib/UefiShellCEntryLib.inf
      ShellCommandLib|ShellPkg/Library/UefiShellCommandLib/UefiShellCommandLib.inf
      SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
  }

!endif
