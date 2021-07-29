#include "ObjectCallbackManager.h"
#include "LpcManager.h"

#define PROCESS_QUERY_LIMITED_INFORMATION 0x1000

namespace Binklac::Kernel::Monitor::ObjectCallback {
    VOID ObjectCallbackManager::InsertProtectedHandle(OBJECT_PROTECT_TYPE Type, HANDLE Id, HANDLE ParentProcessId) {
        OBJECT_PROTECT *Record = ObjectCallbackManager::ProtectRecord.AllocEntity();

        Record->Id              = Id;
        Record->Type            = Type;
        Record->ParentProcessId = ParentProcessId;

        ObjectCallbackManager::ProtectRecord.PushEntity(Record);
    }

    VOID ObjectCallbackManager::RemoveProtectedHandle(OBJECT_PROTECT_TYPE Type, HANDLE Id) {
        return ObjectCallbackManager::ProtectRecord.RemoveEntity([&](OBJECT_PROTECT *Record) -> BOOLEAN { return (Record->Type == Type && Record->Id == Id); });
    }

    BOOLEAN ObjectCallbackManager::IsProtectedHandle(OBJECT_PROTECT_TYPE Type, HANDLE Id) {
        return ObjectCallbackManager::ProtectRecord.SearchEntity([&](OBJECT_PROTECT *Record) -> BOOLEAN { return (Record->Type == Type && Record->Id == Id); });
    }

    BOOLEAN ObjectCallbackManager::IsCallFromParent(OBJECT_PROTECT_TYPE Type, HANDLE Id) {
        return ObjectCallbackManager::ProtectRecord.SearchEntity([&](OBJECT_PROTECT *Record) -> BOOLEAN { return (Record->Type == Type && Record->Id == Id && Record->ParentProcessId == PsGetCurrentProcessId()); });
    }

    VOID ObjectCallbackManager::CreateProcessNotifyRoutineEx(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo) {
        UNREFERENCED_PARAMETER(Process);

        if (CreateInfo != NULL && IsProtectedHandle(BAC_PROTECT_PROCESS, PsGetCurrentProcessId())) {
            InsertProtectedHandle(BAC_PROTECT_PROCESS, ProcessId, PsGetCurrentProcessId());
        } else if (CreateInfo == NULL && IsProtectedHandle(BAC_PROTECT_PROCESS, ProcessId)) {
            RemoveProtectedHandle(BAC_PROTECT_PROCESS, ProcessId);
        }

        if (CreateInfo == NULL) {
            Binklac::Kernel::Lpc::LpcManager::HandleProcessClose(ProcessId);
        }
    }

    VOID ObjectCallbackManager::CreateThreadNotifyRoutine(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create) {
        if (IsProtectedHandle(BAC_PROTECT_PROCESS, ProcessId)) {
            if (Create) {
                InsertProtectedHandle(BAC_PROTECT_THREAD, ThreadId, PsGetCurrentProcessId());
            } else {
                RemoveProtectedHandle(BAC_PROTECT_THREAD, ThreadId);
            }
        }
    }

    VOID ObjectCallbackManager::LoadImageNotifyRoutine(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo) {
        UNREFERENCED_PARAMETER(FullImageName);
        UNREFERENCED_PARAMETER(ProcessId);
        UNREFERENCED_PARAMETER(ImageInfo);

        return VOID();
    }

