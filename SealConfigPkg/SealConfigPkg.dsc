[Defines]
  PLATFORM_NAME                       = SealConfigPkg
  PLATFORM_GUID                       = 9d77348e-9907-4ec0-a352-54a905c74ccf
  PLATFORM_VERSION                    = 0.1
  DSC_SPECIFICATION                   = 0x00010005
  SUPPORTED_ARCHITECTURES             = IA32|X64
  BUILD_TARGETS                       = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER                    = DEFAULT

[Components]
  SealConfigPkg/AdvancedCpuConfigDxe/AdvancedCpuConfigDxe.inf
  SealConfigPkg/SerialPortConfigDxe/SerialPortConfigDxe.inf
