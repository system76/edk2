/** @file

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiPayloadEntry.h"

STATIC UINT32 mTopOfLowerUsableDram = 0;

/**
   Callback function to build resource descriptor HOB

   This function build a HOB based on the memory map entry info.
   It creates only EFI_RESOURCE_MEMORY_MAPPED_IO and EFI_RESOURCE_MEMORY_RESERVED
   resources.

   @param MemoryMapEntry         Memory map entry info got from bootloader.
   @param Params                 A pointer to ACPI_BOARD_INFO.

  @retval EFI_SUCCESS            Successfully build a HOB.
  @retval EFI_INVALID_PARAMETER  Invalid parameter provided.
**/
EFI_STATUS
MemInfoCallbackMmio (
  IN MEMROY_MAP_ENTRY          *MemoryMapEntry,
  IN VOID                      *Params
  )
{
  EFI_PHYSICAL_ADDRESS         Base;
  EFI_RESOURCE_TYPE            Type;
  UINT64                       Size;
  EFI_RESOURCE_ATTRIBUTE_TYPE  Attribue;
  ACPI_BOARD_INFO              *AcpiBoardInfo;

  AcpiBoardInfo = (ACPI_BOARD_INFO *)Params;
  if (AcpiBoardInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Skip types already handled in MemInfoCallback
  //
  if (MemoryMapEntry->Type == E820_RAM || MemoryMapEntry->Type == E820_ACPI) {
    return EFI_SUCCESS;
  }

  if (MemoryMapEntry->Base == AcpiBoardInfo->PcieBaseAddress) {
    //
    // MMCONF is always MMIO
    //
    Type = EFI_RESOURCE_MEMORY_MAPPED_IO;
  } else if (MemoryMapEntry->Base < mTopOfLowerUsableDram) {
    //
    // It's in DRAM and thus must be reserved
    //
    Type = EFI_RESOURCE_MEMORY_RESERVED;
  } else if ((MemoryMapEntry->Base < 0x100000000ULL) && (MemoryMapEntry->Base >= mTopOfLowerUsableDram)) {
    //
    // It's not in DRAM, must be MMIO
    //
    Type = EFI_RESOURCE_MEMORY_MAPPED_IO;
  } else {
    Type = EFI_RESOURCE_MEMORY_RESERVED;
  }

  Base    = MemoryMapEntry->Base;
  Size    = MemoryMapEntry->Size;

  Attribue = EFI_RESOURCE_ATTRIBUTE_PRESENT |
             EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
             EFI_RESOURCE_ATTRIBUTE_TESTED |
             EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE;

  BuildResourceDescriptorHob (Type, Attribue, (EFI_PHYSICAL_ADDRESS)Base, Size);
  DEBUG ((DEBUG_INFO , "buildhob: base = 0x%lx, size = 0x%lx, type = 0x%x\n", Base, Size, Type));

  if (MemoryMapEntry->Type == E820_UNUSABLE ||
    MemoryMapEntry->Type == E820_DISABLED) {
    BuildMemoryAllocationHob (Base, Size, EfiUnusableMemory);
  } else if (MemoryMapEntry->Type == E820_PMEM) {
    BuildMemoryAllocationHob (Base, Size, EfiPersistentMemory);
  }

  return EFI_SUCCESS;
}


/**
   Callback function to find TOLUD (Top of Lower Usable DRAM)

   Estimate where TOLUD (Top of Lower Usable DRAM) resides. The exact position
   would require platform specific code.

   @param MemoryMapEntry         Memory map entry info got from bootloader.
   @param Params                 Not used for now.

  @retval EFI_SUCCESS            Successfully updated mTopOfLowerUsableDram.
**/
EFI_STATUS
FindToludCallback (
  IN MEMROY_MAP_ENTRY          *MemoryMapEntry,
  IN VOID                      *Params
  )
{
  //
  // This code assumes that the memory map on this x86 machine below 4GiB is continous
  // until TOLUD. In addition it assumes that the bootloader provided memory tables have
  // no "holes" and thus the first memory range not covered by e820 marks the end of
  // usable DRAM. In addition it's assumed that every reserved memory region touching
  // usable RAM is also covering DRAM, everything else that is marked reserved thus must be
  // MMIO not detectable by bootloader/OS
  //

  //
  // Skip memory types not RAM or reserved
  //
  if ((MemoryMapEntry->Type == E820_UNUSABLE) || (MemoryMapEntry->Type == E820_DISABLED) ||
    (MemoryMapEntry->Type == E820_PMEM)) {
    return EFI_SUCCESS;
  }

  //
  // Skip resources above 4GiB
  //
  if ((MemoryMapEntry->Base + MemoryMapEntry->Size) > 0x100000000ULL) {
    return EFI_SUCCESS;
  }

  if ((MemoryMapEntry->Type == E820_RAM) || (MemoryMapEntry->Type == E820_ACPI) ||
    (MemoryMapEntry->Type == E820_NVS)) {
    //
    // It's usable DRAM. Update TOLUD.
    //
    if (mTopOfLowerUsableDram < (MemoryMapEntry->Base + MemoryMapEntry->Size)) {
      mTopOfLowerUsableDram = (UINT32)(MemoryMapEntry->Base + MemoryMapEntry->Size);
    }
  } else {
    //
    // It might be 'reserved DRAM' or 'MMIO'.
    //
    // If it touches usable DRAM at Base assume it's DRAM as well,
    // as it could be bootloader installed tables, TSEG, GTT, ...
    //
    if (mTopOfLowerUsableDram == MemoryMapEntry->Base) {
      mTopOfLowerUsableDram = (UINT32)(MemoryMapEntry->Base + MemoryMapEntry->Size);
    }
  }

  return EFI_SUCCESS;
}


/**
   Callback function to build resource descriptor HOB

   This function build a HOB based on the memory map entry info.
   Only add EFI_RESOURCE_SYSTEM_MEMORY.

   @param MemoryMapEntry         Memory map entry info got from bootloader.
   @param Params                 Not used for now.

  @retval RETURN_SUCCESS        Successfully build a HOB.
**/
EFI_STATUS
MemInfoCallback (
  IN MEMROY_MAP_ENTRY          *MemoryMapEntry,
  IN VOID                      *Params
  )
{
  EFI_PHYSICAL_ADDRESS         Base;
  EFI_RESOURCE_TYPE            Type;
  UINT64                       Size;
  EFI_RESOURCE_ATTRIBUTE_TYPE  Attribue;

  //
  // Skip everything not known to be usable DRAM.
  // It will be added later.
  //
  if ((MemoryMapEntry->Type != E820_RAM) && (MemoryMapEntry->Type != E820_ACPI) &&
    (MemoryMapEntry->Type != E820_NVS)) {
    return RETURN_SUCCESS;
  }

  Type    = EFI_RESOURCE_SYSTEM_MEMORY;
  Base    = MemoryMapEntry->Base;
  Size    = MemoryMapEntry->Size;

  Attribue = EFI_RESOURCE_ATTRIBUTE_PRESENT |
             EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
             EFI_RESOURCE_ATTRIBUTE_TESTED |
             EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE;

  BuildResourceDescriptorHob (Type, Attribue, (EFI_PHYSICAL_ADDRESS)Base, Size);
  DEBUG ((DEBUG_INFO , "buildhob: base = 0x%lx, size = 0x%lx, type = 0x%x\n", Base, Size, Type));

  if (MemoryMapEntry->Type == E820_ACPI) {
    BuildMemoryAllocationHob (Base, Size, EfiACPIReclaimMemory);
  } else if (MemoryMapEntry->Type == E820_NVS) {
    BuildMemoryAllocationHob (Base, Size, EfiACPIMemoryNVS);
  }

  return RETURN_SUCCESS;
}



/**
  Find the board related info from ACPI table

  @param  AcpiTableBase          ACPI table start address in memory
  @param  AcpiBoardInfo          Pointer to the acpi board info strucutre

  @retval RETURN_SUCCESS     Successfully find out all the required information.
  @retval RETURN_NOT_FOUND   Failed to find the required info.

**/
RETURN_STATUS
ParseAcpiInfo (
  IN   UINT64                                   AcpiTableBase,
  OUT  ACPI_BOARD_INFO                          *AcpiBoardInfo
  )
{
  EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp;
  EFI_ACPI_DESCRIPTION_HEADER                   *Rsdt;
  UINT32                                        *Entry32;
  UINTN                                         Entry32Num;
  EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE     *Fadt;
  EFI_ACPI_DESCRIPTION_HEADER                   *Xsdt;
  UINT64                                        *Entry64;
  UINTN                                         Entry64Num;
  UINTN                                         Idx;
  UINT32                                        *Signature;
  EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *MmCfgHdr;
  EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE *MmCfgBase;
  UINTN                                         TPM2TablePresent;
  UINTN                                         TCPATablePresent;

  Rsdp = (EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)(UINTN)AcpiTableBase;
  DEBUG ((DEBUG_INFO, "Rsdp at 0x%p\n", Rsdp));
  DEBUG ((DEBUG_INFO, "Rsdt at 0x%x, Xsdt at 0x%lx\n", Rsdp->RsdtAddress, Rsdp->XsdtAddress));

  TPM2TablePresent = 0;
  TCPATablePresent = 0;

  //
  // Search Rsdt First
  //
  Fadt     = NULL;
  MmCfgHdr = NULL;
  Rsdt     = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)(Rsdp->RsdtAddress);
  if (Rsdt != NULL) {
    Entry32  = (UINT32 *)(Rsdt + 1);
    Entry32Num = (Rsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 2;
    for (Idx = 0; Idx < Entry32Num; Idx++) {
      Signature = (UINT32 *)(UINTN)Entry32[Idx];
      if (*Signature == EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
        Fadt = (EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE *)Signature;
        DEBUG ((DEBUG_INFO, "Found Fadt in Rsdt\n"));
      }

      if (*Signature == EFI_ACPI_5_0_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE) {
        MmCfgHdr = (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *)Signature;
        DEBUG ((DEBUG_INFO, "Found MM config address in Rsdt\n"));
      }

      if (*Signature == EFI_ACPI_5_0_TRUSTED_COMPUTING_PLATFORM_2_TABLE_SIGNATURE) {
        TPM2TablePresent = 1;
      }

      if (*Signature == EFI_ACPI_5_0_TRUSTED_COMPUTING_PLATFORM_ALLIANCE_CAPABILITIES_TABLE_SIGNATURE) {
        TCPATablePresent = 1;
      }
    }
  }

  //
  // Search Xsdt Second
  //
  Xsdt     = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)(Rsdp->XsdtAddress);
  if (Xsdt != NULL) {
    Entry64  = (UINT64 *)(Xsdt + 1);
    Entry64Num = (Xsdt->Length - sizeof(EFI_ACPI_DESCRIPTION_HEADER)) >> 3;
    for (Idx = 0; Idx < Entry64Num; Idx++) {
      Signature = (UINT32 *)(UINTN)Entry64[Idx];
      if (*Signature == EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
        Fadt = (EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE *)Signature;
        DEBUG ((DEBUG_INFO, "Found Fadt in Xsdt\n"));
      }

      if (*Signature == EFI_ACPI_5_0_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE) {
        MmCfgHdr = (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *)Signature;
        DEBUG ((DEBUG_INFO, "Found MM config address in Xsdt\n"));
      }

      if (*Signature == EFI_ACPI_5_0_TRUSTED_COMPUTING_PLATFORM_2_TABLE_SIGNATURE) {
        TPM2TablePresent = 1;
      }

      if (*Signature == EFI_ACPI_5_0_TRUSTED_COMPUTING_PLATFORM_ALLIANCE_CAPABILITIES_TABLE_SIGNATURE) {
        TCPATablePresent = 1;
      }
    }
  }

  if (Fadt == NULL) {
    return RETURN_NOT_FOUND;
  }

  AcpiBoardInfo->TPM20Present = TPM2TablePresent;
  AcpiBoardInfo->TPM12Present = TCPATablePresent;

  AcpiBoardInfo->PmCtrlRegBase   = Fadt->Pm1aCntBlk;
  AcpiBoardInfo->PmTimerRegBase  = Fadt->PmTmrBlk;
  AcpiBoardInfo->ResetRegAddress = Fadt->ResetReg.Address;
  AcpiBoardInfo->ResetValue      = Fadt->ResetValue;
  AcpiBoardInfo->PmEvtBase       = Fadt->Pm1aEvtBlk;
  AcpiBoardInfo->PmGpeEnBase     = Fadt->Gpe0Blk + Fadt->Gpe0BlkLen / 2;

  if (MmCfgHdr != NULL) {
    MmCfgBase = (EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE *)((UINT8*) MmCfgHdr + sizeof (*MmCfgHdr));
    AcpiBoardInfo->PcieBaseAddress = MmCfgBase->BaseAddress;
    AcpiBoardInfo->PcieBaseSize = (MmCfgBase->EndBusNumber + 1 - MmCfgBase->StartBusNumber) * 4096 * 32 * 8;
  } else {
    AcpiBoardInfo->PcieBaseAddress = 0;
    AcpiBoardInfo->PcieBaseSize = 0;
  }
  DEBUG ((DEBUG_INFO, "PmCtrl  Reg 0x%lx\n",  AcpiBoardInfo->PmCtrlRegBase));
  DEBUG ((DEBUG_INFO, "PmTimer Reg 0x%lx\n",  AcpiBoardInfo->PmTimerRegBase));
  DEBUG ((DEBUG_INFO, "Reset   Reg 0x%lx\n",  AcpiBoardInfo->ResetRegAddress));
  DEBUG ((DEBUG_INFO, "Reset   Value 0x%x\n", AcpiBoardInfo->ResetValue));
  DEBUG ((DEBUG_INFO, "PmEvt   Reg 0x%lx\n",  AcpiBoardInfo->PmEvtBase));
  DEBUG ((DEBUG_INFO, "PmGpeEn Reg 0x%lx\n",  AcpiBoardInfo->PmGpeEnBase));
  DEBUG ((DEBUG_INFO, "PcieBaseAddr 0x%lx\n", AcpiBoardInfo->PcieBaseAddress));
  DEBUG ((DEBUG_INFO, "PcieBaseSize 0x%lx\n", AcpiBoardInfo->PcieBaseSize));
  DEBUG ((DEBUG_INFO, "TPM 2.0 present %x\n", AcpiBoardInfo->TPM20Present));
  DEBUG ((DEBUG_INFO, "TPM 1.2 present %x\n", AcpiBoardInfo->TPM12Present));

  //
  // Verify values for proper operation
  //
  ASSERT(Fadt->Pm1aCntBlk != 0);
  ASSERT(Fadt->PmTmrBlk != 0);
  ASSERT(Fadt->ResetReg.Address != 0);
  ASSERT(Fadt->Pm1aEvtBlk != 0);
  ASSERT(Fadt->Gpe0Blk != 0);

  DEBUG_CODE_BEGIN ();
    BOOLEAN    SciEnabled;

    //
    // Check the consistency of SCI enabling
    //

    //
    // Get SCI_EN value
    //
   if (Fadt->Pm1CntLen == 4) {
      SciEnabled = (IoRead32 (Fadt->Pm1aCntBlk) & BIT0)? TRUE : FALSE;
    } else {
      //
      // if (Pm1CntLen == 2), use 16 bit IO read;
      // if (Pm1CntLen != 2 && Pm1CntLen != 4), use 16 bit IO read as a fallback
      //
      SciEnabled = (IoRead16 (Fadt->Pm1aCntBlk) & BIT0)? TRUE : FALSE;
    }

    if (!(Fadt->Flags & EFI_ACPI_5_0_HW_REDUCED_ACPI) &&
        (Fadt->SmiCmd == 0) &&
       !SciEnabled) {
      //
      // The ACPI enabling status is inconsistent: SCI is not enabled but ACPI
      // table does not provide a means to enable it through FADT->SmiCmd
      //
      DEBUG ((DEBUG_ERROR, "ERROR: The ACPI enabling status is inconsistent: SCI is not"
        " enabled but the ACPI table does not provide a means to enable it through FADT->SmiCmd."
        " This may cause issues in OS.\n"));
    }
  DEBUG_CODE_END ();

  return RETURN_SUCCESS;
}


