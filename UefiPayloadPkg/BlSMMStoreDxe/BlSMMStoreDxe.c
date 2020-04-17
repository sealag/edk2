/** @file  BlSMMStoreDxe.c

  Copyright (c) 2020, 9elements Agency GmbH<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/SMMStoreLib/SMMStoreLib.h>

#include "BlSMMStoreDxe.h"

STATIC EFI_EVENT mSMMStoreVirtualAddrChangeEvent;

//
// Global variable declarations
//
SMMSTORE_INSTANCE *mSMMStoreInstance;

SMMSTORE_INSTANCE  mSMMStoreInstanceTemplate = {
  SMMSTORE_SIGNATURE, // Signature
  NULL, // Handle ... NEED TO BE FILLED
  {
    0, // MediaId ... NEED TO BE FILLED
    FALSE, // RemovableMedia
    TRUE, // MediaPresent
    FALSE, // LogicalPartition
    FALSE, // ReadOnly
    FALSE, // WriteCaching;
    0, // BlockSize ... NEED TO BE FILLED
    4, //  IoAlign
    0, // LastBlock ... NEED TO BE FILLED
    0, // LowestAlignedLba
    1, // LogicalBlocksPerPhysicalBlock
  }, //Media;

  {
    FvbGetAttributes, // GetAttributes
    FvbSetAttributes, // SetAttributes
    FvbGetPhysicalAddress,  // GetPhysicalAddress
    FvbGetBlockSize,  // GetBlockSize
    FvbRead,  // Read
    FvbWrite, // Write
    FvbEraseBlocks, // EraseBlocks
    NULL, //ParentHandle
  }, //  FvbProtoccol;
  NULL, // ShadowBuffer
  NULL, // ShadowBufferPhys
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8)(OFFSET_OF (NOR_FLASH_DEVICE_PATH, End)),
          (UINT8)(OFFSET_OF (NOR_FLASH_DEVICE_PATH, End) >> 8)
        }
      },
      { 0x0, 0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } }, // GUID ... NEED TO BE FILLED
    },
    0, // Index
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
    }
    } // DevicePath
};

STATIC
EFI_STATUS
SMMStoreCreateInstance (
  IN UINTN                  NumberofBlocks,
  IN UINTN                  BlockSize,
  OUT SMMSTORE_INSTANCE**  SMMStoreInstance
  )
{
  EFI_STATUS Status;
  SMMSTORE_INSTANCE* Instance;

  ASSERT(SMMStoreInstance != NULL);

  Instance = AllocateRuntimeCopyPool (sizeof(SMMSTORE_INSTANCE),&mSMMStoreInstanceTemplate);
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Instance->Media.MediaId = 0;
  Instance->Media.BlockSize = BlockSize;
  Instance->Media.LastBlock = NumberofBlocks - 1;

  CopyGuid (&Instance->DevicePath.Vendor.Guid, &gEfiCallerIdGuid);
  Instance->DevicePath.Index = (UINT8)0;

  Instance->ShadowBuffer = AllocateRuntimePool (BlockSize);
  if (Instance->ShadowBuffer == NULL) {
    FreePool (Instance);
    return EFI_OUT_OF_RESOURCES;
  }
  Instance->ShadowBufferPhys = Instance->ShadowBuffer;

  Status = SMMStoreFvbInitialize (Instance);
  if (EFI_ERROR(Status)) {
    FreePool (Instance->ShadowBuffer);
    FreePool (Instance);
    return Status;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                &Instance->Handle,
                &gEfiDevicePathProtocolGuid, &Instance->DevicePath,
                &gEfiFirmwareVolumeBlockProtocolGuid, &Instance->FvbProtocol,
                NULL
                );
  if (EFI_ERROR(Status)) {
    FreePool (Instance->ShadowBuffer);
    FreePool (Instance);
    return Status;
  }

  *SMMStoreInstance = Instance;
  return Status;
}

/**
  Fixup internal data so that EFI can be call in virtual mode.
  Call the passed in Child Notify event and convert any pointers in
  lib to virtual mode.

  @param[in]    Event   The Event that is being processed
  @param[in]    Context Event Context
**/
VOID
EFIAPI
BlSMMStoreVirtualNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  // Convert Fvb
  EfiConvertPointer (0x0, (VOID**)&mSMMStoreInstance->FvbProtocol.EraseBlocks);
  EfiConvertPointer (0x0, (VOID**)&mSMMStoreInstance->FvbProtocol.GetAttributes);
  EfiConvertPointer (0x0, (VOID**)&mSMMStoreInstance->FvbProtocol.GetBlockSize);
  EfiConvertPointer (0x0, (VOID**)&mSMMStoreInstance->FvbProtocol.GetPhysicalAddress);
  EfiConvertPointer (0x0, (VOID**)&mSMMStoreInstance->FvbProtocol.Read);
  EfiConvertPointer (0x0, (VOID**)&mSMMStoreInstance->FvbProtocol.SetAttributes);
  EfiConvertPointer (0x0, (VOID**)&mSMMStoreInstance->FvbProtocol.Write);

  if (mSMMStoreInstance->ShadowBuffer != NULL) {
    EfiConvertPointer (0x0, (VOID**)&mSMMStoreInstance->ShadowBuffer);
  }
  SMMStoreVirtualNotifyEvent (Event, Context);

  return;
}

