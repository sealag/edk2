/** @file
  This library will parse the coreboot table in memory and extract those required
  information.

  Copyright (c) 2021, Star Labs Systems. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/SmmStoreInfoGuid.h>

/**
  Find the video frame buffer device information
  @param  SmmStoreInfo       Pointer to the SMMSTORE_INFO structure
  @retval RETURN_SUCCESS     Successfully find the Smm store buffer information.
  @retval RETURN_NOT_FOUND   Failed to find the Smm store buffer information .
**/
RETURN_STATUS
EFIAPI
ParseSmmStoreInfo (
  OUT SMMSTORE_INFO       *SmmStoreInfo
  );
