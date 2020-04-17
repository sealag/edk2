/** @file  CorebootSMMStoreDxe.c

  Copyright (c) 2020, 9elements Agency GmbH<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeLib.h>

#include "SMMStoreLib.h"

/*
 * calls into SMM with the given cmd and subcmd in eax, and arg in ebx
 *
 * static inline because the resulting assembly is often smaller than
 * the call sequence due to constant folding.
 */
static inline UINT32 call_smm(UINT8 cmd, UINT8 subcmd, UINT32 arg) {
	UINT32 res = ~0;
	__asm__ __volatile__ (
		"outb %b0, $0xb2"
		: "=a" (res)
		: "a" ((subcmd << 8) | cmd), "b" (arg)
		: "memory");
	return res;
}

#define SMMSTORE_APM_CNT 0xed

#define SMMSTORE_RET_SUCCESS 0
#define SMMSTORE_RET_FAILURE 1
#define SMMSTORE_RET_UNSUPPORTED 2

#define SMMSTORE_CMD_CLEAR 1
#define SMMSTORE_CMD_READ 2
#define SMMSTORE_CMD_APPEND 3
/* Version 2 */
#define SMMSTORE_CMD_INFO 4
#define SMMSTORE_CMD_RAW_READ 5
#define SMMSTORE_CMD_RAW_WRITE 6
#define SMMSTORE_CMD_RAW_CLEAR 7

struct smmstore_params_read {
        UINT32 buf;
        UINT32 bufsize;
};

struct smmstore_params_append {
        UINT32 key;
        UINT32 keysize;
        UINT32 val;
        UINT32 valsize;
};

#pragma pack(1)

/* Version 2 */
/*
 * This allows the payload to store raw data in the flash regions.
 * This can be used by a FaultTolerantWrite implementation, that uses at least
 * two regions in an A/B update scheme.
 */
/*
 * Returns the number of blocks the SMMSTORE supports and their size.
 * For EDK2 this should be at least two blocks with 64KiB each.
 */
struct smmstore_params_info {
	UINT32 num_blocks;
	UINT32 block_size;
  UINT32 mmap_addr;
};

/*
 * Reads a chunk of raw data with size @bufsize from the block specified by
 * @block_id starting at @bufoffset.
 * The read data is placed in @buf.
 *
 * @block_id must be less than num_blocks
 * @bufoffset + @bufsize must be less than block_size
 */
struct smmstore_params_raw_write {
	UINT32 buf;
	UINT32 bufsize;
	UINT32 bufoffset;
	UINT32 block_id;
};

/*
 * Writes a chunk of raw data with size @bufsize to the block specified by
 * @block_id starting at @bufoffset.
 *
 * @block_id must be less than num_blocks
 * @bufoffset + @bufsize must be less than block_size
 */
struct smmstore_params_raw_read {
	UINT32 buf;
	UINT32 bufsize;
	UINT32 bufoffset;
	UINT32 block_id;
};

/*
 * Erases the specified block.
 *
 * @block_id must be less than num_blocks
 */
struct smmstore_params_raw_clear {
	UINT32 block_id;
};

typedef struct smmstore_comm_buffer {
  union {
    struct smmstore_params_append append;
    struct smmstore_params_info info;
    struct smmstore_params_raw_write raw_write;
    struct smmstore_params_raw_read raw_read;
    struct smmstore_params_raw_clear raw_clear;
  };
} SMMSTORE_COMBUF;
#pragma pack(0)

STATIC SMMSTORE_COMBUF *mComBuf;
STATIC UINT32 mComBufPhys;

/**
  Determines if the QEMU flash memory device is present.

  @retval FALSE   The QEMU flash device is not present.
  @retval TRUE    The QEMU flash device is present.

**/
STATIC BOOLEAN  SmmStoreProbed;
STATIC BOOLEAN  SmmStoreAvailable;
STATIC struct smmstore_params_info SmmStoreInfo;

STATIC
BOOLEAN
SMMStoreFlashDetected (
  VOID
  )
{
  UINT32            Result;

  if (SmmStoreProbed == TRUE)
    return SmmStoreAvailable;

  ZeroMem(&mComBuf->info, sizeof(mComBuf->info));

  SmmStoreProbed = TRUE;
  Result = call_smm(SMMSTORE_APM_CNT, SMMSTORE_CMD_INFO, mComBufPhys);
  if (Result != SMMSTORE_RET_SUCCESS) {
    return FALSE;
  }

  CopyMem(&SmmStoreInfo, &mComBuf->info, sizeof(SmmStoreInfo));

  DEBUG ((EFI_D_INFO, "%a: SmmStore: num_blocks 0x%x, block_size 0x%x MMAP Address 0x%x\n",
    __FUNCTION__, SmmStoreInfo.num_blocks, SmmStoreInfo.block_size, SmmStoreInfo.mmap_addr));

  SmmStoreAvailable = !!SmmStoreInfo.num_blocks && !!SmmStoreInfo.block_size;

  return SmmStoreAvailable;
}


