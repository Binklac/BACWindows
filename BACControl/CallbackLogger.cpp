#include "CallbackLogger.h"

void CallbackLogger::LogToCallback(UINT Level, const char* Function, const char* File, const unsigned long Line, const char* Prefix, const char* Formart, va_list Args) {
	for (int i = 0; i < 5; i++) {
		if (CallbackList[i] != nullptr) {
			CallbackList[i](Level, Function, File, Line, Prefix, Formart, Args);
		}
	}
}

void CallbackLogger::Debug(const char* Function, const char* File, const unsigned long Line, const char* Formart, ...) {
	va_list Args = {};
	va_start(Args, Formart);
	LogToCallback(0, Function, File, Line, "[DEBUG]", Formart, Args);
	va_end(Args);
}

void CallbackLogger::Info(const char* Function, const char* File, const unsigned long Line, const char* Formart, ...) {
	va_list Args = {};
	va_start(Args, Formart);
	LogToCallback(1, Function, File, Line, "[Info ]", Formart, Args);
	va_end(Args);
}

void CallbackLogger::Warn(const char* Function, const char* File, const unsigned long Line, const char* Formart, ...) {
	va_list Args = {};
	va_start(Args, Formart);
	LogToCallback(2, Function, File, Line, "[Warn ]", Formart, Args);
	va_end(Args);
}

void CallbackLogger::Error(const char* Function, const char* File, const unsigned long Line, const char* Formart, ...) {
	va_list Args = {};
	va_start(Args, Formart);
	LogToCallback(3, Function, File, Line, "[Error]", Formart, Args);
	va_end(Args);
}

void CallbackLogger::InitLoggerCallback() {
	for (int i = 0; i < 5; i++) {
		CallbackList[i] = nullptr;
	}
}

void CallbackLogger::RegisterLoggerCallback(LoggerCallback callback) {
	for (int i = 0; i < 5; i++) {
		if (CallbackList[i] == callback) {
			return;
		}
	}

	for (int i = 0; i < 5; i++) {
		if (CallbackList[i] == nullptr) {
			while (InterlockedCompareExchangePointer(reinterpret_cast<LPVOID*>(&CallbackList[i]), callback, nullptr) == nullptr);
			break;
		}
	}
}

void CallbackLogger::RemoveLoggerCallback(LoggerCallback callback) {
	for (int i = 0; i < 5; i++) {
		if (CallbackList[i] == callback) {
			while (InterlockedCompareExchangePointer(reinterpret_cast<LPVOID*>(&CallbackList[i]), nullptr, callback) == callback);
			break;
		}
	}
}

LoggerCallback CallbackLogger::CallbackList[5] = { 0 };



