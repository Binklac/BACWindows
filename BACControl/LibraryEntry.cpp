#include <Windows.h>
#include <Shlobj.h>
#include "../SharedFiles/Shared.h"
#include "BACControl.h"
#include "SystemUtil.h"
#include "AntiCheatService.h"
#include "CallbackLogger.h"
#include "LpcClient.h"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        CallbackLogger::InitLoggerCallback();
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

__declspec(dllexport) VOID DebugRegisterLoggerCallback(LoggerCallback Callback) {
    CallbackLogger::RegisterLoggerCallback(Callback);
}

__declspec(dllexport) VOID DebugRemoveLoggerCallback(LoggerCallback Callback) {
    CallbackLogger::RemoveLoggerCallback(Callback);
}

__declspec(dllexport) PCCH GetAntiCheatControlLibraryVersion() {
    return BAC_VERSION;
}

__declspec(dllexport) BOOL IsCurrentProcessRunInAdministratorMode() {
    return SystemUtil::IsProcessRunInAdministratorMode();
}

__declspec(dllexport) BOOL IsAntiCheatServicesInstalled() {
    return AntiCheatService::IsServicesInstalled();
}

__declspec(dllexport) BOOL IsAntiCheatServicesRunning() {
    return AntiCheatService::IsServicesRunning();
}

__declspec(dllexport) BOOL InstallAntiCheatSystemService() {
    return AntiCheatService::CreateServices();
}

__declspec(dllexport) BOOL StartAntiCheatServices() {
    return AntiCheatService::StartServices();
}

__declspec(dllexport) BOOL StopAntiCheatServices() {
    return AntiCheatService::StopServices();
}

__declspec(dllexport) BOOL RemoveAntiCheatServices() {
    return AntiCheatService::RemoveService();
}

__declspec(dllexport) BOOL ConnectLpcControlPort() {
    return LpcClient::ConnectToLpcControlPort(LPC_CLIENT_TYPE::CONTROL);
}