/**
  Read from QEMU Flash

  @param[in] Lba      The starting logical block index to read from.
  @param[in] Offset   Offset into the block at which to begin reading.
  @param[in] NumBytes On input, indicates the requested read size. On
                      output, indicates the actual number of bytes read
  @param[in] Buffer   Pointer to the buffer to read into.

**/
EFI_STATUS
SMMStoreInfo (
  OUT        UINTN                                *NumBlocks,
  OUT        UINTN                                *BlockSize,
  OUT        UINTN                                *SMMStorePhysicalAddress
  )
{
  if (!SMMStoreFlashDetected())
    return EFI_NO_MEDIA;

  *NumBlocks = SmmStoreInfo.num_blocks;
  *BlockSize = SmmStoreInfo.block_size;
  *SMMStorePhysicalAddress = SmmStoreInfo.mmap_addr;

  return EFI_SUCCESS;
}


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
  )
{
  UINT32 Result;

  if (!SMMStoreFlashDetected())
    return EFI_NO_MEDIA;

  if (Lba >= SmmStoreInfo.num_blocks) {
    return EFI_INVALID_PARAMETER;
  }

  mComBuf->raw_read.buf = (UINT32)(UINTN)Buffer;
  mComBuf->raw_read.bufsize = *NumBytes;
  mComBuf->raw_read.bufoffset = Offset;
  mComBuf->raw_read.block_id = Lba;

  Result = call_smm(SMMSTORE_APM_CNT, SMMSTORE_CMD_RAW_READ, mComBufPhys);
  if (Result == SMMSTORE_RET_FAILURE) {
    return EFI_DEVICE_ERROR;
  } else if (Result == SMMSTORE_RET_UNSUPPORTED) {
    return EFI_UNSUPPORTED;
  } else if (Result != SMMSTORE_RET_SUCCESS) {
    return EFI_NO_RESPONSE;
  }

  *NumBytes = mComBuf->raw_read.bufsize;

  return EFI_SUCCESS;
}


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
  IN        EFI_LBA                             Lba,
  IN        UINTN                               Offset,
  IN        UINTN                               *NumBytes,
  IN        UINT8                               *Buffer
  )
{
  UINTN             Result;

  if (!SMMStoreFlashDetected())
    return EFI_NO_MEDIA;

  if (Lba >= SmmStoreInfo.num_blocks) {
    return EFI_INVALID_PARAMETER;
  }

  mComBuf->raw_write.buf = (UINT32)(UINTN)Buffer;
  mComBuf->raw_write.bufsize = *NumBytes;
  mComBuf->raw_write.bufoffset = Offset;
  mComBuf->raw_write.block_id = Lba;

  Result = call_smm(SMMSTORE_APM_CNT, SMMSTORE_CMD_RAW_WRITE, mComBufPhys);
  if (Result == SMMSTORE_RET_FAILURE) {
    return EFI_DEVICE_ERROR;
  } else if (Result == SMMSTORE_RET_UNSUPPORTED) {
    return EFI_UNSUPPORTED;
  } else if (Result != SMMSTORE_RET_SUCCESS) {
    return EFI_NO_RESPONSE;
  }

  *NumBytes = mComBuf->raw_write.bufsize;

  return EFI_SUCCESS;
}


/**
  Erase a SMMStore block

  @param Lba    The logical block index to erase.

**/
EFI_STATUS
SMMStoreEraseBlock (
  IN   EFI_LBA                              Lba
  )
{
  UINTN             Result;

  if (!SMMStoreFlashDetected())
    return EFI_NO_MEDIA;

  if (Lba >= SmmStoreInfo.num_blocks) {
    return EFI_INVALID_PARAMETER;
  }

  mComBuf->raw_clear.block_id = Lba;

  Result = call_smm(SMMSTORE_APM_CNT, SMMSTORE_CMD_RAW_CLEAR, mComBufPhys);
  if (Result == SMMSTORE_RET_FAILURE) {
    return EFI_DEVICE_ERROR;
  } else if (Result == SMMSTORE_RET_UNSUPPORTED) {
    return EFI_UNSUPPORTED;
  } else if (Result != SMMSTORE_RET_SUCCESS) {
    return EFI_NO_RESPONSE;
  }

  return EFI_SUCCESS;
}

VOID
EFIAPI
SMMStoreVirtualNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  EfiConvertPointer (0x0, (VOID**)&mComBuf);
  return;
}

/**
  Initializes SMMStore support

  @retval EFI_WRITE_PROTECTED   The QEMU flash device is not present.
  @retval EFI_SUCCESS           The QEMU flash device is supported.

**/
EFI_STATUS
SMMStoreInitialize (
    IN         VOID                      *Ptr
  )
{
  ASSERT (Ptr != NULL);
  mComBuf = Ptr;
  mComBufPhys = (UINT32)(UINTN)mComBuf;

  if (!SMMStoreFlashDetected ()) {
    return EFI_WRITE_PROTECTED;
  }

  return EFI_SUCCESS;
}
