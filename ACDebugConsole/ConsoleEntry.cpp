#include <Windows.h>
#include "ConsoleLogger.h"
#include "../BACControl/BACControl.h"

BOOL ConsoleEntry() {
	LOG_INFO("Starting Binklac Anti-cheat debug console");
	LOG_INFO("Binklac Anti-cheat control library version: %s", GetAntiCheatControlLibraryVersion());

	DebugRegisterLoggerCallback(ConsoleLogger::LibraryLogCallback);

	if (IsCurrentProcessRunInAdministratorMode()) {
		LOG_DEBUG("Detected administrator privileges, will try to load the kernel driver.");
	} else {
		LOG_ERROR("The program requires administrator privileges to load the kernel driver. Please restart the program with administrator privileges.");
		return FALSE;
	}

	LOG_DEBUG("Prepare to create anti-cheating system services.");

	if (IsAntiCheatServicesInstalled()) {
		LOG_DEBUG("The anti-cheat service already exists in the system, skip the installation.");
	} else {
		if (InstallAntiCheatSystemService()) {
			LOG_INFO("The system service was successfully created.");
		} else {
			LOG_ERROR("Unable to create system service. Please contact the developer for help.");
			return FALSE;
		}
	}

	if (IsAntiCheatServicesRunning()) {
		LOG_DEBUG("The anti-cheat service is already running, skip starting the service.");
	} else {
		if (StartAntiCheatServices()) {
			LOG_INFO("The anti-cheat service started successfully.");
		} else {
			LOG_ERROR("Unable to start the anti-cheat service. Please contact the developer for help.");
			return FALSE;
		}
	}

	return TRUE;
}

int main() {

	if (!ConsoleEntry()) {
        return false;
	}

	WaitKey("Press any key to connect Lpc Port.");

	ConnectLpcControlPort();

	WaitKey("Press any key to clean up the debug environment.");

	if (IsAntiCheatServicesRunning() && StopAntiCheatServices()) {
		LOG_INFO("The anti-cheat service stopped successfully.");
	}

	if (IsAntiCheatServicesInstalled() && RemoveAntiCheatServices()) {
		LOG_INFO("The anti-cheat service was deleted successfully.");
	}

	WaitKey("Press any key to exit the program.");
}