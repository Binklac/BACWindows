#include "AntiCheatService.h"
#include "CallbackLogger.h"

#error PLEASE MODIFY STR_CLASS_GUID AND STR_ALTITUDE 

const TCHAR *AntiCheatService::STR_SERVICE_NAME                   = TEXT("BinklacAntiCheat");
const TCHAR *AntiCheatService::STR_DISPLAY_NAME                   = TEXT("Binklac anti-cheat service");
const TCHAR *AntiCheatService::STR_GROUP_NAME                     = TEXT("FSFilter Activity Monitor");
const TCHAR *AntiCheatService::STR_CLASS                          = TEXT("ActivityMonitor");
const TCHAR *AntiCheatService::STR_CLASS_GUID                     = TEXT("{}");
const TCHAR *AntiCheatService::STR_DEPEND_ON_SERVICE              = TEXT("FltMgr");
const TCHAR *AntiCheatService::STR_DRIVER_FILE                    = TEXT("\\BACKernel.sys");
const TCHAR *AntiCheatService::STR_DRIVER_INSTANCE_NAME           = TEXT("Anticheat");
const TCHAR *AntiCheatService::STR_ALTITUDE                       = TEXT("0");
const TCHAR *AntiCheatService::STR_DRIVER_LINK_NAME               = TEXT("\\\\.\\BINKLACAC");
const TCHAR *AntiCheatService::STR_DRIVER_REGISTRY_ROOT           = TEXT("SYSTEM\\CurrentControlSet\\Services\\BinklacAntiCheat\\");
const TCHAR *AntiCheatService::STR_DRIVER_REGISTRY_INSTANCES_LIST = TEXT("SYSTEM\\CurrentControlSet\\Services\\BinklacAntiCheat\\Instances");
const TCHAR *AntiCheatService::STR_DRIVER_REGISTRY_INSTANCE       = TEXT("SYSTEM\\CurrentControlSet\\Services\\BinklacAntiCheat\\Instances\\Anticheat");

BOOLEAN AntiCheatService::IsServicesInstalled() {
    SC_HANDLE servciceManagerHandle = nullptr;
    SC_HANDLE serviceHandle         = nullptr;
    BOOLEAN   State                 = FALSE;

    if ((servciceManagerHandle = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS)) == NULL) {
        LOG_ERROR("Unable to open the windows service manager, please check the permissions.");
        goto CleanAndExit;
    }

    if ((serviceHandle = OpenService(servciceManagerHandle, STR_SERVICE_NAME, SERVICE_ALL_ACCESS)) != NULL) {
        LOG_DEBUG("ServiceHandle: 0x%p", serviceHandle);
        State = TRUE;
    } else {
        LOG_ERROR("Unable to open service, unknown error.");
    }

CleanAndExit:
    if (servciceManagerHandle != NULL) {
        CloseServiceHandle(servciceManagerHandle);
    }

    if (serviceHandle != NULL) {
        CloseServiceHandle(serviceHandle);
    }

    return State;
}

BOOLEAN AntiCheatService::IsServicesRunning() {
    SC_HANDLE      servciceManagerHandle = nullptr;
    SC_HANDLE      serviceHandle         = nullptr;
    SERVICE_STATUS serviceStatus         = {};
    BOOLEAN        State                 = FALSE;

    if ((servciceManagerHandle = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS)) == NULL) {
        LOG_ERROR("Unable to open the windows service manager, please check the permissions.");
        goto CleanAndExit;
    }

    if ((serviceHandle = OpenService(servciceManagerHandle, STR_SERVICE_NAME, SERVICE_ALL_ACCESS)) == NULL) {
        LOG_ERROR("Unable to open service, unknown error.");
        goto CleanAndExit;
    }

    if (!QueryServiceStatus(serviceHandle, &serviceStatus)) {
        LOG_ERROR("Unable to query service status, unknown error.");
        goto CleanAndExit;
    }

    if (serviceStatus.dwCurrentState == SERVICE_RUNNING) {
        LOG_DEBUG("ServiceStatus.dwCurrentState: 0x%08X", serviceStatus.dwCurrentState);
        State = TRUE;
    }

CleanAndExit:
    if (servciceManagerHandle != NULL) {
        CloseServiceHandle(servciceManagerHandle);
    }

    if (serviceHandle != NULL) {
        CloseServiceHandle(serviceHandle);
    }

    return State;
}

