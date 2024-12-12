// SPDX-License-Identifier: BSD-2-Clause-Patent
// SPDX-FileCopyrightText: 2016 Microsoft Corporation
// SPDX-FileCopyrightText: 2018 Intel Corporation
// SPDX-FileCopyrightText: 2024 System76, Inc.

// Used by PlatformBootManagerLib to show boot logo and progress bar.

#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/PlatformLogo.h>
#include <Protocol/BootLogo2.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

/**
  Show LOGO returned from Edkii Platform Logo protocol on all consoles.

  @retval EFI_SUCCESS     Logo was displayed.
  @retval EFI_UNSUPPORTED Logo was not found or cannot be displayed.
**/
EFI_STATUS
EFIAPI
BootLogoEnableLogo (
  VOID
  )
{
  EFI_STATUS                             Status;
  EDKII_PLATFORM_LOGO_PROTOCOL           *PlatformLogo;
  EDKII_PLATFORM_LOGO_DISPLAY_ATTRIBUTE  Attribute;
  INTN                                   OffsetX;
  INTN                                   OffsetY;
  UINT32                                 SizeOfX;
  UINT32                                 SizeOfY;
  INTN                                   DestX = 0;
  INTN                                   DestY = 0;
  UINT32                                 Instance = 0;
  EFI_IMAGE_INPUT                        Image;
  EFI_GRAPHICS_OUTPUT_PROTOCOL           *GraphicsOutput;
  EDKII_BOOT_LOGO2_PROTOCOL              *BootLogo2;

  Status = gBS->LocateProtocol (&gEdkiiPlatformLogoProtocolGuid, NULL, (VOID **)&PlatformLogo);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  // Try to open GOP first
  Status = gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID **)&GraphicsOutput);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  // Try to open Boot Logo 2 Protocol.
  Status = gBS->LocateProtocol (&gEdkiiBootLogo2ProtocolGuid, NULL, (VOID **)&BootLogo2);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  // Erase Cursor from screen
  gST->ConOut->EnableCursor (gST->ConOut, FALSE);

  SizeOfX = GraphicsOutput->Mode->Info->HorizontalResolution;
  SizeOfY = GraphicsOutput->Mode->Info->VerticalResolution;

  // Get image from PlatformLogo protocol.
  Status = PlatformLogo->GetImage (
                           PlatformLogo,
                           &Instance,
                           &Image,
                           &Attribute,
                           &OffsetX,
                           &OffsetY
                           );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Center logo horizontally and 38.2% from top of screen
  DestX = (SizeOfX - Image.Width) / 2;
  DestY = (SizeOfY * 382) / 1000 - Image.Height / 2;

  DestX += OffsetX;
  DestY += OffsetY;

  Status = GraphicsOutput->Blt (
                             GraphicsOutput,
                             Image.Bitmap,
                             EfiBltBufferToVideo,
                             0,
                             0,
                             (UINTN)DestX,
                             (UINTN)DestY,
                             Image.Width,
                             Image.Height,
                             Image.Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                             );
  if (EFI_ERROR (Status)) {
    FreePool (Image.Bitmap);
    return Status;
  }

  // Attempt to register logo with Boot Logo 2 Protocol
  Status = BootLogo2->SetBootLogo (BootLogo2, Image.Bitmap, DestX, DestY, Image.Width, Image.Height);

  FreePool (Image.Bitmap);

  // Status of this function is EFI_SUCCESS even if registration with Boot
  // Logo 2 Protocol fails.
  return EFI_SUCCESS;
}

/**
  Use SystemTable Conout to turn on video based Simple Text Out consoles. The
  Simple Text Out screens will now be synced up with all non video output devices

  @retval EFI_SUCCESS     Graphics devices are back in text mode and synced up.
**/
EFI_STATUS
EFIAPI
BootLogoDisableLogo (
  VOID
  )
{
  // Enable Cursor on Screen
  gST->ConOut->EnableCursor (gST->ConOut, TRUE);
  return EFI_SUCCESS;
}

/**

  Update progress bar with title above it. It only works in Graphics mode.

  @param TitleForeground Foreground color for Title.
  @param TitleBackground Background color for Title.
  @param Title           Title above progress bar.
  @param ProgressColor   Progress bar color.
  @param Progress        Progress (0-100)
  @param PreviousValue   The previous value of the progress.

  @retval  EFI_STATUS       Success update the progress bar

**/
EFI_STATUS
EFIAPI
BootLogoUpdateProgress (
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  TitleForeground,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  TitleBackground,
  IN CHAR16                         *Title,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  ProgressColor,
  IN UINTN                          Progress,
  IN UINTN                          PreviousValue
  )
{
  EFI_STATUS                     Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL   *GraphicsOutput;
  UINT32                         SizeOfX;
  UINT32                         SizeOfY;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Color;
  UINTN                          BlockHeight;
  UINTN                          BlockWidth;
  UINTN                          BlockNum;
  UINTN                          PosX;
  UINTN                          PosY;
  UINTN                          Index;

  if (Progress > 100) {
    return EFI_INVALID_PARAMETER;
  }

  Status  = gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID **)&GraphicsOutput);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  SizeOfX = GraphicsOutput->Mode->Info->HorizontalResolution;
  SizeOfY = GraphicsOutput->Mode->Info->VerticalResolution;

  BlockWidth  = SizeOfX / 100;
  BlockHeight = SizeOfY / 50;

  BlockNum = Progress;

  PosX = 0;
  PosY = SizeOfY * 48 / 50;

  if (BlockNum == 0) {
    //
    // Clear progress area
    //
    SetMem (&Color, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0x0);

    Status = GraphicsOutput->Blt (
                               GraphicsOutput,
                               &Color,
                               EfiBltVideoFill,
                               0,
                               0,
                               0,
                               PosY - EFI_GLYPH_HEIGHT - 1,
                               SizeOfX,
                               SizeOfY - (PosY - EFI_GLYPH_HEIGHT - 1),
                               SizeOfX * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                               );
  }

  //
  // Show progress by drawing blocks
  //
  for (Index = PreviousValue; Index < BlockNum; Index++) {
    PosX = Index * BlockWidth;
    Status = GraphicsOutput->Blt (
                               GraphicsOutput,
                               &ProgressColor,
                               EfiBltVideoFill,
                               0,
                               0,
                               PosX,
                               PosY,
                               BlockWidth - 1,
                               BlockHeight,
                               (BlockWidth) * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                               );
  }

  PrintXY (
    (SizeOfX - StrLen (Title) * EFI_GLYPH_WIDTH) / 2,
    PosY - EFI_GLYPH_HEIGHT - 1,
    &TitleForeground,
    &TitleBackground,
    Title
    );

  return EFI_SUCCESS;
}
