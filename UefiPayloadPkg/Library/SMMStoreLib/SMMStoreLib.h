/** @file  SMMStoreLib.h

  Copyright (c) 2020, 9elements Agency GmbH<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SMM_STORE_LIB_H__
#define __SMM_STORE_LIB_H__

#include <Base.h>
#include <Uefi/UefiBaseType.h>

#define SMMSTORE_COMBUF_SIZE 16

EFI_STATUS
SMMStoreInfo (
  OUT        UINTN                                *NumBlocks,
  OUT        UINTN                                *BlockSize,
  OUT        UINTN                                *SMMStorePhysicalAddress
  );

/**
  Read from SMMStore

  @param[in] Lba      The starting logical block index to read from.
  @param[in] Offset   Offset into the block at which to begin reading.
  @param[in] NumBytes On input, indicates the requested read size. On
                      output, indicates the actual number of bytes read
  @param[in] Buffer   Pointer to the buffer to read into.

**/
EFI_STATUS
SMMStoreRead (
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN        UINTN                                *NumBytes,
  IN        UINT8                                *Buffer
  );


/**
  Write to SMMStore

  @param[in] Lba      The starting logical block index to write to.
  @param[in] Offset   Offset into the block at which to begin writing.
  @param[in] NumBytes On input, indicates the requested write size. On
                      output, indicates the actual number of bytes written
  @param[in] Buffer   Pointer to the data to write.

**/
EFI_STATUS
SMMStoreWrite (
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN        UINTN                                *NumBytes,
  IN        UINT8                                *Buffer
  );


/**
  Erase a block using the SMMStore

  @param Lba    The logical block index to erase.

**/
EFI_STATUS
SMMStoreEraseBlock (
  IN         EFI_LBA                              Lba
  );

VOID
EFIAPI
SMMStoreVirtualNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  );

/**
  Initializes SMMStore support

  @retval EFI_WRITE_PROTECTED   The QEMU flash device is not present.
  @retval EFI_SUCCESS           The QEMU flash device is supported.

**/
EFI_STATUS
SMMStoreInitialize (
    IN         VOID                              *ComBuf
  );

#endif /* __SMM_STORE_LIB_H__ */
