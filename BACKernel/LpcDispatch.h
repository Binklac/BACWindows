#pragma once
#include "GlobalDeclaration.h"
#include "NtLpcApi.h"

class LpcDispatch {
  public:
    static VOID DispathLpcMessage(PPORT_MESSAGE LpcMessage);
};