/**
  It will build HOBs based on information from bootloaders.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required HOBs.
**/
EFI_STATUS
BuildHobFromBl (
  VOID
  )
{
  EFI_STATUS                       Status;
  SYSTEM_TABLE_INFO                SysTableInfo;
  SYSTEM_TABLE_INFO                *NewSysTableInfo;
  ACPI_BOARD_INFO                  AcpiBoardInfo;
  ACPI_BOARD_INFO                  *NewAcpiBoardInfo;
  SMMSTORE_INFO                    SMMSTOREInfo;
  SMMSTORE_INFO                    *NewSMMSTOREInfo;
  TCG_PHYSICAL_PRESENCE_INFO       PhysicalPresenceInfo;
  TCG_PHYSICAL_PRESENCE_INFO       *NewPhysicalPresenceInfo;
  EFI_PEI_GRAPHICS_INFO_HOB        GfxInfo;
  EFI_PEI_GRAPHICS_INFO_HOB        *NewGfxInfo;
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB GfxDeviceInfo;
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB *NewGfxDeviceInfo;
  UNIVERSAL_PAYLOAD_SMBIOS_TABLE   *SmBiosTableHob;
  UNIVERSAL_PAYLOAD_ACPI_TABLE     *AcpiTableHob;

  //
  // First find TOLUD
  //
  DEBUG ((DEBUG_INFO , "Guessing Top of Lower Usable DRAM:\n"));
  Status = ParseMemoryInfo (FindToludCallback, NULL);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  DEBUG ((DEBUG_INFO , "Assuming TOLUD = 0x%x\n", mTopOfLowerUsableDram));

  //
  // Parse memory info and build memory HOBs for Usable RAM
  //
  DEBUG ((DEBUG_INFO , "Building ResourceDescriptorHobs for usable memory:\n"));
  Status = ParseMemoryInfo (MemInfoCallback, NULL);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Create guid hob for frame buffer information
  //
  Status = ParseGfxInfo (&GfxInfo);
  if (!EFI_ERROR (Status)) {
    NewGfxInfo = BuildGuidHob (&gEfiGraphicsInfoHobGuid, sizeof (GfxInfo));
    ASSERT (NewGfxInfo != NULL);
    CopyMem (NewGfxInfo, &GfxInfo, sizeof (GfxInfo));
    DEBUG ((DEBUG_INFO, "Created graphics info hob\n"));
  }


  Status = ParseGfxDeviceInfo (&GfxDeviceInfo);
  if (!EFI_ERROR (Status)) {
    NewGfxDeviceInfo = BuildGuidHob (&gEfiGraphicsDeviceInfoHobGuid, sizeof (GfxDeviceInfo));
    ASSERT (NewGfxDeviceInfo != NULL);
    CopyMem (NewGfxDeviceInfo, &GfxDeviceInfo, sizeof (GfxDeviceInfo));
    DEBUG ((DEBUG_INFO, "Created graphics device info hob\n"));
  }

  //
  // Create guid hob for SMMSTORE
  //
  Status = ParseSMMSTOREInfo (&SMMSTOREInfo);
  if (!EFI_ERROR (Status)) {
    NewSMMSTOREInfo = BuildGuidHob (&gEfiSMMSTOREInfoHobGuid, sizeof (SMMSTOREInfo));
    ASSERT (NewSMMSTOREInfo != NULL);
    CopyMem (NewSMMSTOREInfo, &SMMSTOREInfo, sizeof (SMMSTOREInfo));
    DEBUG ((DEBUG_INFO, "Created SMMSTORE info hob\n"));
  }

  //
  // Create guid hob for Tcg Physical Presence Interface
  //
  Status = ParseTPMPPIInfo (&PhysicalPresenceInfo);
  if (!EFI_ERROR (Status)) {
    NewPhysicalPresenceInfo = BuildGuidHob (&gEfiTcgPhysicalPresenceInfoHobGuid, sizeof (TCG_PHYSICAL_PRESENCE_INFO));
    ASSERT (NewPhysicalPresenceInfo != NULL);
    CopyMem (NewPhysicalPresenceInfo, &PhysicalPresenceInfo, sizeof (TCG_PHYSICAL_PRESENCE_INFO));
    DEBUG ((DEBUG_INFO, "Created Tcg Physical Presence info hob\n"));
  }

  //
  // Create guid hob for system tables like acpi table and smbios table
  //
  Status = ParseSystemTable(&SysTableInfo);
  ASSERT_EFI_ERROR (Status);
  if (!EFI_ERROR (Status)) {
    NewSysTableInfo = BuildGuidHob (&gUefiSystemTableInfoGuid, sizeof (SYSTEM_TABLE_INFO));
    ASSERT (NewSysTableInfo != NULL);
    CopyMem (NewSysTableInfo, &SysTableInfo, sizeof (SYSTEM_TABLE_INFO));
    DEBUG ((DEBUG_INFO, "Detected Acpi Table at 0x%lx, length 0x%x\n", SysTableInfo.AcpiTableBase, SysTableInfo.AcpiTableSize));
    DEBUG ((DEBUG_INFO, "Detected Smbios Table at 0x%lx, length 0x%x\n", SysTableInfo.SmbiosTableBase, SysTableInfo.SmbiosTableSize));
  }
  //
  // Creat SmBios table Hob
  //
  SmBiosTableHob = BuildGuidHob (&gUniversalPayloadSmbiosTableGuid, sizeof (UNIVERSAL_PAYLOAD_SMBIOS_TABLE));
  ASSERT (SmBiosTableHob != NULL);
  SmBiosTableHob->Header.Revision = UNIVERSAL_PAYLOAD_SMBIOS_TABLE_REVISION;
  SmBiosTableHob->Header.Length = sizeof (UNIVERSAL_PAYLOAD_SMBIOS_TABLE);
  SmBiosTableHob->SmBiosEntryPoint = SysTableInfo.SmbiosTableBase;
  DEBUG ((DEBUG_INFO, "Create smbios table gUniversalPayloadSmbiosTableGuid guid hob\n"));

  //
  // Creat ACPI table Hob
  //
  AcpiTableHob = BuildGuidHob (&gUniversalPayloadAcpiTableGuid, sizeof (UNIVERSAL_PAYLOAD_ACPI_TABLE));
  ASSERT (AcpiTableHob != NULL);
  AcpiTableHob->Header.Revision = UNIVERSAL_PAYLOAD_ACPI_TABLE_REVISION;
  AcpiTableHob->Header.Length = sizeof (UNIVERSAL_PAYLOAD_ACPI_TABLE);
  AcpiTableHob->Rsdp = SysTableInfo.AcpiTableBase;
  DEBUG ((DEBUG_INFO, "Create smbios table gUniversalPayloadAcpiTableGuid guid hob\n"));

  //
  // Create guid hob for acpi board information
  //
  Status = ParseAcpiInfo (SysTableInfo.AcpiTableBase, &AcpiBoardInfo);
  ASSERT_EFI_ERROR (Status);
  if (!EFI_ERROR (Status)) {
    NewAcpiBoardInfo = BuildGuidHob (&gUefiAcpiBoardInfoGuid, sizeof (ACPI_BOARD_INFO));
    ASSERT (NewAcpiBoardInfo != NULL);
    CopyMem (NewAcpiBoardInfo, &AcpiBoardInfo, sizeof (ACPI_BOARD_INFO));
    DEBUG ((DEBUG_INFO, "Create acpi board info guid hob\n"));
  }

  //
  // Parse memory info and build memory HOBs for reserved DRAM and MMIO
  //
  DEBUG ((DEBUG_INFO , "Building ResourceDescriptorHobs for reserved memory:\n"));
  Status = ParseMemoryInfo (MemInfoCallbackMmio, &AcpiBoardInfo);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Parse platform specific information.
  //
  Status = ParsePlatformInfo ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error when parsing platform info, Status = %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}


