#include "IrpDispather.h"

namespace Binklac::Kernel::Irp {
    extern "C" NTSTATUS IrpDispatcher(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
        UNREFERENCED_PARAMETER(DeviceObject);
        UNREFERENCED_PARAMETER(Irp);
        return STATUS_SUCCESS;
    }
} // namespace Binklac::Kernel::Irp
