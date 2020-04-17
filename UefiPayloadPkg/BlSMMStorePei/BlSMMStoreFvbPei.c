/*++ @file  BlSMMStoreFvbDxe.c

 Copyright (c) 2020, 9elements Agency GmbH<BR>

 SPDX-License-Identifier: BSD-2-Clause-Patent

 --*/

#include <PiPei.h>

#include <Library/PcdLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/SMMStoreLib/SMMStoreLib.h>

#include <Guid/VariableFormat.h>
#include <Guid/SystemNvDataGuid.h>
#include <Guid/NvVarStoreFormatted.h>

#include "BlSMMStorePei.h"

///
/// The Firmware Volume Block Protocol is the low-level interface
/// to a firmware volume. File-level access to a firmware volume
/// should not be done using the Firmware Volume Block Protocol.
/// Normal access to a firmware volume must use the Firmware
/// Volume Protocol. Typically, only the file system driver that
/// produces the Firmware Volume Protocol will bind to the
/// Firmware Volume Block Protocol.
///

/**
  Initialises the FV Header and Variable Store Header
  to support variable operations.

  @param[in]  Ptr - Location to initialise the headers

**/
STATIC
EFI_STATUS
InitializeFvAndVariableStoreHeaders (
  IN UINTN                           BlockSize,
  IN UINTN                           NumBlocks
  )
{
  EFI_STATUS                          Status;
  UINT8                               Headers[
    sizeof(EFI_FIRMWARE_VOLUME_HEADER) +
    sizeof(EFI_FV_BLOCK_MAP_ENTRY) +
    sizeof(VARIABLE_STORE_HEADER)
  ];
  UINTN                               HeadersLength;
  EFI_FIRMWARE_VOLUME_HEADER          *FirmwareVolumeHeader;
  VARIABLE_STORE_HEADER               *VariableStoreHeader;

  HeadersLength = sizeof(EFI_FIRMWARE_VOLUME_HEADER) + sizeof(EFI_FV_BLOCK_MAP_ENTRY) + sizeof(VARIABLE_STORE_HEADER);

  // FirmwareVolumeHeader->FvLength is declared to have the Variable area AND the FTW working area AND the FTW Spare contiguous.
  ASSERT(PcdGet32(PcdFlashNvStorageVariableBase) + PcdGet32(PcdFlashNvStorageVariableSize) == PcdGet32(PcdFlashNvStorageFtwWorkingBase));
  ASSERT(PcdGet32(PcdFlashNvStorageFtwWorkingBase) + PcdGet32(PcdFlashNvStorageFtwWorkingSize) == PcdGet32(PcdFlashNvStorageFtwSpareBase));

  // Check if the size of the area is at least one block size
  ASSERT((PcdGet32(PcdFlashNvStorageVariableSize) > 0) && (PcdGet32(PcdFlashNvStorageVariableSize) / BlockSize > 0));
  ASSERT((PcdGet32(PcdFlashNvStorageFtwWorkingSize) > 0) && (PcdGet32(PcdFlashNvStorageFtwWorkingSize) / BlockSize > 0));
  ASSERT((PcdGet32(PcdFlashNvStorageFtwSpareSize) > 0) && (PcdGet32(PcdFlashNvStorageFtwSpareSize) / BlockSize > 0));

  // Ensure the Variable area Base Addresses are aligned on a block size boundaries
  ASSERT(PcdGet32(PcdFlashNvStorageVariableBase) % BlockSize == 0);
  ASSERT(PcdGet32(PcdFlashNvStorageFtwWorkingBase) % BlockSize == 0);
  ASSERT(PcdGet32(PcdFlashNvStorageFtwSpareBase) % BlockSize == 0);

  //
  // EFI_FIRMWARE_VOLUME_HEADER
  //
  FirmwareVolumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*)Headers;
  CopyGuid (&FirmwareVolumeHeader->FileSystemGuid, &gEfiSystemNvDataFvGuid);
  FirmwareVolumeHeader->FvLength =
      PcdGet32(PcdFlashNvStorageVariableSize) +
      PcdGet32(PcdFlashNvStorageFtwWorkingSize) +
      PcdGet32(PcdFlashNvStorageFtwSpareSize);
  FirmwareVolumeHeader->Signature = EFI_FVH_SIGNATURE;
  FirmwareVolumeHeader->Attributes = (EFI_FVB_ATTRIBUTES_2) (
                                          EFI_FVB2_READ_ENABLED_CAP   | // Reads may be enabled
                                          EFI_FVB2_READ_STATUS        | // Reads are currently enabled
                                          EFI_FVB2_STICKY_WRITE       | // A block erase is required to flip bits into EFI_FVB2_ERASE_POLARITY
                                          EFI_FVB2_MEMORY_MAPPED      | // It is memory mapped
                                          EFI_FVB2_ERASE_POLARITY     | // After erasure all bits take this value (i.e. '1')
                                          EFI_FVB2_WRITE_STATUS       | // Writes are currently enabled
                                          EFI_FVB2_WRITE_ENABLED_CAP    // Writes may be enabled
                                      );
  FirmwareVolumeHeader->HeaderLength = sizeof(EFI_FIRMWARE_VOLUME_HEADER) + sizeof(EFI_FV_BLOCK_MAP_ENTRY);
  FirmwareVolumeHeader->Revision = EFI_FVH_REVISION;
  FirmwareVolumeHeader->BlockMap[0].NumBlocks = NumBlocks;
  FirmwareVolumeHeader->BlockMap[0].Length      = BlockSize;
  FirmwareVolumeHeader->BlockMap[1].NumBlocks = 0;
  FirmwareVolumeHeader->BlockMap[1].Length      = 0;
  FirmwareVolumeHeader->Checksum = CalculateCheckSum16 ((UINT16*)FirmwareVolumeHeader,FirmwareVolumeHeader->HeaderLength);

  //
  // VARIABLE_STORE_HEADER
  //
  VariableStoreHeader = (VARIABLE_STORE_HEADER*)((UINTN)Headers + FirmwareVolumeHeader->HeaderLength);
  CopyGuid (&VariableStoreHeader->Signature, &gEfiAuthenticatedVariableGuid);
  VariableStoreHeader->Size = PcdGet32(PcdFlashNvStorageVariableSize) - FirmwareVolumeHeader->HeaderLength;
  VariableStoreHeader->Format            = VARIABLE_STORE_FORMATTED;
  VariableStoreHeader->State             = VARIABLE_STORE_HEALTHY;

  // Install the combined super-header in the store
  Status = SMMStoreWrite (0, 0, &HeadersLength, Headers);

  return Status;
}