/**
  This function will build some generic HOBs that doesn't depend on information from bootloaders.

**/
VOID
BuildGenericHob (
  VOID
  )
{
  UINT32                           RegEax;
  UINT8                            PhysicalAddressBits;
  EFI_RESOURCE_ATTRIBUTE_TYPE      ResourceAttribute;

  // The UEFI payload FV
  BuildMemoryAllocationHob (PcdGet32 (PcdPayloadFdMemBase), PcdGet32 (PcdPayloadFdMemSize), EfiBootServicesData);

  //
  // Build CPU memory space and IO space hob
  //
  AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
  if (RegEax >= 0x80000008) {
    AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
    PhysicalAddressBits = (UINT8) RegEax;
  } else {
    PhysicalAddressBits  = 36;
  }

  BuildCpuHob (PhysicalAddressBits, 16);

  //
  // Report Local APIC range, cause sbl HOB to be NULL, comment now
  //
  ResourceAttribute = (
      EFI_RESOURCE_ATTRIBUTE_PRESENT |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_TESTED
  );
  BuildResourceDescriptorHob (EFI_RESOURCE_MEMORY_MAPPED_IO, ResourceAttribute, 0xFEC80000, SIZE_512KB);
  BuildMemoryAllocationHob ( 0xFEC80000, SIZE_512KB, EfiMemoryMappedIO);

}