BOOLEAN AntiCheatService::CreateServices() {
    PTCHAR    currentPath            = reinterpret_cast<PTCHAR>(malloc(STRING_BUFFER_SIZE));
    PTCHAR    driverFileUnFormatPath = reinterpret_cast<PTCHAR>(malloc(STRING_BUFFER_SIZE));
    PTCHAR    driverFilePath         = reinterpret_cast<PTCHAR>(malloc(STRING_BUFFER_SIZE));
    PTCHAR    serviceRegistryRoot    = reinterpret_cast<PTCHAR>(malloc(STRING_BUFFER_SIZE));
    PTCHAR    filenamePointer        = nullptr;
    SC_HANDLE servciceManagerHandle  = nullptr;
    SC_HANDLE serviceHandle          = nullptr;
    BOOLEAN   State                  = FALSE;

    if (currentPath == nullptr || driverFileUnFormatPath == nullptr || driverFilePath == nullptr || serviceRegistryRoot == nullptr) {
        return FALSE;
    }

    if (GetModuleFileName(NULL, currentPath, MAX_PATH_STR) == 0) {
        LOG_ERROR("Unable to get the path of the program.");
        goto CleanAndExit;
    }

    filenamePointer = _tcsrchr(currentPath, L'\\');

    ZeroMemory(filenamePointer, reinterpret_cast<PUCHAR>(filenamePointer) - reinterpret_cast<PUCHAR>(currentPath));

    if (_tcsncmp(currentPath, TEXT("\\"), 1) != 0) {
        _tcscpy_s(driverFileUnFormatPath, MAX_PATH_STR, TEXT("\\\\?\\"));
        _tcscat_s(driverFileUnFormatPath, MAX_PATH_STR, currentPath);
        _tcscat_s(driverFileUnFormatPath, MAX_PATH_STR, STR_DRIVER_FILE);
    } else {
        _tcscpy_s(driverFileUnFormatPath, MAX_PATH_STR, currentPath);
        _tcscat_s(driverFileUnFormatPath, MAX_PATH_STR, STR_DRIVER_FILE);
    }

    if (GetFullPathName(driverFileUnFormatPath, MAX_PATH_STR, driverFilePath, NULL) == 0) {
        LOG_ERROR("Unable to obtain a valid directory, unable to install the service.");
        goto CleanAndExit;
    }

    if ((servciceManagerHandle = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS)) == NULL) {
        LOG_ERROR("Unable to open the windows service manager, please check the permissions.");
        goto CleanAndExit;
    }

    if ((serviceHandle = CreateService(servciceManagerHandle, STR_SERVICE_NAME, STR_DISPLAY_NAME, SERVICE_ALL_ACCESS, SERVICE_FILE_SYSTEM_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, driverFilePath, STR_GROUP_NAME, NULL, STR_DEPEND_ON_SERVICE, NULL, NULL)) == NULL) {
        LOG_ERROR("Unable to created service, unknown error.");
        goto CleanAndExit;
    }

    if (!AntiCheatService::SetMiniFilterRegistry()) {
        LOG_ERROR("Unable to set minifilter registry.");
        goto CleanAndExit;
    }

    State = TRUE;

CleanAndExit:
    if (currentPath != nullptr) {
        free(currentPath);
    }

    if (driverFileUnFormatPath != nullptr) {
        free(driverFileUnFormatPath);
    }

    if (driverFilePath != nullptr) {
        free(driverFilePath);
    }

    if (serviceRegistryRoot != nullptr) {
        free(serviceRegistryRoot);
    }

    if (servciceManagerHandle != NULL) {
        CloseServiceHandle(servciceManagerHandle);
    }

    if (serviceHandle != NULL) {
        CloseServiceHandle(serviceHandle);
    }

    return State;
}

BOOLEAN AntiCheatService::SetMiniFilterRegistry() {
    HKEY    handleOfDriverRegistryRoot = NULL, handleOfDriverRegistryInstancesList = NULL, handleOfDriverRegistryInstances = NULL;
    BOOLEAN State = FALSE;

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, STR_DRIVER_REGISTRY_ROOT, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &handleOfDriverRegistryRoot, NULL) != ERROR_SUCCESS) {
        LOG_ERROR("create root key error: %d", GetLastError());
        goto CleanAndExit;
    }

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, STR_DRIVER_REGISTRY_INSTANCES_LIST, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &handleOfDriverRegistryInstancesList, NULL) != ERROR_SUCCESS) {
        LOG_ERROR("create instance list key error: %d", GetLastError());
        goto CleanAndExit;
    }

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, STR_DRIVER_REGISTRY_INSTANCE, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &handleOfDriverRegistryInstances, NULL) != ERROR_SUCCESS) {
        LOG_ERROR("create instance key error: %d", GetLastError());
        goto CleanAndExit;
    }

    if (RegSetValueEx(handleOfDriverRegistryRoot, TEXT("Class"), 0, REG_SZ, reinterpret_cast<PBYTE>(const_cast<PTCHAR>(STR_CLASS)), static_cast<DWORD>(STRING_SIZE(STR_CLASS))) != ERROR_SUCCESS) {
        LOG_ERROR("Class error: %d", GetLastError());
        goto CleanAndExit;
    }

    if (RegSetValueEx(handleOfDriverRegistryRoot, TEXT("ClassGuid"), 0, REG_SZ, reinterpret_cast<PBYTE>(const_cast<PTCHAR>(STR_CLASS_GUID)), static_cast<DWORD>(STRING_SIZE(STR_CLASS_GUID))) != ERROR_SUCCESS) {
        LOG_ERROR("ClassGuid error: %d", GetLastError());
        goto CleanAndExit;
    }

    if (RegSetValueEx(handleOfDriverRegistryInstancesList, TEXT("DefaultInstance"), 0, REG_SZ, reinterpret_cast<PBYTE>(const_cast<PTCHAR>(STR_DRIVER_INSTANCE_NAME)), static_cast<DWORD>(STRING_SIZE(STR_DRIVER_INSTANCE_NAME))) != ERROR_SUCCESS) {
        LOG_ERROR("DefaultInstance error: %d", GetLastError());
        goto CleanAndExit;
    }

    if (RegSetValueEx(handleOfDriverRegistryInstances, TEXT("Altitude"), 0, REG_SZ, reinterpret_cast<PBYTE>(const_cast<PTCHAR>(STR_ALTITUDE)), static_cast<DWORD>(STRING_SIZE(STR_ALTITUDE))) != ERROR_SUCCESS) {
        LOG_ERROR("Altitude error: %d", GetLastError());
        goto CleanAndExit;
    }

    if (RegSetValueEx(handleOfDriverRegistryInstances, TEXT("Flags"), 0, REG_DWORD, reinterpret_cast<PBYTE>(const_cast<PDWORD>(&DRIVER_INSTANCE_FLAGS)), sizeof(DWORD)) != ERROR_SUCCESS) {
        LOG_ERROR("Flags error: %d", GetLastError());
        goto CleanAndExit;
    }

    State = TRUE;

