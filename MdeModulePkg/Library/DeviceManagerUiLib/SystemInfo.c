// SPDX-License-Identifier: BSD-2-Clause-Patent
// SPDX-FileCopyrightText: Copyright (c) 2006 - 2021, Intel Corporation. All rights reserved.<BR>
// SPDX-FileCopyrightText: (C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
// SPDX-FileCopyrightText: (c) 2020, AMD Inc. All rights reserved.<BR>
// SPDX-FileCopyrightText: 2026 System76, Inc.

// Hodge podge of system info to show on the form.

#include <Guid/Acpi.h>
#include <IndustryStandard/Pci.h>
#include <Protocol/PciIo.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/UsbIo.h>
#include <Register/Amd/Cpuid.h>
#include <Register/Intel/Cpuid.h>
#include <Register/Intel/Msr.h>

#include "DeviceManager.h"

// From UefiCpuLib
STATIC
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

STATIC
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

STATIC CONST CHAR8 RSDP_SIGNATURE[8] = {'R', 'S', 'D', ' ', 'P', 'T', 'R', ' '};

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

STATIC CONST CHAR8 RSDT_SIGNATURE[4] = {'R', 'S', 'D', 'T'};

STATIC
ACPI_SDT_HEADER *
FindAcpiTable (
    CHAR8 Name[4]
    )
{
  UINTN                    Index;
  EFI_CONFIGURATION_TABLE* ConfigurationTable;
  UINTN                    RsdpPtr;
  ACPI_RSDP*               Rsdp;
  UINTN                    RsdtPtr;
  ACPI_SDT_HEADER*         Rsdt;
  UINTN                    TablePtr;
  ACPI_SDT_HEADER*         Table;

  DEBUG ((DEBUG_INFO, "FindAcpiTable: '%c%c%c%c'\n",
    Name[0],
    Name[1],
    Name[2],
    Name[3]
  ));

  // Search the table for an entry that matches the ACPI Table Guid
  for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
    if (CompareGuid (&gEfiAcpiTableGuid, &(gST->ConfigurationTable[Index].VendorGuid))) {
      ConfigurationTable = &gST->ConfigurationTable[Index];
      break;
    }
  }

  if (ConfigurationTable == NULL) {
      DEBUG ((DEBUG_INFO, "  ACPI Configuration Table missing\n"));
      return NULL;
  }

  RsdpPtr = (UINTN)ConfigurationTable->VendorTable;
  DEBUG ((DEBUG_INFO, "  RSDP 0x%x\n", RsdpPtr));
  Rsdp = (ACPI_RSDP*)RsdpPtr;
  DEBUG ((DEBUG_INFO, "    Signature: '%c%c%c%c%c%c%c%c'\n",
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
      DEBUG ((DEBUG_INFO, "    RSDP invalid signature\n"));
      return NULL;
  }
  DEBUG ((DEBUG_INFO, "    Revision: 0x%x\n", Rsdp->Revision));

  RsdtPtr = (UINTN)Rsdp->RsdtAddress;
  DEBUG ((DEBUG_INFO, "  RSDT 0x%x\n", RsdpPtr));
  Rsdt = (ACPI_SDT_HEADER*)RsdtPtr;
  DEBUG ((DEBUG_INFO, "    Signature: '%c%c%c%c'\n",
    Rsdt->Signature[0],
    Rsdt->Signature[1],
    Rsdt->Signature[2],
    Rsdt->Signature[3]
  ));
  if (CompareMem(Rsdt->Signature, RSDT_SIGNATURE, 4) != 0) {
      DEBUG ((DEBUG_INFO, "    RSDT invalid signature\n"));
      return NULL;
  }
  DEBUG ((DEBUG_INFO, "    Revision: 0x%x\n", Rsdt->Revision));
  DEBUG ((DEBUG_INFO, "    Length: 0x%x\n", Rsdt->Length));

  for (Index = sizeof(ACPI_SDT_HEADER); Index < Rsdt->Length; Index += 4) {
      TablePtr = (UINTN)(*(UINT32*)(RsdtPtr + Index));
      DEBUG ((DEBUG_INFO, "  Table %d: 0x%x\n", Index, TablePtr));
      Table = (ACPI_SDT_HEADER*)TablePtr;
      DEBUG ((DEBUG_INFO, "    Signature: '%c%c%c%c'\n",
        Table->Signature[0],
        Table->Signature[1],
        Table->Signature[2],
        Table->Signature[3]
      ));
      DEBUG ((DEBUG_INFO, "    Revision: 0x%x\n", Table->Revision));
      DEBUG ((DEBUG_INFO, "    Length: 0x%x\n", Table->Length));

      if (CompareMem(Table->Signature, Name, 4) == 0) {
          DEBUG ((DEBUG_INFO, "  Match found\n"));
          return Table;
      }
  }

  DEBUG ((DEBUG_INFO, "  No match found\n"));
  return NULL;
}

