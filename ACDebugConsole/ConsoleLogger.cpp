#include "ConsoleLogger.h"

void ConsoleLogger::SetConsoleTextColour(WORD Color) {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), Color);
}

void ConsoleLogger::LogToConsole(WORD Color, const char* Function, const char* File, const unsigned long Line, const char* Prefix, const char* Formart, va_list Args) {
	PCHAR FormartBuffer = nullptr;

	if ((FormartBuffer = reinterpret_cast<PCHAR>(malloc(LOG_BUFFER_LENGTH))) != nullptr) {
		SetConsoleTextColour(Color);
		vsprintf_s(FormartBuffer, LOG_BUFFER_LENGTH, Formart, Args);
		printf("%s[%s(%s:%d)] %s\n", Prefix, Function, File, Line, FormartBuffer);
		SetConsoleTextColour(FOREGROUND_WHITE);
	}
}

void ConsoleLogger::Debug(const char* Function, const char* File, const unsigned long Line, const char* Formart, ...) {
	va_list Args = {};
	va_start(Args, Formart);
	LogToConsole(FOREGROUND_BLUE | FOREGROUND_INTENSITY, Function, File, Line, "[DEBUG]", Formart, Args);
	va_end(Args);
}

void ConsoleLogger::Info(const char* Function, const char* File, const unsigned long Line, const char* Formart, ...) {
	va_list Args = {};
	va_start(Args, Formart);
	LogToConsole(FOREGROUND_GREEN | FOREGROUND_INTENSITY, Function, File, Line, "[Info ]", Formart, Args);
	va_end(Args);
}

void ConsoleLogger::Warn(const char* Function, const char* File, const unsigned long Line, const char* Formart, ...) {
	va_list Args = {};
	va_start(Args, Formart);
	LogToConsole(FOREGROUND_YELLOW | FOREGROUND_INTENSITY, Function, File, Line, "[Warn ]", Formart, Args);
	va_end(Args);
}

void ConsoleLogger::Error(const char* Function, const char* File, const unsigned long Line, const char* Formart, ...) {
	va_list Args = {};
	va_start(Args, Formart);
	LogToConsole(FOREGROUND_RED | FOREGROUND_INTENSITY, Function, File, Line, "[Error]", Formart, Args);
	va_end(Args);
}

void ConsoleLogger::LibraryLogCallback(UINT Level, const char* Function, const char* File, const unsigned long Line, const char* Prefix, const char* Formart, va_list Args) {
	WORD Color;
	
	switch (Level) {
		case 0:
			Color = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
			break;
		case 1:
			Color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
			break;
		case 2:
			Color = FOREGROUND_YELLOW | FOREGROUND_INTENSITY;
			break;
		case 3:
			Color = FOREGROUND_RED | FOREGROUND_INTENSITY;
			break;
		default:
			Color = FOREGROUND_WHITE;
			break;
	}

	ConsoleLogger::LogToConsole( Color,  Function,  File,  Line,  Prefix,  Formart,  Args);
}
