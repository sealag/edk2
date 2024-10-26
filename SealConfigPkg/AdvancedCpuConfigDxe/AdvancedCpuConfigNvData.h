#ifndef __ADVANCED_CPU_CONFIG_NV_DATA_H__
#define __ADVANCED_CPU_CONFIG_NV_DATA_H__

#include <Guid/HiiPlatformSetupFormset.h>

#define ADVANCED_CPU_CONFIG_FORM_SET_GUID \
  { \
    0x09b39a65, 0x7a37, 0x470d, {0xa0, 0xf7, 0x16, 0x7c, 0xd8, 0x2f, 0xf0, 0x6a} \
  }

extern EFI_GUID gAdvancedCpuConfigFormSetGuid;

#define ADVANCED_CPU_CONFIG_VARIABLE_NAME L"AdvancedCpuConfig"

typedef struct {
  UINT8 PL1;
  UINT8 PL2;
} ADVANCED_CPU_CONFIGURATION;

#endif
