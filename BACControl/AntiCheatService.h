#pragma once

#include <Windows.h>
#include <tchar.h>

#ifdef UNICODE
    #define STRING_FORMART TEXT("%ls")
    #define MAX_PATH_STR 32767
#else
    #define STRING_FORMART TEXT("%s")
    #define MAX_PATH_STR MAX_PATH
#endif
#define STRING_SIZE(S) (_tcslen(S) * sizeof(TCHAR))
#define STRING_BUFFER_SIZE (MAX_PATH_STR * sizeof(TCHAR))

class AntiCheatService {
  private:
    const static TCHAR *STR_SERVICE_NAME;
    const static TCHAR *STR_DISPLAY_NAME;
    const static TCHAR *STR_GROUP_NAME;
    const static TCHAR *STR_CLASS;
    const static TCHAR *STR_CLASS_GUID;
    const static TCHAR *STR_DEPEND_ON_SERVICE;
    const static TCHAR *STR_DRIVER_FILE;
    const static TCHAR *STR_DRIVER_INSTANCE_NAME;
    const static TCHAR *STR_ALTITUDE;
    const static TCHAR *STR_DRIVER_LINK_NAME;
    const static TCHAR *STR_DRIVER_REGISTRY_ROOT;
    const static TCHAR *STR_DRIVER_REGISTRY_INSTANCES_LIST;
    const static TCHAR *STR_DRIVER_REGISTRY_INSTANCE;
    const static DWORD  DRIVER_INSTANCE_FLAGS = 0x0;

  public:
    static BOOLEAN IsServicesInstalled();
    static BOOLEAN IsServicesRunning();
    static BOOLEAN CreateServices();
    static BOOLEAN StartServices();
    static BOOLEAN StopServices();
    static BOOLEAN RemoveService();

  private:
    static BOOLEAN SetMiniFilterRegistry();
};