CleanAndExit:
    if (handleOfDriverRegistryRoot != NULL) {
        RegCloseKey(handleOfDriverRegistryRoot);
    }

    if (handleOfDriverRegistryInstancesList != NULL) {
        RegCloseKey(handleOfDriverRegistryInstancesList);
    }

    if (handleOfDriverRegistryInstances != NULL) {
        RegCloseKey(handleOfDriverRegistryInstances);
    }

    return State;
}

BOOLEAN AntiCheatService::StartServices() {
    SC_HANDLE servciceManagerHandle = nullptr;
    SC_HANDLE serviceHandle         = nullptr;
    BOOLEAN   State                 = FALSE;

    if ((servciceManagerHandle = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS)) == NULL) {
        LOG_ERROR("Unable to open the windows service manager, please check the permissions.");
        goto CleanAndExit;
    }

    if ((serviceHandle = OpenService(servciceManagerHandle, STR_SERVICE_NAME, SERVICE_ALL_ACCESS)) == NULL) {
        LOG_ERROR("Unable to open service, unknown error.");
        goto CleanAndExit;
    }

    if (StartService(serviceHandle, NULL, NULL)) {
        State = TRUE;
    } else if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) {
        State = TRUE;
    } else {
        LOG_ERROR("Unable to start service, unknown error.");
    }

CleanAndExit:
    if (servciceManagerHandle != NULL) {
        CloseServiceHandle(servciceManagerHandle);
    }

    if (serviceHandle != NULL) {
        CloseServiceHandle(serviceHandle);
    }

    return State;
}

BOOLEAN AntiCheatService::StopServices() {
    SC_HANDLE      servciceManagerHandle = nullptr;
    SC_HANDLE      serviceHandle         = nullptr;
    SERVICE_STATUS serviceStatus         = {};
    BOOLEAN        State                 = FALSE;

    if ((servciceManagerHandle = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS)) == NULL) {
        LOG_ERROR("Unable to open the windows service manager, please check the permissions.");
        goto CleanAndExit;
    }

    if ((serviceHandle = OpenService(servciceManagerHandle, STR_SERVICE_NAME, SERVICE_ALL_ACCESS)) == NULL) {
        LOG_ERROR("Unable to open service, unknown error.");
        goto CleanAndExit;
    }

    if (ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus)) {
        LOG_DEBUG("ServiceStatus.dwCurrentState: 0x%08X", serviceStatus.dwCurrentState);
        State = TRUE;
    }

CleanAndExit:
    if (servciceManagerHandle != NULL) {
        CloseServiceHandle(servciceManagerHandle);
    }

    if (serviceHandle != NULL) {
        CloseServiceHandle(servciceManagerHandle);
    }

    return State;
}

BOOLEAN AntiCheatService::RemoveService() {
    SC_HANDLE servciceManagerHandle = nullptr;
    SC_HANDLE serviceHandle         = nullptr;
    BOOLEAN   State                 = FALSE;

    if ((servciceManagerHandle = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS)) == NULL) {
        LOG_ERROR("Unable to open the windows service manager, please check the permissions.");
        goto CleanAndExit;
    }

    if ((serviceHandle = OpenService(servciceManagerHandle, STR_SERVICE_NAME, SERVICE_ALL_ACCESS)) == NULL) {
        LOG_ERROR("Unable to open service, unknown error.");
        goto CleanAndExit;
    }

    if (DeleteService(serviceHandle)) {
        State = TRUE;
    }

CleanAndExit:
    if (servciceManagerHandle != NULL) {
        CloseServiceHandle(servciceManagerHandle);
    }

    if (serviceHandle != NULL) {
        CloseServiceHandle(servciceManagerHandle);
    }

    return State;
}
