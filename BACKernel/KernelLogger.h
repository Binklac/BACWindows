#pragma once

#include "GlobalDeclaration.h"

#ifndef _DEBUG
    #define LOG_DEBUG(F, ...)
    #define LOG_INFO(F, ...)
    #define LOG_WARN(F, ...)
    #define LOG_ERROR(F, ...)
#else
    #define __FILENAME__ (strrchr(__FILE__, '\\') + 1)
    #define LOG_DEBUG(F, ...) Binklac::Kernel::Logger::KernelLogger::Debug(__FUNCTION__, __FILENAME__, __LINE__, F, ##__VA_ARGS__)
    #define LOG_INFO(F, ...) Binklac::Kernel::Logger::KernelLogger::Info(__FUNCTION__, __FILENAME__, __LINE__, F, ##__VA_ARGS__)
    #define LOG_WARN(F, ...) Binklac::Kernel::Logger::KernelLogger::Warn(__FUNCTION__, __FILENAME__, __LINE__, F, ##__VA_ARGS__)
    #define LOG_ERROR(F, ...) Binklac::Kernel::Logger::KernelLogger::Error(__FUNCTION__, __FILENAME__, __LINE__, F, ##__VA_ARGS__)
#endif

namespace Binklac::Kernel::Logger {
    class KernelLogger {
      private:
        static const SIZE_T LOG_BUFFER_LENGTH = 256;

      private:
        void static LogToKernelDebugger(const char *Function, const char *File, const unsigned long Line, const char *Prefix, const char *Formart, va_list Args);

      public:
        void static Debug(const char *Function, const char *File, const unsigned long Line, const char *Formart, ...);
        void static Info(const char *Function, const char *File, const unsigned long Line, const char *Formart, ...);
        void static Warn(const char *Function, const char *File, const unsigned long Line, const char *Formart, ...);
        void static Error(const char *Function, const char *File, const unsigned long Line, const char *Formart, ...);
    };
} // namespace Binklac::Kernel::Logger
