#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "AdvancedCpuConfigNvData.h"

extern UINT8 AdvancedCpuConfigBin[];
extern UINT8 AdvancedCpuConfigDxeStrings[];

EFI_GUID gAdvancedCpuFormSetGuid = ADVANCED_CPU_CONFIG_FORM_SET_GUID;

typedef struct {
  VENDOR_DEVICE_PATH             VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;

STATIC HII_VENDOR_DEVICE_PATH mAdvancedCpuDxeVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    ADVANCED_CPU_CONFIG_FORM_SET_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(END_DEVICE_PATH_LENGTH),
      (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

STATIC
EFI_STATUS
InstallHiiPages (
  VOID
  )
{
  EFI_STATUS      Status;
  EFI_HII_HANDLE  HiiHandle;
  EFI_HANDLE      DriverHandle;

  DriverHandle = NULL;
  Status       = gBS->InstallMultipleProtocolInterfaces (
                        &DriverHandle,
                        &gEfiDevicePathProtocolGuid,
                        &mAdvancedCpuDxeVendorDevicePath,
                        NULL
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HiiHandle = HiiAddPackages (
                &gAdvancedCpuFormSetGuid,
                DriverHandle,
                AdvancedCpuConfigDxeStrings,
                AdvancedCpuConfigBin,
                NULL
                );

  if (HiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mAdvancedCpuDxeVendorDevicePath,
           NULL
           );
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
AdvancedCpuConfigDxeEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS                 Status;
  ADVANCED_CPU_CONFIGURATION AdvancedCpu;
  UINTN                      BufferSize;
  
  BufferSize = sizeof (AdvancedCpu);
  Status = gRT->GetVariable (
                  ADVANCED_CPU_CONFIG_VARIABLE_NAME,
                  &gAdvancedCpuFormSetGuid,
                  NULL,
                  &BufferSize,
                  &AdvancedCpu
                  );
  if (EFI_ERROR (Status)) {
    // TODO read actual current PL1/PL2?
    AdvancedCpu.PL1 = 6;
    AdvancedCpu.PL2 = 25;

    Status = gRT->SetVariable (
                    ADVANCED_CPU_CONFIG_VARIABLE_NAME,
                    &gAdvancedCpuFormSetGuid,
                    EFI_VARIABLE_NON_VOLATILE
		    | EFI_VARIABLE_BOOTSERVICE_ACCESS
		    | EFI_VARIABLE_RUNTIME_ACCESS,
                    sizeof (AdvancedCpu),
                    &AdvancedCpu
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return InstallHiiPages ();
}
