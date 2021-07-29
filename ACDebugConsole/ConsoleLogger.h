#pragma once
#include <stdarg.h>
#include <cstdio>
#include <Windows.h>

#define WaitKey(M) do{printf(M);char ch = getchar();}while(0)

#define __FILENAME__ (strrchr(__FILE__,'\\')+1)
#define LOG_DEBUG(F,...) ConsoleLogger::Debug(__FUNCTION__,__FILENAME__,__LINE__,F,##__VA_ARGS__)
#define LOG_INFO(F,...) ConsoleLogger::Info(__FUNCTION__,__FILENAME__,__LINE__,F,##__VA_ARGS__)
#define LOG_WARN(F,...) ConsoleLogger::Warn(__FUNCTION__,__FILENAME__,__LINE__,F,##__VA_ARGS__)
#define LOG_ERROR(F,...) ConsoleLogger::Error(__FUNCTION__,__FILENAME__,__LINE__,F,##__VA_ARGS__)

class ConsoleLogger {
private:
	INT static const LOG_BUFFER_LENGTH = 256;
	WORD static const FOREGROUND_WHITE = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE;
	WORD static const FOREGROUND_YELLOW = FOREGROUND_GREEN | FOREGROUND_RED;

private:
	void static SetConsoleTextColour(WORD Color);
	void static LogToConsole(WORD Color, const char* Function, const char* File, const unsigned long Line, const char* Prefix, const char* Formart, va_list Args);

public:
	void static Debug(const char* Function, const char* File, const unsigned long Line, const char* Formart, ...);
	void static Info(const char* Function, const char* File, const unsigned long Line, const char* Formart, ...);
	void static Warn(const char* Function, const char* File, const unsigned long Line, const char* Formart, ...);
	void static Error(const char* Function, const char* File, const unsigned long Line, const char* Formart, ...);

public:
	static void LibraryLogCallback(UINT Level, const char* Function, const char* File, const unsigned long Line, const char* Prefix, const char* Formart, va_list Args);
};

