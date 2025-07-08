// SPDX-License-Identifier: BSD-2-Clause-Patent
// SPDX-FileCopyrightText: Copyright (c) 2004 - 2019, Intel Corporation. All rights reserved.<BR>

#include "BootMaintenanceManager.h"

VOID *mStartOpCodeHandle = NULL;
VOID *mEndOpCodeHandle = NULL;
EFI_IFR_GUID_LABEL *mStartLabel = NULL;
EFI_IFR_GUID_LABEL *mEndLabel = NULL;

/// Boot Option from variable Menu
BM_MENU_OPTION BootOptionMenu = {
  BM_MENU_OPTION_SIGNATURE,
  { NULL },
  0
};