    OB_PREOP_CALLBACK_STATUS ObjectCallbackManager::HandleOperationsCallback(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION ObOperationInfo) {
        UNREFERENCED_PARAMETER(RegistrationContext);

        HANDLE              targetProcess        = NULL;
        HANDLE              targetThread         = NULL;
        OBJECT_PROTECT_TYPE targetType           = OBJECT_PROTECT_TYPE::BAC_PROTECT_PROCESS;
        PUNICODE_STRING     operationProcessName = nullptr;
        PUNICODE_STRING     targetProcessName    = nullptr;

        if (ObOperationInfo->ObjectType == *PsProcessType) {
            targetProcess = PsGetProcessId((PEPROCESS)ObOperationInfo->Object);
            targetType    = BAC_PROTECT_PROCESS;
        } else {
            targetProcess = PsGetThreadProcessId((PETHREAD)ObOperationInfo->Object);
            targetThread  = PsGetThreadId((PETHREAD)ObOperationInfo->Object);
        }

        if (PsGetCurrentProcessId() == CsrssHandle || PsGetCurrentProcessId() == targetProcess) {
            return OB_PREOP_SUCCESS;
        }

        if ((IsProtectedHandle(BAC_PROTECT_PROCESS, targetProcess) || IsProtectedHandle(BAC_PROTECT_THREAD, targetThread))) {
            if (!IsCallFromParent(BAC_PROTECT_PROCESS, targetProcess)) {
                SeLocateProcessImageName(PsGetCurrentProcess(), &operationProcessName);
                SeLocateProcessImageName(ObOperationInfo->ObjectType == *PsProcessType ? (PEPROCESS)ObOperationInfo->Object : PsGetThreadProcess((PETHREAD)ObOperationInfo->Object), &targetProcessName);

                LOG_DEBUG("Block %s object operation: ", (targetType == BAC_PROTECT_PROCESS ? "Process" : "Thread"));
                LOG_DEBUG("\tTarget Process: %.*ls(%d)", targetProcessName->Length, targetProcessName->Buffer, targetProcess);
                LOG_DEBUG("\tFrom Process: %.*ls(%d)", operationProcessName->Length, operationProcessName->Buffer, PsGetCurrentProcessId());
                LOG_DEBUG("\tOperation: %s, Space: %s", (ObOperationInfo->Operation == OB_OPERATION_HANDLE_CREATE ? "CREATE" : "DUPLICATE"), (ObOperationInfo->KernelHandle ? "KERNEL" : "USER"));

                if (ObOperationInfo->Operation == OB_OPERATION_HANDLE_CREATE) {
                    ObOperationInfo->Parameters->CreateHandleInformation.DesiredAccess = (SYNCHRONIZE | (targetType == BAC_PROTECT_PROCESS ? PROCESS_QUERY_LIMITED_INFORMATION : THREAD_QUERY_LIMITED_INFORMATION));
                } else {
                    ObOperationInfo->Parameters->DuplicateHandleInformation.DesiredAccess = (SYNCHRONIZE | (targetType == BAC_PROTECT_PROCESS ? PROCESS_QUERY_LIMITED_INFORMATION : THREAD_QUERY_LIMITED_INFORMATION));
                }
            }
        }

        return OB_PREOP_SUCCESS;
    }

    VOID ObjectCallbackManager::SetManageProcessId(HANDLE ManageProcessId) {
        UNREFERENCED_PARAMETER(ManageProcessId);

        return VOID();
    }

    VOID ObjectCallbackManager::SetCsrssProcessId(HANDLE CsrssProcessId) {
        ObjectCallbackManager::CsrssHandle = CsrssProcessId;
    }