/**
  Entry point to the C language phase of UEFI payload.

  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.
**/
EFI_STATUS
EFIAPI
PayloadEntry (
  IN UINTN                     BootloaderParameter
  )
{
  EFI_STATUS                    Status;
  PHYSICAL_ADDRESS              DxeCoreEntryPoint;
  UINTN                         MemBase;
  UINTN                         HobMemBase;
  UINTN                         HobMemTop;
  EFI_PEI_HOB_POINTERS          Hob;

  // Call constructor for all libraries
  ProcessLibraryConstructorList ();

  DEBUG ((DEBUG_INFO, "GET_BOOTLOADER_PARAMETER() = 0x%lx\n", GET_BOOTLOADER_PARAMETER()));
  DEBUG ((DEBUG_INFO, "sizeof(UINTN) = 0x%x\n", sizeof(UINTN)));

  // Initialize floating point operating environment to be compliant with UEFI spec.
  InitializeFloatingPointUnits ();

  // HOB region is used for HOB and memory allocation for this module
  MemBase    = PcdGet32 (PcdPayloadFdMemBase);
  HobMemBase = ALIGN_VALUE (MemBase + PcdGet32 (PcdPayloadFdMemSize), SIZE_1MB);
  HobMemTop  = HobMemBase + FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);

  HobConstructor ((VOID *)MemBase, (VOID *)HobMemTop, (VOID *)HobMemBase, (VOID *)HobMemTop);

  // Build HOB based on information from Bootloader
  Status = BuildHobFromBl ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "BuildHobFromBl Status = %r\n", Status));
    return Status;
  }

  // Build other HOBs required by DXE
  BuildGenericHob ();

  // Load the DXE Core
  Status = LoadDxeCore (&DxeCoreEntryPoint);
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "DxeCoreEntryPoint = 0x%lx\n", DxeCoreEntryPoint));

  //
  // Mask off all legacy 8259 interrupt sources
  //
  IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, 0xFF);
  IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE,  0xFF);

  Hob.HandoffInformationTable = (EFI_HOB_HANDOFF_INFO_TABLE *) GetFirstHob(EFI_HOB_TYPE_HANDOFF);
  HandOffToDxeCore (DxeCoreEntryPoint, Hob);

  // Should not get here
  CpuDeadLoop ();
  return EFI_SUCCESS;
}