// From PciBusDxe
STATIC
EFI_STATUS
PciDevicePresent (
    OUT PCI_TYPE00  *Pci,
    IN  UINT8       Bus,
    IN  UINT8       Device,
    IN  UINT8       Func
    )
{
    UINT64 Address = EFI_PCI_ADDRESS(Bus, Device, Func, 0);
    EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;
    EFI_STATUS Status;
    EFI_HANDLE *PciIoBuffer;
    UINTN PciIoHandleCount = 0;

    Status = gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiPciRootBridgeIoProtocolGuid,
        NULL,
        &PciIoHandleCount,
        &PciIoBuffer
    );

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_INFO, "%a: Failed to get PciRootBridgeIo handles: %r\n", __FUNCTION__, Status));
        return Status;
    }

    for (UINTN Index = 0; Index < PciIoHandleCount; Index++) {
        Status = gBS->OpenProtocol(
            PciIoBuffer[Index],
            &gEfiPciRootBridgeIoProtocolGuid,
            (VOID *)&PciRootBridgeIo,
            NULL,
            NULL,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
        );

        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_INFO, "%a: Failed to open PciRootBridgeIo protocol: %r\n", __FUNCTION__, Status));
            continue;
        }

        // Read the Vendor ID register
        Status = PciRootBridgeIo->Pci.Read(
            PciRootBridgeIo,
            EfiPciWidthUint32,
            Address,
            1,
            Pci
        );

        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_INFO, "%a: Failed to read vendor ID: %r\n", __FUNCTION__, Status));
            continue;
        }

        // Read the entire config header for the device
        Status = PciRootBridgeIo->Pci.Read(
            PciRootBridgeIo,
            EfiPciWidthUint32,
            Address,
            sizeof(PCI_TYPE00) / sizeof(UINT32),
            Pci
        );

        FreePool(PciIoBuffer);
        return Status;
    }

    FreePool(PciIoBuffer);
    return EFI_NOT_FOUND;
}

/*
 * Check for Intel device with class [0780] at 00:16.0.
 */
STATIC
BOOLEAN
HasCsmeDevice (
    VOID
    )
{
    PCI_TYPE00 Pci;

    if (!EFI_ERROR (PciDevicePresent (&Pci, 0x00, 0x16, 0x00))) {
        DEBUG((DEBUG_INFO, "%a: vid=0x%04X, class=[%02X,%02X,%02X]\n", __FUNCTION__,
            Pci.Hdr.VendorId, Pci.Hdr.ClassCode[0], Pci.Hdr.ClassCode[1], Pci.Hdr.ClassCode[2]));
        return Pci.Hdr.VendorId == 0x8086 &&
            Pci.Hdr.ClassCode[2] == PCI_CLASS_SCC &&
            Pci.Hdr.ClassCode[1] == PCI_SUBCLASS_SCC_OTHER;
    }

    return FALSE;
}

VOID
FirmwareConfigurationInformation (
    VOID
    )
{
    EFI_STRING_ID Token;

    Token = STRING_TOKEN (STR_VIRTUALIZATION);
    if (StandardSignatureIsGenuineIntel()) {
        HiiSetString (gDeviceManagerPrivate.HiiHandle, Token, L"Intel Virtualization", NULL);

        Token = STRING_TOKEN (STR_VIRTUALIZATION_STATUS);
        CPUID_VERSION_INFO_ECX VersionInfoEcx;
        AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, &VersionInfoEcx.Uint32, NULL);
        if (VersionInfoEcx.Bits.VMX) {
            HiiSetString (gDeviceManagerPrivate.HiiHandle, Token, L"- VT-x: Active", NULL);
        } else {
            HiiSetString (gDeviceManagerPrivate.HiiHandle, Token, L"- VT-x: Deactivated", NULL);
        }

        Token = STRING_TOKEN (STR_IOMMU_STATUS);
        CHAR8 TableName[4] = {'D', 'M', 'A', 'R'};
        if (FindAcpiTable(TableName)) {
            HiiSetString (gDeviceManagerPrivate.HiiHandle, Token, L"- VT-d: Active", NULL);
        } else {
            HiiSetString (gDeviceManagerPrivate.HiiHandle, Token, L"- VT-d: Deactivated", NULL);
        }

        Token = STRING_TOKEN(STR_ME_STATUS);
        if (HasCsmeDevice()) {
            HiiSetString (gDeviceManagerPrivate.HiiHandle, Token, L"The Intel Management Engine is enabled.", NULL);
        } else {
            HiiSetString (gDeviceManagerPrivate.HiiHandle, Token, L"The Intel Management Engine is disabled at runtime to increase security.", NULL);
        }
    } else if (StandardSignatureIsAuthenticAMD()) {
        // TODO: verify AMD tests

        HiiSetString (gDeviceManagerPrivate.HiiHandle, Token, L"AMD Virtualization", NULL);

        Token = STRING_TOKEN (STR_VIRTUALIZATION_STATUS);
        CPUID_AMD_EXTENDED_CPU_SIG_ECX AmdExtendedCpuSigEcx;
        AsmCpuid (CPUID_EXTENDED_CPU_SIG, NULL, NULL, &AmdExtendedCpuSigEcx.Uint32, NULL);
        if (AmdExtendedCpuSigEcx.Bits.SVM) {
            HiiSetString (gDeviceManagerPrivate.HiiHandle, Token, L"- AMD-V: Active", NULL);
        } else {
            HiiSetString (gDeviceManagerPrivate.HiiHandle, Token, L"- AMD-V: Deactivated", NULL);
        }

        // TODO: proper test for AMD IOMMU
        /*
        Token = STRING_TOKEN (STR_IOMMU_STATUS);
        BOOLEAN iommu_active = FALSE;
        if (iommu_active) {
            HiiSetString (gDeviceManagerPrivate.HiiHandle, Token, L"- AMD-Vi: Active", NULL);
        } else {
            HiiSetString (gDeviceManagerPrivate.HiiHandle, Token, L"- AMD-Vi: Deactivated", NULL);
        }
        */
    }

    Token = STRING_TOKEN (STR_TPM_STATUS);
    CHAR8 TableName[4] = {'T', 'P', 'M', '2'};
    if (FindAcpiTable(TableName)) {
        HiiSetString (gDeviceManagerPrivate.HiiHandle, Token, L"Trusted Platform Module: Active", NULL);
    } else {
        HiiSetString (gDeviceManagerPrivate.HiiHandle, Token, L"Trusted Platform Module: Deactivated", NULL);
    }
}
