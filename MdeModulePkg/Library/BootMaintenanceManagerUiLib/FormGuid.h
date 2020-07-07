/** @file
Formset guids, form id and VarStore data structure for Boot Maintenance Manager.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef _FORM_GUID_H_
#define _FORM_GUID_H_

#define BOOT_MAINT_FORMSET_GUID \
  { \
  0x642237c7, 0x35d4, 0x472d, {0x83, 0x65, 0x12, 0xe0, 0xcc, 0xf2, 0x7a, 0x22} \
  }

#define FORM_BOOT_CHG_ID                     0x1004

#define KEY_VALUE_TRIGGER_FORM_OPEN_ACTION   0x1117

#define VARSTORE_ID_BOOT_MAINT               0x1000

#define LABEL_END                            0xffff
#define MAX_MENU_NUMBER                      100

typedef struct {
  // The value is the OptionNumber+1 because the order list value cannot be 0
  // Use UINT32 to hold the potential value 0xFFFF+1=0x10000
  UINT32  BootOptionOrder[MAX_MENU_NUMBER];
} BMM_FAKE_NV_DATA;

#endif

