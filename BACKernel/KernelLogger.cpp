#include "KernelLogger.h"
namespace Binklac::Kernel::Logger {
    void KernelLogger::LogToKernelDebugger(const char *Function, const char *File, const unsigned long Line, const char *Prefix, const char *Formart, va_list Args) {
        PCHAR FormartBuffer = nullptr;

        if ((FormartBuffer = reinterpret_cast<NTSTRSAFE_PSTR>(ExAllocatePoolZero(PagedPool, LOG_BUFFER_LENGTH, 'CABL'))) != nullptr) {
            RtlStringCbVPrintfA(FormartBuffer, LOG_BUFFER_LENGTH, Formart, Args);
            DbgPrint("%s[%s(%s:%d)] %s\n", Prefix, Function, File, Line, FormartBuffer);
        }

        if (FormartBuffer != nullptr) {
            ExFreePoolWithTag(FormartBuffer, 'CABL');
        }
    }

    void KernelLogger::Debug(const char *Function, const char *File, const unsigned long Line, const char *Formart, ...) {
        va_list Args = {};
        va_start(Args, Formart);
        LogToKernelDebugger(Function, File, Line, "[DEBUG]", Formart, Args);
        va_end(Args);
    }

    void KernelLogger::Info(const char *Function, const char *File, const unsigned long Line, const char *Formart, ...) {
        va_list Args = {};
        va_start(Args, Formart);
        LogToKernelDebugger(Function, File, Line, "[Info ]", Formart, Args);
        va_end(Args);
    }

    void KernelLogger::Warn(const char *Function, const char *File, const unsigned long Line, const char *Formart, ...) {
        va_list Args = {};
        va_start(Args, Formart);
        LogToKernelDebugger(Function, File, Line, "[Warn ]", Formart, Args);
        va_end(Args);
    }

    void KernelLogger::Error(const char *Function, const char *File, const unsigned long Line, const char *Formart, ...) {
        va_list Args = {};
        va_start(Args, Formart);
        LogToKernelDebugger(Function, File, Line, "[Error]", Formart, Args);
        va_end(Args);
    }
} // namespace Binklac::Kernel::Logger