    NTSTATUS ObjectCallbackManager::RegisterSystemOperationsCallbacks() {
        NTSTATUS status = STATUS_SUCCESS;

        LOG_DEBUG("Begin system operations callbacks registerion.");

        OB_CALLBACK_REGISTRATION  callbackRegistration     = {};
        OB_OPERATION_REGISTRATION operationRegistration[2] = {};

        if (obRegisterCallbacksRegistrationHandle == nullptr) {
            ObjectCallbackManager::ProtectRecord.KQueueInit();

            operationRegistration[0].ObjectType   = PsProcessType;
            operationRegistration[0].Operations   = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
            operationRegistration[0].PreOperation = reinterpret_cast<POB_PRE_OPERATION_CALLBACK>(&HandleOperationsCallback);

            operationRegistration[1].ObjectType   = PsThreadType;
            operationRegistration[1].Operations   = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
            operationRegistration[1].PreOperation = reinterpret_cast<POB_PRE_OPERATION_CALLBACK>(&HandleOperationsCallback);

            callbackRegistration.Version                    = ObGetFilterVersion();
            callbackRegistration.OperationRegistrationCount = 2;
            callbackRegistration.RegistrationContext        = NULL;
            RtlInitUnicodeString(&callbackRegistration.Altitude, BAC_FILTER_ALTITUDE);
            callbackRegistration.OperationRegistration = operationRegistration;

            if (!NT_SUCCESS(status = ObRegisterCallbacks(&callbackRegistration, &obRegisterCallbacksRegistrationHandle))) {
                LOG_ERROR("An exception occurred when calling function ObRegisterCallbacks, and the return value provided by the system is [0x%08X].", status);
            } else {
                LOG_DEBUG("Succeed calling function ObRegisterCallbacks.");
            }

            if (!NT_SUCCESS(
                    status = PsSetCreateProcessNotifyRoutineEx(reinterpret_cast<PCREATE_PROCESS_NOTIFY_ROUTINE_EX>(CreateProcessNotifyRoutineEx), FALSE))) {
                LOG_ERROR("An exception occurred when calling function PsSetCreateProcessNotifyRoutineEx, and the return value provided by the system is [0x%08X].", status);
            } else {
                LOG_DEBUG("Succeed calling function PsSetCreateProcessNotifyRoutineEx.");
            }

            if (!NT_SUCCESS(status = PsSetCreateThreadNotifyRoutine(CreateThreadNotifyRoutine))) {
                LOG_ERROR("An exception occurred when calling function PsSetCreateThreadNotifyRoutine, and the return value provided by the system is [0x%X].", status);
            } else {
                LOG_DEBUG("Succeed calling function PsSetCreateThreadNotifyRoutine.");
            }

            if (!NT_SUCCESS(status = PsSetLoadImageNotifyRoutine(LoadImageNotifyRoutine))) {
                LOG_ERROR("An exception occurred when calling function PsSetLoadImageNotifyRoutine, and the return value provided by the system is [0x%08X].", status);
            } else {
                LOG_DEBUG("Succeed calling function PsSetLoadImageNotifyRoutine.");
            }
        }

        LOG_DEBUG("End system operations callbacks registerion.");

        return status;
    }

    VOID ObjectCallbackManager::RemoveSystemOperationsCallbacks() {
        NTSTATUS status = STATUS_SUCCESS;

        LOG_DEBUG("Begin system operations callbacks remove.");

        ObjectCallbackManager::ProtectRecord.KQueueClear();

        if (obRegisterCallbacksRegistrationHandle != nullptr) {
            LOG_DEBUG("Handle operations callbacks is existing, remove it.");
            ObUnRegisterCallbacks(obRegisterCallbacksRegistrationHandle);
            obRegisterCallbacksRegistrationHandle = nullptr;
        }

        if (!NT_SUCCESS(status = PsSetCreateProcessNotifyRoutineEx(reinterpret_cast<PCREATE_PROCESS_NOTIFY_ROUTINE_EX>(CreateProcessNotifyRoutineEx), TRUE))) {
            LOG_ERROR("An exception occurred when calling function PsSetCreateProcessNotifyRoutineEx, and the return value provided by the system is [0x%08X].", status);
        } else {
            LOG_DEBUG("Succeed calling function PsSetCreateProcessNotifyRoutineEx.");
        }

        if (!NT_SUCCESS(status = PsRemoveCreateThreadNotifyRoutine(CreateThreadNotifyRoutine))) {
            LOG_ERROR("An exception occurred when calling function PsRemoveCreateThreadNotifyRoutine, and the return value provided by the system is [0x%08X].", status);
        } else {
            LOG_DEBUG("Succeed calling function PsRemoveCreateThreadNotifyRoutine.");
        }

        if (!NT_SUCCESS(status = PsRemoveLoadImageNotifyRoutine(LoadImageNotifyRoutine))) {
            LOG_ERROR("An exception occurred when calling function PsRemoveLoadImageNotifyRoutine, and the return value provided by the system is [0x%08X].", status);
        } else {
            LOG_DEBUG("Succeed calling function PsRemoveLoadImageNotifyRoutine.");
        }

        LOG_DEBUG("End system operations callbacks remove.");
    }

    HANDLE                 ObjectCallbackManager::obRegisterCallbacksRegistrationHandle = nullptr;
    HANDLE                 ObjectCallbackManager::CsrssHandle                           = nullptr;
    KQueue<OBJECT_PROTECT> ObjectCallbackManager::ProtectRecord                         = KQueue<OBJECT_PROTECT>();
} // namespace Binklac::Kernel::Monitor::ObjectCallback