EFI_STATUS
EFIAPI
BlSMMSTOREInitialise (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS              Status;
  UINTN                   NumBlocks;
  UINTN                   BlockSize;
  UINTN                   PhysicalAddress;
  VOID                    *ComBuf;

  DEBUG((DEBUG_INFO, "BlSMMSTOREInitialise\n"));

  if (PcdGetBool (PcdEmuVariableNvModeEnable)) {
    DEBUG ((DEBUG_WARN, "Variable emulation is active! Skipping driver init.\n"));
    return EFI_SUCCESS;
  }

  ComBuf = AllocateRuntimePool (SMMSTORE_COMBUF_SIZE);
  if (!ComBuf) {
    PcdSetBoolS (PcdEmuVariableNvModeEnable, TRUE);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = SMMStoreInitialize(ComBuf);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR,"a: Failed to initialize SMMStore\n",
      __FUNCTION__));
    PcdSetBoolS (PcdEmuVariableNvModeEnable, TRUE);
    return Status;
  }

  Status = SMMStoreInfo (&NumBlocks, &BlockSize, &PhysicalAddress);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR,"a: Failed to get SMMStoreInfo\n",
      __FUNCTION__));
    PcdSetBoolS (PcdEmuVariableNvModeEnable, TRUE);
    return Status;
  }

  // Update PCDs for Variable/RuntimeDxe
  PcdSet32S (PcdFlashNvStorageVariableBase, PcdGet32 (PcdFlashNvStorageVariableBase) + PhysicalAddress);
  PcdSet32S (PcdFlashNvStorageFtwWorkingBase, PcdGet32 (PcdFlashNvStorageFtwWorkingBase) + PhysicalAddress);
  PcdSet32S (PcdFlashNvStorageFtwSpareBase, PcdGet32 (PcdFlashNvStorageFtwSpareBase) + PhysicalAddress);

  mSMMStoreInstance = AllocateRuntimePool (sizeof(SMMSTORE_INSTANCE*));

  Status = SMMStoreCreateInstance (
    NumBlocks,
    BlockSize,
    &mSMMStoreInstance
  );
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "a: Fail to create instance for SMMStore\n",
      __FUNCTION__));
    PcdSetBoolS (PcdEmuVariableNvModeEnable, TRUE);
    return Status;
  }

  //
  // Register for the virtual address change event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  BlSMMStoreVirtualNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mSMMStoreVirtualAddrChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
