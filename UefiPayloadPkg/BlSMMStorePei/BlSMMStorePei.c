/** @file  BlSMMStorePei.c

  Copyright (c) 2020, 9elements Agency GmbH<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiPei.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>
#include <Library/SMMStoreLib/SMMStoreLib.h>

#include "BlSMMStorePei.h"

EFI_STATUS
EFIAPI
BlPeiSMMStoreInitialise (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS              Status;
  UINTN                   NumBlocks;
  UINTN                   BlockSize;
  UINTN                   PhysicalAddress;
  UINT8                   ComBuf[SMMSTORE_COMBUF_SIZE];

  Status = SMMStoreInitialize(ComBuf);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR,"%a: Failed to initialize SMMStore\n",
      __FUNCTION__));
    PcdSetBoolS (PcdEmuVariableNvModeEnable, TRUE);
    return Status;
  }

  Status = SMMStoreInfo (&NumBlocks, &BlockSize, &PhysicalAddress);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR,"%a: Failed to get SMMStoreInfo\n",
      __FUNCTION__));
    PcdSetBoolS (PcdEmuVariableNvModeEnable, TRUE);
    return Status;
  }

  PcdSet32S (PcdFlashNvStorageVariableBase, PcdGet32 (PcdFlashNvStorageVariableBase) + PhysicalAddress);
  PcdSet32S (PcdFlashNvStorageFtwWorkingBase, PcdGet32 (PcdFlashNvStorageFtwWorkingBase) + PhysicalAddress);
  PcdSet32S (PcdFlashNvStorageFtwSpareBase, PcdGet32 (PcdFlashNvStorageFtwSpareBase) + PhysicalAddress);

  Status = SMMStoreFvbInitialize (BlockSize, NumBlocks);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "%a: Failed to initialize SMMStore\n",
      __FUNCTION__));
      PcdSetBoolS (PcdEmuVariableNvModeEnable, TRUE);
  }

  return Status;
}
