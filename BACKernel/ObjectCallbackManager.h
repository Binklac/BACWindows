#pragma once

#include "GlobalDeclaration.h"
#include "KQueue.h"

namespace Binklac::Kernel::Monitor::ObjectCallback {
    typedef enum _OBJECT_PROTECT_TYPE
    {
        BAC_PROTECT_PROCESS,
        BAC_PROTECT_THREAD
    } OBJECT_PROTECT_TYPE;

    typedef struct _OBJECT_PROTECT {
        OBJECT_PROTECT_TYPE Type;
        HANDLE              Id;
        HANDLE              ParentProcessId;
    } OBJECT_PROTECT, *POBJECT_PROTECT;

    class ObjectCallbackManager {
      private:
        static KQueue<OBJECT_PROTECT> ProtectRecord;

      private:
        static HANDLE obRegisterCallbacksRegistrationHandle;
        static HANDLE CsrssHandle;

      private:
        static VOID    InsertProtectedHandle(OBJECT_PROTECT_TYPE Type, HANDLE Id, HANDLE ParentProcessId);
        static VOID    RemoveProtectedHandle(OBJECT_PROTECT_TYPE Type, HANDLE Id);
        static BOOLEAN IsProtectedHandle(OBJECT_PROTECT_TYPE Type, HANDLE Id);
        static BOOLEAN IsCallFromParent(OBJECT_PROTECT_TYPE Type, HANDLE Id);

      private:
        static VOID                     CreateProcessNotifyRoutineEx(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo);
        static VOID                     CreateThreadNotifyRoutine(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create);
        static VOID                     LoadImageNotifyRoutine(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo);
        static OB_PREOP_CALLBACK_STATUS HandleOperationsCallback(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION ObOperationInfo);

      public:
        static VOID SetManageProcessId(HANDLE ManageProcessId);
        static VOID SetCsrssProcessId(HANDLE CsrssProcessId);

      public:
        static NTSTATUS RegisterSystemOperationsCallbacks();
        static VOID     RemoveSystemOperationsCallbacks();
    };
} // namespace Binklac::Kernel::Monitor::ObjectCallback