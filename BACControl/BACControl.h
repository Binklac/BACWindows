#pragma once

#include <Windows.h>
typedef void (*LoggerCallback)(UINT, const char *, const char *, const unsigned long, const char *, const char *, va_list);

__declspec(dllexport) VOID DebugRegisterLoggerCallback(LoggerCallback Callback);
__declspec(dllexport) VOID DebugRemoveLoggerCallback(LoggerCallback Callback);
__declspec(dllexport) PCCH GetAntiCheatControlLibraryVersion();
__declspec(dllexport) BOOL IsCurrentProcessRunInAdministratorMode();
__declspec(dllexport) BOOL IsAntiCheatServicesInstalled();
__declspec(dllexport) BOOL IsAntiCheatServicesRunning();
__declspec(dllexport) BOOL InstallAntiCheatSystemService();
__declspec(dllexport) BOOL StartAntiCheatServices();
__declspec(dllexport) BOOL StopAntiCheatServices();
__declspec(dllexport) BOOL RemoveAntiCheatServices();
__declspec(dllexport) BOOL ConnectLpcControlPort();