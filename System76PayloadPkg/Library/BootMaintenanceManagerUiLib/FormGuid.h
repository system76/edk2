// SPDX-License-Identifier: BSD-2-Clause-Patent
// SPDX-FileCopyrighText: Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
// SPDX-FileCopyrighText: 2025 System76, Inc.

#ifndef _FORM_GUID_H_
#define _FORM_GUID_H_

#define BOOT_MAINT_FORMSET_GUID { 0x642237c7, 0x35d4, 0x472d, { 0x83, 0x65, 0x12, 0xe0, 0xcc, 0xf2, 0x7a, 0x22 }}

#define FORM_MAIN_ID                         0x1001
#define FORM_BOOT_CHG_ID                     0x1004

#define KEY_VALUE_SAVE_AND_EXIT             0x110B
#define KEY_VALUE_NO_SAVE_AND_EXIT          0x110C
#define KEY_VALUE_TRIGGER_FORM_OPEN_ACTION  0x1117

#define VARSTORE_ID_BOOT_MAINT  0x1000

#define LABEL_END                       0xffff
#define MAX_MENU_NUMBER                 100

///
/// This is the structure that will be used to store the
/// question's current value. Use it at initialize time to
/// set default value for each question. When using at run
/// time, this map is returned by the callback function,
/// so dynamically changing the question's value will be
/// possible through this mechanism
///
typedef struct {
  // Boot Option Order storage
  // The value is the OptionNumber+1 because the order list value cannot be 0
  // Use UINT32 to hold the potential value 0xFFFF+1=0x10000
  UINT32     BootOptionOrder[MAX_MENU_NUMBER];
} BMM_FAKE_NV_DATA;

#endif
