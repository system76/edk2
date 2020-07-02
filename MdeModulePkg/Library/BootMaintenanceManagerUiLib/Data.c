/** @file
Define some data used for Boot Maint

Copyright (c) 2004 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BootMaintenanceManager.h"

VOID                *mStartOpCodeHandle = NULL;
VOID                *mEndOpCodeHandle = NULL;
EFI_IFR_GUID_LABEL  *mStartLabel = NULL;
EFI_IFR_GUID_LABEL  *mEndLabel = NULL;

///
/// Boot Option from variable Menu
///
BM_MENU_OPTION      BootOptionMenu = {
  BM_MENU_OPTION_SIGNATURE,
  {NULL},
  0
};

///
/// Driver Option from variable menu
///
BM_MENU_OPTION      DriverOptionMenu = {
  BM_MENU_OPTION_SIGNATURE,
  {NULL},
  0
};

///
/// Handles in current system selection menu
///
BM_MENU_OPTION      DriverMenu = {
  BM_MENU_OPTION_SIGNATURE,
  {NULL},
  0
};
