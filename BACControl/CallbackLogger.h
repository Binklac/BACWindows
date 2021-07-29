#pragma once
#include <stdarg.h>
#include <cstdio>
#include <Windows.h>

#ifndef _DEBUG
#define LOG_DEBUG(F,...)
#define LOG_INFO(F,...)
#define LOG_WARN(F,...)
#define LOG_ERROR(F,...)
#else
#define __FILENAME__ (strrchr(__FILE__,'\\')+1)
#define LOG_DEBUG(F,...) CallbackLogger::Debug(__FUNCTION__,__FILENAME__,__LINE__,F,##__VA_ARGS__)
#define LOG_INFO(F,...) CallbackLogger::Info(__FUNCTION__,__FILENAME__,__LINE__,F,##__VA_ARGS__)
#define LOG_WARN(F,...) CallbackLogger::Warn(__FUNCTION__,__FILENAME__,__LINE__,F,##__VA_ARGS__)
#define LOG_ERROR(F,...) CallbackLogger::Error(__FUNCTION__,__FILENAME__,__LINE__,F,##__VA_ARGS__)
#endif

typedef void(*LoggerCallback)(UINT, const char*, const char*, const unsigned long, const char*, const char*, va_list);

class CallbackLogger {
private:
	static const INT LOG_BUFFER_LENGTH = 256;
	static LoggerCallback CallbackList[5];

private:
	void static LogToCallback(UINT Level, const char* Function, const char* File, const unsigned long Line, const char* Prefix, const char* Formart, va_list Args);

public:
	void static Debug(const char* Function, const char* File, const unsigned long Line, const char* Formart, ...);
	void static Info(const char* Function, const char* File, const unsigned long Line, const char* Formart, ...);
	void static Warn(const char* Function, const char* File, const unsigned long Line, const char* Formart, ...);
	void static Error(const char* Function, const char* File, const unsigned long Line, const char* Formart, ...);

public:
	static void InitLoggerCallback();
	static void RegisterLoggerCallback(LoggerCallback callback);
	static void RemoveLoggerCallback(LoggerCallback callback);
};



