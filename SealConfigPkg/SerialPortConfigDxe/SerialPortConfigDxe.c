#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "SerialPortConfigNvData.h"

extern UINT8 SerialPortConfigBin[];
extern UINT8 SerialPortConfigDxeStrings[];

EFI_GUID gSerialPortFormSetGuid = SERIAL_PORT_CONFIG_FORM_SET_GUID;

typedef struct {
  VENDOR_DEVICE_PATH             VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;

STATIC HII_VENDOR_DEVICE_PATH mSerialPortDxeVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    SERIAL_PORT_CONFIG_FORM_SET_GUID
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
                        &mSerialPortDxeVendorDevicePath,
                        NULL
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HiiHandle = HiiAddPackages (
                &gSerialPortFormSetGuid,
                DriverHandle,
                SerialPortConfigDxeStrings,
                SerialPortConfigBin,
                NULL
                );

  if (HiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mSerialPortDxeVendorDevicePath,
           NULL
           );
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SerialPortConfigDxeEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS                Status;
  SERIAL_PORT_CONFIGURATION SerialPort;
  UINTN                     BufferSize;
  
  BufferSize = sizeof (SerialPort);
  Status = gRT->GetVariable (
                  SERIAL_PORT_CONFIG_VARIABLE_NAME,
                  &gSerialPortFormSetGuid,
                  NULL,
                  &BufferSize,
                  &SerialPort
                  );
  if (EFI_ERROR (Status)) {
    SerialPort.Port[0].Mode = COM_MODE_SELECT_RS232;
    SerialPort.Port[0].LimitSlewRate = FALSE;
    SerialPort.Port[0].EnableBiasTermResistors = FALSE;
    SerialPort.Port[1].Mode = COM_MODE_SELECT_RS232;
    SerialPort.Port[1].LimitSlewRate = FALSE;
    SerialPort.Port[1].EnableBiasTermResistors = FALSE;
    SerialPort.Port[2].Mode = COM_MODE_SELECT_RS232;
    SerialPort.Port[2].LimitSlewRate = FALSE;
    SerialPort.Port[2].EnableBiasTermResistors = FALSE;
    SerialPort.Port[3].Mode = COM_MODE_SELECT_RS232;
    SerialPort.Port[3].LimitSlewRate = FALSE;
    SerialPort.Port[3].EnableBiasTermResistors = FALSE;

    Status = gRT->SetVariable (
                    SERIAL_PORT_CONFIG_VARIABLE_NAME,
                    &gSerialPortFormSetGuid,
                    EFI_VARIABLE_NON_VOLATILE
		    | EFI_VARIABLE_BOOTSERVICE_ACCESS
		    | EFI_VARIABLE_RUNTIME_ACCESS,
                    sizeof (SerialPort),
                    &SerialPort
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return InstallHiiPages ();
}
