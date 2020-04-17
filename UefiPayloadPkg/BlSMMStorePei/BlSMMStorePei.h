/** @file  BlSMMStorePei.h

  Copyright (c) 2020, 9elements Agency GmbH<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SMM_STORE_PEIH__
#define __SMM_STORE_PEIH__

#include <PiPei.h>
#include <Library/DebugLib.h>

EFI_STATUS
EFIAPI
SMMStoreFvbInitialize (
  IN UINTN                           BlockSize,
  IN UINTN                           NumBlocks
  );

#endif /* __SMM_STORE_PEIH__ */
