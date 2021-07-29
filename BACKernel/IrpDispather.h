#pragma once
#include "GlobalDeclaration.h"

namespace Binklac::Kernel::Irp {
	extern "C" NTSTATUS IrpDispatcher(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
}
