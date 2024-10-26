#ifndef __SERIAL_PORT_CONFIG_NV_DATA_H__
#define __SERIAL_PORT_CONFIG_NV_DATA_H__

#include <Guid/HiiPlatformSetupFormset.h>

#define SERIAL_PORT_CONFIG_FORM_SET_GUID \
  { \
    0xe055c87f, 0xe3aa, 0x4fb8, {0xba, 0xdb, 0x32, 0x2c, 0x32, 0xcb, 0xce, 0xc5} \
  }

extern EFI_GUID gSerialPortConfigFormSetGuid;

#define SERIAL_PORT_CONFIG_VARIABLE_NAME L"SerialPortConfig"

#define COM_MODE_SELECT_RS232     0x0
#define COM_MODE_SELECT_RS485     0x1
#define COM_MODE_SELECT_DISABLED  0x3

typedef struct {
  UINT8   Mode;
  BOOLEAN LimitSlewRate;
  BOOLEAN EnableBiasTermResistors;
} PORT_CONFIG;

typedef struct {
  PORT_CONFIG Port[4];
} SERIAL_PORT_CONFIGURATION;

#endif