/**
  Check the integrity of firmware volume header.

  @param[in] FwVolHeader - A pointer to a firmware volume header

  @retval  EFI_SUCCESS   - The firmware volume is consistent
  @retval  EFI_NOT_FOUND - The firmware volume has been corrupted.

**/
STATIC
EFI_STATUS
ValidateFvHeader (
  VOID
  )
{
  UINT16                      Checksum;
  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader;
  VARIABLE_STORE_HEADER       *VariableStoreHeader;
  UINTN                       VariableStoreLength;
  UINTN                       FvLength;
  EFI_STATUS                  TempStatus;
  UINTN                       BufferSize;
  UINTN                       BufferSizeReqested;
  UINT8                       TempBuffer[1024];

  BufferSizeReqested = sizeof(EFI_FIRMWARE_VOLUME_HEADER);
  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)TempBuffer;
  BufferSize = BufferSizeReqested;
  TempStatus = SMMStoreRead (0, 0, &BufferSize, (UINT8 *)FwVolHeader);
  if (EFI_ERROR (TempStatus) || BufferSizeReqested != BufferSize) {
    return EFI_DEVICE_ERROR;
  }

  FvLength = PcdGet32(PcdFlashNvStorageVariableSize) + PcdGet32(PcdFlashNvStorageFtwWorkingSize) +
      PcdGet32(PcdFlashNvStorageFtwSpareSize);

  //
  // Verify the header revision, header signature, length
  // Length of FvBlock cannot be 2**64-1
  // HeaderLength cannot be an odd number
  //
  if (   (FwVolHeader->Revision  != EFI_FVH_REVISION)
      || (FwVolHeader->Signature != EFI_FVH_SIGNATURE)
      || (FwVolHeader->FvLength  != FvLength)
      )
  {
    DEBUG ((EFI_D_INFO, "%a: No Firmware Volume header present\n",
      __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  // Check the Firmware Volume Guid
  if( CompareGuid (&FwVolHeader->FileSystemGuid, &gEfiSystemNvDataFvGuid) == FALSE ) {
    DEBUG ((EFI_D_INFO, "%a: Firmware Volume Guid non-compatible\n",
      __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  BufferSizeReqested = FwVolHeader->HeaderLength;
  if (BufferSizeReqested > sizeof(TempBuffer)) {
    return EFI_OUT_OF_RESOURCES;
  }
  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)TempBuffer;
  BufferSize = BufferSizeReqested;
  TempStatus = SMMStoreRead (0, 0, &BufferSize, (UINT8 *)FwVolHeader);
  if (EFI_ERROR (TempStatus) || BufferSizeReqested != BufferSize) {
    return EFI_DEVICE_ERROR;
  }

  // Verify the header checksum
  Checksum = CalculateSum16((UINT16*)FwVolHeader, FwVolHeader->HeaderLength);
  if (Checksum != 0) {
    DEBUG ((EFI_D_INFO, "%a: FV checksum is invalid (Checksum:0x%X)\n",
      __FUNCTION__, Checksum));
    return EFI_NOT_FOUND;
  }

  BufferSizeReqested = sizeof(VARIABLE_STORE_HEADER);
  VariableStoreHeader = (VARIABLE_STORE_HEADER*)TempBuffer;
  BufferSize = BufferSizeReqested;
  TempStatus = SMMStoreRead (0, FwVolHeader->HeaderLength,
    &BufferSize,
    (UINT8 *)VariableStoreHeader
    );
  if (EFI_ERROR (TempStatus) || BufferSizeReqested != BufferSize) {
    return EFI_DEVICE_ERROR;
  }

  // Check the Variable Store Guid
  if (!CompareGuid (&VariableStoreHeader->Signature, &gEfiVariableGuid) &&
      !CompareGuid (&VariableStoreHeader->Signature, &gEfiAuthenticatedVariableGuid)) {
    DEBUG ((EFI_D_INFO, "%a: Variable Store Guid non-compatible\n",
      __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  VariableStoreLength = PcdGet32 (PcdFlashNvStorageVariableSize) - FwVolHeader->HeaderLength;
  if (VariableStoreHeader->Size != VariableStoreLength) {
    DEBUG ((EFI_D_INFO, "%a: Variable Store Length does not match\n",
      __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SMMStoreFvbInitialize (
  IN UINTN                           BlockSize,
  IN UINTN                           NumBlocks
  )
{
  EFI_STATUS      Status;
  UINT32          FvbNumLba;
  EFI_BOOT_MODE   BootMode;

  BootMode = GetBootModeHob ();
  if (BootMode == BOOT_WITH_DEFAULT_SETTINGS) {
    Status = EFI_INVALID_PARAMETER;
  } else {
    // Determine if there is a valid header at the beginning of the SmmStore
    Status = ValidateFvHeader ();
  }

  // Install the Default FVB header if required
  if (EFI_ERROR(Status)) {
    // There is no valid header, so time to install one.
    DEBUG ((EFI_D_INFO, "%a: The FVB Header is not valid.\n", __FUNCTION__));
    DEBUG ((EFI_D_INFO, "%a: Installing a correct one for this volume.\n",
      __FUNCTION__));

    // Erase all the NorFlash that is reserved for variable storage
    FvbNumLba = (PcdGet32(PcdFlashNvStorageVariableSize) + PcdGet32(PcdFlashNvStorageFtwWorkingSize) + PcdGet32(PcdFlashNvStorageFtwSpareSize)) / BlockSize;
    for (UINTN i = 0; i < FvbNumLba; i++) {
      Status =  SMMStoreEraseBlock (i);
      if (EFI_ERROR(Status)) {
        return Status;
      }
    }

    // Install all appropriate headers
    Status = InitializeFvAndVariableStoreHeaders (BlockSize, NumBlocks);
    if (EFI_ERROR(Status)) {
      return Status;
    }
  }

  return Status;
}
