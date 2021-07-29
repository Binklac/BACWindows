#include "GlobalDeclaration.h"
#include "IrpDispather.h"
#include "LpcManager.h"

VOID DriverUnload(struct _DRIVER_OBJECT *DriverObject) {
    UNICODE_STRING symbolicLinkName = {};

    Binklac::Kernel::Lpc::LpcManager::CloseLpcPort();

    RtlInitUnicodeString(&symbolicLinkName, SYMBOLIC_LINK_NAME);

    IoDeleteDevice(DriverObject->DeviceObject);
    IoDeleteSymbolicLink(&symbolicLinkName);
}

extern "C" NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);

    UNICODE_STRING deviceName       = {};
    UNICODE_STRING symbolicLinkName = {};
    PDEVICE_OBJECT deviceObject     = nullptr;
    NTSTATUS       status           = STATUS_SUCCESS;

    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    LOG_INFO("Starting Binklac Anti-cheat kernel driver initialization.");
    LOG_INFO("Binklac Anti-cheat kernel driver version: %s.", BAC_VERSION);

    RtlInitUnicodeString(&deviceName, DEVICE_NAME);
    RtlInitUnicodeString(&symbolicLinkName, SYMBOLIC_LINK_NAME);

    DriverObject->DriverUnload = DriverUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = Binklac::Kernel::Irp::IrpDispatcher;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = Binklac::Kernel::Irp::IrpDispatcher;
    DriverObject->MajorFunction[IRP_MJ_READ]           = Binklac::Kernel::Irp::IrpDispatcher;
    DriverObject->MajorFunction[IRP_MJ_WRITE]          = Binklac::Kernel::Irp::IrpDispatcher;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Binklac::Kernel::Irp::IrpDispatcher;

    if (!NT_SUCCESS(status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, TRUE, &deviceObject))) {
        goto CleanExit;
    }

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    deviceObject->Flags |= DO_BUFFERED_IO;
    DriverObject->DeviceObject = deviceObject;
    deviceObject->DriverObject = DriverObject;

    if (!NT_SUCCESS(status = IoCreateSymbolicLink(&symbolicLinkName, &deviceName))) {
        goto CleanExit;
    }

    if (!(Binklac::Kernel::Lpc::LpcManager::Initialization() && Binklac::Kernel::Lpc::LpcManager::CreateLpcPort() && Binklac::Kernel::Lpc::LpcManager::StartLpcServerThread())) {
        status = STATUS_DRIVER_UNABLE_TO_LOAD;
        goto CleanExit;
    }

CleanExit:
    if (!NT_SUCCESS(status) && deviceObject != NULL) {
        IoDeleteDevice(deviceObject);
    }

    if (!NT_SUCCESS(status)) {
        IoDeleteSymbolicLink(&symbolicLinkName);
    }

    LOG_DEBUG("End driver initialization.");

    return status;
}