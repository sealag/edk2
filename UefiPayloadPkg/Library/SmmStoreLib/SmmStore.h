/** @file  SmmStore.h

  Copyright (c) 2022, 9elements GmbH<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef COREBOOT_SMMSTORE_H_
#define COREBOOT_SMMSTORE_H_

#define SMMSTORE_RET_SUCCESS      0
#define SMMSTORE_RET_FAILURE      1
#define SMMSTORE_RET_UNSUPPORTED  2

/* Version 2 only */
#define SMMSTORE_CMD_INIT       4
#define SMMSTORE_CMD_RAW_READ   5
#define SMMSTORE_CMD_RAW_WRITE  6
#define SMMSTORE_CMD_RAW_CLEAR  7

/*
 * This allows the payload to store raw data in the flash regions.
 * This can be used by a FaultTolerantWrite implementation, that uses at least
 * two regions in an A/B update scheme.
 */

#pragma pack(1)

/*
 * Reads a chunk of raw data with size @bufsize from the block specified by
 * @block_id starting at @bufoffset.
 * The read data is placed in @buf.
 *
 * @block_id must be less than num_blocks
 * @bufoffset + @bufsize must be less than block_size
 */
struct smmstore_params_raw_write {
  UINT32    bufsize;
  UINT32    bufoffset;
  UINT32    block_id;
};

/*
 * Writes a chunk of raw data with size @bufsize to the block specified by
 * @block_id starting at @bufoffset.
 *
 * @block_id must be less than num_blocks
 * @bufoffset + @bufsize must be less than block_size
 */
struct smmstore_params_raw_read {
  UINT32    bufsize;
  UINT32    bufoffset;
  UINT32    block_id;
};

/*
 * Erases the specified block.
 *
 * @block_id must be less than num_blocks
 */
struct smmstore_params_raw_clear {
  UINT32    block_id;
};

typedef struct smmstore_comm_buffer {
  union {
    struct smmstore_params_raw_write    raw_write;
    struct smmstore_params_raw_read     raw_read;
    struct smmstore_params_raw_clear    raw_clear;
  };
} SMMSTORE_COMBUF;
#pragma pack(0)

#endif